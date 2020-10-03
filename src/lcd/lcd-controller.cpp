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

#ifdef LEGACY_RENDERING
    void LCDController::renderTick()
    {
        //if (le(regsRef.DISPCNT) & DISPCTL::HBLANK_INTERVAL_FREE_MASK)
        //    throw std::runtime_error("Unsupported feature: HBLANK FREE.");

        /*
            [ 960 cycles for 240 dots ][ 272 cycles for hblank ]
            ...
            [ 960 cycles for 240 dots ][ 272 cycles for hblank ]
            [ 83776 cycles for vblank (no hblank) ]
         */

        /* cycle in the current display period */
        uint32_t displayCycle = scanline.cycle % 280896;
        uint32_t scanlineCycle = scanline.cycle % 1232;

        scanline.vblanking = (displayCycle >= 197120);
        /* h-blank flag is 1 slightly after the scanline has been drawn */
        scanline.hblanking = (scanlineCycle >= 1006);
        scanline.vCount = displayCycle / 1232;

        if (scanlineCycle == 0)
            onVCount();

        if (displayCycle == 197120 && scanline.vblanking) {
            /* on first v-blank cycle */
            onVBlank();
            present();
        } else if (!scanline.vblanking) {
            /* h-blank interrupt, scanline rendering, scanline.x increase is only done when not v-blanking */
            if (scanlineCycle == 0 && !scanline.hblanking) {
                regs = regsRef;
                drawScanline();
            }

            /* 4 cycles per pixel */
            if (scanlineCycle % 4 == 0 && !scanline.hblanking) {
                ++scanline.x;
            } else if (scanlineCycle == 1006 && scanline.hblanking) {
                /* on first h-blank cycle */
                onHBlank();
            }
        }

        /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET>(stat, bmap<uint16_t>(scanline.vblanking));
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET>(stat, bmap<uint16_t>(scanline.hblanking));
        regsRef.DISPSTAT = le(stat);

        ++scanline.cycle;
    }

    void LCDController::onVCount()
    {
        /* update vcount */
        regsRef.VCOUNT = le(scanline.vCount);

        /* use regsRef as those settings are crucial timewise */
        uint16_t stat = le(regsRef.DISPSTAT);
        uint16_t vCountSetting = bitGet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET);
        bool vCountMatch = scanline.vCount == vCountSetting;

        regsRef.DISPSTAT = le(bitSet<uint16_t, DISPSTAT::VCOUNTER_FLAG_MASK, DISPSTAT::VCOUNTER_FLAG_OFFSET>(stat, bmap<uint16_t>(vCountMatch)));

        if (vCountMatch && isBitSet<uint16_t, DISPSTAT::VCOUNTER_IRQ_ENABLE_OFFSET>(stat)) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_COUNTER_MATCH);
        }
    }

    void LCDController::onHBlank()
    {
        /* use regsRef as those settings are crucial timewise */
        if (isBitSet<uint16_t, DISPSTAT::HBLANK_IRQ_ENABLE_OFFSET>(le(regsRef.DISPSTAT)))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

        scanline.x = 0;
        ++scanline.y;
    }

    void LCDController::onVBlank()
    {
        /* use regsRef as those settings are crucial timewise */
        if (isBitSet<uint16_t, DISPSTAT::VBLANK_IRQ_ENABLE_OFFSET>(le(regsRef.DISPSTAT)))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);

        scanline.y = 0;
    }

    void LCDController::drawScanline()
    {
        /* If this bit is set, white lines are displayed. */
        if (le(regsRef.DISPCNT) & DISPCTL::FORCED_BLANK_MASK) {
            color_t *outBuf = frameBuffer.pixels() + scanline.y * frameBuffer.getWidth();

            for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                outBuf[x] = WHITE;
        } else {
            renderer.drawScanline(scanline.y);
        }
    }
#else

    void LCDController::onVCount()
    {
        // use regsRef as those settings are crucial timewise
        uint16_t stat = le(regsRef.DISPSTAT);
        uint16_t vCountSetting = bitGet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET);
        bool vCountMatch = scanline.vCount == vCountSetting;

        regsRef.DISPSTAT = le(bitSet<uint16_t, DISPSTAT::VCOUNTER_FLAG_MASK, DISPSTAT::VCOUNTER_FLAG_OFFSET>(stat, bmap<uint16_t>(vCountMatch)));

        if (vCountMatch && isBitSet<uint16_t, DISPSTAT::VCOUNTER_IRQ_ENABLE_OFFSET>(stat)) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_COUNTER_MATCH);
        }

        // update vcount
        scanline.vCount = (scanline.vCount + 1) % 228;
        regsRef.VCOUNT = le(scanline.vCount);
    }

    void LCDController::onHBlank()
    {
        // update stat
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t, false>()>(stat);
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t, true>()>(stat);
        regsRef.DISPSTAT = le(stat);
        scanline.vblanking = false;
        scanline.hblanking = true;

        // use regsRef as those settings are crucial timewise
        if (isBitSet<uint16_t, DISPSTAT::HBLANK_IRQ_ENABLE_OFFSET>(stat))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

        // scanline.x = 0;
        ++scanline.y;
    }

    void LCDController::onVBlank()
    {
        // update stat
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t, true>()>(stat);
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t, false>()>(stat);
        regsRef.DISPSTAT = le(stat);
        scanline.vblanking = true;
        scanline.hblanking = false;

        // use regsRef as those settings are crucial timewise
        if (isBitSet<uint16_t, DISPSTAT::VBLANK_IRQ_ENABLE_OFFSET>(stat))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);

        scanline.y = 0;
    }

    void LCDController::clearBlankFlags()
    {
        /* clear stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = stat & ~((DISPSTAT::VBLANK_FLAG_MASK << DISPSTAT::VBLANK_FLAG_OFFSET) | (DISPSTAT::HBLANK_FLAG_MASK << DISPSTAT::HBLANK_FLAG_OFFSET));
        regsRef.DISPSTAT = le(stat);
        scanline.vblanking = false;
        scanline.hblanking = false;
    }

    void LCDController::drawScanline()
    {
        regs = regsRef;
        //If this bit is set, white lines are displayed.
        if (le(regsRef.DISPCNT) & DISPCTL::FORCED_BLANK_MASK) {
            color_t *outBuf = frameBuffer.pixels() + scanline.y * frameBuffer.getWidth();

            for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                outBuf[x] = WHITE;
        } else {
            renderer.drawScanline(scanline.y);
        }
    }
#endif

    void LCDController::present()
    {
        color_t *dst = display.pixels();
        int32_t dstStride = display.getWidth();

        const color_t *src = frameBuffer.pixels();
        int32_t srcStride = frameBuffer.getWidth();

        int32_t h = frameBuffer.getHeight();
        int32_t w = frameBuffer.getWidth();

        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
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

    bool LCDController::canAccessPPUMemory(bool isOAMRegion) const
    {
        bool forcedBlank = le(regsRef.DISPCNT) & DISPCTL::FORCED_BLANK_MASK;
        bool hblankIntervalFree = le(regsRef.DISPCNT) & DISPCTL::HBLANK_INTERVAL_FREE_MASK;

        return scanline.vblanking || ((!isOAMRegion || hblankIntervalFree) && scanline.hblanking) || forcedBlank;
    }

    std::string LCDController::getLayerStatusString() const
    {
        return renderer.getLayerStatusString();
    }
} // namespace gbaemu::lcd
