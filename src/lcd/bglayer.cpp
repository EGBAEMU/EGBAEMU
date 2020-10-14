#include "bglayer.hpp"

#include "logging.hpp"
#include <sstream>

namespace gbaemu::lcd
{
    BGMode0EntryAttributes::BGMode0EntryAttributes(BGMode0Entry entry)
    {
        tileNumber = bitGet<uint16_t>(entry, 0x3FF, 0);
        paletteNumber = bitGet<uint16_t>(entry, 0xF, 12);
        hFlip = isBitSet<uint16_t, 10>(entry);
        vFlip = isBitSet<uint16_t, 11>(entry);
    }

    void BGMode0EntryAttributes::load(BGMode0Entry entry)
    {
        tileNumber = bitGet<uint16_t>(entry, 0x3FF, 0);
        paletteNumber = bitGet<uint16_t>(entry, 0xF, 12);
        hFlip = isBitSet<uint16_t, 10>(entry);
        vFlip = isBitSet<uint16_t, 11>(entry);
    }

    BGLayer::BGLayer(LCDColorPalette &plt, Memory &mem, BGIndex idx) : Layer(static_cast<LayerID>(idx), true), index(idx), palette(plt), memory(mem)
    {
    }

    void BGLayer::loadSettings(BGMode bgMode, const LCDIORegs &regs)
    {
        if (!enabled)
            return;

        uint16_t bgControl = le(regs.BGCNT[index]);

        size = (bgControl & BGCNT::SCREEN_SIZE_MASK) >> 14;
        mode = bgMode;

        /* mixed mode, layers 0, 1 are drawn in mode 0, layer 2 is drawn in mode 2 */
        if (mode == Mode1) {
            if (index == BG0 || index == BG1)
                mode = Mode0;
            else
                mode = Mode2;
        }

        /* TODO: not entirely correct */
        if (bgMode == 0 || (bgMode == 1 && index <= BG1)) {
            /* text mode */
            height = (size <= 1) ? 256 : 512;
            width = (size % 2 == 0) ? 256 : 512;
        } else if (bgMode == 2 || (bgMode == 1 && index == BG2)) {
            switch (size) {
                case 0:
                    width = 128;
                    height = 128;
                    break;
                case 1:
                    width = 256;
                    height = 256;
                    break;
                case 2:
                    width = 512;
                    height = 512;
                    break;
                case 3:
                    width = 1024;
                    height = 1024;
                    break;
                default:
                    width = 0;
                    height = 0;
                    LOG_LCD(std::cout << "WARNING: Invalid screen size!\n";);
                    break;
            }
        } else if (bgMode == 3) {
            width = 240;
            height = 160;
        } else if (bgMode == 4) {
            width = 240;
            height = 160;
        } else if (bgMode == 5) {
            width = 160;
            height = 128;
        }

        mosaicEnabled = bgControl & BGCNT::MOSAIC_MASK;

        if (mosaicEnabled) {
            mosaicWidth = bitGet(le(regs.MOSAIC), MOSAIC::BG_MOSAIC_HSIZE_MASK, MOSAIC::BG_MOSAIC_HSIZE_OFFSET) + 1;
            mosaicHeight = bitGet(le(regs.MOSAIC), MOSAIC::BG_MOSAIC_VSIZE_MASK, MOSAIC::BG_MOSAIC_VSIZE_OFFSET) + 1;
        } else {
            mosaicWidth = 1;
            mosaicHeight = 1;
        }

        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = bgControl & BGCNT::COLORS_PALETTES_MASK;
        priority = bgControl & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (bgControl & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (bgControl & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* select which frame buffer to use */
        if (bgMode == 4 || bgMode == 5) {
            useOtherFrameBuffer = le(regs.DISPCNT) & DISPCTL::DISPLAY_FRAME_SELECT_MASK;
        } else {
            useOtherFrameBuffer = false;
        }

        /* wrapping */
        if (bgMode == 0) {
            wrap = true;
        } else if (index == BG2 || index == BG3) {
            wrap = bgControl & BGCNT::DISPLAY_AREA_OVERFLOW_MASK;
        } else {
            wrap = false;
        }

        /* scaling, rotation, only for bg2, bg3 */
        if (bgMode != 0 && (index == BG2 || index == BG3)) {
            useTrans = true;

            const auto rotScalParams = index == BG2 ? regs.BG2P : regs.BG3P;

            affineTransform.d[0] = fixedToFloat<uint16_t, 8, 7>(le(rotScalParams[0]));
            affineTransform.dm[0] = fixedToFloat<uint16_t, 8, 7>(le(rotScalParams[1]));
            affineTransform.d[1] = fixedToFloat<uint16_t, 8, 7>(le(rotScalParams[2]));
            affineTransform.dm[1] = fixedToFloat<uint16_t, 8, 7>(le(rotScalParams[3]));

            if (index == BG2) {
                affineTransform.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2X));
                affineTransform.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2Y));
            } else {
                affineTransform.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3X));
                affineTransform.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3Y));
            }

            if (affineTransform.d[0] == 0 && affineTransform.d[1] == 0) {
                affineTransform.d[0] = 1;
                affineTransform.d[1] = 0;
            }

            if (affineTransform.dm[0] == 0 && affineTransform.dm[1] == 0) {
                affineTransform.dm[0] = 0;
                affineTransform.dm[1] = 1;
            }
        } else {
            useTrans = false;
            wrap = true;

            /* use scrolling parameters */
            affineTransform.origin[0] = static_cast<common::math::real_t>(le(regs.BGOFS[index].h) & 0x1FF);
            affineTransform.origin[1] = static_cast<common::math::real_t>(le(regs.BGOFS[index].v) & 0x1FF);

            affineTransform.d[0] = 1;
            affineTransform.d[1] = 0;
            affineTransform.dm[0] = 0;
            affineTransform.dm[1] = 1;
        }

        /* 32x32 tiles, arrangement depends on resolution */
        const uint8_t *vramBase = memory.vram.rawAccess();
        bgMapBase = vramBase + screenBaseBlock * 0x800;

        /* tile addresses in steps of 0x4000 */
        /* 8x8, also called characters */
        tiles = vramBase + charBaseBlock * 0x4000;

        asFirstTarget = bitGet<uint16_t>(le(regs.BLDCNT), BLDCNT::TARGET_MASK, BLDCNT::BG_FIRST_TARGET_OFFSET(static_cast<uint16_t>(index)));
        asSecondTarget = bitGet<uint16_t>(le(regs.BLDCNT), BLDCNT::TARGET_MASK, BLDCNT::BG_SECOND_TARGET_OFFSET(static_cast<uint16_t>(index)));
    }

    std::string BGLayer::toString() const
    {
        std::stringstream ss;

        ss << "enabled: " << (enabled ? "yes" : "no") << '\n';
        ss << "width height: " << width << 'x' << height << '\n';
        ss << "origin: " << affineTransform.origin << '\n';
        ss << "d dm: " << affineTransform.d << ' ' << affineTransform.dm << '\n';

        return ss.str();
    }

    const void *BGLayer::getBGMap(uint32_t sx, uint32_t sy) const
    {
        switch (mode) {
            case Mode0: {
                int32_t scIndex = (sx / 256) + (sy / 256) * 2;

                /* only exception */
                if (size == 2 && scIndex == 2)
                    scIndex = 1;

                return bgMapBase + scIndex * 0x800;
            }
            case Mode2:
                return bgMapBase;
            default:
                return nullptr;
        }
    }

    const void *BGLayer::getFrameBuffer() const
    {
        size_t fbOff = ((mode == Mode5 || mode == Mode4) && useOtherFrameBuffer) ? 0xA000 : 0;
        return memory.vram.rawAccess() + fbOff;
    }

    void BGLayer::_drawScanline0(int32_t y)
    {
        const vec2 sf = affineTransform.origin + (useTrans ? vec2{0, 0} : affineTransform.dm * y);
        const int32_t sy = fastMod<int32_t>(static_cast<int32_t>(sf[1]), static_cast<int32_t>(height));
        const int32_t msy = sy - (sy % mosaicHeight);
        const int32_t relBGMapY = msy & 255;
        const int32_t tileY = relBGMapY >> 3;
        const int32_t vFlipOptionA = (7 - (relBGMapY & 7));
        const int32_t vFlipOptionB = (relBGMapY & 7);
        const uint16_t colorPaletteShift = colorPalette256 ? 6 : 5;
        const uint8_t fragmentProps = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false).props;
        const uint8_t *tile;
        uint32_t tileOffset;
        BGMode0EntryAttributes attrs;
        uint16_t bgMapIndex = -1;
        int32_t ty;

        int32_t sx = static_cast<int32_t>(sf[0]);

        const void *bgMaps[] = {
            getBGMap(fastMod<int32_t>(sx, width), sy),
            getBGMap(fastMod<int32_t>(sx + 256, width), sy)
        };

        if (colorPalette256) {
            for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x, ++sx) {
                sx = fastMod<int32_t>(sx, width);
                const BGMode0Entry *bgMap = reinterpret_cast<const BGMode0Entry *>(bgMaps[sx / 256]);

                const int32_t msx = sx - (sx % mosaicWidth);

                const int32_t relBGMapX = msx & 255;
                const int32_t tileX = relBGMapX >> 3;

                if ((tileY << 5) + tileX != bgMapIndex) {
                    bgMapIndex = (tileY << 5) + tileX;
                    attrs.load(le(bgMap[bgMapIndex]));
                    ty = attrs.vFlip ? vFlipOptionA : vFlipOptionB;
                    tileOffset = (attrs.tileNumber << colorPaletteShift);
                }

                const int32_t tx = attrs.hFlip ? (7 - (relBGMapX & 7)) : (relBGMapX & 7);
                const uint8_t *tile = reinterpret_cast<const uint8_t *>(tiles) + tileOffset;

                scanline[x].color = palette.getBgColor(tile[(ty << 3) + tx]);
                scanline[x].props = fragmentProps;
            }
        } else {
            for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x, ++sx) {
                sx = fastMod<int32_t>(sx, width);
                const BGMode0Entry *bgMap = reinterpret_cast<const BGMode0Entry *>(bgMaps[sx / 256]);

                const int32_t msx = sx - (sx % mosaicWidth);

                const int32_t relBGMapX = msx & 255;
                const int32_t tileX = relBGMapX >> 3;

                if ((tileY << 5) + tileX != bgMapIndex) {
                    bgMapIndex = (tileY << 5) + tileX;
                    attrs.load(le(bgMap[bgMapIndex]));
                    ty = attrs.vFlip ? vFlipOptionA : vFlipOptionB;
                    tileOffset = (attrs.tileNumber << colorPaletteShift);
                }

                const int32_t tx = attrs.hFlip ? (7 - (relBGMapX & 7)) : (relBGMapX & 7);
                const uint8_t *tile = reinterpret_cast<const uint8_t *>(tiles) + tileOffset;
                const uint32_t row = reinterpret_cast<const uint32_t *>(tile)[ty];
                const uint32_t paletteIndex = (row >> (tx << 2)) & 0xF;

                scanline[x].color = palette.getBgColor(attrs.paletteNumber, paletteIndex);
                scanline[x].props = fragmentProps;
            }
        }
    }

    void BGLayer::_drawScanline2(int32_t y)
    {
        const void *bgMap = getBGMap();
        const uint8_t fragmentProps = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false).props;
        vec2 s = affineTransform.origin + (useTrans ? vec2{0, 0} : affineTransform.dm * y);

        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            int32_t sx = static_cast<int32_t>(s[0]);
            int32_t sy = static_cast<int32_t>(s[1]);

            if (wrap || (0 <= sx && sx < static_cast<int32_t>(width) && 0 <= sy && sy < static_cast<int32_t>(height))) {
                if (wrap) {
                    sx = fastMod<int32_t>(sx, width);
                    sy = fastMod<int32_t>(sy, height);
                }

                int32_t msx = sx - (sx % mosaicWidth);
                int32_t msy = sy - (sy % mosaicHeight);

                int32_t tileX = msx >> 3;
                int32_t tileY = msy >> 3;
                uint32_t tileNumber = reinterpret_cast<const uint8_t *>(bgMap)[tileY * (width / 8) + tileX];
                const uint8_t *tile = reinterpret_cast<const uint8_t *>(tiles) + (tileNumber << 6);
                uint32_t paletteIndex = tile[((msy & 7) << 3) + (msx & 7)];

                scanline[x].color = palette.getBgColor(paletteIndex);
            } else {
                scanline[x].color = TRANSPARENT;
            }

            scanline[x].props = fragmentProps;
            s += affineTransform.d;
        }
    }

    void BGLayer::_drawScanline35(int32_t y)
    {
        const uint16_t *frameBuffer = reinterpret_cast<const uint16_t *>(getFrameBuffer());
        vec2 s = affineTransform.origin + (useTrans ? vec2{0, 0} : affineTransform.dm * y);

        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            int32_t sx = static_cast<int32_t>(s[0]);
            int32_t sy = static_cast<int32_t>(s[1]);

            if (wrap || (0 <= sx && sx < static_cast<int32_t>(width) && 0 <= sy && sy < static_cast<int32_t>(height))) {
                if (wrap) {
                    sx = fastMod<int32_t>(sx, width);
                    sy = fastMod<int32_t>(sy, height);
                }

                int32_t msx = sx - (sx % mosaicWidth);
                int32_t msy = sy - (sy % mosaicHeight);

                scanline[x] = Fragment(frameBuffer[msy * width + msx], asFirstTarget, asSecondTarget, false);
            } else {
                scanline[x] = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false);
            }

            s += affineTransform.d;
        }
    }

    void BGLayer::_drawScanline4(int32_t y)
    {
        const uint8_t *frameBuffer = reinterpret_cast<const uint8_t *>(getFrameBuffer());
        vec2 s = affineTransform.origin + (useTrans ? vec2{0, 0} : affineTransform.dm * y);

        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            int32_t sx = static_cast<int32_t>(s[0]);
            int32_t sy = static_cast<int32_t>(s[1]);

            if (wrap || (0 <= sx && sx < static_cast<int32_t>(width) && 0 <= sy && sy < static_cast<int32_t>(height))) {
                if (wrap) {
                    sx = fastMod<int32_t>(sx, width);
                    sy = fastMod<int32_t>(sy, height);
                }

                int32_t msx = sx - (sx % mosaicWidth);
                int32_t msy = sy - (sy % mosaicHeight);

                scanline[x] = Fragment(palette.getBgColor(frameBuffer[msy * width + msx]),
                    asFirstTarget, asSecondTarget, false);
            } else {
                scanline[x] = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false);
            }

            s += affineTransform.d;
        }
    }

    void BGLayer::drawScanline(int32_t y)
    {
        switch (mode) {
            case Mode0:
                _drawScanline0(y);
                break;
            case Mode2:
                _drawScanline2(y);
                break;
            case Mode3:
            case Mode5:
                _drawScanline35(y);
                break;
            case Mode4:
                _drawScanline4(y);
                break;
            default:
                break;
        }
    }

    bool BGLayer::operator<(const BGLayer &other) const noexcept
    {
        return (priority == other.priority) ? (index < other.index) : (priority < other.priority);
    }
} // namespace gbaemu::lcd