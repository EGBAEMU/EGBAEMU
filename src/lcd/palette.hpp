#ifndef PALETTE_HPP
#define PALETTE_HPP

#include "defs.hpp"
#include <io/memory.hpp>

namespace gbaemu::lcd
{
    struct LCDColorPalette {
        /* 256 entries */
        const color16_t *bgPalette;
        /* 256 entries */
        const color16_t *objPalette;

        static color_t toR8G8B8(color16_t color);
        void loadPalette(Memory &mem);
        /*
            Under certain conditions the palette can be split up into 16 partitions of 16 colors. This is what
            partition number and index refer to.
         */
        color_t getBgColor(uint32_t index) const;
        color_t getBgColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getObjColor(uint32_t index) const;
        color_t getObjColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getBackdropColor() const;
        void drawPalette(int32_t size, color_t *target, int32_t stride);
    };
} // namespace gbaemu::lcd

#endif /* PALETTE_HPP */