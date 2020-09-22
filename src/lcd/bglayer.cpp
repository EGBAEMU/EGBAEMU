#include "bglayer.hpp"

#include "logging.hpp"


namespace gbaemu::lcd
{
    void Background::loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory)
    {
        if (!enabled)
            return;

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
                    LOG_LCD(std::cout << "WARNING: Invalid screen size!\n";);
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
        if (bgMode == 0) {
            wrap = true;
        } else if (bgIndex == 2 || bgIndex == 3) {
            wrap = le(regs.BGCNT[bgIndex]) & BGCNT::DISPLAY_AREA_OVERFLOW_MASK;
        } else {
            wrap = false;
        }

        /* scaling, rotation, only for bg2, bg3 */
        if (bgMode != 0 && (bgIndex == 2 || bgIndex == 3)) {
            if (bgIndex == 2) {
                step.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2X));
                step.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2Y));

                step.d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[0]));
                step.dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[1]));
                step.d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[2]));
                step.dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[3]));

                //std::cout << regs.BG2X << "    " << regs.BG2Y << std::endl;
                //std::cout << step.origin << "    " << step.d << "    " << step.dm << std::endl;
            } else {
                step.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3X));
                step.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3Y));

                step.d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[0]));
                step.dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[1]));
                step.d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[2]));
                step.dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[3]));
            }

            if (step.d[0] == 0 && step.d[1] == 0) {
                step.d[0] = 1;
                step.d[1] = 0;
            }

            if (step.dm[0] == 0 && step.dm[1] == 0) {
                step.dm[0] = 0;
                step.dm[1] = 1;
            }
        } else {
            /* use scrolling parameters */
            step.origin[0] = static_cast<common::math::real_t>(le(regs.BGOFS[bgIndex].h) & 0x1FF);
            step.origin[1] = static_cast<common::math::real_t>(le(regs.BGOFS[bgIndex].v) & 0x1FF);

            //if (bgIndex == 1)
            //    std::cout << step.origin[0] << std::endl;

            step.d[0] = 1;
            step.d[1] = 0;
            step.dm[0] = 0;
            step.dm[1] = 1;
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

        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();


        for (uint32_t scIndex = 0; scIndex < scCount; ++scIndex) {
            uint16_t *bgMap = reinterpret_cast<uint16_t *>(bgMapBase + scIndex * 0x800);

            for (uint32_t mapIndex = 0; mapIndex < 32 * 32; ++mapIndex) {
                uint16_t entry = bgMap[mapIndex];

                /* tile info */
                uint32_t tileNumber = entry & 0x3FF;
                uint32_t paletteNumber = (entry >> 12) & 0xF;

                int32_t tileX = scXOffset[scIndex] / 8 + (mapIndex % 32);
                int32_t tileY = scYOffset[scIndex] / 8 + (mapIndex / 32);

                int32_t offX = colorPalette256 ? scXOffset[scIndex] : 0;
                int32_t offY = colorPalette256 ? scYOffset[scIndex] : 0;

                /* TODO: flipping */
                bool hFlip = (entry >> 10) & 1;
                bool vFlip = (entry >> 11) & 1;

                size_t tileByteSize = colorPalette256 ? 64 : 32;
                const uint8_t *tilePtr = tiles + tileNumber * tileByteSize;

                for (uint32_t ty = 0; ty < 8; ++ty) {
                    uint32_t srcTy = vFlip ? (7 - ty) : ty;
                    uint32_t row = reinterpret_cast<const uint32_t *>(tilePtr)[srcTy];

                    for (uint32_t tx = 0; tx < 8; ++tx) {
                        uint32_t srcTx = hFlip ? (7 - tx) : tx;
                        color_t color;

                        if (!colorPalette256) {
                            uint32_t paletteIndex = (row & (0xF << (srcTx * 4))) >> (srcTx * 4);
                            color = palette.getBgColor(paletteNumber, paletteIndex);
                        } else {
                            color = palette.getBgColor(tilePtr[srcTy * 8 + srcTx]);
                        }

                        pixels[(tileY * 8 + offY + ty) * stride + (tileX * 8 + offX + tx)] = color;
                    }
                }
            }
        }
    }

    void Background::renderBG2(LCDColorPalette &palette)
    {
        if (!enabled)
            return;

        uint8_t *bgMap = reinterpret_cast<uint8_t *>(bgMapBase);
        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();

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
        if (!enabled)
            return;

        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();
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
        if (!enabled)
            return;

        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();
        Memory::MemoryRegion memReg;
        uint32_t fbOff;

        if (useOtherFrameBuffer)
            fbOff = 0xA000;
        else
            fbOff = 0;

        const uint8_t *srcPixels = reinterpret_cast<const uint8_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET + fbOff, nullptr, memReg));

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = srcPixels[y * 240 + x];
                pixels[y * stride + x] = palette.getBgColor(color);
            }
        }
    }

    void Background::renderBG5(LCDColorPalette &palette, Memory &memory)
    {
        if (!enabled)
            return;

        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();
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

    void Background::draw()
    {
        if (!enabled)
            return;

        canvas.clear(TRANSPARENT);
        const common::math::vec<2> screenRef{0, 0};
        canvas.drawSprite(tempCanvas.pixels(), width, height, tempCanvas.getWidth(),
                          step.origin, step.d, step.dm, screenRef, wrap);

        if (id == 2) {
            //std::cout << step.origin << ' ' << step.d << ' ' << step.dm << ' ' << screenRef << std::endl;
        }
    }
}