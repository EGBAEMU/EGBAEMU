#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

#include <array>
#include <memory.hpp>
#include "canvas.hpp"


namespace gbaemu::lcd {
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
     */
    namespace DISPCTL {
        static const uint32_t BG_MODE_MASK = 0b111,
                              CBG_MODE_MASK = 1 << 3,
                              DISPLAY_FRAME_SELECT_MASK = 1 << 4,
                              HBLANK_INTERVAL_FREE_MASK = 1 << 5,
                              OBJ_CHAR_VRAM_MAPPING_MASK = 1 << 6,
                              FORCES_BLANK_MASK = 1 << 7,
                              SCREEN_DISPLAY_BG0_MASK = 1 << 8,
                              SCREEN_DISPLAY_BG1_MASK = 1 << 9,
                              SCREEN_DISPLAY_BG2_MASK = 1 << 10,
                              SCREEN_DISPLAY_BG3_MASK = 1 << 11,
                              SCREEN_DISPLAY_OBJ_ASMK = 1 << 12,
                              WINDOW_0_DISPLAY_FLAG_MASK = 1 << 13,
                              WINDOW_1_DISPLAY_FLAG_MASK = 1 << 14,
                              OBJ_WINDOW_DISPLAY_FLAG_MASK = 1 << 15;
        
        static uint32_t SCREEN_DISPLAY_BGN_MASK(uint32_t n) {
            return 1 << (8 + n);
        }
    }

    namespace DISPSTAT {
        static const uint16_t VBLANK_FLAG_OFFSET = 0,
                              HBLANK_FLAG_OFFSET = 1,
                              VCOUNTER_FLAG_OFFSET = 2,
                              VBLANK_IRQ_ENABLE_OFFSET = 3,
                              HBLANK_IRQ_ENABLE_OFFSET = 4,
                              VCOUNTER_IRQ_ENABLE_OFFSET = 5,
                              VCOUNT_SETTING_OFFSET = 8;

        static const uint16_t VBLANK_FLAG_MASK = 1,
                              HBLANK_FLAG_MASK = 1,
                              VCOUNTER_FLAG_MASK = 1,
                              VBLANK_IRQ_ENABLE_MASK = 1,
                              HBLANK_IRQ_ENABLE_MASK = 1,
                              VCOUNTER_IRQ_ENABLE_MASK = 1,
                              VCOUNT_SETTING_MASK = 0xFF;
    }

    namespace VCOUNT {
        static const uint16_t CURRENT_SCANLINE_OFFSET = 0;
        static const uint16_t CURRENT_SCANLINE_MASK = 0xFF;
    }

    namespace BGCNT {
        static const uint32_t BG_PRIORITY_MASK = 0b11,
                              CHARACTER_BASE_BLOCK_MASK = 0b11 << 2,
                              MOSAIC_MASK = 1 << 6,
                              COLORS_PALETTES_MASK = 1 << 7,
                              SCREEN_BASE_BLOCK_MASK = 0x1F << 8,
                              DISPLAY_AREA_OVERFLOW_MASK = 1 << 13,
                              /*
                                Internal Screen Size (dots) and size of BG Map (bytes):

                                Value  Text Mode      Rotation/Scaling Mode
                                0      256x256 (2K)   128x128   (256 bytes)
                                1      512x256 (4K)   256x256   (1K)
                                2      256x512 (4K)   512x512   (4K)
                                3      512x512 (8K)   1024x1024 (16K)
                               */
                              SCREEN_SIZE_MASK = 0b11 << 14;
    }

    namespace BLDCNT {
        static const uint32_t BG0_TARGET_PIXEL1_MASK = 1,
                              BG1_TARGET_PIXEL1_MASK = 1 << 1,
                              BG2_TARGET_PIXEL1_MASK = 1 << 2,
                              BG3_TARGET_PIXEL1_MASK = 1 << 3,
                              OBJ_TARGET_PIXEL1_MASK = 1 << 4,
                              BD_TARGET_PIXEL1_MASK = 1 << 5,
                              COLOR_SPECIAL_FX_MASK = 0b111 << 6,
                              BG0_TARGET_PIXEL2_MASK = 1 << 8,
                              BG1_TARGET_PIXEL2_MASK = 1 << 9,
                              BG2_TARGET_PIXEL2_MASK = 1 << 10,
                              BG3_TARGET_PIXEL2_MASK = 1 << 11,
                              OBJ_TARGET_PIXEL2_MASK = 1 << 12,
                              BD_TARGET_PIXEL2_MASK = 1 << 13;

