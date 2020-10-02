#include "renderer.hpp"

#include <optional>

namespace gbaemu::lcd
{
    void Renderer::setupLayers()
    {
        backgroundLayers[0] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG0);
        backgroundLayers[1] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG1);
        backgroundLayers[2] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG2);
        backgroundLayers[3] = std::make_shared<BGLayer>(palette, memory, BGIndex::BG3);

        objLayers[0] = std::make_shared<OBJLayer>(memory, palette, regs, 0, objManager);
        objLayers[1] = std::make_shared<OBJLayer>(memory, palette, regs, 1, objManager);
        objLayers[2] = std::make_shared<OBJLayer>(memory, palette, regs, 2, objManager);
        objLayers[3] = std::make_shared<OBJLayer>(memory, palette, regs, 3, objManager);

        for (uint32_t i = 0; i < 8; ++i) {
            if (i <= 3)
                layers[i] = objLayers[i];
            else
                layers[i] = backgroundLayers[i - 4];
        }

        windowOBJLayer = std::make_shared<OBJLayer>(memory, palette, regs, 0, objManager);
        windowFeature.objWindow.objLayer = windowOBJLayer;
    }

    void Renderer::sortLayers()
    {
        std::stable_sort(layers.begin(), layers.end(), [](const std::shared_ptr<Layer> &pa, const std::shared_ptr<Layer> &pb) -> bool {
            return *pa < *pb;
        });
    }

    void Renderer::loadSettings(int32_t y)
    {
        /* copy registers, they cannot be modified when rendering */
        BGMode bgMode = static_cast<BGMode>(le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK);

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        /* All background layers can be enabled/disabled with a single flag. */
        for (uint32_t i = 0; i < 4; ++i)
            objLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_OBJ_MASK;

        for (uint32_t i = 0; i < 4; ++i)
            if (backgroundLayers[i]->enabled)
                backgroundLayers[i]->loadSettings(bgMode, regs);

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);

        /* load objects */
        Memory::MemoryRegion memReg;
        const uint8_t *oamBase = memory.resolveAddr(Memory::OAM_OFFSET, nullptr, memReg);
        objManager->load(oamBase, bgMode);

        /* load objects for each layer */
        for (auto &l : objLayers) {
            l->setMode(bgMode, use2dMapping);
            l->loadOBJs(y, [](const OBJ& obj, real_t fy, uint16_t priority) -> bool {
                return obj.priority == priority &&
                       obj.visible &&
                       obj.mode != OBJ_WINDOW &&
                       obj.intersectsWithScanline(fy); });
        }

        /* window objects */
        windowOBJLayer->setMode(bgMode, use2dMapping);
        windowOBJLayer->loadOBJs(y, [](const OBJ& obj, real_t fy, uint16_t priority) -> bool {
            return obj.visible &&
                   obj.mode == OBJ_WINDOW &&
                   obj.intersectsWithScanline(fy); });

        palette.loadPalette(memory);
        windowFeature.load(regs, y, palette.getBackdropColor());
        colorEffects.load(regs);

        sortLayers();
    }

    void Renderer::blendDefault(int32_t y, int32_t xTo)
    {
        color_t *outBuf = target.pixels() + y * target.getWidth();

        for (int32_t x = 0; x < xTo; ++x) {
            color_t finalColor = palette.getBackdropColor();

            for (const auto &l : layers) {
                if (!l->enabled || !flagLayerEnabled(windowFeature.enabledMask.mask[x], l->layerID))
                    continue;

                color_t color = l->scanline[x].color;

                if (color == TRANSPARENT)
                    continue;

                finalColor = color;
                break;
            }

            outBuf[x] = finalColor;
        }
    }

    void Renderer::blendBrightness(int32_t y, int32_t xTo)
    {
        color_t *outBuf = target.pixels() + y * target.getWidth();
        std::function<color_t(color_t, color_t)> applyColorEffect = colorEffects.getBlendingFunction();

        for (int32_t x = 0; x < xTo; ++x) {
            color_t finalColor = palette.getBackdropColor();
            uint8_t windowMask = windowFeature.enabledMask.mask[x];

            for (const auto &l : layers) {
                if (!l->enabled || !flagLayerEnabled(windowMask, l->layerID))
                    continue;

                color_t color = l->scanline[x].color;

                if (color == TRANSPARENT)
                    continue;

                if (l->scanline[x].asFirstColor() && flagCFXEnabled(windowMask)) {
                    finalColor = applyColorEffect(color, TRANSPARENT);
                } else
                    finalColor = color;

                break;
            }

            outBuf[x] = finalColor;
        }
    }

    void Renderer::blendAlpha(int32_t y, int32_t xTo)
    {
        color_t *outBuf = target.pixels() + y * target.getWidth();
        std::function<color_t(color_t, color_t)> applyColorEffect = colorEffects.getBlendingFunction();

        for (int32_t x = 0; x < xTo; ++x) {
            auto it = layers.cbegin();
            bool asFirst = false;
            bool asSecond = false;
            color_t firstColor = palette.getBackdropColor();
            color_t secondColor = palette.getBackdropColor();
            color_t finalColor = palette.getBackdropColor();

            for (; it != layers.cend(); it++) {
                const auto &l = *it;

                if (!l->enabled || !flagLayerEnabled(windowFeature.enabledMask.mask[x], l->layerID))
                    continue;

                Fragment frag = l->scanline[x];

                if (frag.color == TRANSPARENT)
                    continue;

                if (!frag.asFirstAlpha() && !frag.asFirstColor()) {
                    asFirst = false;
                } else {
                    asFirst = true;
                }

                firstColor = frag.color;
                break;
            }

            /* early abort, no blending */
            if (!asFirst) {
                outBuf[x] = firstColor;
                continue;
            }

            /* find second color */
            if (it != layers.cend()) {
                for (it++; it != layers.cend(); it++) {
                    const auto &l = *it;

                    if (!l->enabled || !flagLayerEnabled(windowFeature.enabledMask.mask[x], l->layerID))
                        continue;

                    Fragment frag = l->scanline[x];

                    if (frag.color == TRANSPARENT)
                        continue;

                    secondColor = frag.color;
                    asSecond = frag.asSecondColor();
                    break;
                }
            }

            /* Who thought of this crap?! */

            /* blend */
            if (asSecond)
                outBuf[x] = applyColorEffect(firstColor, secondColor);
            else
                outBuf[x] = firstColor;
        }
    }

    void Renderer::blendDecomposed(int32_t y)
    {
        for (int32_t i = 0; i < layers.size(); ++i) {
            const auto& l = layers[i];

            if (!l->enabled)
                continue;

            int32_t xOff = (i % 2) * SCREEN_WIDTH;
            int32_t yOff = (i / 2) * SCREEN_HEIGHT;

            color_t *outBuf = target.pixels() + (y + yOff) * target.getWidth() + xOff;

            for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
                color_t color = l->scanline[x].color;
                
                if (color == TRANSPARENT)
                    color = RENDERER_DECOMPOSE_BG_COLOR;

                outBuf[x] = color;
            }
        }
    }

    void Renderer::renderLoop()
    {
        return;
        while (true) {
            renderControlMutex.lock();
            auto ctrl = renderControl;

            if (ctrl == WAIT) {
                renderControlMutex.unlock();
                continue;
            }

            if (ctrl == EXIT) {
                renderControlMutex.unlock();
                break;
            }

            if (ctrl == RUN) {
                auto t = std::chrono::high_resolution_clock::now();

                for (const auto &l : layers) {
                    if (l->enabled) {
                        l->drawScanline(0);
                    }
                }

#if RENDERER_DECOMPOSE_LAYERS == 1
#else
                switch (colorEffects.getEffect()) {
                    case BLDCNT::ColorSpecialEffect::AlphaBlending:
                        blendAlpha(0);
                        break;
                    case BLDCNT::ColorSpecialEffect::BrightnessIncrease:
                    case BLDCNT::ColorSpecialEffect::BrightnessDecrease:
                        blendBrightness(0);
                        break;
                    default:
                    case BLDCNT::ColorSpecialEffect::None:
                        blendDefault(0);
                        break;
                }
#endif
                ctrl = WAIT;
                renderControlMutex.unlock();
            }
        }
    }

    Renderer::Renderer(Memory &mem, InterruptHandler &irq, const LCDIORegs &registers, Canvas<color_t>& targetCanvas) :
        memory(mem), irqHandler(irq), regs(registers), target(targetCanvas), objManager(std::make_shared<OBJManager>())
    {
        setupLayers();

        //renderControl = WAIT;
        //renderThread = std::thread(&Renderer::renderLoop, this);
    }

    Renderer::~Renderer()
    {
        //renderThread.join();
    }

    void Renderer::drawScanline(int32_t y)
    {
        /*
        if (y == 0)
            drawOdd = !drawOdd;

        if (drawOdd == (y % 2 == 0))
            return;
         */

        loadSettings(y);

        auto t = std::chrono::high_resolution_clock::now();

        for (const auto &l : layers)
            if (l->enabled)
                l->drawScanline(y);

        windowOBJLayer->drawScanline(y);

#if (RENDERER_DECOMPOSE_LAYERS == 1)
        blendDecomposed(y);
#else
        switch (colorEffects.getEffect()) {
            case BLDCNT::ColorSpecialEffect::AlphaBlending:
                blendAlpha(y);
                break;
            case BLDCNT::ColorSpecialEffect::BrightnessIncrease:
            case BLDCNT::ColorSpecialEffect::BrightnessDecrease:
                blendBrightness(y);
                break;
            default:
            case BLDCNT::ColorSpecialEffect::None:
                blendDefault(y);
                break;
        }
#endif

        //if (colorEffects.getEffect() != BLDCNT::ColorSpecialEffect::None)
        //    std::cout << std::dec << colorEffects.getEffect() << std::endl;

        /*
        color_t *outBuf = target.pixels() + y * target.getWidth();
        for (int32_t x = 0; x < SCREEN_WIDTH; ++x) {
            color_t color = windowOBJLayer->scanline[x].color;

            if (color != TRANSPARENT)
                outBuf[x] = color;
        }
         */

        //std::cout << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t).count() << std::endl;
    }
} // namespace gbaemu::lcd