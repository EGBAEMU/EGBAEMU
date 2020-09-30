#include "window-regions.hpp"

#include <sstream>
#include "util.hpp"

namespace gbaemu::lcd
{
    void WindowRegion::load(const LCDIORegs &regs)
    {
        switch (id) {
            case WIN0:
                enabled = isBitSet<uint16_t, 13>(le(regs.DISPCNT));
                break;
            case WIN1:
                enabled = isBitSet<uint16_t, 14>(le(regs.DISPCNT));
                break;
            case OBJ_WIN:
                enabled = isBitSet<uint16_t, 15>(le(regs.DISPCNT));
                break;
            case OUTSIDE:
                enabled = isBitSet<uint16_t, 13>(le(regs.DISPCNT)) ||
                          isBitSet<uint16_t, 14>(le(regs.DISPCNT)) ||
                          isBitSet<uint16_t, 15>(le(regs.DISPCNT));
                break;
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

        if (id == WIN0 || id == WIN1) {
            right = std::min<uint16_t>(SCREEN_WIDTH, bitGet<uint16_t>(winh, 0xFF, 0));
            bottom = std::min<uint16_t>(SCREEN_HEIGHT, bitGet<uint16_t>(winv, 0xFF, 0));
            left = std::min<uint16_t>(right, bitGet<uint16_t>(winh, 0xFF, 8));
            top = std::min<uint16_t>(bottom, bitGet<uint16_t>(winv, 0xFF, 8));
        } else {
            left = 0;
            right = 0;
            top = 0;
            bottom = 0;
        }

        uint16_t control;
        int32_t maskOff = 0;

        if (id == WIN0 || id == WIN1)
            control = le(regs.WININ);
        else
            control = le(regs.WINOUT);

        if (id == WIN1 || id == OBJ_WIN)
            maskOff = 8;

        for (int32_t i = 0; i < 4; ++i)
            bg[i] = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, i + maskOff);

        obj = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, 4 + maskOff);
        colorEffect = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, 5 + maskOff);
    }

    std::string WindowRegion::toString() const
    {
        std::stringstream ss;
        ss << "id: ";

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
        }

        ss << '\n';
        ss << "left: " << left << '\n';
        ss << "right: " << right << '\n';
        ss << "top: " << top << '\n';
        ss << "bottom: " << bottom;

        return ss.str();
    }

    bool WindowRegion::inside(int32_t x, int32_t y) const noexcept
    {
        return left <= x && x < right && top <= y && y < bottom;
    }

    bool WindowFeature::anyWindowEnabled() const
    {
        return windows[0].enabled ||
               windows[1].enabled ||
               windows[2].enabled ||
               windows[3].enabled;
    }

    void WindowFeature::composeTrivialScanline(const std::array<std::shared_ptr<Layer>, 8> &layers, color_t *target)
    {
        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            color_t finalColor = backdropColor;

            /*
                ------------------- layer 0
                ------------------- layer 1
                ------------------- layer 2
                ------------------- layer 3

                If the top most non-transparent pixel is not used as a first target draw it without effects.


             */

            /* TODO: put finalColor in final scanline */
            target[x] = finalColor;
        }
    }

    WindowFeature::WindowFeature()
    {
        windows[0].id = WIN0;
        windows[1].id = WIN1;
        windows[2].id = OBJ_WIN;
        windows[3].id = OUTSIDE;
    }

    void WindowFeature::load(const LCDIORegs &regs, color_t bdColor)
    {
        for (auto &win : windows)
            win.load(regs);

        colorEffects.load(regs);
        backdropColor = bdColor;
    }

    const WindowRegion &WindowFeature::getActiveWindow(int32_t x, int32_t y) const
    {
        for (const auto &win : windows)
            if (win.inside(x, y))
                return win;

        return *windows.crbegin();
    }

    void WindowFeature::composeScanline(const std::array<std::shared_ptr<Layer>, 8> &layers, color_t *target)
    {
        /*
        if (!anyWindowEnabled()) {
            composeTrivialScanline(layers);
        } else {

        }
         */
        composeTrivialScanline(layers, target);
    }
} // namespace gbaemu::lcd
