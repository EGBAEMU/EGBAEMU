#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>

namespace gbaemu
{
    typedef uint32_t address_t;
    static const constexpr address_t INVALID_ADDRESS = 0xFFFFFFFF;

    /* converts from or to little endian regardless of the platform */
    template <class T>
    T le(T val);

    template <class T>
    T flipBytes(const T &obj);

    template <class T>
    T bitSet(T val, T mask, T off, T insVal);

    template <class T>
    T bitGet(T val, T mask, T off);

    /* maps a true/false to 1/0 for T */
    template <class T>
    T bmap(bool b);

    template <class T, T FRAC, T INT, class ResultType = double>
    ResultType fixedToFloat(T fp);

    template <class T, T Frac, T Int, class InType = double>
    T floatToFixed(InType f);

    template <class T>
    T clamp(const T &value, const T &mn, const T &mx);

    template <class SignT, class T, uint8_t usedBits>
    SignT signExt(T val)
    {
        // Let the compiler handle the sign extension as it should now whats best for doing so!
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
