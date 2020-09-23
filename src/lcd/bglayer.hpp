#ifndef BGLAYER_HPP
#define BGLAYER_HPP

#include <lcd/palette.hpp>
#include <io/memory.hpp>

#include <memory>


namespace gbaemu::lcd
{
    /*
        Mode  Rot/Scal Layers Size               Tiles Colors       Features
        0     No       0123   256x256..512x515   1024  16/16..256/1 SFMABP
        1     Mixed    012-   (BG0,BG1 as above Mode 0, BG2 as below Mode 2)
        2     Yes      --23   128x128..1024x1024 256   256/1        S-MABP
        3     Yes      --2-   240x160            1     32768        --MABP
        4     Yes      --2-   240x160            2     256/1        --MABP
        5     Yes      --2-   160x128            2     32768        --MABP

        Features: S)crolling, F)lip, M)osaic, A)lphaBlending, B)rightness, P)riority.
     */
    enum BGMode
    {
        Mode0 = 0,
        Mode1,
        Mode2,
        Mode3,
        Mode4,
        Mode5
    };

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