        enum ColorSpecialEffect {
            None,
            AlphaBlending,
            BrightnessIncrease,
            BrightnessDecrease
        };
    }

    namespace BLDALPHA {
        static const uint32_t EVA_COEFF_MASK = 0x1F,
                              EVB_COEFF_MASK = 0x1F << 8;
    }

    namespace BLDY {
        static const uint32_t EVY_COEFF_MASK = 0x1F;
    }

    namespace MOSAIC {
        static const uint32_t BG_MOSAIC_HSIZE_OFFSET = 0,
                              BG_MOSAIC_VSIZE_OFFSET = 4,
                              OBJ_MOSAIC_HSIZE_OFFSET = 8,
                              OBJ_MOSAIC_VSIZE_OFFSET = 12;

        static const uint32_t BG_MOSAIC_HSIZE_MASK = 0xF << BG_MOSAIC_HSIZE_OFFSET,
                              BG_MOSAIC_VSIZE_MASK = 0xF << BG_MOSAIC_VSIZE_OFFSET,
                              OBJ_MOSAIC_HSIZE_MASK = 0xF << OBJ_MOSAIC_HSIZE_OFFSET,
                              OBJ_MOSAIC_VSIZE_MASK = 0xF << OBJ_MOSAIC_VSIZE_OFFSET;
    }

    namespace DIMENSIONS {
        static const int32_t WIDTH = 240,
                             HEIGHT = 160;
    }

    struct LCDIORegs {
        uint16_t DISPCNT;                       // LCD Control
        uint16_t undocumented0;                 // Undocumented - Green Swap
        uint16_t DISPSTAT;                      // General LCD Status (STAT,LYC)
        uint16_t VCOUNT;                        // Vertical Counter (LY)
        uint16_t BGCNT[4];                      // BG0 Control
        
        struct _BGOFS {
            uint16_t h;
            uint16_t v;
        } __attribute__((packed)) BGOFS[4];
        
        //uint16_t BG0HOFS;                       // BG0 X-Offset
        //uint16_t BG0VOFS;                       // BG0 Y-Offset
        //uint16_t BG1HOFS;                       // BG1 X-Offset
        //uint16_t BG1VOFS;                       // BG1 Y-Offset
        //uint16_t BG2HOFS;                       // BG2 X-Offset
        //uint16_t BG2VOFS;                       // BG2 Y-Offset
        //uint16_t BG3HOFS;                       // BG3 X-Offset
        //uint16_t BG3VOFS;                       // BG3 Y-Offset
        uint16_t BG2P[4];                       // BG2 Rotation/Scaling Parameter A (dx)
        uint32_t BG2X;                          // BG2 Reference Point X-Coordinate
        uint32_t BG2Y;                          // BG2 Reference Point Y-Coordinate
        uint16_t BG3P[4];                       // BG3 Rotation/Scaling Parameter A (dx)
        uint32_t BG3X;                          // BG3 Reference Point X-Coordinate
        uint32_t BG3Y;                          // BG3 Reference Point Y-Coordinate
        uint16_t WIN0H;                         // Window 0 Horizontal Dimensions
        uint16_t WIN1H;                         // Window 1 Horizontal Dimensions
        uint16_t WIN0V;                         // Window 0 Vertical Dimensions
        uint16_t WIN1V;                         // Window 1 Vertical Dimensions
        uint16_t WININ;                         // Inside of Window 0 and 1
        uint16_t WINOUT;                        // Inside of OBJ Window & Outside of Windows
        uint16_t MOSAIC;                        // Mosaic Size
        uint16_t unused0;
        uint16_t BLDCNT;                        // Color Special Effects Selection
        uint16_t BLDALPHA;                      // Alpha Blending Coefficients
        uint16_t BLDY;                          // Brightness (Fade-In/Out) Coefficient
    } __attribute__((packed));

    struct LCDBgObj {
        struct ObjAttribute {
            uint16_t attribute[3];
        } __attribute__((packed));

        uint32_t bgMode;

