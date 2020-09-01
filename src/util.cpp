#include "util.hpp"

#include <cstddef>

namespace gbaemu
{
    uint16_t flip16(uint16_t bytes) {
        return bytes;
        //return ((bytes & 0xFF00) >> 8) | ((bytes & 0xFF) << 8);
    }

    uint16_t fflip16(uint16_t bytes) {
        return ((bytes & 0xFF00) >> 8) | ((bytes & 0xFF) << 8);
    }

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
    T bmap(bool b) {
        return static_cast<T>(b ? 1 : 0);
    }
} // namespace gbaemu

template uint16_t gbaemu::bitSet<uint16_t>(uint16_t, uint16_t, uint16_t, uint16_t);
template uint16_t gbaemu::bmap<uint16_t>(bool);
template uint16_t gbaemu::le<uint16_t>(uint16_t);