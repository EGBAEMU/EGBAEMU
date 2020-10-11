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

        void loadPalette(Memory &mem);
        /*
            Under certain conditions the palette can be split up into 16 partitions of 16 colors. This is what
            partition number and index refer to.
         */
        color16_t getBgColor(uint32_t index) const;
        color16_t getBgColor(uint32_t paletteNumber, uint32_t index) const;
        color16_t getObjColor(uint32_t index) const;
        color16_t getObjColor(uint32_t paletteNumber, uint32_t index) const;
        color16_t getBackdropColor() const;
    };
} // namespace gbaemu::lcd

#endif /* PALETTE_HPP */