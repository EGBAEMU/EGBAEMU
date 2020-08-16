#include "memory.hpp"

namespace gbaemu {
    uint8_t Memory::read8(size_t addr) const {
        return buf[addr];
    }
    
    uint16_t Memory::read16(size_t addr) const {
        auto src = buf + addr;

        return (static_cast<uint16_t>(src[0]) << 8) |
                static_cast<uint16_t>(src[1]);
    }
    
    uint32_t Memory::read32(size_t addr) const {
        auto src = buf + addr;

        return (static_cast<uint32_t>(src[0]) << 24) |
               (static_cast<uint32_t>(src[1]) << 16) |
               (static_cast<uint32_t>(src[2]) <<  8) |
                static_cast<uint32_t>(src[3]);
    }

    void Memory::write8(size_t addr, uint8_t value) {
        auto dst = buf + addr;

        dst[0] = value;
    }
    
    void Memory::write16(size_t addr, uint16_t value) {
        auto dst = buf + addr;

        dst[0] = value & 0x00FF;
        dst[1] = value & 0xFF00;
    }
    
    void Memory::write32(size_t addr, uint32_t value) {
        auto dst = buf + addr;

        dst[0] = value & 0x000000FF;
        dst[1] = value & 0x0000FF00;
        dst[2] = value & 0x00FF0000;
        dst[3] = value & 0xFF000000;
    }

}