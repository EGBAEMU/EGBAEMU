#ifndef WINDOW_REGIONS_HPP
#define WINDOW_REGIONS_HPP

#include <lcd/defs.hpp>
#include <lcd/coloreffects.hpp>


namespace gbaemu::lcd
{
    enum WindowID
    {
        WIN0 = 0,
        WIN1,
        OBJ,
        OUTSIDE
    };

    class WindowRegion
    {
      public:
        WindowID id;
      private:
        int32_t left;
        int32_t right;
        int32_t top;
        int32_t bottom;
        /* enabled in window? */
        std::array<bool, 4> bg;
        bool obj;
        bool colorEffect;
      public:
        void load(const LCDIORegs& regs);
        std::string toString() const;
        bool inside(int32_t x, int32_t y) const noexcept;
    };

    class WindowEffects
    {
      private:
        /* ordered in descending priority */
        std::array<WindowRegion, 5> windows;
        ColorEffects colorEffects;
      public:
        WindowEffects();
        void load(const LCDIORegs& regs);
        const WindowRegion& getActiveWindow(int32_t x, int32_t y) const;
    };
}

#endif /* WINDOW_REGIONS_HPP */
