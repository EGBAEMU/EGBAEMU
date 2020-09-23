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
    void LCDController::setupLayers()
    {
        backgroundLayers[0] = std::make_shared<Background>(Layer::LayerId::BG0);
        objLayers[0] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ0);
        objLayers[0]->priority = 0;

        backgroundLayers[1] = std::make_shared<Background>(Layer::LayerId::BG1);
        objLayers[1] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ1);
        objLayers[1]->priority = 1;

        backgroundLayers[2] = std::make_shared<Background>(Layer::LayerId::BG2);
        objLayers[2] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ2);
        objLayers[2]->priority = 2;

        backgroundLayers[3] = std::make_shared<Background>(Layer::LayerId::BG3);
        objLayers[3] = std::make_shared<OBJLayer>(Layer::LayerId::OBJ3);
        objLayers[3]->priority = 3;
    }

    void LCDController::sortLayers()
    {
        sortedBackgroundLayers = backgroundLayers;
        std::sort(sortedBackgroundLayers.begin(), sortedBackgroundLayers.end(),
                  [](const std::shared_ptr<Layer> &l1, const std::shared_ptr<Layer> &l2) -> bool {
                      if (l1->priority != l2->priority)
                          return l1->priority > l2->priority;
                      else
                          return l1->id > l2->id;
                  });

        /*
        uint32_t layerIndex = 0;
        uint32_t sortedIndex = 0;
        int32_t objIndex = 3;

        while (layerIndex < 8) {
            uint16_t prio = sortedBackgroundLayers[sortedIndex]->priority;
            layers[layerIndex++] = sortedBackgroundLayers[sortedIndex++];

            for (; objIndex >= 0 && (sortedIndex == 4 || objLayers[objIndex]->priority == prio); objIndex--) {
                layers[layerIndex++] = objLayers[objIndex];
            }
        }
         */

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
    }

    void LCDController::loadWindowSettings()
    {
        struct Window
        {
            int32_t left, right, top, bottom;
        };

        Window win0, win1;

        win0.right = bitGet(le(regs.WIN0H), WINDOW::LOWER_COORD_MASK, WINDOW::LOWER_COORD_OFFSET);
        win0.left = bitGet(le(regs.WIN0H), WINDOW::UPPER_COORD_MASK, WINDOW::UPPER_COORD_OFFSET);
        win0.bottom = bitGet(le(regs.WIN0V), WINDOW::LOWER_COORD_MASK, WINDOW::LOWER_COORD_OFFSET);
        win0.top = bitGet(le(regs.WIN0V), WINDOW::UPPER_COORD_MASK, WINDOW::UPPER_COORD_OFFSET);

        win1.right = bitGet(le(regs.WIN1H), WINDOW::LOWER_COORD_MASK, WINDOW::LOWER_COORD_OFFSET);
        win1.left = bitGet(le(regs.WIN1H), WINDOW::UPPER_COORD_MASK, WINDOW::UPPER_COORD_OFFSET);
        win1.bottom = bitGet(le(regs.WIN1V), WINDOW::LOWER_COORD_MASK, WINDOW::LOWER_COORD_OFFSET);
        win1.top = bitGet(le(regs.WIN1V), WINDOW::UPPER_COORD_MASK, WINDOW::UPPER_COORD_OFFSET);



    }

    void LCDController::loadSettings()
    {
        /* copy registers, they cannot be modified when rendering */
        //regs = regsRef;

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        std::cout << bgMode << std::endl;
        Memory::MemoryRegion region;
        const uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET, nullptr, region);
        const uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, region);

        /* obj layer */
        for (const auto &layer : objLayers)
            layer->setMode(vramBase, oamBase, bgMode);

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        if (bgMode == 0) {
            for (uint32_t i = 0; i < 4; ++i) {
                backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

                if (backgroundLayers[i]->enabled) {
                    backgroundLayers[i]->loadSettings(0, i, regs, memory);
                }
            }
        } else if (bgMode == 1) {
            backgroundLayers[0]->loadSettings(0, 0, regs, memory);
            backgroundLayers[1]->loadSettings(0, 1, regs, memory);
            backgroundLayers[2]->loadSettings(2, 2, regs, memory);
        } else if (bgMode == 2) {
            backgroundLayers[2]->loadSettings(2, 2, regs, memory);
            backgroundLayers[3]->loadSettings(2, 3, regs, memory);
        } else if (bgMode == 3) {
            backgroundLayers[2]->loadSettings(3, 2, regs, memory);
        } else if (bgMode == 4) {
            backgroundLayers[2]->loadSettings(4, 2, regs, memory);
        } else if (bgMode == 5) {
            backgroundLayers[2]->loadSettings(5, 2, regs, memory);
        } else {
            LOG_LCD(std::cout << "WARNING: unsupported bg mode " << bgMode << "\n";);
        }

        /* load special color effects */
        uint16_t bldcnt = le(regs.BLDCNT);
        uint16_t bldAlpha = le(regs.BLDALPHA);
        uint16_t bldy = le(regs.BLDY);

        /* what actual special effect is used? */
        colorSpecialEffect = static_cast<BLDCNT::ColorSpecialEffect>(
            bitGet(bldcnt, BLDCNT::COLOR_SPECIAL_FX_MASK, BLDCNT::COLOR_SPECIAL_FX_OFFSET));

        switch (colorSpecialEffect) {
            case BLDCNT::BrightnessIncrease: case BLDCNT::BrightnessDecrease:
                /* 0/16, 1/16, 2/16, ..., 16/16, 16/16, ..., 16/16 */
                brightnessEffect.evy = std::min(16, bldy & 0x1F);
                break;
            case BLDCNT::AlphaBlending:
                alphaEffect.eva = std::min(16, bldAlpha & 0x1F);
                alphaEffect.evb = std::min(16, (bldAlpha >> 8) & 0x1F);
                break;
        }

        for (int32_t i = 0; i < 4; ++i) {
            backgroundLayers[i]->asFirstTarget = bitGet(bldcnt, BLDCNT::TARGET_MASK, BLDCNT::BG_FIRST_TARGET_OFFSET(i));
            backgroundLayers[i]->asSecondTarget = bitGet(bldcnt, BLDCNT::TARGET_MASK, BLDCNT::BG_SECOND_TARGET_OFFSET(i));
        }

        /* all equal */
        objLayers[0]->asFirstTarget = bitGet(bldcnt, BLDCNT::TARGET_MASK, BLDCNT::OBJ_FIRST_TARGET_OFFSET);
        objLayers[0]->asSecondTarget = bitGet(bldcnt, BLDCNT::TARGET_MASK, BLDCNT::OBJ_SECOND_TARGET_OFFSET);

        for (int32_t i = 1; i < 4; ++i) {
            objLayers[i]->asFirstTarget = objLayers[i - 1]->asFirstTarget;
            objLayers[i]->asSecondTarget = objLayers[i - 1]->asSecondTarget;
        }

        sortLayers();
    }

    void LCDController::onHBlank()
    {

    }

    void LCDController::onVBlank()
    {
    }

    void LCDController::copyLayer(const Canvas<color_t> &src)
    {
        auto dstPixels = display.target.pixels();
        auto dstStride = display.target.getWidth();

        auto srcPixels = src.pixels();
        auto srcStride = src.getWidth();

        for (int32_t y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                color_t color = srcPixels[y * srcStride + x];

                if (color & 0xFF000000)
                    dstPixels[y * dstStride + x] = color;
            }
        }
    }

    void LCDController::drawToTarget()
    {
        /*
            At this point every background and the object layer contain all the pixels that will be drawn to the screen.
            The last step is to mix the two appropriate layers and draw all other layers according to priority to the display.
         */

        auto dst = display.target.pixels();
        auto w = display.target.getWidth();
        display.target.beginDraw();

        color_t backdropColor = palette.getBackdropColor();

        bool firstTargetEnabled = colorSpecialEffect == BLDCNT::AlphaBlending ||
                              colorSpecialEffect == BLDCNT::BrightnessIncrease ||
                              colorSpecialEffect == BLDCNT::BrightnessDecrease;
        bool secondTargetEnabled = colorSpecialEffect == BLDCNT::AlphaBlending;

        for (int32_t y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                int32_t coord = y * SCREEN_WIDTH + x;
                color_t topColor = backdropColor,
                        bottomColor = backdropColor,
                        finalColor = backdropColor;

#if (RENDERER_ENABLE_COLOR_EFFECTS == 1)

                int32_t topLayer = -1;

                bool topSelectedAsTarget = false;
                bool bottomSelectedAsTarget = false;

                /* select top color */
                for (int32_t i = layers.size() - 1; i >= 0; --i) {
                    if (!layers[i]->enabled)
                        continue;
                    
                    color_t color = layers[i]->canvas.pixels()[coord];

                    /* transparent, ignore */
                    //if (!(color & 0xFF000000))
                    //    continue;

                    if (color & 0xFF000000) {
                        topColor = color;
                        topLayer = i;
                        topSelectedAsTarget = layers[i]->asFirstTarget;

                        break;
                    }
                }

                if (colorSpecialEffect != BLDCNT::None) {
                    for (int32_t i = layers.size() - 1;  i >= 0; -- i) {
                        if (!layers[i]->enabled || i == topLayer)
                            continue;

                        color_t color = layers[i]->canvas.pixels()[coord];

                        /* transparent, ignore */
                        if (!(color & 0xFF000000) || i == topLayer)
                            continue;

                        bottomColor = color;
                        bottomSelectedAsTarget = layers[i]->asSecondTarget;
                        
                        break;
                    }
                }

                if (topSelectedAsTarget && colorSpecialEffect != BLDCNT::None) {
                    switch (colorSpecialEffect) {
                        case BLDCNT::ColorSpecialEffect::None:
                            //just the top layer
                            finalColor = topColor;
                            break;
                        case BLDCNT::ColorSpecialEffect::BrightnessIncrease: {
                            //finalColor = topColor + (255 - topColor) * brightnessEffect.evy;

                            color_t inverted = colSub(0xFFFFFFFF, topColor);
                            color_t scaledEvy = colScale(inverted, brightnessEffect.evy);
                            finalColor = colAdd(topColor, scaledEvy);

                            //finalColor = topColor;

                            break;
                        }
                        case BLDCNT::ColorSpecialEffect::BrightnessDecrease: {
                            //finalColor = topColor - topColor * brightnessEffect.evy;

                            color_t scaledEvy = colScale(topColor, brightnessEffect.evy);
                            finalColor = colSub(topColor, scaledEvy);

                            //finalColor = topColor;

                            break;
                        }
                        case BLDCNT::ColorSpecialEffect::AlphaBlending: {
                            //finalColor = std::min<color_t>(31, topColor * alphaEffect.eva + bottomColor * alphaEffect.evb);

                            finalColor = 0;

                            for (uint32_t i = 0; i < 4; ++i) {
                                color_t top = (topColor >> (i * 8)) & 0xFF;
                                color_t bot = (bottomColor >> (i * 8)) & 0xFF;
                                color_t chan = std::min(255u, top * alphaEffect.eva / 16 + bot * alphaEffect.evb / 16);
                                finalColor |= chan << (i * 8);
                            }

                            //finalColor = topColor;

                            break;
                        }
                        default:
                            finalColor = topColor;
                            break;
                    }
                } else {
                    finalColor = topColor;
                }

#else
                /* just use the top non transparent pixel */
                for (int32_t i = layers.size() - 1; i >= 0; --i) {
                    if (!layers[i]->enabled)
                        continue;

                    color_t color = layers[i]->canvas.pixels()[coord];

                    if (color & 0xFF000000) {
                        finalColor = color;
                        break;
                    }
                }

#endif

                /* upscaling */
                for (int32_t ty = 0; ty < display.yStep; ++ty)
                    for (int32_t tx = 0; tx < display.xStep; ++tx)
                        dst[(display.yStep * y + ty) * w + (display.xStep * x + tx)] = finalColor;
            }
        }

        display.target.endDraw();
    }

    void LCDController::drawLayers()
    {
        display.target.beginDraw();

        for (int32_t i = 0; i < layers.size(); ++i) {
            int32_t xOff = (i % 2) * SCREEN_WIDTH;
            int32_t yOff = (i / 2) * SCREEN_HEIGHT;

            for (int32_t y = 0; y < SCREEN_HEIGHT; ++y) {
                for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                    color_t color = layers[i]->canvas.pixels()[y * SCREEN_WIDTH + x];
                    display.target.pixels()[(y + yOff) * display.target.getWidth() + (x + xOff)] = color;
                }
            }
        }

        display.target.endDraw();
    }

    void LCDController::render()
    {
        ++counters.renderCount;

        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;
        /* clear with brackdrop color */
        color_t clearColor = palette.getBackdropColor();

        switch (bgMode) {
            case 0:
                for (size_t i = 0; i < 4; ++i) {
                    backgroundLayers[i]->renderBG0(palette);
                    backgroundLayers[i]->draw();
                }
                break;
            case 1:
                /*
                    id 0, 1 in BG0
                    id 2 in BG2
                 */
                backgroundLayers[0]->renderBG0(palette);
                backgroundLayers[0]->draw();

                backgroundLayers[1]->renderBG0(palette);
                backgroundLayers[1]->draw();

                backgroundLayers[2]->renderBG2(palette);
                backgroundLayers[2]->draw();
                break;
            case 2:
                backgroundLayers[2]->renderBG2(palette);
                backgroundLayers[2]->draw();

                backgroundLayers[3]->renderBG2(palette);
                backgroundLayers[3]->draw();
                break;
            case 3:
                backgroundLayers[2]->renderBG3(memory);
                backgroundLayers[2]->draw();
                break;
            case 4:
                backgroundLayers[2]->renderBG4(palette, memory);
                backgroundLayers[2]->draw();
                break;
            case 5:
                backgroundLayers[2]->renderBG5(palette, memory);
                backgroundLayers[2]->draw();
                break;

            default:
                LOG_LCD(std::cout << "WARNING: Unsupported background mode " << std::dec << bgMode << "!\n";);
                break;
        }

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);

        objLayers[0]->draw(palette, use2dMapping);
        objLayers[1]->draw(palette, use2dMapping);
        objLayers[2]->draw(palette, use2dMapping);
        objLayers[3]->draw(palette, use2dMapping);

