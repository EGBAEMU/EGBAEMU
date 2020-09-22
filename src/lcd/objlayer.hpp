#ifndef OBJLAYER_HPP
#define OBJLAYER_HPP

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

#include "packed.h"
    struct OBJAttribute {
        uint16_t attribute[3];
    } PACKED;
#include "endpacked.h"

    /* Contains all the properties for a given obj index. */
    struct OBJInfo
    {
        bool visible = false;
        int32_t objIndex;
        OBJShape shape;
        uint16_t priority;
        int32_t xOff, yOff;
        bool doubleSized, vFlip = false, hFlip = false, useColor256, mosaic;
        uint32_t width = 0, height = 0;
        uint16_t paletteNumber;
        uint16_t tileNumber;
        uint16_t tilesPerRow;
        uint16_t bytesPerTile;
        vec2 origin{0, 0}, screenRef, d{1, 0,}, dm{0, 1};

        std::string toString() const;
    };

    class OBJLayer : public Layer
    {
      public:
        uint32_t bgMode;
        /* depends on bg mode */
        const uint8_t *objTiles;
        uint32_t areaSize;
        /* OAM region */
        const uint8_t *attributes;

        int32_t hightlightObjIndex = 0;

      private:
        OBJAttribute getAttribute(uint32_t index) const;
        std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(uint32_t index) const;
      public:

#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
        MemoryCanvas<color_t> debugCanvas;
#endif

        OBJLayer(LayerId layerId);
        OBJInfo getOBJInfo(uint32_t objIndex) const;
        void setMode(const uint8_t *vramBaseAddress, const uint8_t *oamBaseAddress, uint32_t bgMode);
        void draw(LCDColorPalette &palette, bool use2dMapping);
    };
}

#endif /* OBJLAYER_HPP */