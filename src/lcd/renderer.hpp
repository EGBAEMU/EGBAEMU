#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <thread>
#include <io/memory.hpp>
#include <io/interrupts.hpp>
#include <lcd/defs.hpp>
#include <lcd/palette.hpp>
#include <lcd/coloreffects.hpp>
#include <lcd/window-regions.hpp>
#include <lcd/objlayer.hpp>
#include <lcd/bglayer.hpp>


namespace gbaemu::lcd
{
    enum RenderControl {
        WAIT,
        RUN,
        EXIT
    };

    enum RenderState {
        READY,
        IN_PROGRESS
    };

    class Renderer
    {
      private:
        const LCDIORegs& regs;
        Memory &memory;
        InterruptHandler &irqHandler;

        LCDColorPalette palette;
        WindowFeature windowFeature;
        ColorEffects colorEffects;

        /* backdrop layer, BG0-BG4, OBJ0-OBJ4 */
        std::array<std::shared_ptr<BGLayer>, 4> backgroundLayers;
        /* each priority (0-3) gets its own layer */
        OBJManager objManager;
        std::array<std::shared_ptr<OBJLayer>, 4> objLayers;
        /* all layers */
        std::array<std::shared_ptr<Layer>, 8> layers;

        RenderControl renderControl;
        std::mutex renderControlMutex;
        std::thread renderThread;

        bool drawOdd = true;

        void setupLayers();
        void sortLayers();
        void loadSettings(int32_t y);

        void blendDefault(color_t *outBuf);
        void blendBrightness(color_t *outBuf);
        void blendAlpha(color_t *outBuf);

        void renderLoop();
      public:
        Renderer(Memory& mem, InterruptHandler& irq, const LCDIORegs& registers);
        ~Renderer();
        void drawScanline(int32_t y, color_t *outBuf, int32_t stride = 0);
    };
}

#endif /* RENDERER_HPP */
