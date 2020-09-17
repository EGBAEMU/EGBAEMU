/*
    Contains macros, basic types and constants used in rendering.
*/
#ifndef DEFS_HPP
#define DEFS_HPP

#include <algorithm>
#include <cassert>
#include <math/mat.hpp>

namespace gbaemu::lcd
{
    /* The usual 8-8-8-8 bit ARGB color format, also used by SDL. */
    typedef uint32_t color_t;
    static const constexpr color_t TRANSPARENT = 0x00000000,
                                   BLACK = 0xFF000000,
                                   WHITE = 0xFFFFFFFF;

    /* channel wise color addition */
    static color_t colAdd(color_t a, color_t b)
    {
        color_t result = 0;

        for (uint32_t i = 0; i < 4; ++i) {
            color_t ca = (a >> (i * 8)) & 0xFF;
            color_t cb = (b >> (i * 8)) & 0xFF;
            color_t cs = std::max(ca + cb, 0xFFu);

            result |= (cs << (i * 8));
        }

        return result;
    }

    /* channel wise color subtraction */
    static color_t colSub(color_t a, color_t b)
    {
        color_t result = 0;

        for (uint32_t i = 0; i < 4; ++i) {
            color_t ca = (a >> (i * 8)) & 0xFF;
            color_t cb = (b >> (i * 8)) & 0xFF;
            color_t cs = (cb > ca) ? 0 : (ca - cb);

            result |= (cs << (i * 8));
        }

        return result;
    }

    /* channel wise color scale */
    static color_t colScale(color_t a, uint32_t scalar)
    {
        assert(scalar <= 16);

        color_t result = 0;

        for (uint32_t i = 0; i < 4; ++i) {
            color_t ca = (a >> (i * 8)) & 0xFF;
            color_t cs = (ca * scalar) / 16;

            result |= (cs << (i * 8));
        }

        return result;
    }

    /* This type is also used to represent 5-5-5 bit colors. */
    typedef uint16_t color16_t;
    typedef common::math::vec<2> vec2;
    typedef common::math::vec<3> vec3;
    typedef common::math::mat<3, 3> mat3x3;

    static const constexpr uint32_t SCREEN_WIDTH = 240;
    static const constexpr uint32_t SCREEN_HEIGHT = 160;

    namespace DISPCTL
    {
        static const constexpr uint32_t BG_MODE_MASK = 0b111,
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

        static uint32_t SCREEN_DISPLAY_BGN_MASK(uint32_t n)
        {
            return 1 << (8 + n);
        }
    } // namespace DISPCTL

    namespace DISPSTAT
    {
        static const constexpr uint16_t VBLANK_FLAG_OFFSET = 0,
                                        HBLANK_FLAG_OFFSET = 1,
                                        VCOUNTER_FLAG_OFFSET = 2,
                                        VBLANK_IRQ_ENABLE_OFFSET = 3,
                                        HBLANK_IRQ_ENABLE_OFFSET = 4,
                                        VCOUNTER_IRQ_ENABLE_OFFSET = 5,
                                        VCOUNT_SETTING_OFFSET = 8;

        static const constexpr uint16_t VBLANK_FLAG_MASK = 1,
                                        HBLANK_FLAG_MASK = 1,
                                        VCOUNTER_FLAG_MASK = 1,
                                        VBLANK_IRQ_ENABLE_MASK = 1,
                                        HBLANK_IRQ_ENABLE_MASK = 1,
                                        VCOUNTER_IRQ_ENABLE_MASK = 1,
                                        VCOUNT_SETTING_MASK = 0xFF;
    } // namespace DISPSTAT

    namespace VCOUNT
    {
        static const constexpr uint16_t CURRENT_SCANLINE_OFFSET = 0;
        static const constexpr uint16_t CURRENT_SCANLINE_MASK = 0xFF;
    } // namespace VCOUNT

    namespace BGCNT
    {
        static const constexpr uint32_t BG_PRIORITY_MASK = 0b11,
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

