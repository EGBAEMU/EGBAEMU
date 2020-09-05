#include "lcd-controller.hpp"

#include <util.hpp>


/*
    There are 4 background layers.

    BG0     ----------------------------------
    BG1     ----------------------------------
    BG2     ----------------------------------
    BG3     ----------------------------------
    OBJ     ----------------------------------

    The drawing order and which layers should be drawn can be configured. Top layers can be alpha-blended with layers below.
    Brightness of the top layer can also be configured. The OBJ layer contains all the sprites (called OBJs).




 */

namespace gbaemu::lcd {
    void LCDBgObj::setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t mode) {
        bgMode = mode;

        switch (bgMode) {
            case 0: case 1: case 2:
                bg.bgMode012 = vramBaseAddress;
                objTiles = vramBaseAddress + 0x10000;
                break;
            case 3:
                bg.bgMode3 = vramBaseAddress;
                objTiles = vramBaseAddress + 0x14000;
                break;
            case 4:
                bg.bgMode45.frameBuffer0 = vramBaseAddress;
                bg.bgMode45.frameBuffer1 = vramBaseAddress + 0xA000;
                objTiles = vramBaseAddress + 0x14000;
                break;
        }

        attributes = oamBaseAddress;
    }

    LCDBgObj::ObjAttribute *LCDBgObj::accessAttribute(uint32_t index) {
        return reinterpret_cast<ObjAttribute *>(attributes + (index * 0x8));
    }

    uint32_t LCDColorPalette::toR8G8B8(uint16_t color) {
        uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
        uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
        uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;

        return (r << 16) | (g << 8) | b;
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t index) const {
        return toR8G8B8(bgPalette[index]);
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t i1, uint32_t i2) const {
        return getBgColor(i1 * 16 + i2);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t index) const {
        return toR8G8B8(objPalette[index]);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t i1 ,uint32_t i2) const {
        return getObjColor(i1 * 16 + i2);
    }

    void Background::loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs *regs, Memory& memory) {
        id = bgIndex;

        uint16_t size = (le(regs->BGCNT[bgIndex]) & BGCNT::SCREEN_SIZE_MASK) >> 14;

        /* TODO: not entirely correct */
        if (bgMode <= 2) {
            height = (size <= 1) ? 256 : 512;
            width = (size % 2 == 0) ? 256 : 512;
        } else if (bgMode == 3) {
            width = 240;
            height = 160;
        } else if (bgMode == 4) {
            width = 240;
            height = 160;
        } else if (bgMode == 5) {
            width = 160;
            height = 128;
        }
        
        mosaicEnabled = le(regs->BGCNT[bgIndex]) & BGCNT::MOSAIC_MASK;
        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = le(regs->BGCNT[bgIndex]) & BGCNT::COLORS_PALETTES_MASK;
        priority = le(regs->BGCNT[bgIndex]) & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (le(regs->BGCNT[bgIndex]) & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (le(regs->BGCNT[bgIndex]) & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* select which frame buffer to use */
        if (bgMode == 4 || bgMode == 5)
            useOtherFrameBuffer = le(regs->DISPCNT) & DISPCTL::DISPLAY_FRAME_SELECT_MASK;
        else
            useOtherFrameBuffer = false;

        /* wrapping */
        if (bgIndex == 2 || bgIndex == 3) {
            wrap = le(regs->BGCNT[bgIndex]) & BGCNT::DISPLAY_AREA_OVERFLOW_MASK;
        } else {
            wrap = false;
        }

        /* scaling, rotation, only for bg2, bg3 */
        if (bgIndex == 2 || bgIndex == 3) {
            float origin[2];
            float d[2];
            float dm[2];

            if (bgIndex == 2) {
                origin[0] = fpToFloat<uint32_t, 8, 19>(le(regs->BG2X));
                origin[1] = fpToFloat<uint32_t, 8, 19>(le(regs->BG2Y));

                d[0] = fpToFloat<uint16_t, 8, 7>(le(regs->BG2P[0]));
                dm[0] = fpToFloat<uint16_t, 8, 7>(le(regs->BG2P[1]));
                d[1] = fpToFloat<uint16_t, 8, 7>(le(regs->BG2P[2]));
                dm[1] = fpToFloat<uint16_t, 8, 7>(le(regs->BG2P[3]));
            } else {
                origin[0] = fpToFloat<uint32_t, 8, 19>(le(regs->BG3X));
                origin[1] = fpToFloat<uint32_t, 8, 19>(le(regs->BG3Y));

                d[0] = fpToFloat<uint16_t, 8, 7>(le(regs->BG3P[0]));
                dm[0] = fpToFloat<uint16_t, 8, 7>(le(regs->BG3P[1]));
                d[1] = fpToFloat<uint16_t, 8, 7>(le(regs->BG3P[2]));
                dm[1] = fpToFloat<uint16_t, 8, 7>(le(regs->BG3P[3]));
            }

            /* TODO: build trans and invTrans */

            /*
                x2 = A(x1-x0) + B(y1-y0) + x0 = Ax1 - Ax0 + By1 - By0 + x0 = Ax1 + By1 + (-Ax0 - By0 + x0)
                y2 = C(x1-x0) + D(y1-y0) + y0 = Cx1 - Cx0 + Dy1 - Dy0 + y0 = Cx1 + Dy1 + (-Cx0 - Dy0 + y0)

                TODO: invert
                x2 = Ax1 + By1 + (-Ax0 - By0 + x0)
                x2 - (-Ax0 - By0 + x0) = Ax1 + By1
                (x2 - (-Ax0 - By0 + x0) - By1) / A = x1
             */

            /*
            common::math::mat<3, 3> shear{
                {d[0], dm[0], 0},
                {d[1], dm[1], 0},
                {0, 0, 1}
            };
             */

            /* TODO: not sure what to do when parameters are 0 */
            common::math::mat<3, 3> shear = common::math::mat<3, 3>::id();

            /*
                (a b)^-1    =    1/det * (d -b)
                (c d)                    (-c a)
             */

            auto adet = 1 / ((shear[0][0] * shear[1][1]) - (shear[0][1] * shear[0][1]));

            common::math::mat<3, 3> invShear{
                {shear[2][2]  * adet, -shear[0][1] * adet, 0},
                {-shear[0][1] * adet, shear[0][0]  * adet, 0},
                {0, 0, 1}
            };

            common::math::mat<3, 3> translation{
                {1, 0, 20 + -d[0] * origin[0] - dm[0] * origin[1] + origin[0]},
                {0, 1, -d[1] * origin[0] - dm[1] * origin[1] + origin[1]},
                {0, 0, 1}
            };

            common::math::mat<3, 3> invTranslation{
                {1, 0, -translation[0][2]},
                {0, 1, -translation[1][2]},
                {0, 0, 1},
            };

            trans = translation * shear;
            invTrans = invShear * invTranslation;
        } else {
            /* use scrolling parameters */
            trans = common::math::mat<3, 3>::id();
            invTrans = common::math::mat<3, 3>::id();

            trans[0][2] = le(regs->BGOFS[bgIndex].h) & 0x1F;
            trans[1][2] = le(regs->BGOFS[bgIndex].v) & 0x1F;

            invTrans[0][2] = -trans[0][2];
            invTrans[1][2] = -trans[1][2];
        }

        /* 32x32 tiles, arrangement depends on resolution */
        /* TODO: not sure about this one */
        Memory::MemoryRegion memReg;
        uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, memReg);
        bgMapBase = vramBase + screenBaseBlock * 0x800;

        if (bgMode == 0) {
            std::fill_n(scInUse, 4, true);
        } else if (bgMode == 3) {
            scInUse[0] = false;
            scInUse[1] = false;
            scInUse[2] = true;
            scInUse[3] = false;
        }

        switch (size) {
            case 0: scCount = 1; break;
            case 1: scCount = 2; break;
            case 2: scCount = 2; break;
            case 3: scCount = 4; break;
        }
        /* tile addresses in steps of 0x4000 */
        /* 8x8, also called characters */
        tiles = vramBase + charBaseBlock * 0x4000;

        /*
            size = 0:
            +-----+
            | SC0 |
            +-----+

            size = 1:
            +---------+
            | SC0 SC1 |
            +---------+
        
            size = 2:
            +-----+
            | SC0 |
            | SC1 |
            +-----+

            size = 3:
            +---------+
            | SC0 SC1 |
            | SC2 SC3 |
            +---------+
         */
        /* always 0 */
        scXOffset[0] = 0;
        scYOffset[0] = 0;

        scXOffset[1] = (size % 2 == 1) ? 256 : 0;
        scYOffset[1] = (size == 2) ? 256 : 0;

        /* only available in size = 3 */
        scXOffset[2] = 0;
        scYOffset[2] = 256;

        /* only available in size = 3 */
        scXOffset[3] = 256;
        scYOffset[3] = 256;
    }

    void Background::renderBG0(LCDColorPalette& palette) {
        if (!enabled)
            return;

        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();

        for (uint32_t scIndex = 0; scIndex < scCount; ++scIndex) {
            uint16_t *bgMap = reinterpret_cast<uint16_t *>(bgMapBase + scIndex * 0x800);

            for (uint32_t mapIndex = 0; mapIndex < 32 * 32; ++mapIndex) {
                uint16_t entry = bgMap[mapIndex];

                /* tile info */
                uint32_t tileNumber = entry & 0x3FF;

                int32_t tileX = scXOffset[scIndex] + (mapIndex % 32) * 8;
                int32_t tileY = scYOffset[scIndex] + (mapIndex / 32) * 8;

                /* TODO: flipping */
                bool hFlip = (entry >> 10) & 1;
                bool vFlip = (entry >> 11) & 1;

                if (colorPalette256) {
                    uint8_t *tile = tiles + (tileNumber * 64);

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        uint32_t srcTy = vFlip ? (7 - ty) : ty;

                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            uint32_t srcTx = hFlip ? (7 - tx) : tx;
                            uint32_t color = palette.getBgColor(tile[srcTy * 8 + srcTx]);
                            pixels[(tileY + ty) * stride + (tileX + tx)] = color;
                        }
                    }
                } else {
                    uint8_t *tile = tiles + (tileNumber * 32);
                    uint32_t paletteNumber = (entry >> 12) & 0xF;

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        uint32_t srcTy = vFlip ? (7 - ty) : ty;
                        uint32_t row = tile[srcTy];

                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            uint32_t srcTx = hFlip ? (7 - tx) : tx;

                            /* TODO: order correct? */
                            auto paletteIndex = (row & (0b1111 << (srcTx * 4))) >> (srcTx * 4);
                            uint32_t color = palette.getBgColor(paletteNumber, paletteIndex);

                            pixels[(tileY + ty) * stride + (tileX + tx)] = color;
                        }
                    }
                }
            }
        }
    }

    void Background::renderBG3(Memory& memory) {
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();
        Memory::MemoryRegion memReg;
        const uint16_t *srcPixels = reinterpret_cast<const uint16_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET, nullptr, memReg));

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = srcPixels[y * 240 + x];
                pixels[y * stride + x] = LCDColorPalette::toR8G8B8(color);
            }
        }
    }

    void Background::renderBG4(LCDColorPalette& palette, Memory& memory) {
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();
        Memory::MemoryRegion memReg;
        uint32_t fbOff;

        if (useOtherFrameBuffer)
            fbOff = 0xA000;
        else
            fbOff = 0;
        
        const uint8_t *srcPixels = reinterpret_cast<const uint8_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET + fbOff, nullptr, memReg));

        //std::cout << std::hex << (void *)srcPixels << "\n";

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = srcPixels[y * 240 + x];
                pixels[y * stride + x] = palette.getBgColor(color);
            }
        }
    }

    void Background::renderBG5(LCDColorPalette& palette, Memory& memory) {
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();
        Memory::MemoryRegion memReg;
        uint32_t fbOff;

        if (useOtherFrameBuffer)
            fbOff = 0xA000;
        else
            fbOff = 0;
        
        const uint16_t *srcPixels = reinterpret_cast<const uint16_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET + fbOff, nullptr, memReg));

        for (int32_t y = 0; y < 128; ++y) {
            for (int32_t x = 0; x < 160; ++x) {
                uint16_t color = srcPixels[y * 160 + x];
                pixels[y * stride + x] = color;
            }
        }
    }

    void Background::drawToDisplay(LCDisplay& display) {
        display.canvas.beginDraw();
        display.canvas.drawSprite(canvas.pixels(), width, height, canvas.getWidth(), trans, invTrans, wrap);
        display.canvas.endDraw();
    }

    void LCDController::blendBackgrounds() {

    }

    void LCDController::updateReferences()
    {
        Memory::MemoryRegion memReg;
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET, nullptr, memReg));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200, nullptr, memReg));
        regs = reinterpret_cast<LCDIORegs *>(memory.resolveAddr(gbaemu::Memory::IO_REGS_OFFSET, nullptr, memReg));
    }

    uint32_t LCDController::getBackgroundMode() const {
        return regs->DISPCNT & DISPCTL::BG_MODE_MASK;
    }

    void LCDController::render() {
        updateReferences();
        uint32_t bgMode = getBackgroundMode();
        display.canvas.beginDraw();

        Memory::MemoryRegion memReg;
        uint8_t *vram = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, memReg);

        if (bgMode == 0 && false) {
            /*
                Mode  Rot/Scal Layers Size               Tiles Colors       Features
                0     No       0123   256x256..512x515   1024  16/16..256/1 SFMABP
                Features: S)crolling, F)lip, M)osaic, A)lphaBlending, B)rightness, P)riority.
            */
            /* TODO: I guess text mode? */
            for (uint32_t i = 0; i < 4; ++i) {
                backgrounds[i].enabled = regs->DISPCNT & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

                if (backgrounds[i].enabled) {
                    backgrounds[i].loadSettings(0, i, regs, memory);
                    backgrounds[i].renderBG0(palette);
                }
            }

            /* TODO: render top alpha last */
            std::vector<int32_t> backgroundIds = {backgrounds[0].id, backgrounds[1].id, backgrounds[2].id, backgrounds[3].id};
            std::stable_sort(backgroundIds.begin(), backgroundIds.end(), [&](int32_t id1, int32_t id2) {
                return backgrounds[id1].priority - backgrounds[id2].priority; });

            /* TODO: alpha blending */
            for (uint32_t i = 0; i < 4; ++i) {
                auto bgId = backgroundIds[i];
            }

            for (uint32_t i = 0; i < 4; ++i)
                backgrounds[i].drawToDisplay(display);
        } else if (bgMode == 1) {
            backgrounds[0].loadSettings(0, 0, regs, memory);
            backgrounds[1].loadSettings(0, 1, regs, memory);
            backgrounds[2].loadSettings(2, 2, regs, memory);

            backgrounds[0].renderBG0(palette);
            backgrounds[1].renderBG0(palette);
            //backgrounds[2].renderBG2(palette);
        } else if (bgMode == 2) {

        } else if (bgMode == 3) {
            /* TODO: This should easily be extendable to support BG4, BG5 */
            /* BG Mode 3 - 240x160 pixels, 32768 colors */
            backgrounds[2].loadSettings(3, 2, regs, memory);
            backgrounds[2].renderBG3(memory);
            backgrounds[2].drawToDisplay(display);
        } else if (bgMode == 4) {
            backgrounds[2].loadSettings(4, 2, regs, memory);
            backgrounds[2].renderBG4(palette, memory);
            backgrounds[2].drawToDisplay(display);
        } else if (bgMode == 5) {
            backgrounds[2].loadSettings(5, 2, regs, memory);
            backgrounds[2].renderBG5(palette, memory);
            backgrounds[2].drawToDisplay(display);
        } else {
            std::cout << "WARNING: unsupported bg mode " << bgMode << "\n";
        }

        display.canvas.endDraw();
        display.drawToTarget(2);

        blendBackgrounds();
    }

    void LCDController::plotPalette() {
        for (uint32_t i = 0; i < 256; ++i)
            display.canvas.pixels()[i] = palette.getBgColor(i);
    }

    bool LCDController::tick() {
        updateReferences();

        bool result = false;
        uint32_t vState = counters.cycle % 280896;
        counters.vBlanking = vState >= 197120;

        /* TODO: this is guesswork, maybe h and v blanking can be enabled concurrently */
        if (!counters.vBlanking) {
            uint32_t hState = counters.cycle % 1232;
            counters.hBlanking = hState >= 960;
            counters.vCount = counters.cycle / 1232;
        } else {
            counters.vCount = 0;
        }

        /* rendering once per h-blank */
        if (counters.hBlanking && counters.cycle % 1232 == 0) {
            render();
            result = true;
        }

        /* update stat */
        uint16_t stat = le(regs->DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.vBlanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.hBlanking));
        //stat = bitSet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET, counters.vCount);
        regs->DISPSTAT = le(stat);

        /* update vcount */
        uint16_t vcount = le(regs->VCOUNT);
        vcount = bitSet(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, counters.vCount);
        regs->VCOUNT = le(vcount);

        ++counters.cycle;

        return result;
    }
}

