#ifndef OBJLAYER_HPP
#define OBJLAYER_HPP

#include "obj.hpp"
#include "packed.h"
#include "palette.hpp"

#include <array>
#include <io/memory.hpp>
#include <memory>

namespace gbaemu::lcd
{
    /* Loads objects once. Every layer then can take the objects it needs. */
    class OBJManager
    {
      public:
        std::array<OBJ, 128> objects;
        OBJManager();
        void load(const uint8_t *attributes, BGMode bgMode);
    };

    class OBJLayer : public Layer
    {
      public:
        std::shared_ptr<OBJManager> objManager;

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

        Memory &memory;
        LCDColorPalette &palette;
        const LCDIORegs &regs;

        std::vector<OBJ> objects;

        std::vector<OBJ>::const_iterator getLastRenderedOBJ(int32_t cycleBudget) const;

      public:
        OBJLayer(Memory &mem, LCDColorPalette &plt, const LCDIORegs &ioRegs, uint16_t prio, const std::shared_ptr<OBJManager> &manager);
        void setMode(BGMode bgMode, bool mapping2d);
        void loadOBJs(int32_t y, const std::function<bool(const OBJ &, real_t, uint16_t)> &filter);
        void drawScanline(int32_t y) override;
        std::string toString() const;
    };
} // namespace gbaemu::lcd

#endif /* OBJLAYER_HPP */