    namespace BLDCNT
    {
        static uint16_t BG_FIRST_TARGET_OFFSET(uint16_t i) { return i; }
        static uint16_t BG_SECOND_TARGET_OFFSET(uint16_t i) { return i + 8; }

        static const constexpr uint16_t OBJ_FIRST_TARGET_OFFSET = 4,
                                        BD_FIRST_TARGET_OFFSET = 5,
                                        COLOR_SPECIAL_FX_OFFSET = 6,
                                        OBJ_SECOND_TARGET_OFFSET = 12,
                                        BD_SECOND_TARGET_OFFSET = 13;

        static const constexpr uint16_t TARGET_MASK = 1,
                                        COLOR_SPECIAL_FX_MASK = 3;

        enum ColorSpecialEffect : uint16_t {
            None = 0,
            AlphaBlending,
            BrightnessIncrease,
            BrightnessDecrease
        };
    } // namespace BLDCNT

    namespace BLDALPHA
    {
        static const constexpr uint32_t EVA_COEFF_MASK = 0x1F,
                                        EVB_COEFF_MASK = 0x1F << 8;
    }

    namespace BLDY
    {
        static const constexpr uint32_t EVY_COEFF_MASK = 0x1F;
    }

    namespace MOSAIC
    {
        static const constexpr uint32_t BG_MOSAIC_HSIZE_OFFSET = 0,
                                        BG_MOSAIC_VSIZE_OFFSET = 4,
                                        OBJ_MOSAIC_HSIZE_OFFSET = 8,
                                        OBJ_MOSAIC_VSIZE_OFFSET = 12;

        static const constexpr uint32_t BG_MOSAIC_HSIZE_MASK = 0xF << BG_MOSAIC_HSIZE_OFFSET,
                                        BG_MOSAIC_VSIZE_MASK = 0xF << BG_MOSAIC_VSIZE_OFFSET,
                                        OBJ_MOSAIC_HSIZE_MASK = 0xF << OBJ_MOSAIC_HSIZE_OFFSET,
                                        OBJ_MOSAIC_VSIZE_MASK = 0xF << OBJ_MOSAIC_VSIZE_OFFSET;
    } // namespace MOSAIC

    namespace OBJ_ATTRIBUTE
    {
        static const constexpr uint16_t Y_COORD_OFFSET = 0,
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

        static const constexpr uint16_t Y_COORD_MASK = 0xFF,
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
} /* namespace gbaemu::lcd */

#ifdef DEBUG
#define DEBUG_RENDERING
#define DEBUG_DRAW_SPRITE_BOUNDS
#define DEBUG_DRAW_SPRITE_GRID
#endif

#ifdef DEBUG_RENDERING
/* backgrounds */
#ifdef DEBUG_DRAW_BG_BOUNDS
#ifndef BG_BOUNDS_COLOR
/* red */
#define BG_BOUNDS_COLOR 0xFFFF0000
#endif
#endif

#ifdef DEBUG_DRAW_BG_GRID
#ifndef BG_GRID_COLOR
/* red */
#define BG_GRID_COLOR 0xFFFF0000
#endif

#ifndef BG_GRID_SPACING
#define BG_GRID_SPACING 32
#endif
#endif

/* sprites */
#ifdef DEBUG_DRAW_SPRITE_BOUNDS
#ifndef SPRITE_BOUNDS_COLOR
#define SPRITE_BOUNDS_COLOR 0xFF00FF00
#endif

#define SPRITE_ID_TO_COLOR(id) ((id) * (1 << 16) | 0xFF000000)
#endif

#ifdef DEBUG_DRAW_SPRITE_GRID
#ifndef SPRITE_GRID_COLOR
/* green */
#define SPRITE_GRID_COLOR 0xFF00FF00
#endif

#ifndef SPRITE_GRID_SPACING
#define SPRITE_GRID_SPACING 8
#endif
#endif
#endif

#define RENDERER_ENABLE_COLOR_EFFECTS 1
#define RENDERER_DECOMPOSE_LAYERS 0

#define RENDERER_HIGHTLIGHT_OBJ 1
#define OBJ_HIGHLIGHT_COLOR 0xFF00FF00

#endif /* DEFS_HPP */