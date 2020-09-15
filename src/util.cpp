#include "util.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gbaemu
{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    template <class T>
    T le(T val)
    {
        return flipBytes(val);
    }
#else
    template <class T>
    T le(T val)
    {
        return val;
    }
#endif

    template <class T>
    T flipBytes(const T &obj)
    {
        T result;

        for (size_t i = 0; i < sizeof(obj); ++i)
            reinterpret_cast<char *>(&result)[i] = reinterpret_cast<const char *>(&obj)[sizeof(obj) - 1 - i];

        return result;
    }

    template <class T>
    T bitSet(T val, T mask, T off, T insVal)
    {
        return (val & ~(mask << off)) | ((insVal & mask) << off);
    }

    template <class T>
    T bitGet(T val, T mask, T off)
    {
        return (val >> off) & mask;
    }

    template <class T>
    T bmap(bool b)
    {
        return static_cast<T>(b ? 1 : 0);
    }

    template <class T, T FRAC, T INT, class ResultType>
    ResultType fixedToFloat(T fp)
    {
        static_assert(1 + INT + FRAC <= sizeof(T) * 8, "Sign, integer and fraction take up more bits than T provides.");

        /* [1 bit sign][INT bits integer][FRAC bits fractional] */
        /* This is fixed point 2's complement. */
        const T SIGN_OFF = FRAC + INT;
        const ResultType factor = static_cast<ResultType>(static_cast<T>(1) << FRAC);

        T bitValue = fp & ((static_cast<T>(1) << (SIGN_OFF + 1)) - 1);
        bool negative = (fp >> SIGN_OFF) & 1;
        //T compOffset = negative ? (static_cast<T>(1) << INT) : 0;

        if (negative)
            bitValue = ~bitValue + 1;

        return static_cast<ResultType>(bitValue) / factor;// - static_cast<ResultType>(compOffset);
    }

    template <class T, T Frac, T Int, class InType>
    T floatToFixed(InType f)
    {
        static_assert(1 + Int + Frac <= sizeof(T) * 8, "Sign, integer and fraction take up more bits than T provides.");

        const T mask = (static_cast<T>(1) << (Frac + Int + 1)) - 1;
        const InType factor = static_cast<InType>(static_cast<T>(1) << Frac);

        T value = static_cast<T>(std::abs(f) * factor);

        return (std::signbit(f) ? (~value + 1) : value) & mask;
    }

    template <class T>
    T clamp(const T &value, const T &mn, const T &mx)
    {
        return std::min(mx, std::max(mn, value));
    }

    template <class IntType>
    IntType fastMod(IntType value, IntType upperBound)
    {
        return (0 <= value && value < upperBound) ? value : (value % upperBound);
    }
} // namespace gbaemu

template uint16_t gbaemu::bitSet<uint16_t>(uint16_t, uint16_t, uint16_t, uint16_t);

template uint16_t gbaemu::bitGet<uint16_t>(uint16_t, uint16_t, uint16_t);

template uint16_t gbaemu::bmap<uint16_t>(bool);

template uint16_t gbaemu::le<uint16_t>(uint16_t);
template uint32_t gbaemu::le<uint32_t>(uint32_t);

template double gbaemu::fixedToFloat<uint32_t, 8, 19>(uint32_t);
template float gbaemu::fixedToFloat<uint32_t, 8, 19, float>(uint32_t);
template double gbaemu::fixedToFloat<uint16_t, 8, 7>(uint16_t);
template float gbaemu::fixedToFloat<uint16_t, 8, 7, float>(uint16_t);

template uint16_t gbaemu::floatToFixed<uint16_t, 8, 7, float>(float);
template uint16_t gbaemu::floatToFixed<uint16_t, 8, 7, double>(double);
template uint32_t gbaemu::floatToFixed<uint32_t, 8, 19, float>(float);
template uint32_t gbaemu::floatToFixed<uint32_t, 8, 19, double>(double);

template float gbaemu::clamp<float>(const float &, const float &, const float &);
template double gbaemu::clamp<double>(const double &, const double &, const double &);
template int32_t gbaemu::clamp<int32_t>(const int32_t &, const int32_t &, const int32_t &);

template int32_t gbaemu::fastMod<int32_t>(int32_t, int32_t);
