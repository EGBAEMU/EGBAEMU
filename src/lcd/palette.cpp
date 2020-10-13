#include "palette.hpp"

namespace gbaemu::lcd
{
    /* LCDColorPalette */

    void LCDColorPalette::loadPalette(Memory &mem)
    {
        bgPalette = reinterpret_cast<uint16_t *>(mem.bg_obj_ram);
        objPalette = reinterpret_cast<uint16_t *>(mem.bg_obj_ram + 0x200);
    }

    color_t LCDColorPalette::getBgColor(uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return bgPalette[index];
    }

    color_t LCDColorPalette::getBgColor(uint32_t paletteNumber, uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return getBgColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getObjColor(uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return objPalette[index];
    }

    color_t LCDColorPalette::getObjColor(uint32_t paletteNumber, uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return getObjColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getBackdropColor() const
    {
        return bgPalette[0];
    }
} // namespace gbaemu::lcd