#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

#include "canvas.hpp"
#include "cpu/cpu.hpp"
#include "io/interrupts.hpp"
#include "io/io_regs.hpp"
#include "io/memory.hpp"
#include "logging.hpp"
#include "math/mat.hpp"

#include "defs.hpp"

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

    struct LCDColorPalette {
        /* 256 entries */
        const uint16_t *bgPalette;
        /* 256 entries */
        const uint16_t *objPalette;

        static color_t toR8G8B8(uint16_t color);
        /*
            Under certain conditions the palette can be split up into 16 partitions of 16 colors. This is what
            partition number and index refer to.
         */
        color_t getBgColor(uint32_t index) const;
        color_t getBgColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getObjColor(uint32_t index) const;
        color_t getObjColor(uint32_t paletteNumber, uint32_t index) const;
        color_t getBackdropColor() const;
    };

    class Layer
    {
      public:
        enum LayerId : int32_t {
            BG0,
            BG1,
            BG2,
            BG3,
            OBJ0,
            OBJ1,
            OBJ2,
            OBJ3,
            BD
        };

        /* Contains the final image of the layer. Has the same size as the display. */
        MemoryCanvas<color_t> canvas;
        /* What layer are we talking about? */
        LayerId id;
        bool enabled;

        /* The layer with the smallest order is the bottom most layer. */
        int32_t order;
        uint16_t priority;
        bool asFirstTarget;
        bool asSecondTarget;

        Layer(LayerId _id) : canvas(SCREEN_WIDTH, SCREEN_HEIGHT), id(_id), order(0), enabled(false) {}
    };

    class OBJLayer : public Layer
    {
      public:
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
        const uint8_t *objTiles;
        uint32_t areaSize;
        /* OAM region */
        const uint8_t *attributes;

        int32_t hightlightObjIndex = 0;

      private:
        OBJAttribute getAttribute(uint32_t index) const;
        std::tuple<common::math::vec<2>, common::math::vec<2>> getRotScaleParameters(uint32_t index) const;
      public:

#if RENDERER_OBJ_ENABLE_DEBUG_CANVAS == 1
        MemoryCanvas<color_t> debugCanvas;
#endif
      
        OBJLayer(LayerId layerId);
        void setMode(const uint8_t *vramBaseAddress, const uint8_t *oamBaseAddress, uint32_t bgMode);
        void draw(LCDColorPalette &palette, bool use2dMapping);
    };

    class Background : public Layer
    {
      public:
        MemoryCanvas<uint32_t> tempCanvas;
        /* settings */
        bool useOtherFrameBuffer;
        bool mosaicEnabled;
        bool colorPalette256;
        /* actual pixel count */
        uint32_t width;
        uint32_t height;
        bool wrap;
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

        Background(LayerId i) : Layer(i), tempCanvas(1024, 1024)
        {
            order = -(static_cast<int32_t>(i) * 2);
        }

        ~Background()
        {
            LOG_LCD(std::cout << "~Background()" << std::endl;);
        }

        void loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory);
        void renderBG0(LCDColorPalette &palette);
        void renderBG2(LCDColorPalette &palette);
        void renderBG3(Memory &memory);
        void renderBG4(LCDColorPalette &palette, Memory &memory);
        void renderBG5(LCDColorPalette &palette, Memory &memory);
        void draw();
    };

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
            /*  */
            int32_t xStep, yStep;
        } display;

        LCDColorPalette palette;
        LCDIORegs regsRef{0};
        LCDIORegs regs = {0};

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
            *(offset + reinterpret_cast<uint8_t *>(&regsRef)) = value;
        }

        void setupLayers();
        void sortLayers();
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
    };

} // namespace gbaemu::lcd

#endif /* LCD_CONTROLLER_HPP */
