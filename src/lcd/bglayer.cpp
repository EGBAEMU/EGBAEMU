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

    BGLayer::BGLayer(LCDColorPalette &plt, Memory &mem, BGIndex idx) : index(idx), palette(plt), memory(mem)
    {
        layerID = static_cast<LayerID>(index);
        isBGLayer = true;
    }

    void BGLayer::loadSettings(BGMode bgMode, const LCDIORegs &regs)
    {
        if (!enabled)
            return;

        size = (le(regs.BGCNT[index]) & BGCNT::SCREEN_SIZE_MASK) >> 14;
        mode = bgMode;

        /* mixed mode, layers 0, 1 are drawn in mode 0, layer 2 is drawn in mode 2 */
        if (mode == Mode1) {
            if (index == BG0 || index == BG1)
                mode = Mode0;
            else
                mode = Mode2;
        }

        /* TODO: not entirely correct */
        if (bgMode == 0 || (bgMode == 1 && index <= 1)) {
            /* text mode */
            height = (size <= 1) ? 256 : 512;
            width = (size % 2 == 0) ? 256 : 512;
        } else if (bgMode == 2 || (bgMode == 1 && index == 2)) {
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

        mosaicEnabled = le(regs.BGCNT[index]) & BGCNT::MOSAIC_MASK;

        if (mosaicEnabled) {
            mosaicWidth = bitGet(le(regs.MOSAIC), MOSAIC::BG_MOSAIC_HSIZE_MASK, MOSAIC::BG_MOSAIC_HSIZE_OFFSET) + 1;
            mosaicHeight = bitGet(le(regs.MOSAIC), MOSAIC::BG_MOSAIC_VSIZE_MASK, MOSAIC::BG_MOSAIC_VSIZE_OFFSET) + 1;
        } else {
            mosaicWidth = 1;
            mosaicHeight = 1;
        }

        /* if true tiles have 8 bit color depth, 4 bit otherwise */
        colorPalette256 = le(regs.BGCNT[index]) & BGCNT::COLORS_PALETTES_MASK;
        priority = le(regs.BGCNT[index]) & BGCNT::BG_PRIORITY_MASK;
        /* offsets */
        uint32_t charBaseBlock = (le(regs.BGCNT[index]) & BGCNT::CHARACTER_BASE_BLOCK_MASK) >> 2;
        uint32_t screenBaseBlock = (le(regs.BGCNT[index]) & BGCNT::SCREEN_BASE_BLOCK_MASK) >> 8;

        /* select which frame buffer to use */
        if (bgMode == 4 || bgMode == 5)
            useOtherFrameBuffer = le(regs.DISPCNT) & DISPCTL::DISPLAY_FRAME_SELECT_MASK;
        else
            useOtherFrameBuffer = false;

        /* wrapping */
        if (bgMode == 0) {
            wrap = true;
        } else if (index == 2 || index == 3) {
            wrap = le(regs.BGCNT[index]) & BGCNT::DISPLAY_AREA_OVERFLOW_MASK;
        } else {
            wrap = false;
        }

        /* scaling, rotation, only for bg2, bg3 */
        if (bgMode != 0 && (index == 2 || index == 3)) {
            useTrans = true;

            if (index == 2) {
                affineTransform.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2X));
                affineTransform.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG2Y));

                affineTransform.d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[0]));
                affineTransform.dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[1]));
                affineTransform.d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[2]));
                affineTransform.dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG2P[3]));
            } else {
                affineTransform.origin[0] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3X));
                affineTransform.origin[1] = fixedToFloat<uint32_t, 8, 19>(le(regs.BG3Y));

                affineTransform.d[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[0]));
                affineTransform.dm[0] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[1]));
                affineTransform.d[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[2]));
                affineTransform.dm[1] = fixedToFloat<uint16_t, 8, 7>(le(regs.BG3P[3]));
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

            /* use scrolling parameters */
            affineTransform.origin[0] = static_cast<common::math::real_t>(le(regs.BGOFS[index].h) & 0x1FF);
            affineTransform.origin[1] = static_cast<common::math::real_t>(le(regs.BGOFS[index].v) & 0x1FF);

            affineTransform.d[0] = 1;
            affineTransform.d[1] = 0;
            affineTransform.dm[0] = 0;
            affineTransform.dm[1] = 1;
        }

        /* 32x32 tiles, arrangement depends on resolution */
        /* TODO: not sure about this one */
        uint8_t *vramBase = memory.vram;
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

    const void *BGLayer::getBGMap(int32_t sx, int32_t sy) const
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
        return memory.vram + fbOff;
    }

    std::function<color_t(int32_t, int32_t)> BGLayer::getPixelColorFunction()
    {
        const void *bgMap = getBGMap();
        const void *frameBuffer = getFrameBuffer();

        std::function<color_t(int32_t, int32_t)> pixelColor;

        /* select pixel color function */
        switch (mode) {
            case Mode0:
                /*
                    +-----------+
                    | SC0 | SC1 |
                    +-----+-----+
                    | SC2 | SC3 |
                    +-----+-----+

                    Every bg map is at (bg base) + (sc index) * 0x800.
                    Every SC has size 256x256 (32x32 tiles).
                 */
                pixelColor = [this](int32_t sx, int32_t sy) -> color_t {
                    const BGMode0Entry *bgMap = reinterpret_cast<const BGMode0Entry *>(getBGMap(sx, sy));

                    int32_t msx = sx - (sx % mosaicWidth);
                    int32_t msy = sy - (sy % mosaicHeight);

                    /* sx, sy relative to the current bg map (size 32x32) */
                    int32_t relBGMapX = msx % 256;
                    int32_t relBGMapY = msy % 256;
                    int32_t tileX = relBGMapX / 8;
                    int32_t tileY = relBGMapY / 8;

                    BGMode0EntryAttributes attrs(bgMap[tileY * 32 + tileX]);
                    const uint8_t *tile = reinterpret_cast<const uint8_t *>(tiles) + attrs.tileNumber * (colorPalette256 ? 64 : 32);

                    int32_t tx = attrs.hFlip ? (7 - (relBGMapX % 8)) : (relBGMapX % 8);
                    int32_t ty = attrs.vFlip ? (7 - (relBGMapY % 8)) : (relBGMapY % 8);

                    if (colorPalette256) {
                        return palette.getBgColor(tile[ty * 8 + tx]);
                    } else {
                        uint32_t row = reinterpret_cast<const uint32_t *>(tile)[ty];
                        uint32_t paletteIndex = (row >> (tx * 4)) & 0xF;
                        return palette.getBgColor(attrs.paletteNumber, paletteIndex);
                    }
                };
                break;
            case Mode2:
                pixelColor = [bgMap, this](int32_t sx, int32_t sy) -> color_t {
                    int32_t msx = sx - (sx % mosaicWidth);
                    int32_t msy = sy - (sy % mosaicHeight);

                    int32_t tileX = msx / 8;
                    int32_t tileY = msy / 8;
                    uint32_t tileNumber = reinterpret_cast<const uint8_t *>(bgMap)[tileY * (width / 8) + tileX];
                    const uint8_t *tile = reinterpret_cast<const uint8_t *>(tiles) + (tileNumber * 64);
                    uint32_t paletteIndex = tile[(msy % 8) * 8 + (msx % 8)];

                    return palette.getBgColor(paletteIndex);
                };
                break;
            case Mode3:
            case Mode5:
                /* 32768 colors in color16 format */
                pixelColor = [frameBuffer, this](int32_t sx, int32_t sy) -> color_t {
                    int32_t msx = sx - (sx % mosaicWidth);
                    int32_t msy = sy - (sy % mosaicHeight);

                    return LCDColorPalette::toR8G8B8(reinterpret_cast<const color16_t *>(frameBuffer)[msy * width + msx]);
                };
                break;
            case Mode4:
                /* 256 indexed colors */
                pixelColor = [frameBuffer, this](int32_t sx, int32_t sy) -> color_t {
                    int32_t msx = sx - (sx % mosaicWidth);
                    int32_t msy = sy - (sy % mosaicHeight);

                    return palette.getBgColor(reinterpret_cast<const uint8_t *>(frameBuffer)[msy * width + msx]);
                };
                break;
            default:
                LOG_LCD(std::cout << "Invalid mode!" << std::endl;);
                throw std::runtime_error("Invalid mode!");
        }

        return pixelColor;
    }

    void BGLayer::drawScanline(int32_t y)
    {
        auto pixelColor = getPixelColorFunction();
        vec2 s = affineTransform.origin + (useTrans ? vec2{0, 0} : affineTransform.dm * y);

        for (int32_t x = 0; x < static_cast<int32_t>(SCREEN_WIDTH); ++x) {
            int32_t sx = static_cast<int32_t>(s[0]);
            int32_t sy = static_cast<int32_t>(s[1]);

            if (wrap || (0 <= sx && sx < static_cast<int32_t>(width) && 0 <= sy && sy < static_cast<int32_t>(height))) {
                if (wrap) {
                    sx = fastMod<int32_t>(sx, width);
                    sy = fastMod<int32_t>(sy, height);
                }

                scanline[x] = Fragment(pixelColor(sx, sy), asFirstTarget, asSecondTarget, false);
            } else {
                scanline[x] = Fragment(TRANSPARENT, asFirstTarget, asSecondTarget, false);
            }

            s += affineTransform.d;
        }
    }

    bool BGLayer::operator<(const BGLayer &other) const noexcept
    {
        return (priority == other.priority) ? (index < other.index) : (priority < other.priority);
    }
} // namespace gbaemu::lcd