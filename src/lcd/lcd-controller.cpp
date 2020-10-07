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

        scanline.x = std::min<int32_t>(scanlineCycle / 4, SCREEN_WIDTH - 1);
        scanline.y = std::min<int32_t>(scanline.vCount, SCREEN_HEIGHT - 1);

        if (displayCycle == 197120) {
            onVBlank();
            present();
        }

        if (scanlineCycle == 0 && !scanline.vblanking) {
            drawScanline();
        }

        if (scanlineCycle == 0) {
            onVCount();
        }

        if (scanlineCycle == 1006) {
            onHBlank();
        }

        /* update stat */
        uint16_t stat = le(regs.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET>(stat, bmap<uint16_t>(scanline.vblanking));
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET>(stat, bmap<uint16_t>(scanline.hblanking));
        regs.DISPSTAT = le(stat);

        ++scanline.cycle;
    }
#endif

    void LCDController::onVCount()
    {
        /* update vcount */
        regs.VCOUNT = le(scanline.vCount);

        /* use regsRef as those settings are crucial timewise */
        uint16_t stat = le(regs.DISPSTAT);
        uint16_t vCountSetting = bitGet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET);
        bool vCountMatch = scanline.vCount == vCountSetting;

        regs.DISPSTAT = le(bitSet<uint16_t, DISPSTAT::VCOUNTER_FLAG_MASK, DISPSTAT::VCOUNTER_FLAG_OFFSET>(stat, bmap<uint16_t>(vCountMatch)));

        if (vCountMatch && isBitSet<uint16_t, DISPSTAT::VCOUNTER_IRQ_ENABLE_OFFSET>(stat)) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_COUNTER_MATCH);
        }

#ifndef LEGACY_RENDERING
        // update vcount
        scanline.vCount = (scanline.vCount + 1) % 228;
        regs.VCOUNT = le(scanline.vCount);
#endif
    }

    void LCDController::onVBlank()
    {
#ifndef LEGACY_RENDERING
        // update stat
        uint16_t stat = le(regs.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t, true>()>(stat);
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t, false>()>(stat);
        regs.DISPSTAT = le(stat);
        scanline.vblanking = true;
        scanline.hblanking = false;
#endif

        // use regs as those settings are crucial timewise
        if (isBitSet<uint16_t, DISPSTAT::VBLANK_IRQ_ENABLE_OFFSET>(le(regs.DISPSTAT)))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);

        internalRegs = regs;

#ifndef LEGACY_RENDERING
        scanline.y = 0;
#endif        
    }

#ifndef LEGACY_RENDERING
    void LCDController::clearBlankFlags()
    {
        /* clear stat */
        uint16_t stat = le(regs.DISPSTAT);
        stat = stat & ~((DISPSTAT::VBLANK_FLAG_MASK << DISPSTAT::VBLANK_FLAG_OFFSET) | (DISPSTAT::HBLANK_FLAG_MASK << DISPSTAT::HBLANK_FLAG_OFFSET));
        regs.DISPSTAT = le(stat);
        scanline.vblanking = false;
        scanline.hblanking = false;
    }
