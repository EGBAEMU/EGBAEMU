#ifndef OBJ_HPP
#define OBJ_HPP

#include "packed.h"

#include "defs.hpp"
#include "palette.hpp"

namespace gbaemu::lcd
{
    enum OBJShape : uint16_t {
        SQUARE = 0,
        HORIZONTAL,
        VERTICAL
    };

    enum OBJMode : uint16_t {
        NORMAL,
        SEMI_TRANSPARENT,
        OBJ_WINDOW
    };

    struct OBJAffineTransform {
        vec2 origin{0, 0};
        vec2 d{1, 0};
        vec2 dm{0, 1};
        vec2 screenRef{0, 0};
    };

    struct IOBJAffineTransform {
        common::math::vect<2, int32_t> origin{0, 0}, d{0x80, 0}, dm{0, 0x80}, screenRef{0, 0};
    };

    PACK_STRUCT_DEF(OBJAttribute,
                    uint16_t attribute[3];);

    struct OBJBitProps {
        uint8_t field;

        bool visible() const noexcept { return field & 1; }
        bool enabled() const noexcept { return field & 2; }
        bool vFlip() const noexcept { return field & 4; }
        bool hFlip() const noexcept { return field & 8; }
    };

    /* Contains all the properties for a given obj index. */
    class OBJ
    {
      public:
        bool vFlip = false, hFlip = false, useColor256, mosaicEnabled;
        uint32_t width = 0, height = 0;
        uint8_t paletteNumber;
        uint16_t tileNumber;
        uint8_t tilesPerRowShift;
        uint8_t bytesPerTileShift;

        bool visible = false;
        OBJMode mode;
        uint8_t priority;
        int32_t xOff, yOff;
        OBJAffineTransform affineTransform;
        Rect rect;

      private:
        static OBJAttribute getAttribute(const uint8_t *attributes, uint32_t index);
        static std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(const uint8_t *attributes, uint32_t index);
        static std::tuple<common::math::vect<2, int32_t>, common::math::vect<2, int32_t>> getRotScaleParametersI(const uint8_t *attributes, uint32_t index);

      public:
        OBJ(){};
        OBJ(const uint8_t *attributes, int32_t index, BGMode bgMode);
        std::string toString() const;
        color_t pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, const LCDColorPalette &palette, bool use2dMapping) const;
        bool intersectsWithScanline(real_t fy) const;
    };
} // namespace gbaemu::lcd

#endif