#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

#include "cpu/cpu.hpp"
#include "io/interrupts.hpp"
#include "io/io_regs.hpp"
#include "io/memory.hpp"
#include "logging.hpp"
#include "math/mat.hpp"

#include <lcd/bglayer.hpp>
#include <lcd/coloreffects.hpp>
#include <lcd/defs.hpp>
#include <lcd/objlayer.hpp>
#include <lcd/palette.hpp>
#include <lcd/renderer.hpp>
#include <lcd/window-regions.hpp>

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
      private:
        Canvas<color_t> &display;
        Memory &memory;
        InterruptHandler &irqHandler;

        /* rendering is done in a separate thread */
        std::mutex *canDrawToScreenMutex;
        bool *canDrawToScreen;
        Renderer renderer;

        MemoryCanvas<color_t> frameBuffer;

        LCDColorPalette palette;
        LCDIORegs regsRef{0};
        LCDIORegs regs{0};

      public:
        struct
        {
            // uint32_t cycle = 0;
            int32_t /*x = 0, */y = 0;
            /* interlacing */
            bool oddScanline = true;
            bool hblanking = false;
            bool vblanking = false;

            /* the result of drawScanline() */
            std::vector<color_t> buf;

            uint16_t vCount;
        } scanline;

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

        void onVCount();
        void onHBlank();
        void onVBlank();
        void drawScanline();
        void present();
        void clearBlankFlags();

        bool isHBlank() const
        {
            return scanline.hblanking;
        }
        bool isVBlank() const
        {
            return scanline.vblanking;
        }

      public:
        int32_t scale = 3;

      public:
        LCDController(Canvas<color_t> &disp, CPU *cpu, std::mutex *canDrawToscreenMut, bool *canDraw) : display(disp),
            memory(cpu->state.memory), irqHandler(cpu->irqHandler),
            canDrawToScreenMutex(canDrawToscreenMut), canDrawToScreen(canDraw),
            renderer(cpu->state.memory, cpu->irqHandler, regsRef, frameBuffer),
#if (RENDERER_DECOMPOSE_LAYERS == 1)
            frameBuffer(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 4)
#else
            frameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT)
#endif
        {
            memory.ioHandler.registerIOMappedDevice(
                IO_Mapped(
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET),
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET) + sizeof(regsRef) - 1,
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));

            scanline.buf.resize(SCREEN_WIDTH);

#if (RENDERER_DECOMPOSE_LAYERS == 1)
            scale = 1;
#endif
        }

        bool canAccessPPUMemory(bool isOAMRegion = false) const;
        std::string getLayerStatusString() const;
    };

} // namespace gbaemu::lcd

#endif /* LCD_CONTROLLER_HPP */
