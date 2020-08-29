#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>


namespace gbaemu
{
    uint16_t flip16(uint16_t bytes);

    template <class T>
    T flipBytes(const T &obj);

    template <class T>
    T bitSet(T val, T mask, T off, T insVal);

    template <class T>
    T bmap(bool b);

#define STRINGIFY(x) (#x)

#define STRINGIFY_CASE_ID(x) \
    case x:                  \
        return (#x)
} // namespace gbaemu



#endif /* UTIL_HPP */
