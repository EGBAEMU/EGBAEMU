#ifndef RENDERER_HPP
#define RENDERER_HPP

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
        std::array<std::shared_ptr<OBJLayer>, 4> objLayers;
        /* all layers */
        std::array<std::shared_ptr<Layer>, 8> layers;

        void setupLayers();
        void sortLayers();
        void loadSettings();

        void blendDefault(color_t *outBuf);
        void blendBrightness(color_t *outBuf);
        void blendAlpha(color_t *outBuf);
      public:
        Renderer(Memory& mem, InterruptHandler& irq, const LCDIORegs& registers);
        void drawScanline(int32_t y, color_t *outBuf, int32_t stride = 0);
    };
}

#endif /* RENDERER_HPP */
