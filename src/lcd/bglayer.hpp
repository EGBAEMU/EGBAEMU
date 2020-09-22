#ifndef BGLAYER_HPP
#define BGLAYER_HPP

#include <lcd/palette.hpp>
#include <io/memory.hpp>


namespace gbaemu::lcd
{
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

        Background(LayerId i) : Layer(i), tempCanvas(1024, 1024) { }
        ~Background() { }
        void loadSettings(uint32_t bgMode, int32_t bgIndex, const LCDIORegs &regs, Memory &memory);
        void renderBG0(LCDColorPalette &palette);
        void renderBG2(LCDColorPalette &palette);
        void renderBG3(Memory &memory);
        void renderBG4(LCDColorPalette &palette, Memory &memory);
        void renderBG5(LCDColorPalette &palette, Memory &memory);
        void draw();
    };
}

#endif /* BGLAYER_HPP */