#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

#include "cpu/cpu.hpp"
#include "io/interrupts.hpp"
#include "io/io_regs.hpp"
#include "io/memory.hpp"
#include "logging.hpp"
#include "math/mat.hpp"

#include <lcd/defs.hpp>
#include <lcd/palette.hpp>
#include <lcd/objlayer.hpp>
#include <lcd/bglayer.hpp>

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>


namespace gbaemu::lcd
{
    /*
        The table summarizes the facilities of the separate BG modes (video modes).

        Mode  Rot/Scal Layers Size               Tiles Colors       Features
        0     No       0123   256x256..512x515   1024  16/16..256/1 SFMABP
        1     Mixed    012-   (BG0,BG1 as above Mode 0, BG2 as below Mode 2)
        2     Yes      --23   128x128..1024x1024 256   256/1        S-MABP
        3     Yes      --2-   240x160            1     32768        --MABP
        4     Yes      --2-   240x160            2     256/1        --MABP
        5     Yes      --2-   160x128            2     32768        --MABP

        Features: S)crolling, F)lip, M)osaic, A)lphaBlending, B)rightness, P)riority.

        In mode 0 all layers are in text mode.
        In mode 1 layers 0 and 1 are in text mode.
        All other layers are in map mode.
     */
    

    

    

    class LCDController
    {
      private:
        /* not copyable */
        LCDController &operator=(const LCDController &) = delete;
        LCDController(const LCDController &) = delete;
        LCDController() = default;

      public:
        enum RenderControl {
            WAIT,
            RUN,
            EXIT
        };

        enum RenderState {
            READY,
            IN_PROGRESS
        };

      private:
        //LCDisplay &display;
        Memory &memory;
        InterruptHandler &irqHandler;

        /* Describes the location and scale in the final canvas. */
        struct
        {
            Canvas<color_t> &target;
            int32_t x, y;
            /* used for upscaling */
            int32_t xStep, yStep;
        } display;

        LCDColorPalette palette;
        LCDIORegs regsRef{0};
        LCDIORegs regs{0};

        /* backdrop layer, BG0-BG4, OBJ0-OBJ4 */
        std::array<std::shared_ptr<Background>, 4> backgroundLayers;
        std::array<std::shared_ptr<Background>, 4> sortedBackgroundLayers;
        std::array<std::shared_ptr<OBJLayer>, 4> objLayers;

        std::array<std::shared_ptr<Layer>, 4 + 4> layers;
        /* special color effects, 0-3 ~ BG0-3, 4 OBJ, 5, Backdrop(?) */
        BLDCNT::ColorSpecialEffect colorSpecialEffect;

        struct
        {
            uint32_t evy;
        } brightnessEffect;

        struct
        {
            uint32_t eva;
            uint32_t evb;
        } alphaEffect;

        /* rendering is done in a separate thread */
        RenderControl renderControl;
        std::mutex renderControlMutex;

        std::mutex *canDrawToScreenMutex;
        bool *canDrawToScreen;

        std::unique_ptr<std::thread> renderThread;

        struct {
            /*
                For each line:

                takes 960 cycles ---->
                [p1]...[p240]
                h-blanking takes 272 cycles <----

                v-blanking takes 83776 cycles
             */
            uint32_t cycle;
            uint16_t vCount;

            bool hBlanking;
            bool vBlanking;

            uint64_t tickCount = 0;
            uint64_t renderCount = 0;
        } counters;

        int32_t objDebugCanvasIndex = 0;

        void blendBackgrounds();

        uint8_t read8FromReg(uint32_t offset)
        {
            return *(offset + reinterpret_cast<uint8_t *>(&regsRef));
        }
        void write8ToReg(uint32_t offset, uint8_t value)
        {
            uint8_t mask = 0xFF;

            if (offset == offsetof(LCDIORegs, BGCNT) + 1 || offset == offsetof(LCDIORegs, BGCNT) + sizeof(regsRef.BGCNT[0]) + 1) {
                mask = 0xDF;
            } else if (offset >= offsetof(LCDIORegs, WININ) && offset < offsetof(LCDIORegs, MOSAIC)) {
                mask = 0x3F;
            } else if (offset == offsetof(LCDIORegs, BLDCNT) + 1) {
                mask = 0x3F;
            } else if ((offset & ~1) == offsetof(LCDIORegs, BLDALPHA)) {
                mask = 0x1F;
            }

            *(offset + reinterpret_cast<uint8_t *>(&regsRef)) = value & mask;
        }

        void setupLayers();
        void sortLayers();
        void loadWindowSettings();
        void loadSettings();
        void onHBlank();
        void onVBlank();
        void copyLayer(const Canvas<color_t> &src);
        void drawToTarget();
        void drawLayers();
        void render();
        void renderLoop();

      public:
        LCDController(Canvas<color_t> &disp, CPU *cpu, std::mutex *canDrawToscreenMut, bool *canDraw) : display{disp, 0, 0, 3, 3}, memory(cpu->state.memory), irqHandler(cpu->irqHandler),
                                                                                                        canDrawToScreenMutex(canDrawToscreenMut), canDrawToScreen(canDraw)
        {
            counters.cycle = 0;
            counters.vCount = 0;
            counters.hBlanking = false;
            counters.vBlanking = false;
            memory.ioHandler.registerIOMappedDevice(
                IO_Mapped(
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET),
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET) + sizeof(regsRef) - 1,
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));

            setupLayers();

            renderControl = WAIT;
            renderThread = std::make_unique<std::thread>(&LCDController::renderLoop, this);
            // renderThread->detach();
        }

        /* updates all raw pointers into the sections of memory (in case they might change) */
        void updateReferences();
        bool tick();
        void exitThread();

        std::string getLayerStatus() const;
        void objHightlightSetIndex(int32_t index);
        void objSetDebugCanvas(int32_t i);
        std::string getOBJLayerString(uint32_t objIndex) const;
    };

} // namespace gbaemu::lcd

#endif /* LCD_CONTROLLER_HPP */
