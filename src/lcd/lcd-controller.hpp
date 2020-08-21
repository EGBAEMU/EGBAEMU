#ifndef LCD_CONTROLLER_HPP
#define LCD_CONTROLLER_HPP

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
    }

    namespace DISPSTAT {
        static const uint32_t VBANK_FLAG_MASK = 1,
                              HBANK_FLAG_MASK = 1 << 1,
                              VCOUNTER_FLAG_MASK = 1 << 2,
                              VBLANK_IRQ_ENABLE_MASK = 1 << 3,
                              HBLANK_IRQ_ENABLE_MASK = 1 << 4,
                              VCOUNTER_IRQ_ENABLE_MASK = 1 << 5,
                              VCOUNT_SETTING_MASK = 0xFF << 8;
    }

    namespace VCOUNT {
        static const uint32_t CURRENT_SCANLINE_MASK = 0xFF;
    }

    struct LCDIORegs {
        uint16_t DISPCNT;                       // LCD Control
        uint16_t undocumented0;                 // Undocumented - Green Swap
        uint16_t DISPSTAT;                      // General LCD Status (STAT,LYC)
        uint16_t VCOUNT;                        // Vertical Counter (LY)
        uint16_t BGCNT[4];                      // BG0 Control
        uint16_t BG0HOFS;                       // BG0 X-Offset
        uint16_t BG0VOFS;                       // BG0 Y-Offset
        uint16_t BG1HOFS;                       // BG1 X-Offset
        uint16_t BG1VOFS;                       // BG1 Y-Offset
        uint16_t BG2HOFS;                       // BG2 X-Offset
        uint16_t BG2VOFS;                       // BG2 Y-Offset
        uint16_t BG3HOFS;                       // BG3 X-Offset
        uint16_t BG3VOFS;                       // BG3 Y-Offset
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

    class LCDController {
    private:
        Canvas<uint32_t>& canvas;
        Memory& memory;

        void renderBG3();
    public:
        LCDController(Canvas<uint32_t>& canv, Memory& mem):
            canvas(canv), memory(mem) {}

        uint32_t getBackgroundMode() const;
        /* renders to the current screen to canvas */
        void render();
    };
}


#endif /* LCD_CONTROLLER_HPP */
