#ifndef MATH3D_HPP
#define MATH3D_HPP

#include "mat.hpp"

namespace common::math
{
    inline real_t near_from_angle(real_t alpha)
    {
        return 1 / std::atan(alpha);
    }

    inline real_t fovv_corrected(real_t screen_width, real_t screen_height, real_t near)
    {
        return 2 * std::atan(screen_height / (screen_width * near));
    }

    inline mat<4, 4> projection_matrix(real_t alpha, real_t beta, real_t near, real_t far)
    {
        mat<4, 4> result = mat<4, 4>::zero();

        //alpha = 2 * std::atan(1 / near);
        //beta = 2 * std::atan(1 / near);

        result[0][0] = 1 / std::tan(alpha / 2);
        result[1][1] = 1 / std::tan(beta / 2);
        //result[2][2] = (1 + near / far) / (1 - near / far);
        //result[2][3] = -near * 2 / (1 - near / far);

        /* hope this works */
        result[2][2] = (far + near) / (far - near);
        result[2][3] = -2 * near * far / (far - near);
        /*
        result[2][2] = far / (far - near);
        result[2][3] = -near * far / (far - near);
         */

        result[3][2] = 1;

        return result;
    }

    inline mat<3, 4> screen_matrix(real_t width, real_t height, real_t near, real_t far)
    {
        auto result = mat<3, 4>::zero();

        result[0][0] = (width - 1) / 2;
        result[0][3] = (width - 1) / 2;

        result[1][1] = -(height - 1) / 2;
        result[1][3] = (height - 1) / 2;

        const real_t a = (far + near) / (far - near);
        const real_t b = -2 * near * far / (far - near);

        /*
        const real_t a = far / (far - near);
        const real_t b = -near * far / (far - near);
         */

        /*
            (Az + B) / z = A + B/z = A + B*1/z
            1/z = ((A + B*1/z) - A) * 1/B
            1/z = (z' - A) * 1/B
            1/z = z' * 1/B - A/B
         */

        /*
            To have easy access to 1/z in the rasterizer, we undo the
            step we did in the projection matrix. We still leave
            this step in the projection matrix so that we can clip
            -w <= z <= w like all the other coordinates.
         */
        result[2][2] = 1 / b;
        result[2][3] = -a / b;

        return result;
    }

    inline mat<4, 4> rotation_matrix(real_t alpha, const vec<3> &dir)
    {
        auto result = mat<4, 4>::zero();
        auto ndir = dir.normal();

        auto n1 = ndir[0];
        auto n2 = ndir[1];
        auto n3 = ndir[2];
        auto cosa = std::cos(alpha);
        auto sina = std::sin(alpha);

        result[0][0] = n1 * n1 * (1 - cosa) + cosa;
        result[0][1] = n1 * n2 * (1 - cosa) - n3 * sina;
        result[0][2] = n1 * n3 * (1 - cosa) + n2 * sina;

        result[1][0] = n2 * n1 * (1 - cosa) + n3 * sina;
        result[1][1] = n2 * n2 * (1 - cosa) + cosa;
        result[1][2] = n2 * n3 * (1 - cosa) - n1 * sina;

        result[2][0] = n3 * n1 * (1 - cosa) - n2 * sina;
        result[2][1] = n3 * n2 * (1 - cosa) + n1 * sina;
        result[2][2] = n3 * n3 * (1 - cosa) + cosa;

        result[3][3] = 1;

        return result;
    }

    inline mat<4, 4> translation_matrix(real_t x, real_t y, real_t z)
    {
        auto result = mat<4, 4>::id();

        result[0][3] = x;
        result[1][3] = y;
        result[2][3] = z;

        return result;
    }

    inline mat<4, 4> translation_matrix(const vec<3> &v)
    {
        return translation_matrix(v[0], v[1], v[2]);
    }

    inline mat<4, 4> scale_matrix(real_t x, real_t y, real_t z)
    {
        auto result = mat<4, 4>::id();
        result[0][0] = x;
        result[1][1] = x;
        result[2][2] = x;
        return result;
    }

    inline mat<4, 4> scale_matrix(const vec<3> &v)
    {
        return scale_matrix(v[0], v[1], v[2]);
    }

    inline mat<4, 4> view_to_clip_matrix(real_t alpha, real_t beta, real_t near, real_t far)
    {
        auto result = mat<4, 4>::zero();

        result[0][0] = 1 / std::tan(alpha / 2);
        result[1][1] = 1 / std::tan(beta / 2);
        result[2][2] = -1 - 2 * far / (near - far);
        result[2][3] = 2 * near * far / (near - far);
        result[3][2] = 1;

        return result;
    }

    inline mat<4, 4> rotation_around_matrix(real_t angle, const math::vec<3> &axis, const math::vec<3> &center)
    {
        return translation_matrix(center[0], center[1], center[2]) *
               rotation_matrix(angle, axis) *
               translation_matrix(-center[0], -center[1], -center[2]);
    }
} // namespace common::math

#endif // MATH3D_HPP
