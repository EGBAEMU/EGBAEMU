#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>


namespace gbaemu
{
    uint16_t flip16(uint16_t bytes);

    uint16_t fflip16(uint16_t bytes);

    /* returns val in little endian regardless of the platform */
    template <class T>
    T le(T val);

    template <class T>
    T flipBytes(const T &obj);

    template <class T>
    T bitSet(T val, T mask, T off, T insVal);

    /* maps a true/false to 1/0 for T */
    template <class T>
    T bmap(bool b);

#define STRINGIFY(x) (#x)

#define STRINGIFY_CASE_ID(x) \
    case x:                  \
        return (#x)

    #define BREAK(cond) if (cond) { asm("int $3"); }
} // namespace gbaemu



#endif /* UTIL_HPP */
