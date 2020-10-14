#include "palette.hpp"
#include "io/memory.hpp"

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

    void LCDColorPalette::loadPalette(Memory &mem)
    {
        bgPalette = reinterpret_cast<uint16_t *>(mem.bg_obj_ram);
        objPalette = reinterpret_cast<uint16_t *>(mem.bg_obj_ram + 0x200);
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

    void LCDColorPalette::drawPalette(int32_t size, color_t *target, int32_t stride)
    {
        for (int32_t y = 0; y < size; ++y)
            for (int32_t x = 0; x < size * 256; ++x) {
                int32_t index = x / size;
                color_t color;

                if (index == 0)
                    color = getBackdropColor();
                else
                    color = getBgColor(index);

                target[y * stride + x] = color;
            }
    }
} // namespace gbaemu::lcd