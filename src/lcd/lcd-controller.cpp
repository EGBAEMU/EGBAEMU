#include "lcd-controller.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <algorithm>

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
    /* LCDColorPalette */

    color_t LCDColorPalette::toR8G8B8(uint16_t color)
    {
        uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
        uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
        uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    color_t LCDColorPalette::getBgColor(uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return toR8G8B8(bgPalette[index]);
    }

    color_t LCDColorPalette::getBgColor(uint32_t paletteNumber, uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return getBgColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getObjColor(uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return toR8G8B8(objPalette[index]);
    }

    color_t LCDColorPalette::getObjColor(uint32_t paletteNumber, uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return getObjColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getBackdropColor() const
    {
        return toR8G8B8(bgPalette[0]);
    }

    /* OBJLayer */

    OBJLayer::OBJAttribute OBJLayer::getAttribute(uint32_t index) const
    {
        auto uints = reinterpret_cast<const uint16_t *>(attributes + (index * 0x8));
        return OBJAttribute{le(uints[0]), le(uints[1]), le(uints[2])};
    }

    std::tuple<common::math::vec<2>, common::math::vec<2>> OBJLayer::getRotScaleParameters(uint32_t index) const
    {
        const uint16_t *uints = reinterpret_cast<const uint16_t *>(attributes);

        return std::make_tuple<common::math::vec<2>, common::math::vec<2>>(
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[index * 4 + 3])),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 2) * 4 + 3]))},
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 1) * 4 + 3])),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 3) * 4 + 3]))});
    }

    OBJLayer::OBJLayer(LayerId layerId) : Layer(layerId)
    {
        order = -(static_cast<int32_t>(layerId) * 2 + 1);
        enabled = true;
    }

    void OBJLayer::setMode(const uint8_t *vramBaseAddress, const uint8_t *oamBaseAddress, uint32_t mode)
    {
        bgMode = mode;

        switch (bgMode) {
            case 0:
            case 1:
            case 2:
                objTiles = vramBaseAddress + 0x10000;
                areaSize = 32 * 1024;
                break;
            case 3:
            case 4:
                objTiles = vramBaseAddress + 0x14000;
                areaSize = 16 * 1024;
                break;
        }

        attributes = oamBaseAddress;
    }

    void OBJLayer::draw(LCDColorPalette &palette, bool use2dMapping)
    {
        typedef common::math::real_t real_t;
        std::array<color_t, 64 * 64> tempBuffer;

        canvas.clear(TRANSPARENT);

        for (int32_t i = 127; i >= 0; --i) {
            OBJAttribute attr = getAttribute(i);
            uint16_t priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));

            /* This sprite does not belong on this layer. */
            if (priority != this->priority)
                continue;

            bool useRotScale = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::ROT_SCALE_MASK, OBJ_ATTRIBUTE::ROT_SCALE_OFFSET);
            bool vFlip = false;
            bool hFlip = false;

            if (!useRotScale) {
                bool disabled = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DISABLE_MASK, OBJ_ATTRIBUTE::DISABLE_OFFSET);

                if (disabled)
                    continue;

                vFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::V_FLIP_MASK, OBJ_ATTRIBUTE::V_FLIP_OFFSET);
                hFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::H_FLIP_MASK, OBJ_ATTRIBUTE::H_FLIP_OFFSET);
            } else {
            }

            /* 16x16 palette */
            bool useColor256 = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::COLOR_PALETTE_MASK, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET);

            int32_t yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
            int32_t xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);

            OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0],
                                                                    OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
            uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
            uint16_t tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);

            if (useColor256)
                tileNumber /= 2;

            bool useMosaic = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MOSAIC_MASK, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET);
            OBJMode mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

            if (3 <= bgMode && bgMode <= 5 && tileNumber < 512)
                continue;

            /*
                Size  Square   Horizontal  Vertical
                0     8x8      16x8        8x16
                1     16x16    32x8        8x32
                2     32x32    32x16       16x32
                3     64x64    64x32       32x64
             */

            uint32_t width, height;

            /* TODO: Maybe there is a better way to do this. */
            switch (shape) {
                case SQUARE:
                    if (size == 0) {
                        width = 8;
                        height = 8;
                    } else if (size == 1) {
                        width = 16;
                        height = 16;
                    } else if (size == 2) {
                        width = 32;
                        height = 32;
                    } else {
                        width = 64;
                        height = 64;
                    }

                    break;
                case HORIZONTAL:
                    if (size == 0) {
                        width = 16;
                        height = 8;
                    } else if (size == 1) {
                        width = 32;
                        height = 8;
                    } else if (size == 2) {
                        width = 32;
                        height = 16;
                    } else {
                        width = 64;
                        height = 32;
                    }

                    break;
                case VERTICAL:
                    if (size == 0) {
                        width = 8;
                        height = 16;
                    } else if (size == 1) {
                        width = 8;
                        height = 32;
                    } else if (size == 2) {
                        width = 16;
                        height = 32;
                    } else {
                        width = 32;
                        height = 64;
                    }

                    break;
            }

            std::fill_n(tempBuffer.begin(), 64 * 64, 0);

            /* This value might be trash, depending on the mode. */
            uint16_t paletteNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PALETTE_NUMBER_MASK, OBJ_ATTRIBUTE::PALETTE_NUMBER_OFFSET);
            uint32_t tilesPerRow = useColor256 ? 16 : 32;
            size_t bytesPerTile = useColor256 ? 64 : 32;

            for (uint32_t tileY = 0; tileY < height / 8; ++tileY) {
                for (uint32_t tileX = 0; tileX < width / 8; ++tileX) {
                    /* This is where our tile data lies. */
                    const void *tilePtr;
                    size_t tileIndex;

                    uint32_t flippedTileX = hFlip ? (width / 8 - 1 - tileX) : tileX;
                    uint32_t flippedTileY = vFlip ? (height / 8 - 1 - tileY) : tileY;

                    if (use2dMapping)
                        tileIndex = tileNumber + flippedTileX + flippedTileY * tilesPerRow;
                    else
                        tileIndex = tileNumber + flippedTileX + flippedTileY * (width / 8);

                    tilePtr = objTiles + tileIndex * bytesPerTile;

                    if (!useColor256) {
                        const uint32_t *tile = reinterpret_cast<const uint32_t *>(tilePtr);

                        for (uint32_t py = 0; py < 8; ++py) {
                            uint32_t ty = vFlip ? (7 - py) : py;
                            uint32_t row = tile[ty];

                            for (uint32_t px = 0; px < 8; ++px) {
                                uint32_t tx = hFlip ? (7 - px) : px;

                                uint32_t paletteIndex = (row & (0xF << (tx * 4))) >> (tx * 4);
                                color_t color = palette.getObjColor(paletteNumber, paletteIndex);

                                tempBuffer[(tileY * 8 + py) * 64 + (tileX * 8 + px)] = color;
                            }
                        }
                    } else {
                        const uint8_t *tile = reinterpret_cast<const uint8_t *>(tilePtr);

                        for (uint32_t py = 0; py < 8; ++py) {
                            uint32_t ty = vFlip ? (7 - py) : py;

                            for (uint32_t px = 0; px < 8; ++px) {
                                uint32_t tx = hFlip ? (7 - px) : px;
                                color_t color = palette.getObjColor(tile[ty * 8 + tx]);
                                tempBuffer[(tileY * 8 + py) * 64 + (tileX * 8 + px)] = color;
                            }
                        }
                    }
                }
            }

            /* get parameters */
            common::math::vec<2> d{1, 0}, dm{0, 1};
            bool doubleSized = useRotScale && bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DOUBLE_SIZE_MASK, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET);

            if (useRotScale) {
                uint16_t index = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::ROT_SCALE_PARAM_MASK, OBJ_ATTRIBUTE::ROT_SCALE_PARAM_OFFSET);
                auto result = getRotScaleParameters(index);

                d = std::get<0>(result);
                dm = std::get<1>(result);
            }

            common::math::vec<2> origin{
                static_cast<real_t>(width) / 2,
                static_cast<real_t>(height) / 2};

