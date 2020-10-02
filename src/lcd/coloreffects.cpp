#include "coloreffects.hpp"

#include <util.hpp>

namespace gbaemu::lcd
{
    void ColorEffects::load(const LCDIORegs &regs) noexcept
    {
        uint16_t bldcnt = le(regs.BLDCNT);
        uint16_t bldAlpha = le(regs.BLDALPHA);
        uint16_t bldy = le(regs.BLDY);

        effect = static_cast<BLDCNT::ColorSpecialEffect>(bitGet(bldcnt, BLDCNT::COLOR_SPECIAL_FX_MASK, BLDCNT::COLOR_SPECIAL_FX_OFFSET));

        switch (effect) {
            case BLDCNT::BrightnessIncrease:
            case BLDCNT::BrightnessDecrease:
                /* 0/16, 1/16, 2/16, ..., 16/16, 16/16, ..., 16/16 */
                evy = std::min(16, bldy & 0x1F);
                break;
            case BLDCNT::AlphaBlending:
                eva = std::min(16, bldAlpha & 0x1F);
                evb = std::min(16, (bldAlpha >> 8) & 0x1F);
                break;
            case BLDCNT::None:
                break;
        }
    }

    color_t ColorEffects::apply(color_t first, color_t second) const
    {
        color_t finalColor = 0;

        if (effect != BLDCNT::AlphaBlending)
            return first;

        switch (effect) {
            case BLDCNT::None:
                finalColor = first;
                break;
            case BLDCNT::BrightnessIncrease: {
                color_t inverted = colSub(0xFFFFFFFF, first);
                color_t scaledEvy = colScale(inverted, evy);
                finalColor = colAdd(first, scaledEvy);
                break;
            }
            case BLDCNT::BrightnessDecrease: {
                color_t scaledEvy = colScale(first, evy);
                finalColor = colSub(first, scaledEvy);
                break;
            }
            case BLDCNT::AlphaBlending:
                for (uint32_t i = 0; i < 4; ++i) {
                    color_t top = (first >> (i * 8)) & 0xFF;
                    color_t bot = (second >> (i * 8)) & 0xFF;
                    color_t chan = std::min(255u, top * eva / 16 + bot * evb / 16);
                    finalColor |= chan << (i * 8);
                }
                break;
        }

        return finalColor;
    }

    bool ColorEffects::secondColorRequired() const
    {
        return effect == BLDCNT::AlphaBlending;
    }

    BLDCNT::ColorSpecialEffect ColorEffects::getEffect() const
    {
        return effect;
    }
} // namespace gbaemu::lcd