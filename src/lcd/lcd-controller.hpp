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

        Renderer renderer;

        LCDColorPalette palette;
        LCDIORegs regs{0};
        LCDIORegs internalRegs{0};
        /* 2/3 then x/y */
        bool bgRefPointDirty[2][2]{0};

      public:
        struct
        {
            uint32_t cycle = 0;
            int32_t x = 0, y = 0;
            /* interlacing */
            bool oddScanline = true;
            bool hblanking = false;
            bool vblanking = false;

            /* the result of drawScanline() */
            std::vector<color_t> buf;

            uint16_t vCount = 0;
        } scanline;

        uint8_t read8FromReg(uint32_t offset)
        {
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        }

        void write8ToReg(uint32_t offset, uint8_t value)
        {
            uint8_t mask = 0xFF;

            if (offset == offsetof(LCDIORegs, BGCNT) + 1 || offset == offsetof(LCDIORegs, BGCNT) + sizeof(regs.BGCNT[0]) + 1) {
                mask = 0xDF;
            } else if (offset >= offsetof(LCDIORegs, WININ) && offset < offsetof(LCDIORegs, MOSAIC)) {
                mask = 0x3F;
            } else if (offset == offsetof(LCDIORegs, BLDCNT) + 1) {
                mask = 0x3F;
            } else if ((offset & ~1) == offsetof(LCDIORegs, BLDALPHA)) {
                mask = 0x1F;
            }

            bgRefPointDirty[0][0] |= (offset == offsetof(LCDIORegs, BG2X)) || (offset == offsetof(LCDIORegs, BG2X) + 1) || (offset == offsetof(LCDIORegs, BG2X) + 2) || (offset == offsetof(LCDIORegs, BG2X) + 3);
            bgRefPointDirty[0][1] |= (offset == offsetof(LCDIORegs, BG2Y)) || (offset == offsetof(LCDIORegs, BG2Y) + 1) || (offset == offsetof(LCDIORegs, BG2Y) + 2) || (offset == offsetof(LCDIORegs, BG2Y) + 3);
            bgRefPointDirty[1][0] |= (offset == offsetof(LCDIORegs, BG3X)) || (offset == offsetof(LCDIORegs, BG3X) + 1) || (offset == offsetof(LCDIORegs, BG3X) + 2) || (offset == offsetof(LCDIORegs, BG3X) + 3);
            bgRefPointDirty[1][1] |= (offset == offsetof(LCDIORegs, BG3Y)) || (offset == offsetof(LCDIORegs, BG3Y) + 1) || (offset == offsetof(LCDIORegs, BG3Y) + 2) || (offset == offsetof(LCDIORegs, BG3Y) + 3);

            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value & mask;
        }

        void onVCount();
        void onHBlank();
        void onVBlank();
        void drawScanline();

#ifndef LEGACY_RENDERING
        void clearBlankFlags();
#else
        void renderTick();
#endif

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
        LCDController(Canvas<color_t> &disp, CPU *cpu) : display(disp),
                                                         memory(cpu->state.memory), irqHandler(cpu->irqHandler),
                                                         renderer(cpu->state.memory, cpu->irqHandler, internalRegs, display)
        {
            scanline.buf.resize(SCREEN_WIDTH);

#if (RENDERER_DECOMPOSE_LAYERS == 1) || (RENDERER_USE_FB_CANVAS == 1)
            scale = 1;
#endif
        }

        bool canAccessPPUMemory(bool isOAMRegion = false) const;
        std::string getLayerStatusString() const;

        friend class IO_Handler;
    };

} // namespace gbaemu::lcd

#endif /* LCD_CONTROLLER_HPP */
