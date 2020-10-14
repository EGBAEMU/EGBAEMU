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

    /* Contains all the properties for a given obj index. */
    class OBJ
    {
      private:
        int32_t objIndex;

      public:
        bool visible = false;
        bool enabled;

        OBJShape shape;
        OBJMode mode;
        uint16_t priority;
        int32_t xOff, yOff;
        bool doubleSized, vFlip = false, hFlip = false, useColor256, mosaicEnabled;
        uint32_t width = 0, height = 0;
        uint16_t paletteNumber;
        uint16_t tileNumber;
        uint16_t tilesPerRow;
        uint16_t bytesPerTile;
        OBJAffineTransform affineTransform;
        int32_t cyclesRequired;

        struct
        {
            int32_t left;
            int32_t right;
            int32_t top;
            int32_t bottom;
        } rect;

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

        void writeAndDecode16(uint8_t offset, uint16_t value);
    };
} // namespace gbaemu::lcd

#endif