#endif

    void LCDController::drawScanline()
    {
        /* If this bit is set, white lines are displayed. */
        if (le(regs.DISPCNT) & DISPCTL::FORCED_BLANK_MASK) {
            color_t *outBuf = frameBuffer.pixels() + scanline.y * frameBuffer.getWidth();

            for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                outBuf[x] = WHITE;
        } else {
            renderer.drawScanline(scanline.y);
        }
    }

    void LCDController::onHBlank()
    {
#ifndef LEGACY_RENDERING
        /* update stat */
        uint16_t stat = le(regs.DISPSTAT);
        stat = bitSet<uint16_t, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t, false>()>(stat);
        stat = bitSet<uint16_t, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t, true>()>(stat);
        regs.DISPSTAT = le(stat);
        scanline.vblanking = false;
        scanline.hblanking = true;
#endif

        /* use regsRef as those settings are crucial timewise */
        if (isBitSet<uint16_t, DISPSTAT::HBLANK_IRQ_ENABLE_OFFSET>(le(regs.DISPSTAT)))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

        internalRegs.DISPCNT = regs.DISPCNT;
        internalRegs.DISPSTAT = regs.DISPSTAT;
        internalRegs.VCOUNT = regs.VCOUNT;
        internalRegs.BGCNT[0] = regs.BGCNT[0];
        internalRegs.BGCNT[1] = regs.BGCNT[1];
        internalRegs.BGCNT[2] = regs.BGCNT[2];
        internalRegs.BGCNT[3] = regs.BGCNT[3];
        internalRegs.BGOFS[0] = regs.BGOFS[0];
        internalRegs.BGOFS[1] = regs.BGOFS[1];
        internalRegs.BGOFS[2] = regs.BGOFS[2];
        internalRegs.BGOFS[3] = regs.BGOFS[3];
        internalRegs.BG2P[0] = regs.BG2P[0];
        internalRegs.BG2P[1] = regs.BG2P[1];
        internalRegs.BG2P[2] = regs.BG2P[2];
        internalRegs.BG2P[3] = regs.BG2P[3];
        internalRegs.BG3P[0] = regs.BG3P[0];
        internalRegs.BG3P[1] = regs.BG3P[1];
        internalRegs.BG3P[2] = regs.BG3P[2];
        internalRegs.BG3P[3] = regs.BG3P[3];
        internalRegs.WIN0H = regs.WIN0H;
        internalRegs.WIN1H = regs.WIN1H;
        internalRegs.WIN0V = regs.WIN0V;
        internalRegs.WIN1V = regs.WIN1V;
        internalRegs.WININ = regs.WININ;
        internalRegs.WINOUT = regs.WINOUT;
        internalRegs.MOSAIC = regs.MOSAIC;
        internalRegs.BLDCNT = regs.BLDCNT;
        internalRegs.BLDALPHA = regs.BLDALPHA;
        internalRegs.BLDY = regs.BLDY;

        /* Set them to unit vectors in case they have length 0. */
        if (internalRegs.BG2P[0] == 0 && internalRegs.BG2P[2] == 0) {
            internalRegs.BG2P[0] = 0x100;
            internalRegs.BG2P[2] = 0;
        }

        if (internalRegs.BG2P[1] == 0 && internalRegs.BG2P[3] == 0) {
            internalRegs.BG2P[1] = 0;
            internalRegs.BG2P[3] = 0x100;
        }

        if (internalRegs.BG3P[0] == 0 && internalRegs.BG3P[2] == 0) {
            internalRegs.BG3P[0] = 0x100;
            internalRegs.BG3P[2] = 0;
        }

        if (internalRegs.BG3P[1] == 0 && internalRegs.BG3P[3] == 0) {
            internalRegs.BG3P[1] = 0;
            internalRegs.BG3P[3] = 0x100;
        }

        if (scanline.vCount < SCREEN_HEIGHT) {
            internalRegs.BG2X += signExt<int32_t, uint16_t, 16>(internalRegs.BG2P[1]);
            internalRegs.BG2Y += signExt<int32_t, uint16_t, 16>(internalRegs.BG2P[3]);

            internalRegs.BG3X += signExt<int32_t, uint16_t, 16>(internalRegs.BG3P[1]);
            internalRegs.BG3Y += signExt<int32_t, uint16_t, 16>(internalRegs.BG3P[3]);

            if (bgRefPointDirty[0][0]) {
                internalRegs.BG2X = regs.BG2X;
            }

            if (bgRefPointDirty[0][1]) {
                internalRegs.BG2Y = regs.BG2Y;
            }

            if (bgRefPointDirty[1][0]) {
                internalRegs.BG3X = regs.BG3X;
            }

            if (bgRefPointDirty[1][1]) {
                internalRegs.BG3Y = regs.BG3Y;
            }
        }

        bgRefPointDirty[0][0] = false;
        bgRefPointDirty[0][1] = false;
        bgRefPointDirty[1][0] = false;
        bgRefPointDirty[1][1] = false;

#ifndef LEGACY_RENDERING
        ++scanline.y;
#endif
    }

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
        bool forcedBlank = le(regs.DISPCNT) & DISPCTL::FORCED_BLANK_MASK;
        bool hblankIntervalFree = le(regs.DISPCNT) & DISPCTL::HBLANK_INTERVAL_FREE_MASK;

        return scanline.vblanking || ((!isOAMRegion || hblankIntervalFree) && scanline.hblanking) || forcedBlank;
    }

    std::string LCDController::getLayerStatusString() const
    {
        return renderer.getLayerStatusString();
    }
} // namespace gbaemu::lcd
