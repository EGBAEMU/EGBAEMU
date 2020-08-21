#include "util.hpp"

#include <cstddef>

namespace gbaemu
{
    uint16_t flip16(uint16_t bytes) {
        return ((bytes & 0xFF00) >> 8) | ((bytes & 0xFF) << 8);
    }

    template <class T>
    T flipBytes(const T &obj)
    {
        T result;

        for (size_t i = 0; i < sizeof(obj); ++i)
            reinterpret_cast<char *>(&result)[i] = reinterpret_cast<const char *>(&obj)[sizeof(obj) - 1 - i];

        return result;
    }
} // namespace gbaemu