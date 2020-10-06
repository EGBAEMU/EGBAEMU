#ifndef WINDOW_REGIONS_HPP
#define WINDOW_REGIONS_HPP

#include <array>
#include <io/memory.hpp>
#include <lcd/coloreffects.hpp>
#include <lcd/defs.hpp>
#include <lcd/palette.hpp>
#include <lcd/objlayer.hpp>
#include <memory>
#include <bitset>
#include <sstream>

namespace gbaemu::lcd
{
    typedef uint8_t WindowSettingsFlag;

    inline WindowSettingsFlag createFlag(bool bg0, bool bg1, bool bg2, bool bg3, bool obj, bool cfx)
    {
        return (bg0 ? 1  : 0) |
               (bg1 ? 2  : 0) |
               (bg2 ? 4  : 0) |
               (bg3 ? 8  : 0) |
               (obj ? 16 : 0) |
               (cfx ? 32 : 0);
    }

    inline bool flagLayerEnabled(WindowSettingsFlag flag, LayerID id)
    {
        return (flag >> static_cast<uint32_t>((id > LAYER_OBJ0) ? LAYER_OBJ0 : id)) & 1;
    }

    inline bool flagCFXEnabled(WindowSettingsFlag flag)
    {
        return isBitSet<uint8_t, 5>(flag);
    }

    inline std::string flagToString(WindowSettingsFlag flag)
    {
        std::stringstream ss;
        ss << std::boolalpha;

        ss << "BG0: " << flagLayerEnabled(flag, LAYER_BG0) << '\n';
        ss << "BG1: " << flagLayerEnabled(flag, LAYER_BG1) << '\n';
        ss << "BG2: " << flagLayerEnabled(flag, LAYER_BG2) << '\n';
        ss << "BG3: " << flagLayerEnabled(flag, LAYER_BG3) << '\n';
        ss << "OBJ: " << flagLayerEnabled(flag, LAYER_OBJ0) << '\n';
        ss << "CFX: " << flagCFXEnabled(flag) << '\n';

        return ss.str();
    }

    struct EnabledMask
    {
        std::array<uint8_t, SCREEN_WIDTH> mask;
    };

    enum WindowID {
        WIN0 = 0,
        WIN1,
        OBJ_WIN,
        /* is enabled if any of the windows above are enabled */
        OUTSIDE,
        DEFAULT_WIN
    };

    class WindowRegion
    {
      public:
        WindowID id;
        bool enabled;
        WindowSettingsFlag flag;
        Rect rect;

        std::string toString() const;
    };

    /* Usually you would do this with polymorphism but we need that sweet compile time optimization. */
    class NormalWindow : public WindowRegion
    {
      public:
        
        void load(const LCDIORegs &regs);
        bool inside(int32_t x, int32_t y) const noexcept;
    };

    class OBJWindow : public WindowRegion
    {
      public:
        std::shared_ptr<OBJLayer> objLayer;

        void load(const LCDIORegs &regs);
        bool inside(int32_t x, int32_t y) const noexcept;
    };

    class OutsideWindow : public WindowRegion
    {
      public:
        void load(const LCDIORegs &regs);
    };

    class WindowFeature
    {
      public:
        /* ordered in descending priority */
        NormalWindow normalWindows[2];
        OBJWindow objWindow;
        OutsideWindow outsideWindow;

        ColorEffects colorEffects;
        color_t backdropColor;
      public:
        EnabledMask enabledMask;

        WindowFeature();
        void load(const LCDIORegs &regs, int32_t y, color_t bdColor);
        bool isEnabled() const;
        std::string toString() const;
    };
} // namespace gbaemu::lcd

#endif /* WINDOW_REGIONS_HPP */
