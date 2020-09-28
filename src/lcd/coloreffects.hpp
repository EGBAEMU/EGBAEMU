#ifndef COLOREFFECTS_HPP
#define COLOREFFECTS_HPP

#include <memory>
#include "defs.hpp"


namespace gbaemu::lcd
{
    class ColorEffects
    {
      public:
        typedef std::array<std::shared_ptr<Layer>, 8>::const_iterator iterator;
      private:
        BLDCNT::ColorSpecialEffect effect;
        uint32_t eva;
        uint32_t evb;
        uint32_t evy;
      public:
        void load(const LCDIORegs& regs) noexcept;
        color_t apply(color_t first, color_t second = TRANSPARENT) const;
        bool secondColorRequired() const;
        BLDCNT::ColorSpecialEffect getEffect() const;
    };
}

#endif /* COLOREFFECTS_HPP */
