#include "palette.hpp"


namespace gbaemu::lcd
{
    /* LCDColorPalette */

    color_t LCDColorPalette::toR8G8B8(color16_t color)
    {
        uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
        uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
        uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    color_t LCDColorPalette::getBgColor(uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return toR8G8B8(bgPalette[index]);
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

        return toR8G8B8(objPalette[index]);
    }

    color_t LCDColorPalette::getObjColor(uint32_t paletteNumber, uint32_t index) const
    {
        if (index == 0)
            return TRANSPARENT;

        return getObjColor(paletteNumber * 16 + index);
    }

    color_t LCDColorPalette::getBackdropColor() const
    {
        return toR8G8B8(bgPalette[0]);
    }
}