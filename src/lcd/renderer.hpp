#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <io/interrupts.hpp>
#include <io/memory.hpp>
#include <lcd/bglayer.hpp>
#include <lcd/coloreffects.hpp>
#include <lcd/defs.hpp>
#include <lcd/objlayer.hpp>
#include <lcd/palette.hpp>
#include <lcd/window-regions.hpp>

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
        Memory &memory;
        InterruptHandler &irqHandler;
        const LCDIORegs &regs;

        LCDColorPalette palette;
        WindowFeature windowFeature;
        ColorEffects colorEffects;

        /* backdrop layer, BG0-BG4, OBJ0-OBJ4 */
        std::array<std::shared_ptr<BGLayer>, 4> backgroundLayers;
        /* each priority (0-3) gets its own layer */
        std::array<std::shared_ptr<OBJLayer>, 4> objLayers;
        /* all layers */
        std::array<std::shared_ptr<Layer>, 8> layers;
        std::shared_ptr<OBJLayer> windowOBJLayer;

        Canvas<color_t>& target;

        bool drawOdd = true;

        void setupLayers();
        void sortLayers();
        void loadSettings(int32_t y);

        void blendDefault(int32_t y, int32_t xTo = SCREEN_WIDTH);
        void blendBrightness(int32_t y, int32_t xTo = SCREEN_WIDTH);
        void blendAlpha(int32_t y, int32_t xTo = SCREEN_WIDTH);
        void blendDecomposed(int32_t y);
        static color_t toBGR656(color_t color);

      public:
        Renderer(Memory &mem, InterruptHandler &irq, const LCDIORegs &registers, Canvas<color_t>& targetCanvas);
        void drawScanline(int32_t y);
        std::string getLayerStatusString() const;
    };
} // namespace gbaemu::lcd

#endif /* RENDERER_HPP */
