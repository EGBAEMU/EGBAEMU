#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <algorithm>
#include <cstdint>


namespace gbaemu::lcd {
    template <class PixelType>
    class Canvas {
      protected:
        int32_t width;
        int32_t height;
      public:
        /* some target devices require locking before pixel access */
        virtual void beginDraw() = 0;
        virtual void endDraw() = 0;
        /* returns a contiguous array of pixels */
        virtual PixelType *pixels() = 0;

        void clear(PixelType color) {
            auto pixs = pixels();

            for (int32_t y = 0; y < height; ++y)
                for (int32_t x = 0; x < width; ++x)
                    pixs[y * width + x] = color;
        }

        /* TODO: implement GBA basic drawing functions */
        void drawLine(int32_t x1, int32_t x2, int32_t y1, int32_t y2, PixelType color) {

        }

        void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, PixelType color) {
            /* clipping */
            int32_t xFrom = std::max(0, x);
            /* exclusive */
            int32_t xTo = std::min(static_cast<int32_t>(width), x + w);

            int32_t yFrom = std::max(0, y);
            /* exclusive */
            int32_t yTo = std::min(static_cast<int32_t>(height), y + h);

            auto pix = pixels();

            /* drawing */
            for (; yFrom < yTo; ++yFrom) {
                auto rowPtr = pix + (yFrom * width);

                for (int32_t ix = xFrom; ix < xTo; ++ix)
                    rowPtr[ix] = color;
            }
        }
    };
}

#endif /* CANVAS_HPP */