#ifdef DEBUG_DRAW_SPRITE_BOUNDS
            {
                color_t color = (i == 0) ? 0xFFFF0000 : 0xFF00FF00;

                for (int32_t y = 0; y < height; ++y) {
                    tempBuffer[y * 64] = color;
                    tempBuffer[y * 64 + width - 1] = color;
                }

                for (int32_t x = 0; x < width; ++x) {
                    tempBuffer[x] = color;
                    tempBuffer[(height - 1) * 64 + x] = color;
                }
            }
#endif

#ifdef DEBUG_DRAW_SPRITE_GRID
            {
                color_t color = (i == 0) ? 0xFFFF0000 : 0xFF00FF00;

                for (int32_t y = 0; y < height; ++y)
                    for (int32_t x = 0; x < width; x += SPRITE_GRID_SPACING)
                        tempBuffer[y * 64 + x] = color;

                for (int32_t y = 0; y < height; y += SPRITE_GRID_SPACING)
                    for (int32_t x = 0; x < width; ++x)
                        tempBuffer[y * 64 + x] = color;
            }
#endif

            common::math::vec<2> screenRef{
                static_cast<real_t>(xOff + width / 2),
                static_cast<real_t>(yOff + height / 2)};

            if (doubleSized) {
                screenRef[0] = static_cast<real_t>(xOff + width);
                screenRef[1] = static_cast<real_t>(yOff + height);
            }

            /*
            display.canvas.drawSprite(tempBuffer, width, height, 64,
                origin, d, dm, screenRef);
             */

            /* 0 = highest priority */
            canvas.drawSprite(tempBuffer.data(), width, height, 64,
                              origin, d, dm, screenRef);
        }
    }

    /* Background */

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
    }

    /*
        LCDController
     */

    void LCDController::setupLayers()
    {
        backgroundLayers[0] = std::make_shared<Background>(Layer::LayerId::BG0);
        objLayers[0] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ0);
        objLayers[0]->priority = 0;

        backgroundLayers[1] = std::make_shared<Background>(Layer::LayerId::BG1);
        objLayers[1] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ1);
        objLayers[1]->priority = 1;

        backgroundLayers[2] = std::make_shared<Background>(Layer::LayerId::BG2);
        objLayers[2] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ2);
        objLayers[2]->priority = 2;

        backgroundLayers[3] = std::make_shared<Background>(Layer::LayerId::BG3);
        objLayers[3] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ3);
        objLayers[3]->priority = 3;
    }

    void LCDController::sortLayers()
    {
        sortedBackgroundLayers = backgroundLayers;
        std::sort(sortedBackgroundLayers.begin(), sortedBackgroundLayers.end(),
                  [](const std::shared_ptr<Layer> &l1, const std::shared_ptr<Layer> &l2) -> bool {
                      if (l1->priority != l2->priority)
                          return l1->priority > l2->priority;
                      else
                          return l1->id > l2->id;
                  });

        uint32_t layerIndex = 0;
        uint32_t sortedIndex = 0;
        int32_t objIndex = 3;

        while (layerIndex < 8) {
            uint16_t prio = sortedBackgroundLayers[sortedIndex]->priority;
            layers[layerIndex++] = sortedBackgroundLayers[sortedIndex++];

            for (; objIndex >= 0 && (sortedIndex == 4 || objLayers[objIndex]->priority == prio); objIndex--) {
                layers[layerIndex++] = objLayers[objIndex];
            }
        }
    }

    void LCDController::onHBlank()
    {
        updateReferences();

        /* copy registers, they cannot be modified when rendering */
        regs = regsRef;

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        Memory::MemoryRegion region;
        const uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, region);
        const uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, region);

        /* obj layer */
        for (const auto &layer : objLayers)
            layer->setMode(vramBase, oamBase, bgMode);

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        if (bgMode == 0) {
            for (uint32_t i = 0; i < 4; ++i) {
                backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

                if (backgroundLayers[i]->enabled) {
                    backgroundLayers[i]->loadSettings(0, i, regs, memory);
                    backgroundLayers[i]->renderBG0(palette);
                }
            }
        } else if (bgMode == 1) {
            backgroundLayers[0]->loadSettings(0, 0, regs, memory);
            backgroundLayers[1]->loadSettings(0, 1, regs, memory);
            backgroundLayers[2]->loadSettings(2, 2, regs, memory);
        } else if (bgMode == 2) {
            backgroundLayers[2]->loadSettings(2, 2, regs, memory);
            backgroundLayers[3]->loadSettings(2, 3, regs, memory);
        } else if (bgMode == 3) {
            backgroundLayers[2]->loadSettings(3, 2, regs, memory);
        } else if (bgMode == 4) {
            backgroundLayers[2]->loadSettings(4, 2, regs, memory);
        } else if (bgMode == 5) {
            backgroundLayers[2]->loadSettings(5, 2, regs, memory);
        } else {
            LOG_LCD(std::cout << "WARNING: unsupported bg mode " << bgMode << "\n";);
        }

        /* load special color effects */
        uint16_t bldcnt = le(regs.BLDCNT);

        /* what actual special effect is used? */
        colorSpecialEffect = static_cast<BLDCNT::ColorSpecialEffect>(
            bitGet(bldcnt, BLDCNT::COLOR_SPECIAL_FX_MASK, BLDCNT::COLOR_SPECIAL_FX_OFFSET));

        sortLayers();
    }

    void LCDController::onVBlank()
    {
    }

    void LCDController::copyLayer(const Canvas<color_t> &src)
    {
        auto dstPixels = display.target.pixels();
        auto dstStride = display.target.getWidth();

        auto srcPixels = src.pixels();
        auto srcStride = src.getWidth();

        for (int32_t y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                color_t color = srcPixels[y * srcStride + x];

                if (color & 0xFF000000)
                    dstPixels[y * dstStride + x] = color;
            }
        }
    }

    void LCDController::drawToTarget()
    {
        /*
            At this point every background and the object layer contain all the pixels that will be drawn to the screen.
            The last step is to mix the two appropriate layers and draw all other layers according to priority to the display.
         */

        auto dst = display.target.pixels();
        auto w = display.target.getWidth();
        display.target.beginDraw();

        for (int32_t y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                int32_t coord = y * SCREEN_WIDTH + x;
                color_t firstColor, secondColor, finalColor = 0xFF000000;

#if RENDERER_ENABLE_COLOR_EFFECTS
#error "Color effect rendering is not implemented."
                color_t firstPixel, secondPixels;

                for (int32_t i = 0; i < 4; ++i) {
                    if (i <= 3) {
                        if (backgrounds[i]->enabled && backgrounds[i]->asFirstTarget)
                            firstPixel = backgrounds[i]->canvas.pixels()[coord];
                    } else if (i == 4 && objLayer.asFirstTarget) {
                        firstPixel = objLayer.layers[i]->pixels()[coord];
                    } else if (asFirstTarget) {
                        firstPixel = palette.getBackdropColor();
                    }

                    if (i <= 3) {
                        if (backgrounds[i]->enabled && backgrounds[i]->asSecondTarget)
                            secondPixel = backgrounds[i]->canvas.pixels()[coord];
                    } else if (i == 4 && objLayer.asSecondTarget) {
                        secondPixel = objLayer.layers[i]->pixels()[coord];
                    } else if (asSecondTarget) {
                        secondPixel = palette.getBackdropColor();
                    }
                }

                switch (colorSpecialEffect) {
                    case BLDCNT::ColorSpecialEffect::None:
                        finalColor = firstPixel;
                        break;
                    case BLDCNT::ColorSpecialEffect::BrightnessIncrease:
                        finalColor = firstPixel + (31 - firstPixel) * brightnessEffect.evy;
                        break;
                    case BLDCNT::ColorSpecialEffect::BrightnessDecrease:
                        finalColor = firstPixel - firstPixel * brightnessEffect.evy;
                        break;
                    case BLDCNT::ColorSpecialEffect::AlphaBlending:
                        finalColor = std::min<color_t>(31, firstPixel * alphaEffect.eva + secondPixel * alphaEffect.evb);
                        break;
                    default:
                        break;
                }

#else
                finalColor = palette.getBackdropColor();

                for (int32_t i = 0; i < layers.size(); ++i) {
                    if (layers[i]->enabled) {
                        color_t color = layers[i]->canvas.pixels()[y * SCREEN_WIDTH + x];

                        if (color & 0xFF000000)
                            finalColor = color;
                    }
                }
#endif

                for (int32_t ty = 0; ty < display.yStep; ++ty)
                    for (int32_t tx = 0; tx < display.xStep; ++tx)
                        dst[(display.yStep * y + ty) * w + (display.xStep * x + tx)] = finalColor;
            }
        }

        display.target.endDraw();
    }

    void LCDController::render()
    {
        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        /* clear with brackdrop color */
        color_t clearColor = palette.getBackdropColor();

        switch (bgMode) {
            case 0:
                for (size_t i = 0; i < 4; ++i) {
                    backgroundLayers[i]->renderBG0(palette);
                    backgroundLayers[i]->draw();
                }
                break;
            case 1:
                /*
                    id 0, 1 in BG0
                    id 2 in BG2
                 */
                backgroundLayers[0]->renderBG0(palette);
                backgroundLayers[0]->draw();

                backgroundLayers[1]->renderBG0(palette);
                backgroundLayers[1]->draw();

                backgroundLayers[2]->renderBG2(palette);
                backgroundLayers[2]->draw();
                break;
            case 2:
                backgroundLayers[2]->renderBG2(palette);
                backgroundLayers[2]->draw();

                backgroundLayers[3]->renderBG2(palette);
                backgroundLayers[3]->draw();
                break;
            case 3:
                backgroundLayers[2]->renderBG3(memory);
                backgroundLayers[2]->draw();
                break;
            case 4:
                backgroundLayers[2]->renderBG4(palette, memory);
                backgroundLayers[2]->draw();
                break;
            case 5:
                backgroundLayers[2]->renderBG5(palette, memory);
                backgroundLayers[2]->draw();
                break;

            default:
                LOG_LCD(std::cout << "WARNING: Unsupported background mode " << std::dec << bgMode << "!\n";);
                break;
        }

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);

        objLayers[0]->draw(palette, use2dMapping);
        objLayers[1]->draw(palette, use2dMapping);
        objLayers[2]->draw(palette, use2dMapping);
        objLayers[3]->draw(palette, use2dMapping);

        drawToTarget();
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
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.vBlanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.hBlanking));
        //stat = bitSet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET, counters.vCount);
        regsRef.DISPSTAT = le(stat);

        /* update vcount */
        uint16_t vcount = le(regsRef.VCOUNT);
        vcount = bitSet(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, counters.vCount);
        regsRef.VCOUNT = le(vcount);

        ++counters.cycle;

        return result;
    }

    void LCDController::exitThread()
    {
        if (!renderThread->joinable())
            return;

        LOG_LCD(std::cout << "Waiting for render thread to exit." << std::endl;);
        renderControlMutex.lock();
        renderControl = EXIT;
        renderControlMutex.unlock();
        renderThread->join();
        LOG_LCD(std::cout << "Render thread exited." << std::endl;);
    }
} // namespace gbaemu::lcd
