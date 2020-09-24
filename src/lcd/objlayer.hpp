#ifndef OBJLAYER_HPP
#define OBJLAYER_HPP

#include "palette.hpp"

#include <io/memory.hpp>


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

    struct OBJAffineTransform
    {
        vec2 origin{0, 0};
        vec2 d{1, 0};
        vec2 dm{0, 1};
        vec2 screenRef{0, 0};
    };

#include "packed.h"
    struct OBJAttribute {
        uint16_t attribute[3];
    } PACKED;
#include "endpacked.h"

    /* Contains all the properties for a given obj index. */
    class OBJ
    {
      private:
        int32_t objIndex;
      public:
        bool visible = false;
        
        OBJShape shape;
        uint16_t priority;
        int32_t xOff, yOff;
        bool doubleSized, vFlip = false, hFlip = false, useColor256, mosaic;
        uint32_t width = 0, height = 0;
        uint16_t paletteNumber;
        uint16_t tileNumber;
        uint16_t tilesPerRow;
        uint16_t bytesPerTile;
        OBJAffineTransform affineTransform;
      private:
        static OBJAttribute getAttribute(const uint8_t *attributes, uint32_t index);
        static std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(const uint8_t *attributes, uint32_t index);
      public:
        OBJ() = default;
        OBJ(const uint8_t *attributes, uint16_t prioFilter, int32_t index);
        std::string toString() const;
        color_t pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, LCDColorPalette& palette, bool use2dMapping) const;
    };

    class OBJLayer : public Layer
    {
      public:
        BGMode mode;
        /* depends on bg mode */
        const uint8_t *objTiles;
        uint32_t areaSize;
        /* OAM region */
        const uint8_t *attributes;

        int32_t hightlightObjIndex = 0;

        bool use2dMapping;

        Memory& memory;
        LCDColorPalette& palette;

        std::vector<OBJ> objects;

      public:
        OBJLayer(Memory& mem, LCDColorPalette& plt, uint16_t prio);
        void setMode(BGMode bgMode, bool mapping2d);
        void loadOBJs();
        void drawScanline(int32_t y) override;
        std::string toString() const;
    };
}

#endif /* OBJLAYER_HPP */