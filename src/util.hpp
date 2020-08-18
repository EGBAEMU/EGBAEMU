#ifndef UTIL_HPP
#define UTIL_HPP

namespace gbaemu
{
    template <class T>
    T flipBytes(const T &obj);

#define STRINGIFY(x) (#x)

#define STRINGIFY_CASE_ID(x) \
    case x:             \
        return (#x)
} // namespace gbaemu

#endif /* UTIL_HPP */
