#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>


namespace gbaemu
{
    uint16_t flip16(uint16_t bytes);

    template <class T>
    T flipBytes(const T &obj);

#define STRINGIFY(x) (#x)

#define STRINGIFY_CASE_ID(x) \
    case x:                  \
        return (#x)
} // namespace gbaemu

#endif /* UTIL_HPP */
