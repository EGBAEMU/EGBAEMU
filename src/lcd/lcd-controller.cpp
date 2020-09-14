#include "lcd-controller.hpp"

#include <util.hpp>
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
        return getObjColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getBackdropColor() const
    {
        return toR8G8B8(bgPalette[0]);
    }

    /* OBJLayer */

    OBJLayer::OBJLayer()
    {
        for (size_t i = 0; i < layers.size(); ++i)
            layers[i] = std::make_unique<MemoryCanvas<color_t>>(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    void OBJLayer::setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t mode)
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
            case 5:
                objTiles = vramBaseAddress + 0x14000;
                areaSize = 16 * 1024;
                break;
        }

        attributes = oamBaseAddress;
    }

    OBJLayer::OBJAttribute *OBJLayer::accessAttribute(uint32_t index)
    {
        return reinterpret_cast<OBJAttribute *>(attributes + (index * 0x8));
    }

    OBJLayer::OBJAttribute OBJLayer::getAttribute(uint32_t index)
    {
        auto uints = reinterpret_cast<const uint16_t *>(attributes + (index * 0x8));
        return OBJAttribute{le(uints[0]), le(uints[1]), le(uints[2])};
    }

    std::tuple<common::math::vec<2>, common::math::vec<2>> OBJLayer::getRotScaleParameters(uint32_t index)
    {
        const uint16_t *uints = reinterpret_cast<const uint16_t *>(attributes);

        return std::make_tuple<common::math::vec<2>, common::math::vec<2>>(
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[index       * 4 + 3])),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 2) * 4 + 3]))
            },
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 1) * 4 + 3])),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(le(uints[(index + 3) * 4 + 3]))
            }
        );
    }

    void OBJLayer::draw(LCDColorPalette &palette, bool use2dMapping)
    {
        typedef common::math::real_t real_t;
        std::array<color_t, 64 * 64> tempBuffer;

        for (size_t i = 0; i < layers.size(); ++i)
            layers[i]->clear(0x0);

        for (int32_t i = 127; i >= 0; --i) {
            OBJAttribute attr = getAttribute(i);
            uint16_t priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));
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
            bool usePalette16 = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::COLOR_PALETTE_MASK, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET);

            uint16_t yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
            uint16_t xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);

            OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0],
                                                                    OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
            uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
            uint16_t tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);
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

            /* draw all tiles */

            /*
            tileNumber = i * 1;
            xOff = (i % 8) * 8;
            yOff = (i / 8) * 8;
            usePalette16 = true;
            width = 8;
            height = 8;

            if (i >= 128)
                return;
             */

            /* I don't know why this is inverted... */
            if (!usePalette16) {
                uint16_t paletteNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PALETTE_NUMBER_MASK, OBJ_ATTRIBUTE::PALETTE_NUMBER_OFFSET);
                uint32_t *firstTile = reinterpret_cast<uint32_t *>(objTiles + (tileNumber * 64));

                for (uint32_t y = 0; y < height; ++y) {
                    uint32_t row = firstTile[y];

                    for (uint32_t x = 0; x < width; ++x) {
                        uint32_t index = (row & (0xF << (7 - x))) >> (7 - x);
                        uint32_t color = palette.getBgColor(paletteNumber, index);
                        tempBuffer[y * 64 + x] = color;
                    }
                }
            } else {
                for (uint32_t tileY = 0; tileY < height / 8; ++tileY) {
                    for (uint32_t tileX = 0; tileX < width / 8; ++tileX) {
                        uint32_t tempYOff = tileY * 8;
                        uint32_t tempXOff = tileX * 8;

                        uint8_t *firstTile;

                        /*
                            This is actually not what the documentation says. Actually the first bit of tileNumber
                            should be ignored. Ignoring = dividing by 2?!
                         */

                        if (use2dMapping) {
                            firstTile = objTiles + ((tileNumber / 2 + tileX + tileY * 32) * 64);
                        } else {
                            firstTile = objTiles + ((tileNumber / 2 + tileX + (tileY * (width / 8))) * 64);
                        }

                        for (uint32_t py = 0; py < 8; ++py) {
                            for (uint32_t px = 0; px < 8; ++px) {
                                uint32_t paletteIndex = firstTile[py * 8 + px];
                                uint32_t color = palette.getObjColor(paletteIndex);
                                tempBuffer[(tempYOff + py) * 64 + (tempXOff + px)] = color;
                            }
                        }
                    }
                }
            }

            /* get parameters */
            common::math::vec<2> d{1, 0}, dm{0, 1};
            bool doubleSized =  useRotScale && bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DOUBLE_SIZE_MASK, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET);

            if (useRotScale) {
                uint16_t index = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::ROT_SCALE_PARAM_MASK, OBJ_ATTRIBUTE::ROT_SCALE_PARAM_OFFSET);
                auto result = getRotScaleParameters(index);

                d = std::get<0>(result);
                dm = std::get<1>(result);
            }

            common::math::vec<2> origin{
                static_cast<real_t>(width) / 2,
                static_cast<real_t>(height) / 2
            };

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
                static_cast<real_t>(yOff + height / 2)
            };

            if (doubleSized) {
                screenRef[0] = static_cast<real_t>(xOff + width);
                screenRef[1] = static_cast<real_t>(yOff + height);
            }

            /*
            display.canvas.drawSprite(tempBuffer, width, height, 64,
                origin, d, dm, screenRef);
             */

            /* 0 = highest priority */
            layers[3 - priority]->drawSprite(tempBuffer.data(), width, height, 64,
                origin, d, dm, screenRef);
        }
    }

    void Background::loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory)
    {
        id = static_cast<LayerId>(bgIndex);

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
            step.origin[0] = le(regs.BGOFS[bgIndex].h) & 0x1F;
            step.origin[1] = le(regs.BGOFS[bgIndex].v) & 0x1F;

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
                            uint8_t colorIndex = tile[srcTy * 8 + srcTx];
                            uint32_t color = palette.getBgColor(colorIndex);
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
        auto pixels = tempCanvas.pixels();
        auto stride = tempCanvas.getWidth();
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

    void Background::draw(color_t clearColor)
    {
        canvas.clear(clearColor);
        const common::math::vec<2> screenRef{0, 0};
        canvas.drawSprite(tempCanvas.pixels(), tempCanvas.getWidth(), tempCanvas.getHeight(), tempCanvas.getWidth(),
            step.origin, step.d, step.dm, screenRef);
    }

    /*
        LCDController
     */

    void LCDController::onHBlank()
    {
        updateReferences();

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        Memory::MemoryRegion region;
        uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, region);
        uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, region);

        /* obj layer */
        objLayer.setMode(vramBase, oamBase, bgMode);

        /* reset id's */
        for (int32_t i = 0; i < 4; ++i)
            backgrounds[i]->id = static_cast<Layer::LayerId>(i);

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgrounds[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

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
        std::sort(backgrounds.begin(), backgrounds.end(), [](const std::unique_ptr<Background> &b1, const std::unique_ptr<Background> &b2) -> bool {
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

    void LCDController::copyLayer(const Canvas<color_t>& src)
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
                color_t finalColor = 0xFF000000;

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
                for (int32_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled)
                        finalColor = backgrounds[i]->canvas.pixels()[y * SCREEN_WIDTH + x];

                    color_t objColor = objLayer.layers[i]->pixels()[y * SCREEN_WIDTH + x];

                    if (objColor & 0xFF000000)
                        finalColor = objColor;
                }
#endif

                dst[y * w + x] = finalColor;
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
                    if (backgrounds[i]->enabled) {
                        backgrounds[i]->renderBG0(palette);
                        backgrounds[i]->draw(clearColor);
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
                        backgrounds[i]->draw(clearColor);
                    } else if (backgrounds[i]->id == 2) {
                        backgrounds[i]->renderBG2(palette);
                        backgrounds[i]->draw(clearColor);
                    }
                }

                break;
            case 2:
                for (size_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled && 2 <= backgrounds[i]->id && backgrounds[i]->id <= 3) {
                        backgrounds[i]->renderBG2(palette);
                        backgrounds[i]->draw(clearColor);
                    }
                }

                break;
            case 3:
                for (size_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled && backgrounds[i]->id == 2) {
                        backgrounds[i]->renderBG3(memory);
                        backgrounds[i]->draw(clearColor);
                    }
                }

                break;
            case 4:
                for (size_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled && backgrounds[i]->id == 2) {
                        backgrounds[i]->renderBG4(palette, memory);
                        backgrounds[i]->draw(clearColor);
                    }
                }

                break;
            case 5:
                for (size_t i = 0; i < 4; ++i) {
                    if (backgrounds[i]->enabled && backgrounds[i]->id == 2) {
                        backgrounds[i]->renderBG5(palette, memory);
                        backgrounds[i]->draw(clearColor);
                    }
                }

                break;

            default:
                std::cout << "WARNING: Unsupported background mode " << std::dec << bgMode << "!\n";
                break;
        }

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);
        objLayer.draw(palette, use2dMapping);

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

    void LCDController::exitThread()
    {
        if (!renderThread->joinable())
            return;

        renderControlMutex.lock();
        renderControl = EXIT;
        renderControlMutex.unlock();
        renderThread->join();
    }
} // namespace gbaemu::lcd
