#include "util.hpp"

#include <cstddef>


namespace gbaemu {
    template <class T>
    T flipBytes(const T& obj) {
        T result;

        for (size_t i = 0; i < sizeof(obj); ++i)
            reinterpret_cast<char *>(&result)[i] = reinterpret_cast<const char *>(&obj)[sizeof(obj) - 1 - i];

        return result;
    }
}