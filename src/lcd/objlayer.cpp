#include "objlayer.hpp"

#include <array>
#include <sstream>

namespace gbaemu::lcd
{
    OBJManager::OBJManager()
    {
        for (auto& obj : objects)
            obj.enabled = false;
    }

    void OBJManager::load(const uint8_t *attributes, BGMode bgMode)
    {
        for (int32_t i = 0; i < 128; ++i)
            objects[i] = OBJ(attributes, i, bgMode);
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

    OBJLayer::OBJLayer(Memory &mem, LCDColorPalette &plt, const LCDIORegs &ioRegs, uint16_t prio, const std::shared_ptr<OBJManager>& manager) :
        Layer(static_cast<LayerID>(prio + 4), false),
        memory(mem), palette(plt), regs(ioRegs), objects(128), objManager(manager)
    {
        /* OBJ layers are always enabled */
        enabled = true;
        priority = prio;
        objects.reserve(128);

        if (prio >= 4)
            throw std::runtime_error("0 <= Priority <= 3 is not satisfied");
    }

    void OBJLayer::setMode(BGMode bgMode, bool mapping2d)
    {
        mode = bgMode;

        const uint8_t *vramBase = memory.vram.rawAccess();
        const uint8_t *oamBase = memory.oam.mem;
        objTiles = vramBase + 0x10000;

        switch (mode) {
            case Mode0: case Mode1: case Mode2:
                areaSize = 32 * 1024;
                break;
            case Mode3: case Mode4:
                areaSize = 16 * 1024;
                break;
        }

        attributes = oamBase;
        use2dMapping = mapping2d;
        mosaicWidth = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_HSIZE_MASK, MOSAIC::OBJ_MOSAIC_HSIZE_OFFSET) + 1;
        mosaicHeight = bitGet(le(regs.MOSAIC), MOSAIC::OBJ_MOSAIC_VSIZE_MASK, MOSAIC::OBJ_MOSAIC_VSIZE_OFFSET) + 1;
    }

    void OBJLayer::loadOBJs(int32_t y, const std::function<bool(const OBJ&, real_t, uint16_t)>& filter)
    {
        real_t fy = static_cast<real_t>(y);
        objects.resize(0);

        for (const auto& obj : objManager->objects) {
            if (!filter(obj, fy, priority))
                continue;

            if (obj.visible)
                objects.push_back(obj);
        }

        asFirstTarget = isBitSet<uint16_t, BLDCNT::OBJ_FIRST_TARGET_OFFSET>(le(regs.BLDCNT));
        asSecondTarget = isBitSet<uint16_t, BLDCNT::OBJ_SECOND_TARGET_OFFSET>(le(regs.BLDCNT));
    }

    void OBJLayer::drawScanline(int32_t y)
    {
        const real_t fy = static_cast<real_t>(y);
        real_t fx = 0;

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
                    //obj->pixelColor(msx, msy, objTiles, palette, use2dMapping);

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