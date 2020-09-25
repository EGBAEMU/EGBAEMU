#include "window-regions.hpp"

#include <sstream>


namespace gbaemu::lcd
{
    void Window::load(const LCDIORegs& regs)
    {
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

        if (id == WIN1 || id == OBJ)
            maskOff = 8;

        for (int32_t i = 0; i < 4; ++i)
            bg[i] = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, i + maskOff);
        
        obj = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, 4 + maskOff);
        colorEffect = bitGet<uint16_t>(control, WININOUT::ENABLE_MASK, 5 + maskOff);
    }

    std::string Window::toString() const
    {
        std::stringstream ss;
        ss << "id: ";

        switch (id) {
            case WIN0: ss << "WIN0"; break;
            case WIN1: ss << "WIN1"; break;
            case OBJ: ss << "OBJ"; break;
            case OUTSIDE: ss << "OUTSIDE"; break;
        }

        ss << '\n';
        ss << "left: " << left << '\n';
        ss << "right: " << right << '\n';
        ss << "top: " << top << '\n';
        ss << "bottom: " << bottom;

        return ss.str();
    }

    bool Window::inside(int32_t x, int32_t y) const noexcept
    {
        return left <= x && x < right && top <= y && y < bottom;
    }

    WindowEffects::WindowEffects()
    {
        windows[0].id = WIN0;
        windows[1].id = WIN1;
        windows[2].id = OBJ;
        windows[3].id = OUTSIDE;
    }

    void WindowEffects::load(const LCDIORegs& regs)
    {
        for (auto& win : windows)
            win.load(regs);

        colorEffects.load(regs);
    }

    const Window& WindowEffects::getActiveWindow(int32_t x, int32_t y) const
    {
        for (const auto& win : windows)
            if (win.inside(x, y))
                return win;

        return *windows.crbegin();
    }
}
