#ifndef BGLAYER_HPP
#define BGLAYER_HPP

#include <lcd/palette.hpp>
#include <io/memory.hpp>

#include <memory>


namespace gbaemu::lcd
{
    enum BGIndex
    {
        BG0 = 0,
        BG1,
        BG2,
        BG3
    };

    struct BGAffineTransform
    {
        vec2 d;
        vec2 dm;
        vec2 origin;
    };

    typedef uint16_t BGMode0Entry;

    struct BGMode0EntryAttributes
    {
        BGMode0EntryAttributes(BGMode0Entry entry);

        uint16_t tileNumber;
        uint16_t paletteNumber;
        bool vFlip, hFlip;
    };

    class BGLayer : public Layer
    {
      public:
        BGIndex index;      
        /* settings */
        bool useOtherFrameBuffer;
        bool mosaicEnabled;
        bool colorPalette256;
        /* actual pixel count */
        uint32_t width;
        uint32_t height;
        bool wrap;
        const uint8_t *bgMapBase;
        const uint8_t *tiles;

        /* this is not the actual bg mode, but the mode in which this Background should be rendered */
        BGMode mode;
        LCDColorPalette& palette;
        Memory& memory;
        uint16_t size;
        BGAffineTransform affineTransform;

        BGLayer(LCDColorPalette& plt, Memory& mem, BGIndex idx);
        ~BGLayer() { }
        void loadSettings(BGMode bgMode, const LCDIORegs &regs);
        std::string toString() const;

        /* used in modes 0, 1, 2 */
        const void *getBGMap(int32_t sx = 0, int32_t sy = 0) const;
        /* used in modes 3, 4, 5 */
        const void *getFrameBuffer() const;

        std::function<color_t(int32_t, int32_t)> getPixelColorFunction();
        void drawScanline(int32_t y) override;
    };
}

#endif /* BGLAYER_HPP */