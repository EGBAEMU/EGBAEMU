#include "lcd-controller.hpp"

#include <util.hpp>


namespace gbaemu::lcd {
    void LCDBgObj::setMode(uint8_t *vramBaseAddress, uint8_t *oamBaseAddress, uint32_t mode) {
        bgMode = mode;

        switch (bgMode) {
            case 0: case 1: case 2:
                bg.bgMode012 = vramBaseAddress;
                objTiles = vramBaseAddress + 0x10000;
                break;
            case 3:
                bg.bgMode3 = vramBaseAddress;
                objTiles = vramBaseAddress + 0x14000;
                break;
            case 4:
                bg.bgMode45.frameBuffer0 = vramBaseAddress;
                bg.bgMode45.frameBuffer1 = vramBaseAddress + 0xA000;
                objTiles = vramBaseAddress + 0x14000;
                break;
        }

        attributes = oamBaseAddress;
    }

    LCDBgObj::ObjAttribute *LCDBgObj::accessAttribute(uint32_t index) {
        return reinterpret_cast<ObjAttribute *>(attributes + (index * 0x8));
    }

    uint32_t LCDColorPalette::toR8G8B8(uint16_t color) {
        uint32_t r = static_cast<uint32_t>(color & 0x1F) << 3;
        uint32_t g = static_cast<uint32_t>((color >> 5) & 0x1F) << 3;
        uint32_t b = static_cast<uint32_t>((color >> 10) & 0x1F) << 3;

        return (r << 16) | (g << 8) | b;
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t index) const {
        return toR8G8B8(bgPalette[index]);
    }

    uint32_t LCDColorPalette::getBgColor(uint32_t i1, uint32_t i2) const {
        return getBgColor(i1 * 16 + i2);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t index) const {
        return toR8G8B8(objPalette[index]);
    }

    uint32_t LCDColorPalette::getObjColor(uint32_t i1 ,uint32_t i2) const {
        return getObjColor(i1 * 16 + i2);
    }

    void Background::loadSettings(int32_t bgIndex, const LCDIORegs *regs, Memory& memory) {
        id = bgIndex;

        uint16_t size = (flip16(regs->BGCNT[bgIndex]) & BGCNT::SCREEN_SIZE_MASK) >> 14;
        uint32_t height = (size <= 1) ? 256 : 512;
        uint32_t width = (size % 2 == 0) ? 256 : 512;
        mosaicEnabled = flip16(regs->BGCNT[bgIndex]) & BGCNT::MOSAIC_MASK;
        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = flip16(regs->BGCNT[bgIndex]) & BGCNT::COLORS_PALETTES_MASK;
        priority = flip16(regs->BGCNT[bgIndex]) & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (flip16(regs->BGCNT[bgIndex]) & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (flip16(regs->BGCNT[bgIndex]) & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* scrolling, TODO: check sign */
        xOff = flip16(regs->BGOFS[bgIndex].h & 0x1F);
        yOff = flip16(regs->BGOFS[bgIndex].v & 0x1F);

        /* 32x32 tiles, arrangement depends on resolution */
        /* TODO: not sure about this one */
        uint8_t *vramBase = memory.resolveAddr(Memory::VRAM_OFFSET);
        bgMapBase = vramBase + screenBaseBlock * 0x800;
        scCount = (size <= 2) ? 2 : 4;
        /* tile addresses in steps of 0x4000 */
        /* 8x8, also called characters */
        tiles = vramBase + charBaseBlock * 0x4000;

        /*
            size = 0:
            +-----+
            | SC0 |
            +-----+

            size = 1:
            +---------+
            | SC0 SC1 |
            +---------+
        
            size = 2:
            +-----+
            | SC0 |
            | SC1 |
            +-----+

            size = 3:
            +---------+
            | SC0 SC1 |
            | SC2 SC3 |
            +---------+
         */
        /* always 0 */
        scXOffset[0] = 0;
        scYOffset[0] = 0;

        scXOffset[1] = (size % 2 == 1) ? 256 : 0;
        scYOffset[1] = (size == 2) ? 256 : 0;

        /* only available in size = 3 */
        scXOffset[2] = 0;
        scYOffset[2] = 256;

        /* only available in size = 3 */
        scXOffset[3] = 256;
        scYOffset[3] = 256;
    }

    void Background::render(LCDColorPalette& palette) {
        auto pixels = canvas.pixels();
        auto stride = canvas.getWidth();

        for (uint32_t scIndex = 0; scIndex < scCount; ++scIndex) {
            uint16_t *bgMap = reinterpret_cast<uint16_t *>(bgMapBase + scIndex * 0x800);

            for (uint32_t mapIndex = 0; mapIndex < 32 * 32; ++mapIndex) {
                uint16_t entry = bgMap[mapIndex];

                /* tile info */
                uint32_t tileNumber = entry & 0x3F;

                int32_t tileX = scXOffset[scIndex] + (mapIndex % 32) * 8;
                int32_t tileY = scYOffset[scIndex] + (mapIndex / 32) * 8;

                /* TODO: flipping */
                bool hFlip = (entry >> 10) & 1;
                bool vFlip = (entry >> 11) & 1;

                if (colorPalette256) {
                    uint8_t *tile = tiles + (tileNumber * 64);

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            uint32_t color = palette.getBgColor(tile[ty * 8 + tx]);
                            pixels[(tileY + ty) * stride + (tileX + tx)] = color;
                        }
                    }
                } else {
                    uint8_t *tile = tiles + (tileNumber * 32);
                    uint32_t paletteNumber = (entry >> 12) & 0xF;

                    for (uint32_t ty = 0; ty < 8; ++ty) {
                        uint32_t row = tile[ty];

                        for (uint32_t tx = 0; tx < 8; ++tx) {
                            /* TODO: order correct? */
                            uint32_t color = palette.getBgColor(paletteNumber, (row & (0b1111 << (tx * 4))) >> (tx * 4));
                            pixels[(tileY + ty) * stride + (tileX + tx)] = color;
                        }
                    }
                }
            }
        }
    }

