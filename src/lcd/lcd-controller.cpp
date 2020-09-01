#include "lcd-controller.hpp"

#include <util.hpp>


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
        uint32_t height = (size <= 1) ? 256 : 512;
        uint32_t width = (size % 2 == 0) ? 256 : 512;
        mosaicEnabled = le(regs->BGCNT[bgIndex]) & BGCNT::MOSAIC_MASK;
        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = le(regs->BGCNT[bgIndex]) & BGCNT::COLORS_PALETTES_MASK;
        priority = le(regs->BGCNT[bgIndex]) & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (le(regs->BGCNT[bgIndex]) & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (le(regs->BGCNT[bgIndex]) & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* scrolling, TODO: check sign */
        xOff = le(regs->BGOFS[bgIndex].h) & 0x1F;
        yOff = le(regs->BGOFS[bgIndex].v) & 0x1F;

        /* 32x32 tiles, arrangement depends on resolution */
        /* TODO: not sure about this one */
        uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr);
        bgMapBase = vramBase + screenBaseBlock * 0x800;
        
        if (bgMode == 0) {
            std::fill_n(scInUse, 4, true);
        } else if (bgMode == 3) {
            scInUse[0] = false;
            scInUse[1] = false;
            scInUse[2] = true;
            scInUse[3] = false;
        }

        scCount = (size <= 2) ? 2 : 4;
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
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();

        for (uint32_t scIndex = 0; scIndex < scCount; ++scIndex) {
            uint16_t *bgMap = reinterpret_cast<uint16_t *>(bgMapBase + scIndex * 0x800);

            for (uint32_t mapIndex = 0; mapIndex < 32 * 32; ++mapIndex) {
                uint16_t entry = bgMap[mapIndex];

                /* tile info */
                uint32_t tileNumber = entry & 0x3F;

                int32_t tileX = scXOffset[scIndex] + (mapIndex % 32) * 8;
                int32_t tileY = scYOffset[scIndex] + (mapIndex / 32) * 8;

                /* TODO: flipping */
                bool hFlip = (entry >> 10) & 1;
                bool vFlip = (entry >> 11) & 1;

                if (colorPalette256) {
                    uint8_t *tile = tiles + (tileNumber * 64);

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            uint32_t color = palette.getBgColor(tile[ty * 8 + tx]);
                            pixels[(tileY + ty) * stride + (tileX + tx)] = color;
                        }
                    }
                } else {
                    uint8_t *tile = tiles + (tileNumber * 32);
                    uint32_t paletteNumber = (entry >> 12) & 0xF;

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        uint32_t row = tile[ty];

                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            /* TODO: order correct? */
                            uint32_t color = palette.getBgColor(paletteNumber, (row & (0b1111 << (tx * 4))) >> (tx * 4));
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
        const uint16_t *srcPixels = reinterpret_cast<const uint16_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET, nullptr));

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = srcPixels[y * 240 + x];
                /* TODO: endianess? */
                color = ((color & 0xFF) << 16) | ((color & 0xFF00) >> 16);

                /* convert to R8G8B8 */
                uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
                uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
                uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;
                uint32_t a8r8g8b8 = 0xFF000000 | (r << 16) | (g << 8) | b;

                pixels[y * stride + x] = a8r8g8b8;
            }
        }
    }

    void Background::renderBG4(LCDColorPalette& palette, Memory& memory) {
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();
        const uint8_t *srcPixels = reinterpret_cast<const uint8_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET, nullptr));

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = srcPixels[y * 240 + x];
                /* TODO: endianess? */
                //color = ((color & 0xFF) << 16) | ((color & 0xFF00) >> 16);

                /* convert to R8G8B8 */
                //uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
                //uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
                //uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;
                //uint32_t a8r8g8b8 = 0xFF000000 | (r << 16) | (g << 8) | b;
                uint32_t a8r8g8b8 = palette.getBgColor(color);

                pixels[y * stride + x] = a8r8g8b8;
            }
        }
    }

    void Background::drawToDisplay(LCDisplay& display) {
        auto xFrom = std::max(0, xOff);
        auto xTo = std::min(DIMENSIONS::WIDTH, xOff + canvas.getWidth());

        auto yFrom = std::max(0, yOff);
        auto yTo = std::min(DIMENSIONS::HEIGHT, yOff + canvas.getHeight());

        auto dispPixels = display.canvas.pixels();
        auto dispStride = display.stride();

        auto canvPixels = canvas.pixels();
        auto canvStride = canvas.getWidth();

        display.canvas.beginDraw();

        for (auto y = yFrom; y < yTo; ++y) {
            for (auto x = xFrom; x < xTo; ++x) {
                auto cx = x + xOff;
                auto cy = y + yOff;
                auto color = canvPixels[cy * canvStride + cx];
                dispPixels[y * dispStride + x] = color;
            }
        }

        display.canvas.endDraw();
    }

    void LCDController::blendBackgrounds() {

    }

    void LCDController::updateReferences() {
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET, nullptr));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200, nullptr));
        regs = reinterpret_cast<LCDIORegs *>(memory.resolveAddr(gbaemu::Memory::IO_REGS_OFFSET, nullptr));
    }

    uint32_t LCDController::getBackgroundMode() const {
        return regs->DISPCNT & DISPCTL::BG_MODE_MASK;
    }

    void LCDController::render() {
        updateReferences();
        uint32_t bgMode = getBackgroundMode();
        display.canvas.beginDraw();

        uint8_t *vram = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr);

        if (bgMode == 0) {
            /*
                Mode  Rot/Scal Layers Size               Tiles Colors       Features
                0     No       0123   256x256..512x515   1024  16/16..256/1 SFMABP
                Features: S)crolling, F)lip, M)osaic, A)lphaBlending, B)rightness, P)riority.
            */
            /* TODO: I guess text mode? */
            for (uint32_t i = 0; i < 4; ++i) {
                backgrounds[i].loadSettings(0, i, regs, memory);
                backgrounds[i].renderBG0(palette);
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
        }

        display.canvas.endDraw();
        blendBackgrounds();
    }

    void LCDController::plotMemory() {
        display.canvas.beginDraw();
        /*
        for (uint32_t y = 0; y < DIMENSIONS::HEIGHT * 2; y++)
            for (uint32_t x = 0; x < DIMENSIONS::WIDTH; ++x)
                display.canvas.pixels()[y * display.canvas.getWidth() + x] = reinterpret_cast<uint32_t *>(vram)[y * 240 + x];

        return;
         */

        static const Memory::MemoryRegionOffset offs[] = {
            //Memory::BIOS_OFFSET,
            Memory::WRAM_OFFSET,
            Memory::IWRAM_OFFSET,
            //Memory::IO_REGS_OFFSET,
            //Memory::BG_OBJ_RAM_OFFSET,
            Memory::VRAM_OFFSET,
            //Memory::OAM_OFFSET,
            Memory::EXT_ROM_OFFSET,
            Memory::EXT_SRAM_OFFSET
        };

        static const Memory::MemoryRegionLimit limits[] = {
            //Memory::BIOS_LIMIT,
            Memory::WRAM_LIMIT,
            Memory::IWRAM_LIMIT,
            //Memory::IO_REGS_LIMIT,
            //Memory::BG_OBJ_RAM_LIMIT,
            Memory::VRAM_LIMIT,
            //Memory::OAM_LIMIT,
            Memory::EXT_ROM1_LIMIT,
            Memory::EXT_SRAM_LIMIT
        };

        static const uint32_t WIDTH = 240;

        for (uint32_t i = 0; i < sizeof(offs)/sizeof(offs[0]); ++i) {
            uint32_t *ram = reinterpret_cast<uint32_t *>(memory.resolveAddr(offs[i], nullptr));
            uint32_t size = limits[i] - offs[i];

            if (offs[i] == Memory::EXT_ROM_OFFSET) {
                size = memory.getRomSize();
            }

            for (uint32_t j = 0; j < size / 4; ++j) {
                auto x = j % WIDTH + (WIDTH + 16) * i;
                auto y = j / WIDTH;

                auto w = display.canvas.getWidth();
                auto h = display.canvas.getHeight();

                if (x >= w || y >= h)
                    break;

                display.canvas.pixels()[y * w + x] = ram[j];
            }
        }

        display.canvas.endDraw();
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
        stat = bitSet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET, counters.vCount);

        regs->DISPSTAT = le(stat);

        ++counters.cycle;

        return result;
    }
}

