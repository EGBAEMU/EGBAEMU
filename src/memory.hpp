#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <cstddef>
#include <algorithm>


namespace gbaemu {
    class Memory {
      public:
        void *buf;
        size_t bufSize;
      public:
        Memory(): buf(nullptr), bufSize(0) { }
        
        Memory (size_t size): buf(new char[size]), bufSize(size) { }
        
        Memory(const char *src, size_t size): buf(new char[size]), bufSize(size) {
            std::copy_n(src, size, reinterpret_cast<char *>(buf));
        }

        ~Memory() {
            delete[] buf;
        }

        template <class T>
        T read(size_t addr) const;

        template <class T>
        void write(size_t addr, T value);
    };
}

#endif /* MEMORY_HPP */
