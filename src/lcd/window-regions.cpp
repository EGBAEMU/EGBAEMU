#include "window-regions.hpp"

#include <sstream>
#include "util.hpp"

namespace gbaemu::lcd
{
    std::string WindowRegion::toString() const
    {
        std::stringstream ss;
        ss << std::boolalpha << "id: ";

        switch (id) {
            case WIN0:
                ss << "WIN0";
                break;
            case WIN1:
                ss << "WIN1";
                break;
            case OBJ_WIN:
                ss << "OBJ";
                break;
            case OUTSIDE:
                ss << "OUTSIDE";
                break;
            case DEFAULT_WIN:
                ss << "DEFAULT_WIN";
                break;
            default:
                throw std::runtime_error("Invalid window id!");
        }

        ss << '\n';

        /*
        for (size_t i = 0; i < bg.size(); ++i)
            ss << "bg " << i << ": " << bg[i];

        ss << "obj enabled: " << obj << '\n';
        ss << "color effects enabled: " << colorEffect;
         */

        return ss.str();
    }

    void NormalWindow::load(const LCDIORegs &regs)
    {
        switch (id) {
            case WIN0:
                enabled = isBitSet<uint16_t, 13>(le(regs.DISPCNT));
                break;
            case WIN1:
                enabled = isBitSet<uint16_t, 14>(le(regs.DISPCNT));
                break;
            default:
                throw std::runtime_error("Invalid window id!");
        }

        if (!enabled)
            return;

        uint16_t winh, winv;

        if (id == WIN0) {
            winh = le(regs.WIN0H);
            winv = le(regs.WIN0V);
        } else if (id == WIN1) {
            winh = le(regs.WIN1H);
            winv = le(regs.WIN1V);
        }

        rect.right = std::min<uint16_t>(SCREEN_WIDTH, bitGet<uint16_t>(winh, 0xFF, 0));
        rect.bottom = std::min<uint16_t>(SCREEN_HEIGHT, bitGet<uint16_t>(winv, 0xFF, 0));
        rect.left = std::min<uint16_t>(rect.right, bitGet<uint16_t>(winh, 0xFF, 8));
        rect.top = std::min<uint16_t>(rect.bottom, bitGet<uint16_t>(winv, 0xFF, 8));

        uint16_t control = le(regs.WININ);

        if (id == WIN1) {
            flag = createFlag(isBitSet<uint16_t,  8>(control),
                              isBitSet<uint16_t,  9>(control),
                              isBitSet<uint16_t, 10>(control),
                              isBitSet<uint16_t, 11>(control),
                              isBitSet<uint16_t, 12>(control),
                              isBitSet<uint16_t, 13>(control));
        } else {
            flag = createFlag(isBitSet<uint16_t, 0>(control),
                              isBitSet<uint16_t, 1>(control),
                              isBitSet<uint16_t, 2>(control),
                              isBitSet<uint16_t, 3>(control),
                              isBitSet<uint16_t, 4>(control),
                              isBitSet<uint16_t, 5>(control));
        }
    }

    bool NormalWindow::inside(int32_t x, int32_t y) const noexcept
    {
        return rect.inside(x, y);
    }

    void OBJWindow::load(const LCDIORegs &regs)
    {
        enabled = isBitSet<uint16_t, 15>(le(regs.DISPCNT));

        if (!enabled)
            return;

        uint16_t control = le(regs.WINOUT);

        flag = createFlag(isBitSet<uint16_t,  8>(control),
                          isBitSet<uint16_t,  9>(control),
                          isBitSet<uint16_t, 10>(control),
                          isBitSet<uint16_t, 11>(control),
                          isBitSet<uint16_t, 12>(control),
                          isBitSet<uint16_t, 13>(control));
    }

    bool OBJWindow::inside(int32_t x, int32_t y) const noexcept
    {
        return objLayer->scanline[x].color != TRANSPARENT;
    }

    void OutsideWindow::load(const LCDIORegs &regs)
    {
        /* TODO: Maybe make enabled as default. */
        enabled = isBitSet<uint16_t, 13>(le(regs.DISPCNT)) ||
            isBitSet<uint16_t, 14>(le(regs.DISPCNT)) ||
            isBitSet<uint16_t, 15>(le(regs.DISPCNT));

        if (!enabled)
            return;

        uint16_t control = le(regs.WINOUT);

        flag = createFlag(isBitSet<uint16_t, 0>(control),
                          isBitSet<uint16_t, 1>(control),
                          isBitSet<uint16_t, 2>(control),
                          isBitSet<uint16_t, 3>(control),
                          isBitSet<uint16_t, 4>(control),
                          isBitSet<uint16_t, 5>(control));
    }

    WindowFeature::WindowFeature()
    {
        normalWindows[0].id = WIN0;
        normalWindows[1].id = WIN1;
        objWindow.id = OBJ_WIN;
        outsideWindow.id = OUTSIDE;
    }

    void WindowFeature::load(const LCDIORegs &regs, int32_t y, color_t bdColor)
    {
        normalWindows[0].load(regs);
        normalWindows[1].load(regs);
        objWindow.load(regs);
        outsideWindow.load(regs);

        for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
            if (normalWindows[0].enabled && normalWindows[0].inside(x, y))
                enabledMask.mask[x] = normalWindows[0].flag;
            else if (normalWindows[1].enabled && normalWindows[1].inside(x, y))
                enabledMask.mask[x] = normalWindows[1].flag;
            else if (objWindow.enabled && objWindow.inside(x, y))
                enabledMask.mask[x] = objWindow.flag;
            else if (outsideWindow.enabled)
                enabledMask.mask[x] = outsideWindow.flag;
            else
                enabledMask.mask[x] = 0xFF;
        }

        colorEffects.load(regs);
        backdropColor = bdColor;
    }
} // namespace gbaemu::lcd
