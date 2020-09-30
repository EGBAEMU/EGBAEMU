#include "lcd-controller.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <algorithm>
#include <cassert>
#include <future>
#include <sstream>

/*
    There are 4 background layers.

    BG0     ----------------------------------
    BG1     ----------------------------------
    BG2     ----------------------------------
    BG3     ----------------------------------
    OBJ     ----------------------------------

    The drawing order and which layers should be drawn can be configured. Top layers can be alpha-blended with layers below.
    Brightness of the top layer can also be configured. The OBJ layer contains all the sprites (called OBJs).
 */

namespace gbaemu::lcd
{

    void LCDController::onVCount()
    {
        /* use regsRef as those settings are crucial timewise */
        uint16_t stat = le(regsRef.DISPSTAT);
        uint16_t vCountSetting = bitGet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET);
        bool vCountMatch = scanline.vCount == vCountSetting;
        if (vCountMatch && bitGet(stat, DISPSTAT::VCOUNTER_IRQ_ENABLE_MASK, DISPSTAT::VCOUNTER_IRQ_ENABLE_OFFSET)) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_COUNTER_MATCH);
        }

        regsRef.DISPSTAT = le(bitSet(stat, DISPSTAT::VCOUNTER_FLAG_MASK, DISPSTAT::VCOUNTER_FLAG_OFFSET, bmap<uint16_t>(vCountMatch)));

        /* update vcount */
        scanline.vCount = (scanline.vCount + 1) % 228;
        uint16_t vcount = le(regsRef.VCOUNT);
        vcount = bitSet<uint16_t>(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, scanline.vCount);
        regsRef.VCOUNT = le(vcount);
    }

    void LCDController::onHBlank()
    {
        /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(false));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(true));
        regsRef.DISPSTAT = le(stat);

        /* use regsRef as those settings are crucial timewise */
        if (bitGet(stat, DISPSTAT::HBLANK_IRQ_ENABLE_MASK, DISPSTAT::HBLANK_IRQ_ENABLE_OFFSET))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

        // scanline.x = 0;
        ++scanline.y;
    }

    void LCDController::onVBlank()
    {
        /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(true));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(false));
        regsRef.DISPSTAT = le(stat);

        /* use regsRef as those settings are crucial timewise */
        if (bitGet(stat, DISPSTAT::VBLANK_IRQ_ENABLE_MASK, DISPSTAT::VBLANK_IRQ_ENABLE_OFFSET))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);

        scanline.y = 0;
    }

    void LCDController::clearBlankFlags()
    {
        /* clear stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = stat & ~(DISPSTAT::VBLANK_FLAG_MASK << DISPSTAT::VBLANK_FLAG_OFFSET) & ~(DISPSTAT::HBLANK_FLAG_MASK << DISPSTAT::HBLANK_FLAG_OFFSET);
        regsRef.DISPSTAT = le(stat);
    }

    void LCDController::drawScanline()
    {
        regs = regsRef;
        renderer.drawScanline(scanline.y, frameBuffer.pixels() + frameBuffer.getWidth() * scanline.y);
    }

    void LCDController::present()
    {
        color_t *dst = display.pixels();
        int32_t dstStride = display.getWidth();

        const color_t *src = frameBuffer.pixels();
        int32_t srcStride = frameBuffer.getWidth();

        for (int32_t y = 0; y < static_cast<int32_t>(SCREEN_HEIGHT); ++y) {
            for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
                color_t color = src[y * srcStride + x];

                for (int32_t sy = 0; sy < scale; ++sy) {
                    for (int32_t sx = 0; sx < scale; ++sx) {
                        dst[(y * scale + sy) * dstStride + (x * scale + sx)] = color;
                    }
                }
            }
        }

        /* race conditions are acceptable here */
        *canDrawToScreen = true;
    }

    std::string LCDController::getLayerStatusString() const
    {
        std::stringstream ss;

        /*
        for (const auto& pLayer : layers) {
            ss << "================================\n";
            ss << "enabled: " << (pLayer->enabled ? "yes" : "no") << '\n';
            ss << "id: " << layerIDToString(pLayer->layerID) << '\n';
            ss << "priority: " << pLayer->priority << '\n';
            //ss << "as first target: " << (pLayer->asFirstTarget ? "yes" : "no") << '\n';
            //ss << "as second target: " << (pLayer->asSecondTarget ? "yes" : "no") << '\n';
        }
         */

        return ss.str();
    }
} // namespace gbaemu::lcd
