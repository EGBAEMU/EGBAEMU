#include "canvas.hpp"
#include "util.hpp"

#include <algorithm>
#include <cassert>

namespace gbaemu::lcd
{
    template <class PixelType>
    int32_t Canvas<PixelType>::getWidth() const
    {
        return width;
    }

    template <class PixelType>
    int32_t Canvas<PixelType>::getHeight() const
    {
        return height;
    }

    template <class PixelType>
    void Canvas<PixelType>::clear(PixelType color)
    {
        auto pixs = pixels();

        for (int32_t y = 0; y < height; ++y)
            for (int32_t x = 0; x < width; ++x)
                pixs[y * width + x] = color;
    }

    template <class PixelType>
    void Canvas<PixelType>::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, PixelType color)
    {
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
                                       const common::math::mat<3, 3> &trans, const common::math::mat<3, 3> &invTrans, bool wrap)
    {
        typedef common::math::vec<2> vec2;
        typedef common::math::vec<3> vec3;
        typedef common::math::mat<3, 3> mat;
        typedef common::math::real_t real_t;

        /* I hoped to never have to implement this crap. Well here we go... */

        /*
            target canvas
            +------------------------------->
            |          bounds
            |          ......................                        
            |          .          ---->
            |          .      ----
            |          .  ----
            |          . +  sprite
            |          .  |
            |          .   |
            |          .    |
            |          .     |
            |          .     \/
            \/

            trans: sprite space -> canvas space
            invTrans: canvas space -> sprite space
         */

        /* Calculate the bounds of the transformed sprite on the canvas. */
        vec3 corners[] = {
            trans * vec3{static_cast<real_t>(0), static_cast<real_t>(0), static_cast<real_t>(1)},
            trans * vec3{static_cast<real_t>(srcWidth - 1), static_cast<real_t>(0), static_cast<real_t>(1)},
            trans * vec3{static_cast<real_t>(0), static_cast<real_t>(srcHeight - 1), static_cast<real_t>(1)},
            trans * vec3{static_cast<real_t>(srcWidth - 1), static_cast<real_t>(srcHeight - 1), static_cast<real_t>(1)}};

        auto min4 = [](real_t a, real_t b, real_t c, real_t d) {
            return std::min(a, std::min(b, std::min(c, d)));
        };

        auto max4 = [](real_t a, real_t b, real_t c, real_t d) {
            return std::max(a, std::max(b, std::max(c, d)));
        };

        int32_t fromX = static_cast<int32_t>(min4(corners[0][0], corners[1][0], corners[2][0], corners[3][0]));
        int32_t fromY = static_cast<int32_t>(min4(corners[0][1], corners[1][1], corners[2][1], corners[3][1]));
        int32_t toX = static_cast<int32_t>(max4(corners[0][0], corners[1][0], corners[2][0], corners[3][0]));
        int32_t toY = static_cast<int32_t>(max4(corners[0][1], corners[1][1], corners[2][1], corners[3][1]));

        /*
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
         */

        auto width = getWidth();
        auto destPixels = pixels();

        /* highlight bounds */
        /*
        if (wrap) {

        } else {
            destPixels[clamp(fromY, 0, height - 1) * width + clamp(fromX, 0, width - 1)] = 0xFF0000FF;
            destPixels[clamp(toY, 0, height - 1) * width + clamp(fromX, 0, width - 1)] = 0xFF0000FF;
            destPixels[clamp(fromY, 0, height - 1) * width + clamp(toX, 0, width - 1)] = 0xFF0000FF;
            destPixels[clamp(toY, 0, height - 1) * width + clamp(toX, 0, width - 1)] = 0xFF0000FF;
        }
         */

        for (int32_t y = fromY, canvY = fromY; y <= toY; ++y, ++canvY) {
            if (!wrap) {
                if (y < 0)
                    continue;
                else if (y >= getHeight())
                    break;
            } else {
                if (canvY < 0 || canvY >= getHeight())
                    canvY = canvY % getHeight();
            }

            /*
                The line segment (fromX, y) ---- (toX, y) in canvas space must intersect with sprite in
                sprite space.
             */

            const auto a = invTrans * vec3{static_cast<real_t>(fromX), static_cast<real_t>(y), static_cast<real_t>(1)};
            const auto b = invTrans * vec3{static_cast<real_t>(toX), static_cast<real_t>(y), static_cast<real_t>(1)};

            //real_t lambda;
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
            for (int32_t x = fromX, canvX = fromX; x <= toX; ++x, ++canvX) {
                if (!wrap) {
                    if (x < 0) {
                        spriteCoord += spriteStep;
                        continue;
                    } else if (x >= getWidth())
                        break;
                } else {
                    if (canvX < 0 || canvX >= getWidth())
                        canvX = canvX % getWidth();
                }

                int32_t sx = static_cast<int32_t>(spriteCoord[0]);
                int32_t sy = static_cast<int32_t>(spriteCoord[1]);

                if (0 <= sx && sx < srcWidth &&
                    0 <= sy && sy < srcHeight) {
                    PixelType srcColor = src[sy * srcStride + sx];

                    /* TODO: assumes PixelType = uint32_t */
                    if (srcColor & 0xFF000000) {
                        assert(0 <= canvX && canvX < width);
                        assert(0 <= canvY && canvY < height);
                        assert(0 <= spriteCoord[0] && spriteCoord[0] < srcWidth);
                        assert(0 <= spriteCoord[1] && spriteCoord[1] < srcHeight);

                        destPixels[canvY * width + canvX] = srcColor;
                    }
                }

                spriteCoord += spriteStep;
            }
        }
    }

    template <class PixelType>
    void Canvas<PixelType>::drawSprite(const PixelType *src, int32_t srcWidth, int32_t srcHeight, int32_t srcStride,
                                       const common::math::vec<2> &origin,
                                       const common::math::vec<2> &d,
                                       const common::math::vec<2> &dm,
                                       const common::math::vec<2> &screenRef,
                                       bool wrap)
    {
        /* TODO: Implement wrapping. */
        /* Implemented according to pseudo code on https://www.coranac.com/tonc/text/affobj.htm. */
        typedef common::math::vec<2> vec2;
        typedef common::math::real_t real_t;

        PixelType *destPixels = pixels();
        vec2 spriteCoordScanLine = d * (-screenRef[0]) + dm * (-screenRef[1]) + origin;

        for (int32_t y = 0; y < height; ++y) {
            vec2 spriteCoord = spriteCoordScanLine;

            for (int32_t x = 0; x < width; ++x) {
                int32_t sx = spriteCoord[0];
                int32_t sy = spriteCoord[1];

                /* assumes PixelType = uint32_t */
                if (!wrap) {
                    if (0 <= sx && sx < srcWidth && 0 <= sy && sy < srcHeight) {
                        PixelType color = src[sy * srcStride + sx];

                        if (color & 0xFF000000) {
                            destPixels[y * width + x] = color;
                        }
                    }
                } else {
                    sx = fastMod(sx, srcWidth);
                    sy = fastMod(sy, srcHeight);

                    PixelType color = src[sy * srcStride + sx];

                    if (color & 0xFF000000) {
                        destPixels[y * width + x] = color;
                    }
                }

                spriteCoord += d;
            }

            spriteCoordScanLine += dm;
        }
    }

    template class Canvas<uint32_t>;
} // namespace gbaemu::lcd