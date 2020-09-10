#include "util.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>


namespace gbaemu
{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    template <class T>
    T le(T val) {
        return flipBytes(val);
    }
#else
    template <class T>
    T le(T val) {
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
    T bitSet(T val, T mask, T off, T insVal) {
        return (val & ~(mask << off)) | ((insVal & mask) << off);
    }

    template <class T>
    T bitGet(T val, T mask, T off)
    {
        return (val >> off) & mask;
    }

    template <class T>
    T bmap(bool b) {
        return static_cast<T>(b ? 1 : 0);
    }

    template <class T, T FRAC, T INT, class ResultType>
    ResultType fixedToFloat(T fp) {
        /* [1 bit sign][INT bits integer][FRAC bits fractional] */
        const T SIGN_OFF = FRAC + INT;

        auto sign = (fp >> SIGN_OFF) & 1;

        ResultType value = static_cast<ResultType>(fp & ((static_cast<T>(1) << SIGN_OFF) - 1));
        value = sign ? -value : value;
        return value / static_cast<ResultType>(static_cast<T>(1) << FRAC);
    }

    template <class T, T Frac, T Int, class InType>
    T floatToFixed(InType f)
    {
        T signBit = std::signbit(f) ? (static_cast<T>(1) << (Frac + Int)) : static_cast<T>(0);
        return static_cast<T>(std::abs(f) * (static_cast<T>(1) << Frac)) | signBit;
    }

    template <class T>
    T clamp(const T& value, const T& mn, const T& mx)
    {
        return std::min(mx, std::max(mn, value));
    }
} // namespace gbaemu

template uint16_t gbaemu::bitSet<uint16_t>(uint16_t, uint16_t, uint16_t, uint16_t);

template uint16_t gbaemu::bitGet<uint16_t>(uint16_t, uint16_t, uint16_t);

template uint16_t gbaemu::bmap<uint16_t>(bool);

template uint16_t gbaemu::le<uint16_t>(uint16_t);
template uint32_t gbaemu::le<uint32_t>(uint32_t);

template double gbaemu::fixedToFloat<uint32_t, 8, 19>(uint32_t);
template double gbaemu::fixedToFloat<uint16_t, 8, 7>(uint16_t);

template uint16_t gbaemu::floatToFixed<uint16_t, 8, 7, float>(float);
template uint16_t gbaemu::floatToFixed<uint16_t, 8, 7, double>(double);
template uint32_t gbaemu::floatToFixed<uint32_t, 8, 19, float>(float);
template uint32_t gbaemu::floatToFixed<uint32_t, 8, 19, double>(double);

template float gbaemu::clamp<float>(const float&, const float&, const float&);
template double gbaemu::clamp<double>(const double&, const double&, const double&);
template int32_t gbaemu::clamp<int32_t>(const int32_t&, const int32_t&, const int32_t&);