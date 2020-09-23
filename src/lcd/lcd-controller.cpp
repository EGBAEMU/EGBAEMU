#include "lcd-controller.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <algorithm>
#include <cassert>
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
    void LCDController::drawScanline()
    {
        int32_t i = 0;

        static color_t color = 0xFFFF0000;
        if (scanline.y == 0 || true) {
            if (color == 0xFFFF0000)
                color = 0xFF00FF00;
            else
                color = 0xFFFF0000;
        }

        for (const auto& l : backgroundLayers) {
            if (l->enabled) {
                l->drawScanline(scanline.y);

                int32_t yOff = (i / 2) * SCREEN_HEIGHT;
                int32_t xOff = (i % 2) * SCREEN_WIDTH;

                for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                    display.target.pixels()[(yOff + scanline.y) * display.target.getWidth() + (x + xOff)] = l->scanline[x];
            }

            ++i;
        }
    }

    void LCDController::renderTick()
    {
        /*
            [ 960 cycles for 240 dots ][ 272 cycles for hblank ]
            ...
            [ 960 cycles for 240 dots ][ 272 cycles for hblank ]
            [ 83776 cycles for vblank (no hblank) ]
         */

        /* cycle in the current display period */
        uint32_t displayCycle = scanline.cycle % 280896;
        scanline.vblanking = (displayCycle >= 197120);

        if (displayCycle == 197120 && scanline.vblanking) {
            /* on first v-blank cycle */
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);
            scanline.y = 0;

            /* tell the window it can present the image */
            canDrawToScreenMutex->lock();
            *canDrawToScreen = true;
            canDrawToScreenMutex->unlock();
        } else if (!scanline.vblanking) {
            /* cycle in the current scanline period */
            uint32_t scanlineCycle = scanline.cycle % 1232;
            scanline.hblanking = scanlineCycle >= 960;

            if (scanlineCycle == 0 && !scanline.hblanking) {
                loadSettings();
                drawScanline();
            }

            /* 4 cycles per pixel */
            if (scanlineCycle % 4 == 0 && !scanline.hblanking) {
                ++scanline.x;
            } else if (scanlineCycle == 960 && scanline.hblanking) {
                /* on first h-blank cycle */
                irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

                scanline.x = 0;
                ++scanline.y;
            }
        }

        /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(scanline.vblanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(scanline.hblanking));
        regsRef.DISPSTAT = le(stat);

        /* update vcount */
        uint16_t vcount = le(regsRef.VCOUNT);
        vcount = bitSet<uint16_t>(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, scanline.y);
        regsRef.VCOUNT = le(vcount);

        ++scanline.cycle;
    }

    void LCDController::setupLayers()
    {
        backgroundLayers[0] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG0);
        backgroundLayers[1] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG1);
        backgroundLayers[2] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG2);
        backgroundLayers[3] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG3);
    }

    void LCDController::sortLayers()
    {
        /*
        sortedBackgroundLayers = backgroundLayers;
        std::sort(sortedBackgroundLayers.begin(), sortedBackgroundLayers.end(),
                  [](const std::shared_ptr<Layer> &l1, const std::shared_ptr<Layer> &l2) -> bool {
                      if (l1->priority != l2->priority)
                          return l1->priority > l2->priority;
                      else
                          return l1->id > l2->id;
                  });

        int32_t bgLayer = 0;
        int32_t objLayer = 3;
        int32_t index = 0;

        while (true) {
            if (bgLayer < 4 && objLayer >= 0) {
                auto bgPrio = sortedBackgroundLayers[bgLayer]->priority;
                auto objPrio = objLayers[objLayer]->priority;

                if (bgPrio >= objPrio) {
                    layers[index] = sortedBackgroundLayers[bgLayer++];
                } else {
                    layers[index] = objLayers[objLayer--];
                }
            } else if (bgLayer >= 4 && objLayer >= 0) {
                layers[index] = objLayers[objLayer--];
            } else if (objLayer < 0 && bgLayer < 4) {
                layers[index] = sortedBackgroundLayers[bgLayer++];
            } else {
                break;
            }

            ++index;
        }
         */
    }

    void LCDController::loadSettings()
    {
        /* copy registers, they cannot be modified when rendering */
        regs = regsRef;

        Memory::MemoryRegion memReg;
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET, nullptr, memReg));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200, nullptr, memReg));

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        Memory::MemoryRegion region;
        const uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, region);
        const uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, region);

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        for (uint32_t i = 0; i < 4; ++i)
            if (backgroundLayers[i]->enabled)
                backgroundLayers[i]->loadSettings(static_cast<BGMode>(bgMode), regs);

        sortLayers();
    }
} // namespace gbaemu::lcd
