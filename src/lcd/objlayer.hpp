#ifndef OBJLAYER_HPP
#define OBJLAYER_HPP

#include "palette.hpp"
#include "packed.h"

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
      private:
        static OBJAttribute getAttribute(const uint8_t *attributes, uint32_t index);
        static std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(const uint8_t *attributes, uint32_t index);
      public:
        OBJ() {};
        OBJ(const uint8_t *attributes, int32_t index);
        std::string toString() const;
        color_t pixelColor(int32_t sx, int32_t sy, const uint8_t *objTiles, LCDColorPalette& palette, bool use2dMapping) const;
        bool intersectsWithScanline(int32_t y) const;
    };

    /* a container class storing all objects in the scene */
    class OBJManager
    {
      public:
        std::vector<OBJ> allObjects;
        //std::array<std::vector<OBJ>, 4> objectsByPriority;

        OBJManager();
        void loadOBJs(const uint8_t *attributes);
        /* returns the OBJ after the last OBJ that fits into the cycle budget in that scanline */
        std::vector<OBJ>::const_iterator lastOBJ(int32_t y, int32_t cycleBudget) const;
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

        int32_t mosaicWidth;
        int32_t mosaicHeight;

        Memory& memory;
        LCDColorPalette& palette;
        const LCDIORegs& regs;
        OBJManager& objManager;

        std::vector<OBJ> objects;

        std::vector<OBJ>::const_iterator getLastRenderedOBJ(int32_t cycleBudget) const;

      public:
        OBJLayer(Memory& mem, LCDColorPalette& plt, const LCDIORegs& ioRegs, OBJManager& objMgr, uint16_t prio);
        void setMode(BGMode bgMode, bool mapping2d);
        void loadOBJs();
        void drawScanline(int32_t y) override;
        std::string toString() const;
    };
}

#endif /* OBJLAYER_HPP */