#include "objlayer.hpp"


namespace gbaemu::lcd
{
    OBJAttribute OBJLayer::getAttribute(uint32_t index) const
    {
        auto uints = reinterpret_cast<const uint16_t *>(attributes + (index * 0x8));
        return OBJAttribute{le(uints[0]), le(uints[1]), le(uints[2])};
    }

    std::tuple<common::math::vec<2>, common::math::vec<2>> OBJLayer::getRotScaleParameters(uint32_t index) const
    {
        const uint16_t *uints = reinterpret_cast<const uint16_t *>(attributes);
        uint32_t group = index * 4 * 4;

        uint16_t a = le(uints[group +      3]);
        uint16_t c = le(uints[group + 8  + 3]);
        uint16_t b = le(uints[group + 4  + 3]);
        uint16_t d = le(uints[group + 12 + 3]);

        return std::make_tuple<common::math::vec<2>, common::math::vec<2>>(
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(a),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(c)},
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(b),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(d)});
    }

    OBJLayer::OBJLayer(LayerId layerId) : Layer(layerId)
#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
    , debugCanvas(512, 512)
#endif
    {
        enabled = true;
    }

    OBJInfo OBJLayer::getOBJInfo(uint32_t objIndex) const
    {
        OBJInfo info;

        OBJAttribute attr = getAttribute(objIndex);
        info.priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));

        bool useRotScale = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::ROT_SCALE_MASK, OBJ_ATTRIBUTE::ROT_SCALE_OFFSET);

        if (!useRotScale) {
            bool disabled = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DISABLE_MASK, OBJ_ATTRIBUTE::DISABLE_OFFSET);

            if (disabled)
                return info;

            info.vFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::V_FLIP_MASK, OBJ_ATTRIBUTE::V_FLIP_OFFSET);
            info.hFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::H_FLIP_MASK, OBJ_ATTRIBUTE::H_FLIP_OFFSET);
        } else {
        }

        /* 16x16 palette */
        info.useColor256 = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::COLOR_PALETTE_MASK, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET);

        info.yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
        info.xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);
        info.xOff = signExt<int32_t, uint16_t, 9>(info.xOff);

        OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
        uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
        info.tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);
        info.mosaic = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MOSAIC_MASK, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET);

        if (info.useColor256)
            info.tileNumber /= 2;

        bool useMosaic = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MOSAIC_MASK, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET);
        OBJMode mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

        if (3 <= bgMode && bgMode <= 5 && info.tileNumber < 512)
            return info;

        /*
            Size  Square   Horizontal  Vertical
            0     8x8      16x8        8x16
            1     16x16    32x8        8x32
            2     32x32    32x16       16x32
            3     64x64    64x32       32x64
            */

        /* TODO: Maybe there is a better way to do this. */
        switch (shape) {
            case SQUARE:
                if (size == 0) {
                    info.width = 8;
                    info.height = 8;
                } else if (size == 1) {
                    info.width = 16;
                    info.height = 16;
                } else if (size == 2) {
                    info.width = 32;
                    info.height = 32;
                } else {
                    info.width = 64;
                    info.height = 64;
                }

                break;
            case HORIZONTAL:
                if (size == 0) {
                    info.width = 16;
                    info.height = 8;
                } else if (size == 1) {
                    info.width = 32;
                    info.height = 8;
                } else if (size == 2) {
                    info.width = 32;
                    info.height = 16;
                } else {
                    info.width = 64;
                    info.height = 32;
                }

                break;
            case VERTICAL:
                if (size == 0) {
                    info.width = 8;
                    info.height = 16;
                } else if (size == 1) {
                    info.width = 8;
                    info.height = 32;
                } else if (size == 2) {
                    info.width = 16;
                    info.height = 32;
                } else {
                    info.width = 32;
                    info.height = 64;
                }

                break;
        }

        info.paletteNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PALETTE_NUMBER_MASK, OBJ_ATTRIBUTE::PALETTE_NUMBER_OFFSET);
        info.tilesPerRow = info.useColor256 ? 16 : 32;
        info.bytesPerTile = info.useColor256 ? 64 : 32;
        info.doubleSized = useRotScale && bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DOUBLE_SIZE_MASK, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET);

        if (useRotScale) {
            uint16_t index = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::ROT_SCALE_PARAM_MASK, OBJ_ATTRIBUTE::ROT_SCALE_PARAM_OFFSET);
            auto result = getRotScaleParameters(index);

            info.d = std::get<0>(result);
            info.dm = std::get<1>(result);
        }

        info.origin[0] = static_cast<common::math::real_t>(info.width) / 2;
        info.origin[1] = static_cast<common::math::real_t>(info.height) / 2;

        if (info.doubleSized) {
            info.screenRef[0] = static_cast<common::math::real_t>(info.xOff + info.width);
            info.screenRef[1] = static_cast<common::math::real_t>(info.yOff + info.height);
        } else {
            info.screenRef[0] = static_cast<common::math::real_t>(info.xOff + info.width / 2);
            info.screenRef[1] = static_cast<common::math::real_t>(info.yOff + info.height / 2);
        }

        if (info.yOff + info.height * (info.doubleSized ? 2 : 1) > 256)
            info.yOff -= 256;

        info.visible = true;
        return info;
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

