#ifndef MAT_HPP
#define MAT_HPP

#include "vec.hpp"
#include <functional>

namespace common::math
{
    template <size_t M, size_t N>
    struct mat {
        real_t m[M][N];

        mat()
        {
        }

        mat(std::initializer_list<std::initializer_list<real_t>> list)
        {
            size_t i = 0;

            for (auto it = list.begin(); it != list.end(); it++)
                std::copy(it->begin(), it->end(), m[i++]);
        }

        template <size_t O, size_t P>
        mat(const mat<O, P> &other)
        {
            for (size_t y = 0; y < M; ++y)
                for (size_t x = 0; x < N; ++x)
                    m[y][x] = 0;

            for (size_t y = 0; y < std::min(M, O); ++y)
                for (size_t x = 0; x < std::min(N, P); ++x)
                    m[y][x] = other[y][x];
        }

        real_t *operator[](size_t i)
        {
            return m[i];
        }

        const real_t *operator[](size_t i) const
        {
            return m[i];
        }

        mat<N, M> transpose() const
        {
            mat<N, M> result;

            for (size_t i = 0; i < N; ++i)
                for (size_t j = 0; j < M; ++j)
                    result[i][j] = (*this)[j][i];

            return result;
        }

        mat<M, N> apply(const std::function<real_t(real_t, size_t, size_t)> f) const
        {
            mat<M, N> result;

            for (size_t y = 0; y < M; ++y)
                for (size_t x = 0; x < N; ++x)
                    result[y][x] = f((*this)[y][x], y, x);

            return result;
        }

        mat<M, N> add(const mat<M, N> &other) const
        {
            mat<M, N> result;

            for (size_t i = 0; i < M; ++i)
                for (size_t j = 0; j < N; ++j)
                    result[i][j] = (*this)[i][j] + other[i][j];

            return result;
        }

        mat<M, N> sub(const mat<M, N> &other) const
        {
            mat<M, N> result;

            for (size_t i = 0; i < M; ++i)
                for (size_t j = 0; j < N; ++j)
                    result[i][j] = (*this)[i][j] + other[i][j];

            return result;
        }

        mat<M, N> scale(real_t scalar) const
        {
            mat<M, N> result;

            for (size_t i = 0; i < M; ++i)
                for (size_t j = 0; j < N; ++j)
                    result[i][j] = (*this)[i][j] * scalar;

            return result;
        }

        template <size_t O>
        mat<M, N> mul(const mat<N, O> &other) const
        {
            mat<M, O> result;

            for (size_t i = 0; i < M; ++i)
                for (size_t j = 0; j < O; ++j) {
                    real_t dot = 0;

                    for (size_t k = 0; k < N; ++k)
                        dot += (*this)[i][k] * other[k][j];

                    result[i][j] = dot;
                }

            return result;
        }

        vec<M> mul(const vec<N> &other) const
        {
            vec<M> result;

            for (size_t i = 0; i < M; ++i) {
                real_t dot = 0;

                for (size_t j = 0; j < N; ++j)
                    dot += (*this)[i][j] * other[j];

                result[i] = dot;
            }

            return result;
        }

        mat<M, N> operator+(const mat<M, N> &other) const
        {
            return add(other);
        }

        mat<M, N> operator-(const mat<M, N> &other) const
        {
            return sub(other);
        }

        template <size_t O>
        mat<M, N> operator*(const mat<N, O> &other) const
        {
            return mul(other);
        }

        mat<M, N> operator*(real_t scalar) const
        {
            return scale(scalar);
        }

        vec<M> operator*(const vec<N> &other) const
        {
            return mul(other);
        }

        friend std::ostream &operator<<(std::ostream &os, const mat<M, N> &m)
        {
            os << '[';

            for (size_t i = 0; i < M; ++i) {
                os << '<';

                for (size_t j = 0; j < N; ++j) {
                    os << m[i][j];

                    if (j < N - 1)
                        os << ", ";
                }

                os << '>';

                if (i < M - 1)
                    os << ", ";
            }

            return os << ']';
        }

        static mat<M, N> zero()
        {
            mat<M, N> result;

            for (size_t i = 0; i < M; ++i)
                for (size_t j = 0; j < N; ++j)
                    result[i][j] = 0;

            return result;
        }

        static mat<M, N> id()
        {
            static_assert(M == N);

            mat<M, N> result = zero();

            for (size_t i = 0; i < M; ++i)
                result[i][i] = 1;

            return result;
        }
    };
} // namespace common::math

#endif // MAT_HPP