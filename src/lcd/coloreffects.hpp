#ifndef COLOREFFECTS_HPP
#define COLOREFFECTS_HPP

#include "defs.hpp"
#include <array>
#include <memory>

namespace gbaemu::lcd
{
    class ColorEffects
    {
      public:
        typedef std::array<std::shared_ptr<Layer>, 8>::const_iterator iterator;

      public:
        BLDCNT::ColorSpecialEffect effect;
        uint32_t eva;
        uint32_t evb;
        uint32_t evy;

      public:
        void load(const LCDIORegs &regs) noexcept;
        std::function<color_t(color_t, color_t)> getBlendingFunction() const;
        bool secondColorRequired() const;
        BLDCNT::ColorSpecialEffect getEffect() const;
        std::string toString() const;
    };
} // namespace gbaemu::lcd

#endif /* COLOREFFECTS_HPP */
