#include "obj.hpp"

#include <sstream>

namespace gbaemu::lcd {
    void OBJ::writeAndDecode16(uint8_t offset, uint16_t value) {
        //TODO
    }
    OBJAttribute OBJ::getAttribute(const uint8_t *attributes, uint32_t index)
    {
        auto uints = reinterpret_cast<const uint16_t *>(attributes + (index * 0x8));
        return OBJAttribute{le(uints[0]), le(uints[1]), le(uints[2])};
    }

    std::tuple<common::math::vec<2>, common::math::vec<2>> OBJ::getRotScaleParameters(const uint8_t *attributes, uint32_t index)
    {
        const uint16_t *uints = reinterpret_cast<const uint16_t *>(attributes);
        uint32_t group = index * 4 * 4;

        uint16_t a = le(uints[group + 3]);
        uint16_t c = le(uints[group + 8 + 3]);
        uint16_t b = le(uints[group + 4 + 3]);
        uint16_t d = le(uints[group + 12 + 3]);

        return std::make_tuple<common::math::vec<2>, common::math::vec<2>>(
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(a),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(c)},
            common::math::vec<2>{
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(b),
                fixedToFloat<uint16_t, 8, 7, common::math::real_t>(d)});
    }

    std::tuple<common::math::vect<2, int32_t>, common::math::vect<2, int32_t>> OBJ::getRotScaleParametersI(const uint8_t *attributes, uint32_t index)
    {
        const uint16_t *uints = reinterpret_cast<const uint16_t *>(attributes);
        uint32_t group = index * 4 * 4;

        uint16_t a = le(uints[group + 3]);
        uint16_t c = le(uints[group + 8 + 3]);
        uint16_t b = le(uints[group + 4 + 3]);
        uint16_t d = le(uints[group + 12 + 3]);

        return std::make_tuple<common::math::vect<2, int32_t>, common::math::vect<2, int32_t>>(
            common::math::vect<2, int32_t>{
                signExt<int32_t, uint16_t, 16>(a),
                signExt<int32_t, uint16_t, 16>(c)},
            common::math::vect<2, int32_t>{
                signExt<int32_t, uint16_t, 16>(b),
                signExt<int32_t, uint16_t, 16>(d)});
    }

    OBJ::OBJ(const uint8_t *attributes, int32_t index, BGMode bgMode)
    {
        /* by default */
        visible = false;

        const OBJAttribute attr = getAttribute(attributes, index);
        priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));

        /* this OBJ does not belong on the current layer */
        //if (priority != prioFilter)
        //    return;

        bool useRotScale = isBitSet<uint16_t, OBJ_ATTRIBUTE::ROT_SCALE_OFFSET>(attr.attribute[0]);

        if (!useRotScale) {
            bool enabled = !isBitSet<uint16_t, OBJ_ATTRIBUTE::DISABLE_OFFSET>(attr.attribute[0]);

            if (!enabled)
                return;

            vFlip = isBitSet<uint16_t, OBJ_ATTRIBUTE::V_FLIP_OFFSET>(attr.attribute[1]);
            hFlip = isBitSet<uint16_t, OBJ_ATTRIBUTE::H_FLIP_OFFSET>(attr.attribute[1]);
        }

        /* 16x16 palette */
        useColor256 = isBitSet<uint16_t, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET>(attr.attribute[0]);

        tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);

        if (useColor256) {
            tileNumber /= 2;
            // tilesPerRow = 16;
            tilesPerRowShift = 4;
            // bytesPerTile = 64;
            bytesPerTileShift = 6;
        } else {
            // tilesPerRow = 32;
            tilesPerRowShift = 5;
            // bytesPerTile = 32;
            bytesPerTileShift = 5;
        }

        /* bitmap modes */
        if (Mode3 <= bgMode && bgMode <= Mode5 && tileNumber < 512)
            return;

        visible = true;

        yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
        xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);
        xOff = signExt<int32_t, uint16_t, 9>(xOff);

        OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
        uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
        mosaicEnabled = isBitSet<uint16_t, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET>(attr.attribute[0]);
        mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

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
                width = height = 8 << size;
                break;
            case HORIZONTAL:
                width = 16 << ((size + 1) >> 1);
                height = 8 << (size >> 1);
                break;
            case VERTICAL:
                // as in HORIZONTAL but with changed variables
                height = 16 << ((size + 1) >> 1);
                width = 8 << (size >> 1);
                break;
        }

        paletteNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PALETTE_NUMBER_MASK, OBJ_ATTRIBUTE::PALETTE_NUMBER_OFFSET);
        bool doubleSized = useRotScale && isBitSet<uint16_t, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET>(attr.attribute[0]);

        if (useRotScale) {
            uint16_t index = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::ROT_SCALE_PARAM_MASK, OBJ_ATTRIBUTE::ROT_SCALE_PARAM_OFFSET);
            auto result = getRotScaleParameters(attributes, index);

            affineTransform.d = std::get<0>(result);
            affineTransform.dm = std::get<1>(result);
        } else {
            affineTransform.d[0] = 1;
            affineTransform.d[1] = 0;
            affineTransform.dm[0] = 0;
            affineTransform.dm[1] = 1;
        }

        affineTransform.origin[0] = static_cast<common::math::real_t>(width) / 2;
        affineTransform.origin[1] = static_cast<common::math::real_t>(height) / 2;

        if (yOff + height * (doubleSized ? 2 : 1) > 256)
            yOff -= 256;

        if (doubleSized) {
            affineTransform.screenRef[0] = static_cast<common::math::real_t>(xOff + static_cast<int32_t>(width));
            affineTransform.screenRef[1] = static_cast<common::math::real_t>(yOff + static_cast<int32_t>(height));
        } else {
            affineTransform.screenRef[0] = static_cast<common::math::real_t>(xOff + static_cast<int32_t>(width) / 2);
            affineTransform.screenRef[1] = static_cast<common::math::real_t>(yOff + static_cast<int32_t>(height) / 2);
        }

        rect.left = xOff;
        rect.top = yOff;
        rect.right = xOff + width * (doubleSized ? 2 : 1);
        rect.bottom = yOff + height * (doubleSized ? 2 : 1);
    }

    std::string OBJ::toString() const
    {
        std::stringstream ss;

        return ss.str();
    }

    color_t OBJ::pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, const LCDColorPalette &palette, bool use2dMapping) const
    {
        /* calculating index */
        const int32_t tileX = sx / 8;
        const int32_t tileY = sy / 8;

        const int32_t flippedTileX = hFlip ? (width / 8 - 1 - tileX) : tileX;
        const int32_t flippedTileY = vFlip ? (height / 8 - 1 - tileY) : tileY;

        //const uint32_t tileIndex = use2dMapping ? (tileNumber + flippedTileX + flippedTileY * tilesPerRow) : (tileNumber + flippedTileX + flippedTileY * (width / 8));
        const uint32_t tileIndex = use2dMapping ? (tileNumber + flippedTileX + (flippedTileY << tilesPerRowShift)) : (tileNumber + flippedTileX + flippedTileY * (width / 8));

        /* finding the actual tile */
        // const uint8_t *tile = objTiles + tileIndex * bytesPerTile;
        const uint8_t *tile = objTiles + (tileIndex << bytesPerTileShift);
        const int32_t tx = hFlip ? (7 - (sx % 8)) : (sx % 8);
        const int32_t ty = vFlip ? (7 - (sy % 8)) : (sy % 8);

        if (useColor256) {
            uint32_t paletteIndex = tile[ty * 8 + tx];
            return (mode == OBJ_WINDOW) ? (paletteIndex == 0 ? TRANSPARENT : BLACK) : palette.getObjColor(paletteIndex);
        } else {
            uint32_t row = reinterpret_cast<const uint32_t *>(tile)[ty];
            uint32_t paletteIndex = (row >> (tx * 4)) & 0xF;
            return (mode == OBJ_WINDOW) ? (paletteIndex == 0 ? TRANSPARENT : BLACK) : palette.getObjColor(paletteNumber, paletteIndex);
        }
    }

    bool OBJ::intersectsWithScanline(real_t fy) const
    {
        /* check for screen rect */
        if (fy < rect.top || fy >= rect.bottom)
            return false;

        const vec2 temp = affineTransform.dm * (fy - affineTransform.screenRef[1]) + affineTransform.origin;

        //const vec2 s0 = affineTransform.d * (0 - affineTransform.screenRef[0]) + temp;
        //const vec2 s1 = affineTransform.d * (SCREEN_WIDTH - 1 - affineTransform.screenRef[0]) + temp;

        const vec2 s0 = affineTransform.d * (0 - affineTransform.screenRef[0]) +
                        affineTransform.dm * (fy - affineTransform.screenRef[1]) +
                        affineTransform.origin;
        const vec2 s1 = affineTransform.d * (SCREEN_WIDTH - 1 - affineTransform.screenRef[0]) +
                        affineTransform.dm * (fy - affineTransform.screenRef[1]) +
                        affineTransform.origin;

        const vec2 d = s1 - s0;
        const vec2 ortho{d[1], -d[0]};

        /* we give a single pixel margin */
        const real_t dots[] = {
            (vec2{-1, -1} - s0).dot(ortho),
            (vec2{static_cast<real_t>(width), -1} - s0).dot(ortho),
            (vec2{-1, static_cast<real_t>(height)} - s0).dot(ortho),
            (vec2{static_cast<real_t>(width), static_cast<real_t>(height)} - s0).dot(ortho)};

        const bool negDot = std::any_of(dots, dots + 4, [](real_t r) { return r <= 0; });
        const bool posDot = std::any_of(dots, dots + 4, [](real_t r) { return r >= 0; });

        return negDot && posDot;
    }
}