#ifndef vec_HPP
#define vec_HPP

#include <iostream>
#include <cmath>

namespace common::math
{
    typedef float real_t;

    template <size_t N, class T = real_t>
    struct vect
    {
        T m[N];

        vect()
        {
            
        }

        vect(std::initializer_list<T> list)
        {
            std::copy(list.begin(), list.end(), m);
        }

        template <size_t M>
        vect(const vect<M, T>& other)
        {
            for (size_t i = 0; i < std::min(N, M); ++i)
                (*this)[i] = other[i];
        }

        inline T& operator [](size_t i)
        {
            return m[i];
        }

        inline const T& operator [](size_t i) const
        {
            return m[i];
        }

        inline vect<N, T> add(const vect<N, T>& other) const
        {
            vect<N, T> result;

            for (size_t i = 0; i < N; ++i)
                result[i] = (*this)[i] + other[i];

            return result;
        }

        inline vect<N, T> sub(const vect<N, T>& other) const
        {
            vect<N, T> result;

            for (size_t i = 0; i < N; ++i)
                result[i] = (*this)[i] - other[i];

            return result;
        }

        inline vect<N, T> scale(T scalar) const
        {
            vect<N, T> result;

            for (size_t i = 0; i < N; ++i)
                result[i] = (*this)[i] * scalar;

            return result;
        }

        inline vect<N, T> inv() const
        {
            vect<N, T> result;

            for (size_t i = 0; i < N; ++i)
                result[i] = 1 / m[i];

            return result;
        }

        T dot(const vect<N, T>& other) const
        {
            T sum = 0;

            for (size_t i = 0; i < N; ++i)
                sum += (*this)[i] * other[i];

            return sum;
        }

        T len2() const
        {
            return dot(*this);
        }

        T len() const
        {
            return std::sqrt(len2());
        }

        vect<N, T> normal() const
        {
            return *this * (1 / len());
        }

        vect<3, T> cross(const vect<3, T>& other) const
        {
            return vect<3, T>{
                (*this)[1] * other[2] - (*this)[2] * other[1],
                (*this)[2] * other[0] - (*this)[0] * other[2],
                (*this)[0] * other[1] - (*this)[1] * other[0]  
            };
        }

        inline vect<N, T> operator +(const vect<N, T>& other) const
        {
            return add(other);
        }

        inline vect<N, T> operator -(const vect<N, T>& other) const
        {
            return sub(other);
        }

        inline vect<N, T> operator *(T scalar) const
        {
            return scale(scalar);
        }

        inline vect<N, T> operator /(T scalar) const
        {
            return scale(1 / scalar);
        }

        inline void operator +=(const vect<N, T>& other)
        {
            for (size_t i = 0; i < N; ++i)
                m[i] += other[i];
        }

        inline void operator -=(const vect<N, T>& other)
        {
            for (size_t i = 0; i < N; ++i)
                m[i] -= other[i];
        }

        inline void operator *=(T scalar)
        {
            for (size_t i = 0; i < N; ++i)
                m[i] *= scalar;
        }

        vect<N, T> persp_div() const
        {
            static_assert(N == 4);
            return scale(1 / (*this)[3]);
        }

        template <class U>
        vect<N, T> dot_mul(const vect<N, U>& other) const
        {
            vect<N, T> result;

            for (size_t i = 0; i < N; ++i)
                result[i] = (*this)[i] * other[i];

            return result;
        }

        friend std::ostream& operator <<(std::ostream& os, const vect<N, T>& vec)
        {
            os << '<';

            for (size_t i = 0; i < N; ++i) {
                os << vec[i];

                if (i < N - 1)
                    os << ", ";
            }

            return os << '>';
        }
    };

    template <size_t N>
    using vec = vect<N, real_t>;
}

#endif // vec_HPP