    void LCDController::makeBgPriorityList() {
        for (uint32_t i = 0; i < 4; ++i)
            bgPriorityList[i] = i;

        /* don't swap elements if priority is equal */
        std::stable_sort(bgPriorityList, bgPriorityList + 4, [&](int32_t i, int32_t j) {
            return regs->BGCNT[i] - regs->BGCNT[j];
        });
    }

    void LCDController::updateReferences() {
        palette.bgPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET));
        palette.objPalette = reinterpret_cast<uint16_t *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET + 0x200));
        regs = reinterpret_cast<LCDIORegs *>(memory.resolveAddr(gbaemu::Memory::BG_OBJ_RAM_OFFSET));
    }

    void LCDController::renderBGMode0() {
        /*
            Mode  Rot/Scal Layers Size               Tiles Colors       Features
            0     No       0123   256x256..512x515   1024  16/16..256/1 SFMABP
            Features: S)crolling, F)lip, M)osaic, A)lphaBlending, B)rightness, P)riority.
         */
        /* TODO: I guess text mode? */
        for (uint32_t i = 0; i < 4; ++i) {
            backgrounds[i].loadSettings(i, regs, memory);
            backgrounds[i].render(palette);
        }

        /* TODO: render top alpha last */
        std::vector<int32_t> backgroundIds = {backgrounds[0].id, backgrounds[1].id, backgrounds[2].id, backgrounds[3].id};
        std::stable_sort(backgroundIds.begin(), backgroundIds.end(), [&](int32_t id1, int32_t id2) {
            return backgrounds[id1].priority - backgrounds[id2].priority; });

        /* TODO: alpha blending */
        for (uint32_t i = 0; i < 4; ++i) {
            auto bgId = backgroundIds[i];
        }
    }

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

    void LCDController::renderBG4() {
        /* TODO: This should easily be extendable to support BG4, BG5 */
        /* BG Mode 3 - 240x160 pixels, 32768 colors */
        uint8_t *pixs = memory.resolveAddr(gbaemu::Memory::VRAM_OFFSET);

        for (int32_t y = 0; y < 160; ++y) {
            for (int32_t x = 0; x < 240; ++x) {
                uint8_t index = pixs[y * 240 + x];
                uint32_t color = palette.getBgColor(index);
            }
        }
    }

    uint32_t LCDController::getBackgroundMode() const {
        return 0;
    }

    void LCDController::render() {
        uint32_t bgMode = getBackgroundMode();
        uint32_t vram = gbaemu::Memory::MemoryRegionOffset::VRAM_OFFSET;

        if (bgMode == 0) {
            renderBGMode0();
        }
    }
}