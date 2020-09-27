#include "objlayer.hpp"

#include <sstream>
#include <array>

namespace gbaemu::lcd
{
    OBJAttribute OBJ::getAttribute(const uint8_t *attributes, uint32_t index)
    {
        auto uints = reinterpret_cast<const uint16_t *>(attributes + (index * 0x8));
        return OBJAttribute{le(uints[0]), le(uints[1]), le(uints[2])};
    }
    
    std::tuple<common::math::vec<2>, common::math::vec<2>> OBJ::getRotScaleParameters(const uint8_t *attributes, uint32_t index)
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

    OBJ::OBJ(const uint8_t *attributes, uint16_t prioFilter, int32_t index)
    {
        /* by default */
        visible = false;
        objIndex = index;

        OBJAttribute attr = getAttribute(attributes, objIndex);
        priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));

        /* this OBJ does not belong on the current layer */
        if (priority != prioFilter)
            return;

        bool useRotScale = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::ROT_SCALE_MASK, OBJ_ATTRIBUTE::ROT_SCALE_OFFSET);

        if (!useRotScale) {
            bool disabled = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DISABLE_MASK, OBJ_ATTRIBUTE::DISABLE_OFFSET);

            if (disabled)
                return;

            vFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::V_FLIP_MASK, OBJ_ATTRIBUTE::V_FLIP_OFFSET);
            hFlip = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::H_FLIP_MASK, OBJ_ATTRIBUTE::H_FLIP_OFFSET);
        } else {
        }

        /* 16x16 palette */
        useColor256 = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::COLOR_PALETTE_MASK, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET);

        yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
        xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);
        xOff = signExt<int32_t, uint16_t, 9>(xOff);

        OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
        uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
        tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);
        mosaicEnabled = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MOSAIC_MASK, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET);
        mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

        if (useColor256)
            tileNumber /= 2;

        bool useMosaic = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MOSAIC_MASK, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET);
        OBJMode mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

        if (3 <= mode && mode <= 5 && tileNumber < 512)
            return;

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

        paletteNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PALETTE_NUMBER_MASK, OBJ_ATTRIBUTE::PALETTE_NUMBER_OFFSET);
        tilesPerRow = useColor256 ? 16 : 32;
        bytesPerTile = useColor256 ? 64 : 32;
        doubleSized = useRotScale && bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::DOUBLE_SIZE_MASK, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET);

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

        assert(affineTransform.d[0] != 0 || affineTransform.d[1] != 0);

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

        visible = true;
    }

    std::string OBJ::toString() const
    {
        std::stringstream ss;

        ss << "visible: " << (visible ? "yes" : "no") << '\n';
        ss << "obj index: " << std::dec << objIndex << '\n';
        ss << "xy off: " << xOff << ' ' << yOff << '\n';
        ss << "double sized: " << (doubleSized ? "yes" : "no") << '\n';
        ss << "width height: " << width << 'x' << height << '\n';
        ss << "origin: " << affineTransform.origin << '\n';
        ss << "screen ref: " << affineTransform.screenRef << '\n';
        ss << "d dm: " << affineTransform.d << ' ' << affineTransform.dm << '\n';

        return ss.str();
    }

    color_t OBJ::pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, LCDColorPalette& palette, bool use2dMapping) const
    {
        /* calculating index */
        int32_t tileX = sx / 8;
        int32_t tileY = sy / 8;

        int32_t flippedTileX = hFlip ? (width / 8 - 1 - tileX) : tileX;
        int32_t flippedTileY = vFlip ? (height / 8 - 1 - tileY) : tileY;

        uint32_t tileIndex;

         if (use2dMapping)
            tileIndex = tileNumber + flippedTileX + flippedTileY * tilesPerRow;
        else
            tileIndex = tileNumber + flippedTileX + flippedTileY * (width / 8);

        /* finding the actual tile */
        const uint8_t *tile = objTiles + tileIndex * bytesPerTile;
        int32_t tx = hFlip ? (7 - (sx % 8)) : (sx % 8);
        int32_t ty = vFlip ? (7 - (sy % 8)) : (sy % 8);

        if (useColor256) {
            uint32_t paletteIndex = tile[ty * 8 + tx];
            return palette.getObjColor(paletteIndex);
        } else {
            uint32_t row = reinterpret_cast<const uint32_t *>(tile)[ty];
            uint32_t paletteIndex = (row >> (tx * 4)) & 0xF;
            return palette.getObjColor(paletteNumber, paletteIndex);
        }
    }

    OBJLayer::OBJLayer(Memory& mem, LCDColorPalette& plt, const LCDIORegs& ioRegs, uint16_t prio) : memory(mem), palette(plt), regs(ioRegs), objects(128)
    {
        /* OBJ layers are always enabled */
        enabled = true;
        priority = prio;
        objects.reserve(128);

        if (prio >= 4)
            throw new std::runtime_error("0 <= Priority <= 3 is not satisfied");

        layerID = static_cast<LayerID>(priority + 4);
        isBGLayer = false;
    }

    void OBJLayer::setMode(BGMode bgMode, bool mapping2d)
    {
        mode = bgMode;

        Memory::MemoryRegion memReg;
        const uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, memReg);
        const uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, memReg);

        switch (mode) {
            case Mode0: case Mode1: case Mode2:
                objTiles = vramBase + 0x10000;
                areaSize = 32 * 1024;
                break;
            case Mode3: case Mode4:
                objTiles = vramBase + 0x14000;
                areaSize = 16 * 1024;
                break;
        }

        attributes = oamBase;
        use2dMapping = mapping2d;
        mosaicWidth = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_HSIZE_MASK, MOSAIC::OBJ_MOSAIC_HSIZE_OFFSET);
        mosaicHeight = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_VSIZE_MASK, MOSAIC::OBJ_MOSAIC_VSIZE_OFFSET);
    }

    void OBJLayer::loadOBJs()
    {
        objects.resize(0);

        for (int32_t objIndex = 0; objIndex < 128; ++objIndex) {
            OBJ obj(attributes, priority, objIndex);

            if (obj.visible)
                objects.push_back(obj);
        }

        asFirstTarget = bitGet<uint16_t>(le(regs.BLDCNT), BLDCNT::TARGET_MASK, BLDCNT::OBJ_FIRST_TARGET_OFFSET);
        asSecondTarget = bitGet<uint16_t>(le(regs.BLDCNT), BLDCNT::TARGET_MASK, BLDCNT::OBJ_SECOND_TARGET_OFFSET);
    }

    void OBJLayer::drawScanline(int32_t y)
    {
        for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
            /* clear */
            scanline[x] = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false);

            /* iterate over the objects beginning with OBJ0 (on top) */
            for (auto obj = objects.cbegin(); obj != objects.cend(); ++obj) {
                if (!obj->visible)
                    continue;

                vec2 s = obj->affineTransform.d * (x - obj->affineTransform.screenRef[0]) +
                    obj->affineTransform.dm * (y - obj->affineTransform.screenRef[1]) +
                    obj->affineTransform.origin;

                int32_t sx = static_cast<int32_t>(s[0]);
                int32_t sy = static_cast<int32_t>(s[1]);

                int32_t msx = obj->mosaicEnabled ? (sx - (sx % mosaicWidth)) : sx;
                int32_t msy = obj->mosaicEnabled ? (sy - (sy % mosaicHeight)) : sy;

                if (0 <= msx && msx < obj->width && 0 <= msy && msy < obj->height) {
                    color_t color = obj->pixelColor(msx, msy, objTiles, palette, use2dMapping);

                    if (color == TRANSPARENT)
                        continue;

                    /* if the color is not transparent it's the final color */
                    scanline[x] = Fragment(color, asFirstTarget, asSecondTarget, (obj->mode == SEMI_TRANSPARENT));
                    break;
                }
            }
        }
    }

    std::string OBJLayer::toString() const
    {
        std::stringstream ss;

        for (int32_t i = 0; i < objects.size(); ++i) {
            ss << objects[i].toString() << '\n';
        }

        return ss.str();
    }
}