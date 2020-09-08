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

namespace gbaemu::lcd
{
    void LCDBgObj::setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t mode)
    {
        bgMode = mode;

        switch (bgMode) {
            case 0:
            case 1:
            case 2:
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

    LCDBgObj::ObjAttribute *LCDBgObj::accessAttribute(uint32_t index)
    {
        return reinterpret_cast<ObjAttribute *>(attributes + (index * 0x8));
    }

    uint32_t LCDColorPalette::toR8G8B8(uint16_t color)
    {
        uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
        uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
        uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t index) const
    {
        /* 0 = transparent */
        if (index == 0)
            return 0x0;

        return toR8G8B8(bgPalette[index]);
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t i1, uint32_t i2) const
    {
        return getBgColor(i1 * 16 + i2);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t index) const
    {
        /* 0 = transparent */
        if (index == 0)
            return 0x0;

        return toR8G8B8(objPalette[index]);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t i1, uint32_t i2) const
    {
        return getObjColor(i1 * 16 + i2);
    }

    void Background::loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory)
    {
        id = bgIndex;

        uint16_t size = (le(regs.BGCNT[bgIndex]) & BGCNT::SCREEN_SIZE_MASK) >> 14;

        /* TODO: not entirely correct */
        if (bgMode == 0 || (bgMode == 1 && bgIndex <= 1)) {
            /* text mode */
            height = (size <= 1) ? 256 : 512;
            width = (size % 2 == 0) ? 256 : 512;
        } else if (bgMode == 2 || (bgMode == 1 && bgIndex == 2)) {
            switch (size) {
                case 0:
                    width = 128;
                    height = 128;
                    break;
                case 1:
                    width = 256;
                    height = 256;
                    break;
                case 2:
                    width = 512;
                    height = 512;
                    break;
                case 3:
                    width = 1024;
                    height = 1024;
                    break;
                default:
                    width = 0;
                    height = 0;
                    std::cout << "WARNING: Invalid screen size!\n";
                    break;
            }
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

        mosaicEnabled = le(regs.BGCNT[bgIndex]) & BGCNT::MOSAIC_MASK;
        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = le(regs.BGCNT[bgIndex]) & BGCNT::COLORS_PALETTES_MASK;
        priority = le(regs.BGCNT[bgIndex]) & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (le(regs.BGCNT[bgIndex]) & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (le(regs.BGCNT[bgIndex]) & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* select which frame buffer to use */
        if (bgMode == 4 || bgMode == 5)
            useOtherFrameBuffer = le(regs.DISPCNT) & DISPCTL::DISPLAY_FRAME_SELECT_MASK;
        else
            useOtherFrameBuffer = false;

        /* wrapping */
        if (bgIndex == 2 || bgIndex == 3) {
            wrap = le(regs.BGCNT[bgIndex]) & BGCNT::DISPLAY_AREA_OVERFLOW_MASK;
        } else {
            wrap = false;
        }

        /* scaling, rotation, only for bg2, bg3 */
        if (bgIndex == 2 || bgIndex == 3) {
            float origin[2];
            float d[2];
            float dm[2];

            if (bgIndex == 2) {
                origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2X));
                origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2Y));

                d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[0]));
                dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[1]));
                d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[2]));
                dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[3]));
            } else {
                origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3X));
                origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3Y));

                d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[0]));
                dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[1]));
                d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[2]));
                dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[3]));
            }

            common::math::mat<3, 3> translation{
                {1, 0, origin[0]},
                {0, 1, origin[1]},
                {0, 0, 1}
            };

            //std::cout << translation << '\n';

            common::math::mat<3, 3> invTranslation{
                {1, 0, -translation[0][2]},
                {0, 1, -translation[1][2]},
                {0, 0, 1}
            };

            common::math::mat<3, 3> shear{
                {d[0], dm[0], 0},
                {d[1], dm[1], 0},
                {0, 0, 1}
            };

            /*
                TODO: This is guesswork. Some demos don't touch these scrolling/scaling registers and leave them at zero.
                It could be that the GBA interprets this as the id transformation. So we check if the transformation is
                invertable and set trans to id if that's the case.
            */
            if (std::abs((shear[0][0] * shear[1][1]) - (shear[0][1] * shear[1][0])) < 1e-6)
                shear = common::math::mat<3, 3>::id();
            
            common::math::real_t adet =  1 / (shear[0][0] * shear[1][1]) - (shear[0][1] * shear[1][0]);

            common::math::mat<3, 3> invShear{
                {shear[1][1] * adet, -shear[0][1] * adet, 0},
                {-shear[1][0] * adet, shear[0][0] * adet, 0},
                {0, 0, 1}
            };

            //std::cout << origin[0] << ' ' << origin[1] << ' ' << d[0] << ' ' << d[1] << ' ' << dm[0] << ' ' << dm[1] << '\n';
            //std::cout << shear << '\n';

            //std::cout << rotation << std::endl;

            //std::cout << shear * invShear << std::endl;

            trans = shear * translation;
            invTrans = invTranslation * invShear;

            //std::cout << trans * invTrans << std::endl;
        } else {
            /* use scrolling parameters */
            trans = common::math::mat<3, 3>::id();
            invTrans = common::math::mat<3, 3>::id();

            trans[0][2] = le(regs.BGOFS[bgIndex].h) & 0x1F;
            trans[1][2] = le(regs.BGOFS[bgIndex].v) & 0x1F;

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
            case 0:
                scCount = 1;
                break;
            case 1:
                scCount = 2;
                break;
            case 2:
                scCount = 2;
                break;
            case 3:
                scCount = 4;
                break;
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

    void Background::renderBG0(LCDColorPalette &palette)
    {
        if (!enabled)
            return;

        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();

        if (mosaicEnabled)
            std::cout << "INFO: Mosaic enabled" << std::endl;

        for (uint32_t scIndex = 0; scIndex < scCount; ++scIndex) {
            uint16_t *bgMap = reinterpret_cast<uint16_t *>(bgMapBase + scIndex * 0x800);

            for (uint32_t mapIndex = 0; mapIndex < 32 * 32; ++mapIndex) {
                uint16_t entry = bgMap[mapIndex];

                /* tile info */
                uint32_t tileNumber = entry & 0x3FF;
                uint32_t paletteNumber = (entry >> 12) & 0xF;

                int32_t tileX = scXOffset[scIndex] + (mapIndex % 32);
                int32_t tileY = scYOffset[scIndex] + (mapIndex / 32);

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
                            pixels[(tileY * 8 + scYOffset[scIndex] + ty) * stride + (tileX * 8 + scXOffset[scIndex] + tx)] = color;
                        }
                    }
                } else {
                    uint32_t *tile = reinterpret_cast<uint32_t *>(tiles + (tileNumber * 32));

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        uint32_t srcTy = vFlip ? (7 - ty) : ty;
                        uint32_t row = tile[srcTy];

                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            uint32_t srcTx = hFlip ? (7 - tx) : tx;

                            /* TODO: order correct? */
                            auto paletteIndex = (row & (0b1111 << (srcTx * 4))) >> (srcTx * 4);
                            uint32_t color = palette.getBgColor(paletteNumber, paletteIndex);

                            pixels[(tileY * 8 + ty) * stride + (tileX * 8 + tx)] = color;
                        }
                    }
                }
            }
        }
    }

    void Background::renderBG2(LCDColorPalette &palette)
    {
        uint8_t *bgMap = reinterpret_cast<uint8_t *>(bgMapBase);
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();

        for (uint32_t mapIndex = 0; mapIndex < (width / 8) * (height / 8); ++mapIndex) {
            uint8_t tileNumber = bgMap[mapIndex];

            int32_t tileX = mapIndex % (width / 8);
            int32_t tileY = mapIndex / (height / 8);

            uint8_t *tile = tiles + (tileNumber * 64);

            for (uint32_t ty = 0; ty < 8; ++ty) {
                for (uint32_t tx = 0; tx < 8; ++tx) {
                    uint32_t j = ty * 8 + tx;
                    uint32_t k = tile[j];
                    uint32_t color = palette.getBgColor(k);
                    pixels[(tileY * 8 + ty) * stride + (tileX * 8 + tx)] = color;
                }
            }
        }
    }

    void Background::renderBG3(Memory &memory)
    {
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

    void Background::renderBG4(LCDColorPalette &palette, Memory &memory)
    {
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

    void Background::renderBG5(LCDColorPalette &palette, Memory &memory)
    {
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
                pixels[y * stride + x] = LCDColorPalette::toR8G8B8(color);
            }
        }
    }

    void Background::drawToDisplay(LCDisplay &display)
    {
        display.canvas.beginDraw();
        display.canvas.drawSprite(canvas.pixels(), width, height, canvas.getWidth(), trans, invTrans, wrap);
        display.canvas.endDraw();
    }

    /*
        LCDController
     */

    void LCDController::onHBlank()
    {
        updateReferences();

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgrounds[i]->enabled = regs.DISPCNT & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        if (bgMode == 0) {
            for (uint32_t i = 0; i < 4; ++i) {
                backgrounds[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

                if (backgrounds[i]->enabled) {
                    backgrounds[i]->loadSettings(0, i, regs, memory);
                    backgrounds[i]->renderBG0(palette);
                }
            }
        } else if (bgMode == 1) {
            backgrounds[0]->loadSettings(0, 0, regs, memory);
            backgrounds[1]->loadSettings(0, 1, regs, memory);
            backgrounds[2]->loadSettings(2, 2, regs, memory);
        } else if (bgMode == 2) {
            backgrounds[2]->loadSettings(2, 2, regs, memory);
            backgrounds[3]->loadSettings(2, 3, regs, memory);
        } else if (bgMode == 3) {
            backgrounds[2]->loadSettings(3, 2, regs, memory);
        } else if (bgMode == 4) {
            backgrounds[2]->loadSettings(4, 2, regs, memory);
        } else if (bgMode == 5) {
            backgrounds[2]->loadSettings(5, 2, regs, memory);
        } else {
            std::cout << "WARNING: unsupported bg mode " << bgMode << "\n";
        }

        /* sort backgrounds by priority, disabled backgrounds will be skipped in rendering */
        std::sort(backgrounds.begin(), backgrounds.end(), [](const std::unique_ptr<Background>& b1, const std::unique_ptr<Background>& b2) -> bool {
            int32_t delta = b2->priority - b1->priority;
            return (delta == 0) ? (b1->id - b2->id < 0) : (delta < 0);
        });

        /* load special color effects */
        uint16_t bldcnt = le(regs.BLDCNT);

        firstTargetLayerID = -1;
        secondTargetLayerID = -1;

        for (size_t i = 4; i > 0; --i) {
            if ((i - 1) <= 3 && backgrounds[i - 1]->enabled) {
                if (bitGet<uint16_t>(bldcnt, 1, backgrounds[i - 1]->id))
                    firstTargetLayerID = backgrounds[i - 1]->id;
                
                if (bitGet<uint16_t>(bldcnt, 1, backgrounds[i - 1]->id + BLDCNT::BG0_TARGET_PIXEL2_OFFSET))
                    secondTargetLayerID = backgrounds[i - 1]->id;
            }
        }

        /* first/second target layers are not background layers */
        if (firstTargetLayerID == -1) {
            if (bitGet<uint16_t>(bldcnt, BLDCNT::OBJ_TARGET_PIXEL1_MASK, BLDCNT::OBJ_TARGET_PIXEL1_OFFSET))
                firstTargetLayerID = 4;
            else if (bitGet<uint16_t>(bldcnt, BLDCNT::BD_TARGET_PIXEL1_MASK, BLDCNT::BD_TARGET_PIXEL1_OFFSET))
                firstTargetLayerID = 5;
        }

        if (secondTargetLayerID == -1) {
            if (bitGet<uint16_t>(bldcnt, BLDCNT::OBJ_TARGET_PIXEL2_MASK, BLDCNT::OBJ_TARGET_PIXEL2_OFFSET))
                secondTargetLayerID = 4;
            else if (bitGet<uint16_t>(bldcnt, BLDCNT::BD_TARGET_PIXEL2_MASK, BLDCNT::BD_TARGET_PIXEL2_OFFSET))
                secondTargetLayerID = 5;
        }

        /* what actual special effect is used? */
        colorSpecialEffect = static_cast<BLDCNT::ColorSpecialEffect>(
            bitGet(bldcnt, BLDCNT::COLOR_SPECIAL_FX_MASK, BLDCNT::COLOR_SPECIAL_FX_OFFSET));
    }

    void LCDController::onVBlank()
    {

    }

    void LCDController::render()
    {
        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;

        display.canvas.beginDraw();
        display.canvas.clear(0xFF000000);

        switch (bgMode) {
            case 0:
                for (uint32_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled) {
                        backgrounds[i]->renderBG0(palette);
                        backgrounds[i]->drawToDisplay(display);
                    }
                }

                break;
            case 1:
                /*
                    id 0, 1 in BG0
                    id 2 in BG2
                 */

                for (size_t i = 0; i < 4; ++i) {
                    if (!backgrounds[i]->enabled)
                        continue;

                    if (backgrounds[i]->id <= 1) {
                        backgrounds[i]->renderBG0(palette);
                        backgrounds[i]->drawToDisplay(display);
                    } else if (backgrounds[i]->id == 2) {
                        backgrounds[i]->renderBG2(palette);
                        backgrounds[i]->drawToDisplay(display);
                    }
                }

                break;
            case 2:
                if (backgrounds[2]->enabled) {
                    backgrounds[2]->renderBG2(palette);
                    backgrounds[2]->drawToDisplay(display);
                }

                if (backgrounds[3]->enabled) {
                    backgrounds[3]->renderBG2(palette);
                    backgrounds[3]->drawToDisplay(display);
                }

                break;
            case 3:
                if (backgrounds[2]->enabled) {
                    backgrounds[2]->renderBG3(memory);
                    backgrounds[2]->drawToDisplay(display);
                }

                break;
            case 4:
                if (backgrounds[2]->enabled) {
                    backgrounds[2]->renderBG4(palette, memory);
                    backgrounds[2]->drawToDisplay(display);
                }

                break;
            case 5:
                if (backgrounds[2]->enabled) {
                    backgrounds[2]->renderBG5(palette, memory);
                    backgrounds[2]->drawToDisplay(display);
                }    

                break;

            default:
                std::cout << "WARNING: Unsupported background mode " << std::dec << bgMode << "!\n";
                break;
        }

        display.canvas.endDraw();
        display.drawToTarget(2);
    }

    void LCDController::renderLoop()
    {
        while (true) {
            //canDrawToScreenMutex->lock();
            renderControlMutex.lock();

            bool exitLoop = (renderControl == EXIT);
            bool wait = (renderControl == WAIT);
            renderControl = WAIT;

            if (wait) {
                renderControlMutex.unlock();
                continue;
            } else if (exitLoop) {
                renderControlMutex.unlock();
                break;
            }

            render();

            /* Tell the window we are done, if it isn't ready it has to try next time. */
            canDrawToScreenMutex->lock();
            *canDrawToScreen = true;
            canDrawToScreenMutex->unlock();

            /*
                Allow another call to render() only if render() has finished. If onHBlank() gets executed
                while we are rendering we crash.
             */
            renderControlMutex.unlock();
        }
    }

    void LCDController::updateReferences()
    {
        Memory::MemoryRegion memReg;
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET, nullptr, memReg));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200, nullptr, memReg));
    }

    bool LCDController::tick()
    {
        static bool irqTriggeredV = false;
        static bool irqTriggeredH = false;

        bool result = false;
        uint32_t vState = counters.cycle % 280896;
        counters.vBlanking = vState >= 197120;

        /* TODO: this is guesswork, maybe h and v blanking can be enabled concurrently */
        if (!counters.vBlanking) {
            uint32_t hState = counters.cycle % 1232;
            counters.hBlanking = hState >= 960;
            counters.vCount = counters.cycle / 1232;
            irqTriggeredV = false;
            if (!counters.hBlanking) {
                irqTriggeredH = false;
            }
        } else {
            counters.vCount = 0;
        }

        if (!irqTriggeredV && counters.vBlanking) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);
            irqTriggeredV = true;
        }
        if (!irqTriggeredH && counters.hBlanking) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);
            irqTriggeredH = true;
        }

        /* rendering once per h-blank */
        if (counters.hBlanking && counters.cycle % (1231 * 4) == 0) {
            /*
                Rendering will not be able to keep up with each hblank, but that's ok because we per scanline updates
                are not visible to the human eye.
             */

            /* No blocking, otherwise we have gained nothing. */
            
            if (renderControlMutex.try_lock()) {
                onHBlank();
                renderControl = RUN;
                renderControlMutex.unlock();
            }
        }

        if (counters.vBlanking && counters.cycle % 197120 == 0) {
            onVBlank();
        }

        /* update stat */
        uint16_t stat = le(regs.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.vBlanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.hBlanking));
        //stat = bitSet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET, counters.vCount);
        regs.DISPSTAT = le(stat);

        /* update vcount */
        uint16_t vcount = le(regs.VCOUNT);
        vcount = bitSet(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, counters.vCount);
        regs.VCOUNT = le(vcount);

        ++counters.cycle;

        return result;
    }
} // namespace gbaemu::lcd
