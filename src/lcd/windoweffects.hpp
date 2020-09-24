#ifndef WINDOWEFFECTS_HPP
#define WINDOWEFFECTS_HPP

#include "defs.hpp"


namespace gbaemu::lcd
{
    enum WindowID
    {
        WIN0 = 0,
        WIN1,
        OBJ,
        OUTSIDE
    };

    class Window
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
    };

    class WindowEffects
    {
      private:
        std::array<Window, 4> windows;
      public:
        WindowEffects();
        void load(const LCDIORegs& regs);
    };
}

#endif /* WINDOWEFFECTS_HPP */
