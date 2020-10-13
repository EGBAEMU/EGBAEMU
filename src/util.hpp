#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <type_traits>

namespace gbaemu
{
    typedef uint32_t address_t;
    static const constexpr address_t INVALID_ADDRESS = 0xFFFFFFFF;

    /* converts from or to little endian regardless of the platform */

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    template <class T>
    T le(T val);
#else
    template <class T>
    constexpr T le(T val)
    {
        return val;
    }
#endif

    template <class T>
    T flipBytes(const T &obj);

    template <class T, T mask, T off>
    T bitSet(T val, T insVal)
    {
        insVal = insVal << off;
        // See: http://graphics.stanford.edu/~seander/bithacks.html#MaskedMerge
        const constexpr T mask_ = mask << off;
        // return (val & ~(mask << off)) | ((insVal & mask) << off);
        return val ^ ((val ^ insVal) & mask_);
    }

    template <class T, T mask, T off, T insVal>
    T bitSet(T val)
    {
        // See: http://graphics.stanford.edu/~seander/bithacks.html#MaskedMerge
        const constexpr T mask_ = ~(mask << off);
        const constexpr T insVal_ = (insVal & mask) << off;
        // return (val & ~(mask << off)) | ((insVal & mask) << off);
        return (val & mask_) | insVal_;
    }

    template <class T, T off>
    bool isBitSet(T val)
    {
        const constexpr T mask = static_cast<T>(1) << off;
        return val & mask;
    }

    template <class T>
    T bitGet(T val, T mask, T off);

    /* maps a true/false to 1/0 for T */
    template <class T>
    T bmap(bool b);

    template <class T, bool b>
    constexpr T bmap()
    {
        const constexpr T mapped = static_cast<T>(b ? 1 : 0);
        return mapped;
    }

    template <class T, T FRAC, T INT, class ResultType = double>
    ResultType fixedToFloat(T fp);

    template <class T, T Frac, T Int, class InType = double>
    T floatToFixed(InType f);

    template <class T>
    T clamp(const T &value, const T &mn, const T &mx);

    template <class SignT, class T, uint8_t usedBits>
    SignT signExt(T val)
    {
        static_assert(std::is_signed<SignT>::value);

        // Let the compiler handle the sign extension as it should know whats best for doing so!
        struct {
            SignT x : usedBits;
        } s;
        return s.x = val;
    }

    template <class T>
    uint8_t countBitsSet(T v)
    {
        // does work up to 128 bits
        static_assert(sizeof(T) <= 128 / 8);
        // Taken from: http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        constexpr T alt1BitMask = static_cast<T>((~static_cast<T>(0)) / 3);          // alternaing bit mask i.e. 0b0101010101010101... (every second bit clear)
        constexpr T alt2BitMask = static_cast<T>((~static_cast<T>(0)) / 0xF) * 3;    // alternaing bit mask i.e. 0b0011001100110011... (first 2 bits set per nibble)
        constexpr T alt4BitMask = static_cast<T>((~static_cast<T>(0)) / 0xFF) * 0xF; // alternaing bit mask i.e. 0b0000111100001111... (first 4 bits set per byte)
        constexpr T finalBitMask = static_cast<T>((~static_cast<T>(0)) / 0xFF);      // alternaing bit mask i.e. 0b0000000100000001... (only first bit set per byte)
        v = v - ((v >> 1) & alt1BitMask);
        v = (v & alt2BitMask) + ((v >> 2) & alt2BitMask);
        v = (v + (v >> 4)) & alt4BitMask;
        return static_cast<T>(v * finalBitMask) >> ((sizeof(T) - 1) * 8);
    }

#ifdef __GNUC__
#define popcnt(x) __builtin_popcount(x)
#elif _MSC_VER
#include <intrin.h>
#define popcnt(x) __popcnt(x)
#else
#define popcnt(x) countBitsSet<uint32_t>(x)
#endif

#ifdef __GNUC__
#define ctz(x) __builtin_ctz(x)
#elif _MSC_VER
    uint8_t ctz(uint32_t value)
    {
        uint8_t trailing_zero = 0;
        _BitScanForward(&trailing_zero, value);
        return trailing_zero;
    }
#else
    uint8_t ctz(uint32_t x)
    {
        return popcnt((x & -x) - 1);
    }
#endif

    template <class IntType>
    IntType fastMod(IntType value, IntType upperBound);

    template <class IntType>
    IntType ultraFastMod(IntType value, IntType mod);

#define STRINGIFY(x) (#x)

#define STRINGIFY_CASE_ID(x) \
    case x:                  \
        return (#x)

#define BREAK(cond)    \
    if (cond) {        \
        asm("int $3"); \
    }
} // namespace gbaemu

#endif /* UTIL_HPP */
