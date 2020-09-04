#include "canvas.hpp"

namespace gbaemu::lcd
{
    template <class PixelType>
    int32_t Canvas<PixelType>::getWidth() const {
        return width;
    }

    template <class PixelType>
    int32_t Canvas<PixelType>::getHeight() const {
        return height;
    }

    template <class PixelType>
    void Canvas<PixelType>::clear(PixelType color) {
        auto pixs = pixels();

        for (int32_t y = 0; y < height; ++y)
            for (int32_t x = 0; x < width; ++x)
                pixs[y * width + x] = color;
    }

    template <class PixelType>
    void Canvas<PixelType>::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, PixelType color) {
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

    template <class PixelType>
    void Canvas<PixelType>::drawSprite(const PixelType *src, int32_t srcWidth, int32_t srcHeight, int32_t srcStride,
            const common::math::mat<3, 3>& trans, const common::math::mat<3, 3>& invTrans) {
        typedef common::math::vec<2> vec2;
        typedef common::math::vec<3> vec3;
        typedef common::math::mat<3, 3> mat;
        typedef common::math::real_t real_t;

        /* I hoped to never have to implement this crap. Well here we go... */
        /* The screen and the sprite are small, sequential pixel access is therefore not that important. Cache should be big enough. */

        /*
            +------------------------------->
            |
            |          ......................                        
            |          .          ---->
            |          .      ----
            |          .  ----
            |          . +
            |          .  |
            |          .   |
            |          .    |
            |          .     |
            |          .     \/
            \/
        
        
         */

        
        /* Calculate the bounds of the transformed sprite on the canvas. */
        vec3 corners[] = {
            trans * vec3{0, 0, 1},
            trans * vec3{srcWidth - 1, 0, 1},
            trans * vec3{0, srcHeight - 1, 1},
            trans * vec3{srcWidth - 1, srcHeight - 1, 1}
        };

        auto min4 = [](real_t a, real_t b, real_t c, real_t d)  {
            return std::min(a, std::min(b, std::min(c, d)));
        };

        auto max4 = [](real_t a, real_t b, real_t c, real_t d)  {
            return std::max(a, std::max(b, std::max(c, d)));
        };

        /* clip to canvas bounds */
        int32_t fromX = std::max(0,             static_cast<int32_t>(min4(corners[0][0], corners[1][0], corners[2][0], corners[3][0])));
        int32_t fromY = std::max(0,             static_cast<int32_t>(min4(corners[0][1], corners[1][1], corners[2][1], corners[3][1])));
        int32_t toX = std::min(getWidth() - 1,  static_cast<int32_t>(max4(corners[0][0], corners[1][0], corners[2][0], corners[3][0])));
        int32_t toY = std::min(getHeight() - 1, static_cast<int32_t>(max4(corners[0][1], corners[1][1], corners[2][1], corners[3][1])));

        auto getLineIntersection = [](const vec3& a, const vec3& b, real_t f, uint32_t coord, real_t& lambda) -> bool {
            auto abDelta = b[coord] - a[coord];
            auto afDelta = f - a[coord];

            if (abDelta >= -1e-6 && abDelta <= 1e-6)
                return false;

            lambda = afDelta / abDelta;

            return 0 <= lambda && lambda <= 1;
        };

        auto lerp = [](const vec3& a, const vec3& b, real_t lambda) {
            return a * (1 - lambda) + b * lambda;
        };

        auto width = getWidth();
        auto destPixels = pixels();

        for (int32_t y = fromY; y <= toY; ++y) {
            /*
                The line segment (fromX, y) ---- (toX, y) in canvas space must intersect with sprite in
                sprite space.
             */

            const auto a = invTrans * vec3{fromX, y, 1};
            const auto b = invTrans * vec3{toX, y, 1};

            real_t lambda;
            vec3 spriteFrom = a, spriteTo = b;

            /* TODO: implement proper clipping */
            /* left, right */
            //if (getLineIntersection(spriteFrom, spriteTo, 0, 0, lambda))
            //    spriteFrom = lerp(spriteFrom, spriteTo, lambda);

            //if (getLineIntersection(spriteFrom, spriteTo, srcWidth - 1, 0, lambda))
                //spriteTo = lerp(spriteFrom, spriteTo, lambda);

            /* top, bottom */
            //if (getLineIntersection(spriteFrom, spriteTo, 0, 1, lambda))
                //spriteFrom = lerp(spriteFrom, spriteTo, lambda);

            //if (getLineIntersection(spriteFrom, spriteTo, srcHeight - 1, 1, lambda))
                //spriteTo = lerp(spriteFrom, spriteTo, lambda);

            /* line segment in sprite space is spriteFrom ---- spriteTo */
            const vec3 spriteDelta = spriteTo - spriteFrom;
            const vec3 spriteStep = spriteDelta / (toX - fromX);
            vec3 spriteCoord = spriteFrom;

            /* draw scanline */
            for (int32_t x = fromX; x <= toX; ++x) {
                if (0 <= spriteCoord[0] && spriteCoord[0] <= srcWidth - 1 &&
                    0 <= spriteCoord[1] && spriteCoord[1] <= srcHeight - 1) {
                    int32_t sx = static_cast<int32_t>(spriteCoord[0]);
                    int32_t sy = static_cast<int32_t>(spriteCoord[1]);

                    destPixels[y * width + x] = src[sy * srcStride + sx];
                }

                spriteCoord += spriteStep;
            }
        }
    }

    template class Canvas<uint32_t>;
}