        /* VRAM region */
        union {
            /* bg map and tiles */
            /*
                Text BG Screen (2 bytes per entry)
                Specifies the tile number and attributes. Note that BG tile numbers are always specified in steps of 1 (unlike OBJ tile numbers which are using steps of two in 256 color/1 palette mode).

                Bit   Expl.
                0-9   Tile Number     (0-1023) (a bit less in 256 color mode, because
                                        there'd be otherwise no room for the bg map)
                10    Horizontal Flip (0=Normal, 1=Mirrored)
                11    Vertical Flip   (0=Normal, 1=Mirrored)
                12-15 Palette Number  (0-15)    (Not used in 256 color/1 palette mode)

                A Text BG Map always consists of 32x32 entries (256x256 pixels), 400h entries = 800h bytes. However, depending on the BG Size, one, two, or four of these Maps may be used together, allowing to create backgrounds of 256x256, 512x256, 256x512, or 512x512 pixels, if so, the first map (SC0) is located at base+0, the next map (SC1) at base+800h, and so on.     
             

                Rotation/Scaling BG Screen (1 byte per entry)
                In this mode, only 256 tiles can be used. There are no x/y-flip attributes, the color depth is always 256 colors/1 palette.

                Bit   Expl.
                0-7   Tile Number     (0-255)

                The dimensions of Rotation/Scaling BG Maps depend on the BG size. For size 0-3 that are: 16x16 tiles (128x128 pixels), 32x32 tiles (256x256 pixels), 64x64 tiles (512x512 pixels), or 128x128 tiles (1024x1024 pixels).

                The size and VRAM base address of the separate BG maps for BG0-3 are set up by BG0CNT-BG3CNT registers.             
             */
            uint8_t *bgMode012;
            /* frame buffer 0 */
            uint8_t *bgMode3;
            /* frame buffer 0 and 1*/
            struct {
                uint8_t *frameBuffer0;
                uint8_t *frameBuffer1;
            } bgMode45;
        } bg;

        /* depends on bg mode */
        uint8_t *objTiles;
        /* OAM region */
        uint8_t *attributes;

        void setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t bgMode);
        ObjAttribute *accessAttribute(uint32_t index);
    };

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
    };

    class LCDisplay {
    public:
        MemoryCanvas<uint32_t> canvas;
        int32_t targetX, targetY;
        Canvas<uint32_t>& target;

        LCDisplay(uint32_t x, uint32_t y, Canvas<uint32_t>& targ):
            canvas(DIMENSIONS::WIDTH, DIMENSIONS::HEIGHT), targetX(x), targetY(y), target(targ) { }

        int32_t stride() const {
            return canvas.getWidth();
        }

        void drawToTarget(int32_t scale = 1) {
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
        uint32_t priority;
        uint32_t charBaseBlock;
        int32_t xOff, yOff;
        bool scInUse[4];
        uint32_t scCount;
        uint32_t scXOffset[4];
        uint32_t scYOffset[4];
        uint8_t *bgMapBase;
        uint8_t *tiles;

        /* only for bg2, bg3, updated on each vblank */
        struct {
            /* xy coordinate of the upper left corner */
            double origin[2];
            /* dx, dy */
            double d[2];
            /* dmx, dmy */
            double dm[2];

            /*
                x0, y0      rotation center
                x1, y1      old pixel position
                x2, y2      new pixel position

                dx = cos(alpha) / xMag
                dmx = sin(alpha) / xMag
                dy = Sin (alpha) / yMag
                dmy = Cos (alpha) / yMag

                x2 = dx(x1-x0) + dmx(y1-y0) + x0
                y2 = dy(x1-x0) + dmy(y1-y0) + y0
             */
        } scale_rotate;

        Background(): id(-1), canvas(512, 512) { }

        void loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs *regs, Memory& memory);
        void renderBG0(LCDColorPalette& palette);
        void renderBG3(Memory& memory);
        void renderBG4(LCDColorPalette& palette, Memory& memory);
        void renderBG5(LCDColorPalette& palette, Memory& memory);
        void drawToDisplay(LCDisplay& display);
    };

    class LCDController {
    private:
        LCDisplay& display;
        Memory& memory;
        LCDColorPalette palette;
        LCDIORegs *regs;
        std::array<Background, 4> backgrounds;

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
    public:
        LCDController(LCDisplay& disp, Memory& mem):
            display(disp), memory(mem) {
            counters.cycle = 0;
            counters.vCount = 0;
            counters.hBlanking = false;
            counters.vBlanking = false;
        }

        /* updates all raw pointers into the sections of memory (in case they might change) */
        void updateReferences();

        uint32_t getBackgroundMode() const;
        /* renders the current screen to canvas */
        void render();
        void plotPalette();
        bool tick();
    };
}


#endif /* LCD_CONTROLLER_HPP */
