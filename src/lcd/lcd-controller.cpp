#include "lcd-controller.hpp"


namespace gbaemu::lcd {
    void LCDController::renderBG3() {
        /* TODO: This should easily be extendable to support BG4, BG5 */
        /* BG Mode 3 - 240x160 pixels, 32768 colors */
        uint16_t *pixs = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET));

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint16_t color = pixs[y * 240 + x];
                /* TODO: endianess? */
                color = ((color & 0xFF) << 16) | ((color & 0xFF00) >> 16);

                /* convert to R8G8B8 */
                uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
                uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
                uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;
                uint32_t r8g8b8 = (r << 16) | (g << 8) | b;
            }
        }
    }

    void LCDController::render() {
        uint32_t bgMode = getBackgroundMode();
        uint32_t vram = gbaemu::Memory::MemoryRegionOffset::VRAM_OFFSET;

        /*  */
        if (bgMode == 3) {
            renderBG3();
        }
    }
}