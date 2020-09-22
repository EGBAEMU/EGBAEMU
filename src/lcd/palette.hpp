#ifndef PALETTE_HPP
#define PALETTE_HPP

#include "defs.hpp"


namespace gbaemu::lcd
{
    struct LCDColorPalette {
        /* 256 entries */
        const color16_t *bgPalette;
        /* 256 entries */
        const color16_t *objPalette;

        static color_t toR8G8B8(color16_t color);
        /*
            Under certain conditions the palette can be split up into 16 partitions of 16 colors. This is what
            partition number and index refer to.
         */
        color_t getBgColor(uint32_t index) const;
        color_t getBgColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getObjColor(uint32_t index) const;
        color_t getObjColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getBackdropColor() const;
    };
}

#endif /* PALETTE_HPP */