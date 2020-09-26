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
        auto t = std::chrono::high_resolution_clock::now();

        for (const auto& l : layers)
            if (l->enabled)
                l->drawScanline(scanline.y);

        sortLayers();

#if RENDERER_DECOMPOSE_LAYERS == 1
        {
            int32_t i = 0;

            for (const auto& l : layers) {
                if (l->enabled) {
                    int32_t yOff = (i / 2) * SCREEN_HEIGHT;
                    int32_t xOff = (i % 2) * SCREEN_WIDTH;

                    for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                        display.target.pixels()[(yOff + scanline.y) * display.target.getWidth() + (x + xOff)] = l->scanline[x];
                }

                ++i;
            }
        }
#else
        windowFeature.composeScanline(layers, display.target.pixels() + scanline.y * display.target.getWidth());
#endif

        //std::cout << std::dec << std::chrono::duration_cast<std::chrono::microseconds>((std::chrono::high_resolution_clock::now() - t)).count() << std::endl;
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
        uint32_t scanlineCycle = scanline.cycle % 1232;

        scanline.vblanking = (displayCycle >= 197120);
        /* h-blank flag is 1 slightly after the scanline has been drawn */
        scanline.hblanking = (scanlineCycle >= 1006);
        scanline.vCount = displayCycle / 1232;

        onVCount();

        if (displayCycle == 197120 && scanline.vblanking) {
            /* on first v-blank cycle */
            onVBlank();
            /* race conditions are acceptable here */
            *canDrawToScreen = true;
        } else if (!scanline.vblanking) {
            /* h-blank interrupt, scanline rendering, scanline.x increase is only done when not v-blanking */
            if (scanlineCycle == 0 && !scanline.hblanking) {
                loadSettings();
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

        /* update vcount */
        uint16_t vcount = le(regsRef.VCOUNT);
        vcount = bitSet<uint16_t>(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, scanline.vCount);
        regsRef.VCOUNT = le(vcount);

         /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(scanline.vblanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(scanline.hblanking));
        regsRef.DISPSTAT = le(stat);

        ++scanline.cycle;
    }

    void LCDController::onVCount()
    {
        /* use regsRef as those settings are crucial timewise */
        if (bitGet(le(regsRef.DISPSTAT), DISPSTAT::VCOUNTER_IRQ_ENABLE_MASK, DISPSTAT::VCOUNTER_IRQ_ENABLE_OFFSET)) {
            uint16_t vCountSetting = bitGet(le(regsRef.DISPSTAT), DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET);

            if (scanline.vCount == vCountSetting)
                irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_COUNTER_MATCH);
        }
    }

    void LCDController::onHBlank()
    {
        /* use regsRef as those settings are crucial timewise */
        if (bitGet(le(regsRef.DISPSTAT), DISPSTAT::HBLANK_IRQ_ENABLE_MASK, DISPSTAT::HBLANK_IRQ_ENABLE_OFFSET))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);

        scanline.x = 0;
        ++scanline.y;
    }

    void LCDController::onVBlank()
    {
        /* use regsRef as those settings are crucial timewise */
        if (bitGet(le(regsRef.DISPSTAT), DISPSTAT::VBLANK_IRQ_ENABLE_MASK, DISPSTAT::VBLANK_IRQ_ENABLE_OFFSET))
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);

        scanline.y = 0;
    }

    void LCDController::setupLayers()
    {
        backgroundLayers[0] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG0);
        backgroundLayers[1] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG1);
        backgroundLayers[2] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG2);
        backgroundLayers[3] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG3);

        objLayers[0] = std::make_shared<OBJLayer>(memory, palette, 0);
        objLayers[1] = std::make_shared<OBJLayer>(memory, palette, 1);
        objLayers[2] = std::make_shared<OBJLayer>(memory, palette, 2);
        objLayers[3] = std::make_shared<OBJLayer>(memory, palette, 3);

        for (uint32_t i = 0; i < 8; ++i) {
            if (i <= 3)
                layers[i] = objLayers[i];
            else
                layers[i] = backgroundLayers[i - 4];
        }
    }

    void LCDController::sortLayers()
    {
        std::stable_sort(layers.begin(), layers.end(), [](const std::shared_ptr<Layer>& pa, const std::shared_ptr<Layer>& pb) -> bool
        {
            return *pa < *pb;
        });
    }

    void LCDController::loadSettings()
    {
        /* copy registers, they cannot be modified when rendering */
        regs = regsRef;

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        for (uint32_t i = 0; i < 4; ++i)
            if (backgroundLayers[i]->enabled)
                backgroundLayers[i]->loadSettings(static_cast<BGMode>(bgMode), regs);

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);

        for (auto& l : objLayers) {
            l->setMode(static_cast<BGMode>(bgMode), use2dMapping);
            l->loadOBJs();
        }

        palette.loadPalette(memory);
        windowFeature.load(regs, palette.getBackdropColor());

        sortLayers();
    }

    std::string LCDController::getLayerStatusString() const
    {
        std::stringstream ss;

        for (const auto& pLayer : layers) {
            ss << "================================\n";
            ss << "enabled: " << (pLayer->enabled ? "yes" : "no") << '\n';
            ss << "id: " << layerIDToString(pLayer->layerID) << '\n';
            ss << "priority: " << pLayer->priority << '\n';
        }

        return ss.str();
    }
} // namespace gbaemu::lcd
