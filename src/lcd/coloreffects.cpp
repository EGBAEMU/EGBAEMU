#include "coloreffects.hpp"

#include <sstream>
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

    std::function<color_t(color_t, color_t)> ColorEffects::getBlendingFunction() const
    {
        switch (effect) {
            case BLDCNT::None:
                return [](color_t first, color_t second) -> color_t {
                    return first;
                };
            case BLDCNT::AlphaBlending:
                return [&](color_t first, color_t second) -> color_t {
                    color_t finalColor = 0;

                    for (uint32_t i = 0; i < 3; ++i) {
                        color_t top = (first >> (i * 5)) & 0x1F;
                        color_t bot = (second >> (i * 5)) & 0x1F;
                        color_t chan = std::min<color_t>(31u, top * eva / 16 + bot * evb / 16);
                        finalColor |= chan << (i * 5);
                    }

                    return finalColor;
                };
            case BLDCNT::BrightnessIncrease:
                return [&](color_t first, color_t second) -> color_t {
                    color_t finalColor = 0;

                    for (uint32_t i = 0; i < 3; ++i) {
                        color_t top = (first >> (i * 5)) & 0x1F;
                        color_t inv = 0x1F - top;
                        color_t scaled = (inv * evy) / 16;
                        finalColor |= std::min<color_t>(top + scaled, 0x1F) << (i * 5);
                    }

                    return finalColor;
                };
            case BLDCNT::BrightnessDecrease:
                return [&](color_t first, color_t second) -> color_t {
                    color_t finalColor = 0;

                    for (uint32_t i = 0; i < 3; ++i) {
                        color_t top = (first >> (i * 5)) & 0x1F;
                        /* top - top * evy >= 0 because 0 <= evy <= 1 */
                        finalColor |= (top - (top * evy) / 16) << (i * 5);
                    }

                    return finalColor;
                };
            default:
                throw std::runtime_error("Invalid color effect.");
        }
    }

    bool ColorEffects::secondColorRequired() const
    {
        return effect == BLDCNT::AlphaBlending;
    }

    BLDCNT::ColorSpecialEffect ColorEffects::getEffect() const
    {
        return effect;
    }

    std::string ColorEffects::toString() const
    {
        std::stringstream ss;
        ss << "color effect: ";

        switch (effect) {
            default: case BLDCNT::None:
                ss << "none\n";
                break;
            case BLDCNT::AlphaBlending:
                ss << "alpha blending\n";
                ss << "eva: " << eva << '\n';
                ss << "evb: " << evb << '\n';
                break;
            case BLDCNT::BrightnessDecrease:
                ss << "brightness decrease\n";
                ss << "evy: " << evy << '\n';
                break;
            case BLDCNT::BrightnessIncrease:
                ss << "brightness increase\n";
                ss << "evy: " << evy << '\n';
                break;
        }

        return ss.str();
    }
} // namespace gbaemu::lcd