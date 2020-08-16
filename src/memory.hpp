#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <iostream>


namespace gbaemu {
    class Memory {
      public:
        char *buf;
        size_t bufSize;
      public:
        Memory(): buf(nullptr), bufSize(0) { }
        
        Memory (size_t size): buf(new char[size]), bufSize(size) {
        }
        
        Memory(const char *src, size_t size): buf(new char[size]), bufSize(size) {
            std::copy_n(src, size, reinterpret_cast<char *>(buf));
        }

        ~Memory() {
            delete[] buf;
        }

        void operator =(const Memory& other) {
            buf = new char[other.bufSize];
            bufSize = other.bufSize;

            std::copy_n(other.buf, bufSize, buf);
        }

        uint8_t read8(size_t addr) const;
        uint16_t read16(size_t addr) const;
        uint32_t read32(size_t addr) const;
        void write8(size_t addr, uint8_t value);
        void write16(size_t addr, uint16_t value);
        void write32(size_t addr, uint32_t value);
    };
}

#endif /* MEMORY_HPP */
