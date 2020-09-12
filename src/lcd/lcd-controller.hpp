#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

#include "canvas.hpp"
#include "cpu/cpu.hpp"
#include "io/interrupts.hpp"
#include "io/io_regs.hpp"
#include "io/memory.hpp"
#include "math/mat.hpp"

#include "defs.hpp"

#include <array>
#include <condition_variable>
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

    struct LCDIORegs {
        uint16_t DISPCNT;       // LCD Control
        uint16_t undocumented0; // Undocumented - Green Swap
        uint16_t DISPSTAT;      // General LCD Status (STAT,LYC)
        uint16_t VCOUNT;        // Vertical Counter (LY)
        uint16_t BGCNT[4];      // BG0 Control

#include "packed.h"
        struct _BGOFS {
            uint16_t h;
            uint16_t v;
        } PACKED BGOFS[4];

        //uint16_t BG0HOFS;                       // BG0 X-Offset
        //uint16_t BG0VOFS;                       // BG0 Y-Offset
        //uint16_t BG1HOFS;                       // BG1 X-Offset
        //uint16_t BG1VOFS;                       // BG1 Y-Offset
        //uint16_t BG2HOFS;                       // BG2 X-Offset
        //uint16_t BG2VOFS;                       // BG2 Y-Offset
        //uint16_t BG3HOFS;                       // BG3 X-Offset
        //uint16_t BG3VOFS;                       // BG3 Y-Offset
        uint16_t BG2P[4]; // BG2 Rotation/Scaling Parameter A (dx)
        uint32_t BG2X;    // BG2 Reference Point X-Coordinate
        uint32_t BG2Y;    // BG2 Reference Point Y-Coordinate
        uint16_t BG3P[4]; // BG3 Rotation/Scaling Parameter A (dx)
        uint32_t BG3X;    // BG3 Reference Point X-Coordinate
        uint32_t BG3Y;    // BG3 Reference Point Y-Coordinate
        uint16_t WIN0H;   // Window 0 Horizontal Dimensions
        uint16_t WIN1H;   // Window 1 Horizontal Dimensions
        uint16_t WIN0V;   // Window 0 Vertical Dimensions
        uint16_t WIN1V;   // Window 1 Vertical Dimensions
        uint16_t WININ;   // Inside of Window 0 and 1
        uint16_t WINOUT;  // Inside of OBJ Window & Outside of Windows
        uint16_t MOSAIC;  // Mosaic Size
        uint16_t unused0;
        uint16_t BLDCNT;   // Color Special Effects Selection
        uint16_t BLDALPHA; // Alpha Blending Coefficients
        uint16_t BLDY;     // Brightness (Fade-In/Out) Coefficient
    } PACKED;