#if RENDERER_DECOMPOSE_LAYERS == 1
        drawLayers();
#else
        drawToTarget();
#endif
        /*
        static int i = 0;

        ++i;
        if (i % 100 == 0)
            std::cout << getLayerStatus() << std::endl;
         */

#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
        {
            //static int i = 0;

            int32_t locX = SCREEN_WIDTH * display.xStep;
            int32_t locY = 0;

            display.target.beginDraw();

            int32_t i = objDebugCanvasIndex;

            for (int32_t y = 0; y < objLayers[i]->debugCanvas.getHeight(); ++y) {
                for (int32_t x = 0; x < objLayers[i]->debugCanvas.getWidth(); ++x) {
                    color_t color = objLayers[i]->debugCanvas.pixels()[y * objLayers[i]->debugCanvas.getWidth() + x];
                    display.target.pixels()[(y + locY) * display.target.getWidth() + (x + locX)] = color;
                }
            }

            display.target.endDraw();

            if (counters.renderCount % 10 == 0)
                i = (i + 1) % 4;
        }
#endif
    }

    void LCDController::renderLoop()
    {
        while (true) {
            renderControlMutex.lock();
            bool exitLoop = (renderControl == EXIT);
            bool wait = (renderControl == WAIT);
            renderControl = WAIT;
            renderControlMutex.unlock();

            if (wait) {
                continue;
            } else if (exitLoop) {
                break;
            }

            loadSettings();
            render();

            /* Tell the window we are done, if it isn't ready it has to try next time. */
            canDrawToScreenMutex->lock();
            *canDrawToScreen = true;
            canDrawToScreenMutex->unlock();

            /*
                Allow another call to render() only if render() has finished. If onHBlank() gets executed
                while we are rendering we crash.
             */
        }
    }

    void LCDController::updateReferences()
    {
        Memory::MemoryRegion memReg;
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET, nullptr, memReg));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200, nullptr, memReg));
    }

    bool LCDController::tick()
    {
        static bool irqTriggeredV = false;
        static bool irqTriggeredH = false;

        ++counters.tickCount;

        bool result = false;
        uint32_t vState = counters.cycle % 280896;
        counters.vBlanking = vState >= 197120;

        /* TODO: this is guesswork, maybe h and v blanking can be enabled concurrently */
        if (!counters.vBlanking) {
            uint32_t hState = counters.cycle % 1232;
            counters.hBlanking = hState >= 960;
            counters.vCount = counters.cycle / 1232;
            irqTriggeredV = false;
            if (!counters.hBlanking) {
                irqTriggeredH = false;
            }
        } else {
            counters.vCount = 0;
        }

        if (!irqTriggeredV && counters.vBlanking) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_V_BLANK);
            irqTriggeredV = true;
        }
        if (!irqTriggeredH && counters.hBlanking) {
            irqHandler.setInterrupt(InterruptHandler::InterruptType::LCD_H_BLANK);
            irqTriggeredH = true;
        }

        /* rendering once per v-blank */
        if (!counters.vBlanking && vState == 0) {
            /* This is a milestone: Fixing this bug took 11 hours to find and 30 seconds to fix. */
            onVBlank();

            regs = regsRef;
        
            renderControlMutex.lock();
            renderControl = RUN;
            renderControlMutex.unlock();
        }

        /* update stat */
        uint16_t stat = le(regsRef.DISPSTAT);
        stat = bitSet(stat, DISPSTAT::VBLANK_FLAG_MASK, DISPSTAT::VBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.vBlanking));
        stat = bitSet(stat, DISPSTAT::HBLANK_FLAG_MASK, DISPSTAT::HBLANK_FLAG_OFFSET, bmap<uint16_t>(counters.hBlanking));
        //stat = bitSet(stat, DISPSTAT::VCOUNT_SETTING_MASK, DISPSTAT::VCOUNT_SETTING_OFFSET, counters.vCount);
        regsRef.DISPSTAT = le(stat);

        /* update vcount */
        uint16_t vcount = le(regsRef.VCOUNT);
        vcount = bitSet(vcount, VCOUNT::CURRENT_SCANLINE_MASK, VCOUNT::CURRENT_SCANLINE_OFFSET, counters.vCount);
        regsRef.VCOUNT = le(vcount);

        ++counters.cycle;

        return result;
    }

    void LCDController::exitThread()
    {
        if (!renderThread->joinable())
            return;

        LOG_LCD(std::cout << "Waiting for render thread to exit." << std::endl;);
        renderControlMutex.lock();
        renderControl = EXIT;
        renderControlMutex.unlock();
        renderThread->join();
        LOG_LCD(std::cout << "Render thread exited." << std::endl;);
    }

    std::string LCDController::getLayerStatus() const
    {
        std::stringstream ss;

        ss << "background mode " << std::dec << (le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK) << '\n';

        for (int32_t i = 0; i < layers.size(); ++i) {
            ss << "==== layer " << i << " <" << (layers[i]->id <= 3 ? "BG" : "OBJ") << "> ====\n";
            ss << "enabled: " << (layers[i]->enabled ? "true" : "false") << '\n';
            ss << "first target: " << (layers[i]->asFirstTarget ? "true" : "false") << '\n';
            ss << "second target: " << (layers[i]->asSecondTarget ? "true" : "false") << '\n';
            ss << "priority: " << layers[i]->priority << '\n';
            ss << "id: " << layers[i]->id << '\n';
        }

        return ss.str();
    }

    void LCDController::objHightlightSetIndex(int32_t index)
    {
        for (auto& l : objLayers)
            l->hightlightObjIndex = index;
    }

    void LCDController::objSetDebugCanvas(int32_t i)
    {
        objDebugCanvasIndex = i;
    }

    std::string LCDController::getOBJLayerString(uint32_t objIndex) const
    {
        return objLayers[0]->getOBJInfo(objIndex).toString();
    }
} // namespace gbaemu::lcd
