#include "memory.hpp"

namespace gbaemu {
    template <class T>
    T Memory::read(size_t addr) const {
        T val;
        char *dst = reinterpret_cast<char *>(val);
        const char *src = reinterpret_cast<char *>(buf);

        for (size_t i = 0; i < sizeof(T); ++i)
            dst[i] = src[addr + sizeof(T) - 1 - i];

        return val;
    }

    template <class T>
    void Memory::write(size_t addr, T value) {
        T val;
        char *dst = reinterpret_cast<char *>(buf);
        const char *src = reinterpret_cast<char *>(val);

        for (size_t i = 0; i < sizeof(T); ++i)
            dst[addr + i] = src[sizeof(T) - 1 - i];

        return val;
    }
}