#include "endpacked.h"

    namespace OBJ_ATTRIBUTE
    {
        static const uint16_t Y_COORD_OFFSET = 0,
                              ROT_SCALE_OFFSET = 8,
                              DOUBLE_SIZE_OFFSET = 9,
                              DISABLE_OFFSET = 9,
                              OBJ_MODE_OFFSET = 10,
                              OBJ_MOSAIC_OFFSET = 12,
                              COLOR_PALETTE_OFFSET = 13,
                              OBJ_SHAPE_OFFSET = 14,
                              X_COORD_OFFSET = 0,
                              ROT_SCALE_PARAM_OFFSET = 9,
                              H_FLIP_OFFSET = 12,
                              V_FLIP_OFFSET = 13,
                              OBJ_SIZE_OFFSET = 14,
                              CHAR_NAME_OFFSET = 0,
                              PRIORITY_OFFSET = 10,
                              PALETTE_NUMBER_OFFSET = 12;

        static const uint16_t Y_COORD_MASK = 0xFF,
                              ROT_SCALE_MASK = 1,
                              DOUBLE_SIZE_MASK = 1,
                              DISABLE_MASK = 1,
                              OBJ_MODE_MASK = 3,
                              OBJ_MOSAIC_MASK = 1,
                              COLOR_PALETTE_MASK = 1,
                              OBJ_SHAPE_MASK = 3,
                              X_COORD_MASK = 0x1FF,
                              ROT_SCALE_PARAM_MASK = 0x1F,
                              H_FLIP_MASK = 1,
                              V_FLIP_MASK = 1,
                              OBJ_SIZE_MASK = 3,
                              CHAR_NAME_MASK = 0x3FF,
                              PRIORITY_MASK = 3,
                              PALETTE_NUMBER_MASK = 0xF;
    } // namespace OBJ_ATTRIBUTE

    struct LCDColorPalette {
        /* TODO: maybe this can be const */
        /* 256 entries */
        uint16_t *bgPalette;
        /* 256 entries */
        uint16_t *objPalette;

        static uint32_t toR8G8B8(uint16_t color);
        uint32_t getBgColor(uint32_t index) const;
        uint32_t getBgColor(uint32_t i1, uint32_t i2) const;
        uint32_t getObjColor(uint32_t index) const;
        uint32_t getObjColor(uint32_t i1, uint32_t i2) const;
        uint32_t getBackdropColor() const;
    };

    class LCDisplay
    {
      public:
        MemoryCanvas<uint32_t> canvas;
        int32_t targetX, targetY;
        Canvas<uint32_t> &target;

        LCDisplay(uint32_t x, uint32_t y, Canvas<uint32_t> &targ) : canvas(SCREEN_WIDTH, SCREEN_HEIGHT), targetX(x), targetY(y), target(targ) {}

        int32_t stride() const
        {
            return canvas.getWidth();
        }

        void drawToTarget(int32_t scale = 1)
        {
            target.beginDraw();

            auto stride = target.getWidth();
            auto dest = target.pixels();

            auto src = canvas.pixels();
            auto srcStride = canvas.getWidth();

            for (int32_t y = 0; y < canvas.getHeight(); ++y)
                for (int32_t x = 0; x < canvas.getWidth(); ++x) {
                    auto color = src[y * srcStride + x];

                    for (int32_t iy = 0; iy < scale; ++iy)
                        for (int32_t ix = 0; ix < scale; ++ix)
                            dest[(y * scale + iy + targetY) * stride + (x * scale + ix + targetX)] = color;
                }

            target.endDraw();
        }

        void clear()
        {
            canvas.clear(0xFF000000);
        }
    };

    struct OBJLayer {
        enum OBJShape : uint16_t {
            SQUARE = 0,
            HORIZONTAL,
            VERTICAL
        };

        enum OBJMode : uint16_t {
            NORMAL,
            SEMI_TRANSPARENT,
            OBJ_WINDOW
        };

#include "packed.h"
        struct OBJAttribute {
            uint16_t attribute[3];
        } PACKED;
#include "endpacked.h"

        uint32_t bgMode;
        /* depends on bg mode */
        uint8_t *objTiles;
        uint32_t areaSize;
        /* OAM region */
        uint8_t *attributes;

        uint32_t tempBuffer[64 * 64];

        void setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t bgMode);
        OBJAttribute *accessAttribute(uint32_t index);
        OBJAttribute getAttribute(uint32_t index);
        std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(uint32_t index);
        void draw(LCDColorPalette &palette, bool use2dMapping, LCDisplay &display);
    };

    struct Background {
        /* BG0, BG1, BG2, BG3 */
        int32_t id;
        MemoryCanvas<uint32_t> canvas;
        /* settings */
        bool enabled;
        bool useOtherFrameBuffer;
        bool mosaicEnabled;
        bool colorPalette256;
        /* actual pixel count */
        uint32_t width;
        uint32_t height;
        bool wrap;
        int32_t priority;
        bool scInUse[4];
        uint32_t scCount;
        uint32_t scXOffset[4];
        uint32_t scYOffset[4];
        uint8_t *bgMapBase;
        uint8_t *tiles;

        /* general transformation of background in target display space */
        struct {
            common::math::vec<2> d;
            common::math::vec<2> dm;
            common::math::vec<2> origin;
        } step;

        common::math::mat<3, 3> trans;
        common::math::mat<3, 3> invTrans;

        Background(int32_t i) : id(i), canvas(1024, 1024), enabled(false) {}

        void loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory);
        void renderBG0(LCDColorPalette &palette);
        void renderBG2(LCDColorPalette &palette);
        void renderBG3(Memory &memory);
        void renderBG4(LCDColorPalette &palette, Memory &memory);
        void renderBG5(LCDColorPalette &palette, Memory &memory);
        void drawToDisplay(LCDisplay &display);
    };

    class LCDController
    {
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
        LCDisplay &display;
        Memory &memory;
        InterruptHandler &irqHandler;

        LCDColorPalette palette;
        LCDIORegs regs = {0};
        std::array<std::unique_ptr<Background>, 4> backgrounds;
        OBJLayer objLayer;
        /* special color effects, 0-3 ~ BG0-3, 4 OBJ, 5, Backdrop(?) */
        int32_t firstTargetLayerID;
        int32_t secondTargetLayerID;
        BLDCNT::ColorSpecialEffect colorSpecialEffect;

        /* rendering is done in a separate thread */
        RenderControl renderControl;
        std::mutex renderControlMutex;

        std::mutex *canDrawToScreenMutex;
        bool *canDrawToScreen;

        std::unique_ptr<std::thread> renderThread;
        std::condition_variable cv;

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
        } counters;

        void blendBackgrounds();

        uint8_t read8FromReg(uint32_t offset)
        {
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        }
        void write8ToReg(uint32_t offset, uint8_t value)
        {
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }

        void onHBlank();
        void onVBlank();
        void render();
        void renderLoop();

      public:
        LCDController(LCDisplay &disp, CPU *cpu, std::mutex *canDrawToscreenMut, bool *canDraw) : display(disp), memory(cpu->state.memory), irqHandler(cpu->irqHandler), canDrawToScreenMutex(canDrawToscreenMut), canDrawToScreen(canDraw)
        {
            counters.cycle = 0;
            counters.vCount = 0;
            counters.hBlanking = false;
            counters.vBlanking = false;
            memory.ioHandler.registerIOMappedDevice(
                IO_Mapped(
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET),
                    static_cast<uint32_t>(gbaemu::Memory::IO_REGS_OFFSET) + sizeof(regs),
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&LCDController::read8FromReg, this, std::placeholders::_1),
                    std::bind(&LCDController::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));

            for (int32_t i = 0; i < 4; ++i)
                backgrounds[i] = std::make_unique<Background>(i);

            renderControl = WAIT;
            renderThread = std::make_unique<std::thread>(&LCDController::renderLoop, this);
            // renderThread->detach();
        }

        /* updates all raw pointers into the sections of memory (in case they might change) */
        void updateReferences();
        bool tick();
        void exitThread();
    };

} // namespace gbaemu::lcd

#endif /* LCD_CONTROLLER_HPP */
