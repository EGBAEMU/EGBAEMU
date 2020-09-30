#include "objlayer.hpp"

#include <array>
#include <sstream>

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

    OBJ::OBJ(const uint8_t *attributes, int32_t index)
    {
        /* by default */
        visible = false;
        enabled = true;
        objIndex = index;

        const OBJAttribute attr = getAttribute(attributes, objIndex);
        priority = static_cast<uint16_t>(bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::PRIORITY_MASK, OBJ_ATTRIBUTE::PRIORITY_OFFSET));

        /* this OBJ does not belong on the current layer */
        //if (priority != prioFilter)
        //    return;

        bool useRotScale = isBitSet<uint16_t, OBJ_ATTRIBUTE::ROT_SCALE_OFFSET>(attr.attribute[0]);

        if (!useRotScale) {
            enabled = !isBitSet<uint16_t, OBJ_ATTRIBUTE::DISABLE_OFFSET>(attr.attribute[0]);

            if (!enabled)
                return;

            vFlip = isBitSet<uint16_t, OBJ_ATTRIBUTE::V_FLIP_OFFSET>(attr.attribute[1]);
            hFlip = isBitSet<uint16_t, OBJ_ATTRIBUTE::H_FLIP_OFFSET>(attr.attribute[1]);
        } else {
            enabled = true;
        }

        /* 16x16 palette */
        useColor256 = isBitSet<uint16_t, OBJ_ATTRIBUTE::COLOR_PALETTE_OFFSET>(attr.attribute[0]);

        yOff = bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::Y_COORD_MASK, OBJ_ATTRIBUTE::Y_COORD_OFFSET);
        xOff = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::X_COORD_MASK, OBJ_ATTRIBUTE::X_COORD_OFFSET);
        xOff = signExt<int32_t, uint16_t, 9>(xOff);

        OBJShape shape = static_cast<OBJShape>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_SHAPE_MASK, OBJ_ATTRIBUTE::OBJ_SHAPE_OFFSET));
        uint16_t size = bitGet<uint16_t>(attr.attribute[1], OBJ_ATTRIBUTE::OBJ_SIZE_MASK, OBJ_ATTRIBUTE::OBJ_SIZE_OFFSET);
        tileNumber = bitGet<uint16_t>(attr.attribute[2], OBJ_ATTRIBUTE::CHAR_NAME_MASK, OBJ_ATTRIBUTE::CHAR_NAME_OFFSET);
        mosaicEnabled = isBitSet<uint16_t, OBJ_ATTRIBUTE::OBJ_MOSAIC_OFFSET>(attr.attribute[0]);
        mode = static_cast<OBJMode>(bitGet<uint16_t>(attr.attribute[0], OBJ_ATTRIBUTE::OBJ_MODE_MASK, OBJ_ATTRIBUTE::OBJ_MODE_OFFSET));

        if (useColor256)
            tileNumber /= 2;

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
        doubleSized = useRotScale && isBitSet<uint16_t, OBJ_ATTRIBUTE::DOUBLE_SIZE_OFFSET>(attr.attribute[0]);

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

        //assert(affineTransform.d[0] != 0 || affineTransform.d[1] != 0);

        /*
        if (affineTransform.d[0] == 0 && affineTransform.d[1] == 0) {
            affineTransform.d[0] = 1;
            affineTransform.d[1] = 0;
        }

        if (affineTransform.dm[0] == 0 && affineTransform.dm[1] == 0) {
            affineTransform.dm[0] = 0;
            affineTransform.dm[1] = 1;
        }
         */

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

        cyclesRequired = width * height * (doubleSized || useRotScale ? 2 : 1) + (doubleSized || useRotScale ? 10 : 0);

        rect.left = xOff;
        rect.top = yOff;
        rect.right = xOff + width * (doubleSized ? 2 : 1);
        rect.bottom = yOff + height * (doubleSized ? 2 : 1);
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

    color_t OBJ::pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, const LCDColorPalette &palette, bool use2dMapping) const
    {
        /* calculating index */
        const int32_t tileX = sx / 8;
        const int32_t tileY = sy / 8;

        const int32_t flippedTileX = hFlip ? (width / 8 - 1 - tileX) : tileX;
        const int32_t flippedTileY = vFlip ? (height / 8 - 1 - tileY) : tileY;

        const uint32_t tileIndex = use2dMapping ? (tileNumber + flippedTileX + flippedTileY * tilesPerRow) : (tileNumber + flippedTileX + flippedTileY * (width / 8));

        /* finding the actual tile */
        const uint8_t *tile = objTiles + tileIndex * bytesPerTile;
        const int32_t tx = hFlip ? (7 - (sx % 8)) : (sx % 8);
        const int32_t ty = vFlip ? (7 - (sy % 8)) : (sy % 8);

        if (useColor256) {
            uint32_t paletteIndex = tile[ty * 8 + tx];
            return palette.getObjColor(paletteIndex);
        } else {
            uint32_t row = reinterpret_cast<const uint32_t *>(tile)[ty];
            uint32_t paletteIndex = (row >> (tx * 4)) & 0xF;
            return palette.getObjColor(paletteNumber, paletteIndex);
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

    std::vector<OBJ>::const_iterator OBJLayer::getLastRenderedOBJ(int32_t cycleBudget) const
    {
        auto it = objects.cbegin();
        int32_t usedCycles = 0;

        for (; it != objects.cend(); it++) {
            usedCycles += it->cyclesRequired;

            if (usedCycles > cycleBudget)
                break;
        }

        return it;
    }

    OBJManager::OBJManager() : allObjects(128)
    {
    }

    void OBJManager::loadOBJs(const uint8_t *attributes)
    {
        allObjects.resize(0);

        //for (auto& obj : objectsByPriority)
        //    obj.resize(0);

        for (int32_t objIndex = 0; objIndex < 128; ++objIndex) {
            OBJ obj(attributes, objIndex);

            if (!obj.enabled)
                continue;

            allObjects.push_back(obj);
            //objectsByPriority[obj.priority].push_back(obj);
        }
    }

    std::vector<OBJ>::const_iterator OBJManager::lastOBJ(int32_t y, int32_t cycleBudget) const
    {
        auto it = allObjects.cbegin();
        int32_t cyclesUsed = 0;

        for (; it != allObjects.cend(); it++) {
            if (!it->visible || !it->enabled || !it->intersectsWithScanline(y))
                continue;

            if (cyclesUsed += it->cyclesRequired > cycleBudget)
                break;
        }

        return it;
    }

    OBJLayer::OBJLayer(Memory &mem, LCDColorPalette &plt, const LCDIORegs &ioRegs, OBJManager &objMgr, uint16_t prio) : memory(mem), palette(plt), regs(ioRegs), objManager(objMgr), objects(128)
    {
        /* OBJ layers are always enabled */
        enabled = true;
        priority = prio;
        objects.reserve(128);

        if (prio >= 4)
            throw std::runtime_error("0 <= Priority <= 3 is not satisfied");

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
            case Mode0:
            case Mode1:
            case Mode2:
                objTiles = vramBase + 0x10000;
                areaSize = 32 * 1024;
                break;
            case Mode3:
            case Mode4:
                objTiles = vramBase + 0x14000;
                areaSize = 16 * 1024;
                break;
        }

        attributes = oamBase;
        use2dMapping = mapping2d;
        mosaicWidth = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_HSIZE_MASK, MOSAIC::OBJ_MOSAIC_HSIZE_OFFSET) + 1;
        mosaicHeight = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_VSIZE_MASK, MOSAIC::OBJ_MOSAIC_VSIZE_OFFSET) + 1;
    }

    void OBJLayer::loadOBJs(int32_t y)
    {
        real_t fy = static_cast<real_t>(y);
        objects.resize(0);

        for (int32_t objIndex = 0; objIndex < 128; ++objIndex) {
            OBJ obj(attributes, objIndex);

            if (obj.priority != priority || !obj.intersectsWithScanline(fy))
                continue;

            if (obj.visible)
                objects.push_back(obj);
        }

        //objManager.loadOBJs(attributes);
        asFirstTarget = isBitSet<uint16_t, BLDCNT::OBJ_FIRST_TARGET_OFFSET>(le(regs.BLDCNT));
        asSecondTarget = isBitSet<uint16_t, BLDCNT::OBJ_SECOND_TARGET_OFFSET>(le(regs.BLDCNT));
    }

    void OBJLayer::drawScanline(int32_t y)
    {
        const real_t fy = static_cast<real_t>(y);
        real_t fx = 0;

        /*
        std::vector<const OBJ *> onScanline;
        onScanline.reserve(128);
        for (size_t i = 0; i < objects.size(); ++i)
            if (objects[i].intersectsWithScanline(fy))
                onScanline.push_back(objects.data() + i);
         */

        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            /* clear */
            scanline[x] = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false);

            /* iterate over the objects beginning with OBJ0 (on top) */
            for (auto obj = objects.cbegin(); obj != objects.cend(); ++obj) {
                /* only the screen rectangle of that sprite is scanned */
                if (x < obj->rect.left || x >= obj->rect.right)
                    continue;

                //const auto obj = *pObj;
                const vec2 s = obj->affineTransform.d * (fx - obj->affineTransform.screenRef[0]) +
                               obj->affineTransform.dm * (fy - obj->affineTransform.screenRef[1]) +
                               obj->affineTransform.origin;

                //const common::math::vect<2, int32_t> s = obj->affineTransform.d * (x - obj->affineTransform.screenRef[0]) +
                //    obj->affineTransform.dm * (y - obj->affineTransform.screenRef[1]) +
                //    obj->affineTransform.origin;

                const int32_t sx = static_cast<int32_t>(s[0]);
                const int32_t sy = static_cast<int32_t>(s[1]);

                if (0 <= sx && sx < static_cast<int32_t>(obj->width) && 0 <= sy && sy < static_cast<int32_t>(obj->height)) {
                    const int32_t msx = obj->mosaicEnabled ? (sx - (sx % mosaicWidth)) : sx;
                    const int32_t msy = obj->mosaicEnabled ? (sy - (sy % mosaicHeight)) : sy;
                    const color_t color = obj->pixelColor(msx, msy, objTiles, palette, use2dMapping);

                    if (color == TRANSPARENT)
                        continue;

                    /* if the color is not transparent it's the final color */
                    scanline[x].color = color;
                    scanline[x].props |= (obj->mode == SEMI_TRANSPARENT) ? 4 : 0;
                    break;
                }
            }

            fx += 1.0;
        }
    }

    std::string OBJLayer::toString() const
    {
        std::stringstream ss;

        for (size_t i = 0; i < objects.size(); ++i) {
            ss << objects[i].toString() << '\n';
        }

        return ss.str();
    }
} // namespace gbaemu::lcd