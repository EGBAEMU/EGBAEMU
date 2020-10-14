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

        Memory &memory;
        LCDColorPalette &palette;
        const LCDIORegs &regs;

        std::vector<OBJ> objects;

      public:
        OBJLayer(Memory &mem, LCDColorPalette &plt, const LCDIORegs &ioRegs, uint16_t prio);
        void setMode(BGMode bgMode, bool mapping2d);
        // void loadOBJs(int32_t y, const std::function<bool(const OBJ&, real_t, uint16_t)>& filter);
        void prepareLoadOBJs();
        void drawScanline(int32_t y) override;
        std::string toString() const;
    };
} // namespace gbaemu::lcd

#endif /* OBJLAYER_HPP */