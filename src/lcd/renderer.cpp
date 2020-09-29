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

        objLayers[0] = std::make_shared<OBJLayer>(memory, palette, regs, objManager, 0);
        objLayers[1] = std::make_shared<OBJLayer>(memory, palette, regs, objManager, 1);
        objLayers[2] = std::make_shared<OBJLayer>(memory, palette, regs, objManager, 2);
        objLayers[3] = std::make_shared<OBJLayer>(memory, palette, regs, objManager, 3);

        for (uint32_t i = 0; i < 8; ++i) {
            if (i <= 3)
                layers[i] = objLayers[i];
            else
                layers[i] = backgroundLayers[i - 4];
        }
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
        uint32_t bgMode = le(regs.DISPCNT) & DISPCTL::BG_MODE_MASK;

        /* Which background layers are enabled to begin with? */
        for (uint32_t i = 0; i < 4; ++i)
            backgroundLayers[i]->enabled = le(regs.DISPCNT) & DISPCTL::SCREEN_DISPLAY_BGN_MASK(i);

        for (uint32_t i = 0; i < 4; ++i)
            if (backgroundLayers[i]->enabled)
                backgroundLayers[i]->loadSettings(static_cast<BGMode>(bgMode), regs);

        bool use2dMapping = !(le(regs.DISPCNT) & DISPCTL::OBJ_CHAR_VRAM_MAPPING_MASK);

        for (auto &l : objLayers) {
            l->setMode(static_cast<BGMode>(bgMode), use2dMapping);
            l->loadOBJs(y);
        }

        palette.loadPalette(memory);
        windowFeature.load(regs, palette.getBackdropColor());
        colorEffects.load(regs);

        sortLayers();
    }

    void Renderer::blendDefault(color_t *outBuf)
    {
        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            color_t finalColor = palette.getBackdropColor();

            for (const auto &l : layers) {
                if (!l->enabled)
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

    void Renderer::blendBrightness(color_t *outBuf)
    {
        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            color_t finalColor = palette.getBackdropColor();

            for (const auto &l : layers) {
                if (!l->enabled)
                    continue;

                color_t color = l->scanline[x].color;

                if (color == TRANSPARENT)
                    continue;

                if (l->scanline[x].asFirstColor())
                    finalColor = colorEffects.apply(color);
                else
                    finalColor = color;

                break;
            }

            outBuf[x] = finalColor;
        }
    }

    void Renderer::blendAlpha(color_t *outBuf)
    {
        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            auto it = layers.cbegin();
            bool asFirst = false;
            bool asSecond = false;
            color_t firstColor = palette.getBackdropColor();
            color_t secondColor = palette.getBackdropColor();
            color_t finalColor = palette.getBackdropColor();

            for (; it != layers.cend(); it++) {
                const auto &l = *it;

                if (!l->enabled)
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

                    if (!l->enabled)
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
                outBuf[x] = colorEffects.apply(firstColor, secondColor);
            else
                outBuf[x] = firstColor;
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
                {
                    int32_t i = 0;

                    for (const auto &l : layers) {
                        if (l->enabled) {
                            int32_t yOff = (i / 2) * SCREEN_HEIGHT;
                            int32_t xOff = (i % 2) * SCREEN_WIDTH;

                            for (int32_t x = 0; x < SCREEN_WIDTH; ++x)
                                ;
                            outBuf[(yOff + y) * stride + (x + xOff)] = l->scanline[x].color;
                        }

                        ++i;
                    }
                }
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

    Renderer::Renderer(Memory &mem, InterruptHandler &irq, const LCDIORegs &registers) : memory(mem), irqHandler(irq), regs(registers)
    {
        setupLayers();

        //renderControl = WAIT;
        //renderThread = std::thread(&Renderer::renderLoop, this);
    }

    Renderer::~Renderer()
    {
        //renderThread.join();
    }

    void Renderer::drawScanline(int32_t y, color_t *outBuf, int32_t stride)
    {
        /*
        if (y == 0)
            drawOdd = !drawOdd;

        if (drawOdd == (y % 2 == 0))
            return;
         */

        loadSettings(y);

        auto t = std::chrono::high_resolution_clock::now();

        for (const auto &l : layers) {
            if (l->enabled) {
                l->drawScanline(y);
            }
        }

        switch (colorEffects.getEffect()) {
            case BLDCNT::ColorSpecialEffect::AlphaBlending:
                blendAlpha(outBuf);
                break;
            case BLDCNT::ColorSpecialEffect::BrightnessIncrease:
            case BLDCNT::ColorSpecialEffect::BrightnessDecrease:
                blendBrightness(outBuf);
                break;
            default:
            case BLDCNT::ColorSpecialEffect::None:
                blendDefault(outBuf);
                break;
        }

        

        //std::cout << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t).count() << std::endl;
    }
} // namespace gbaemu::lcd