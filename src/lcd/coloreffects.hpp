#ifndef COLOREFFECTS_HPP
#define COLOREFFECTS_HPP

#include "defs.hpp"


namespace gbaemu::lcd
{
    class ColorEffects
    {
      private:
        BLDCNT::ColorSpecialEffect effect;
        uint32_t eva;
        uint32_t evb;
        uint32_t evy;
      public:
        void load(const LCDIORegs& regs) noexcept;
        color_t apply(color_t first, color_t second = TRANSPARENT) const;
    };
}

#endif /* COLOREFFECTS_HPP */
