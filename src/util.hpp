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