#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
        int32_t locationX = 0;
        int32_t locationY = 0;
#endif

        for (int32_t i = 127; i >= 0; --i) {
            OBJInfo info = getOBJInfo(i);

            /* This sprite does not belong on this layer or is not visible. */
            if (info.priority != this->priority)
                continue;

            std::fill_n(tempBuffer.begin(), 64 * 64, 0);

            for (uint32_t tileY = 0; tileY < info.height / 8; ++tileY) {
                for (uint32_t tileX = 0; tileX < info.width / 8; ++tileX) {
                    /* This is where our tile data lies. */
                    const void *tilePtr;
                    size_t tileIndex;

                    uint32_t flippedTileX = info.hFlip ? (info.width / 8 - 1 - tileX) : tileX;
                    uint32_t flippedTileY = info.vFlip ? (info.height / 8 - 1 - tileY) : tileY;

                    uint32_t tilePosX = tileX * 8;
                    uint32_t tilePosY = tileY * 8;

                    if (use2dMapping)
                        tileIndex = info.tileNumber + flippedTileX + flippedTileY * info.tilesPerRow;
                    else
                        tileIndex = info.tileNumber + flippedTileX + flippedTileY * (info.width / 8);

                    tilePtr = objTiles + tileIndex * info.bytesPerTile;

                    if (!info.useColor256) {
                        const uint32_t *tile = reinterpret_cast<const uint32_t *>(tilePtr);

                        for (uint32_t py = 0; py < 8; ++py) {
                            uint32_t ty = info.vFlip ? (7 - py) : py;
                            uint32_t row = tile[ty];

                            for (uint32_t px = 0; px < 8; ++px) {
                                uint32_t tx = info.hFlip ? (7 - px) : px;

                                uint32_t paletteIndex = (row & (0xF << (tx * 4))) >> (tx * 4);
                                color_t color = palette.getObjColor(info.paletteNumber, paletteIndex);

#if RENDERER_HIGHTLIGHT_OBJ == 0
                                tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = color;
#else
                                if (i == hightlightObjIndex)
                                    tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = OBJ_HIGHLIGHT_COLOR;
                                else
                                    tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = color;
#endif
                            }
                        }
                    } else {
                        const uint8_t *tile = reinterpret_cast<const uint8_t *>(tilePtr);

                        for (uint32_t py = 0; py < 8; ++py) {
                            uint32_t ty = info.vFlip ? (7 - py) : py;

                            for (uint32_t px = 0; px < 8; ++px) {
                                uint32_t tx = info.hFlip ? (7 - px) : px;
                                color_t color = palette.getObjColor(tile[ty * 8 + tx]);

#if RENDERER_HIGHTLIGHT_OBJ == 0
                                tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = color;
#else
                                if (i == hightlightObjIndex)
                                    tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = OBJ_HIGHLIGHT_COLOR;
                                else
                                    tempBuffer[(tilePosY + py) * 64 + (tilePosX + px)] = color;
#endif
                            }
                        }
                    }
                }
            }

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

#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
            {
                locationX = (i % 32) * 8;
                locationY = (i / 32) * 8;

                for (int32_t y = 0; y < height; ++y) {
                    for (int32_t x = 0; x < width; ++x) {
                        color_t color = tempBuffer[y * 64 + x];
                        debugCanvas.pixels()[(y + locationY) * debugCanvas.getWidth() + (x + locationX)] = color;
                    }
                }
            }
#endif

            /* 0 = highest priority */
            canvas.drawSprite(tempBuffer.data(), info.width, info.height, 64,
                              info.origin, info.d, info.dm, info.screenRef);
        }
    }
}