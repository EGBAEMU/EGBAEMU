#ifndef WINDOW_REGIONS_HPP
#define WINDOW_REGIONS_HPP

#include <memory>
#include <lcd/defs.hpp>
#include <lcd/coloreffects.hpp>
#include <array>


namespace gbaemu::lcd
{
    enum WindowID
    {
        WIN0 = 0,
        WIN1,
        OBJ,
        /* is enabled if any of the windows above are enabled */
        OUTSIDE
    };

    class WindowRegion
    {
      public:
        WindowID id;
        bool enabled;
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

    class WindowFeature
    {
      private:
        /* ordered in descending priority */
        std::array<WindowRegion, 4> windows;
        ColorEffects colorEffects;

        bool anyWindowEnabled() const;
        void composeTrivialScanline(const std::array<std::shared_ptr<Layer>, 8>& layers, color_t *target);
      public:
        WindowFeature();
        void load(const LCDIORegs& regs);
        const WindowRegion& getActiveWindow(int32_t x, int32_t y) const;
        void composeScanline(const std::array<std::shared_ptr<Layer>, 8>& layers, color_t *target);
    };
}

#endif /* WINDOW_REGIONS_HPP */
