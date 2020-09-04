#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

#include "mat.hpp"


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

        int32_t getWidth() const;
        int32_t getHeight() const;
        void clear(PixelType color);
        void drawLine(int32_t x1, int32_t x2, int32_t y1, int32_t y2, PixelType color);
        void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, PixelType color);
        /* We trust the user that he provides the correct inverse. */
        void drawSprite(const PixelType *src, int32_t srcWidth, int32_t srcHeight, int32_t srcStride,
            const common::math::mat<3, 3>& trans, const common::math::mat<3, 3>& invTrans);
    };

    template <class PixelType>
    class MemoryCanvas : public Canvas<PixelType> {
    private:
        std::vector<PixelType> pixs;
    public:
        void beginDraw() override { }
        void endDraw() override { }

        PixelType *pixels() override {
            return pixs.data();
        }

        MemoryCanvas(int32_t w, int32_t h) {
            Canvas<PixelType>::width = w;
            Canvas<PixelType>::height = h;
            pixs.resize(w * h);
        }

        MemoryCanvas() {
            Canvas<PixelType>::width = 0;
            Canvas<PixelType>::height = 0;
        }
    };
}

#endif /* CANVAS_HPP */
