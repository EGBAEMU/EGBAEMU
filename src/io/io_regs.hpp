#ifndef IO_REGS_HPP
#define IO_REGS_HPP

#include <cstdint>
#include <set>
#include <functional>

namespace gbaemu
{
    class IO_Handler
    {
        uint8_t externalRead8(uint32_t addr) const;
        uint16_t externalRead16(uint32_t addr) const;
        uint32_t externalRead32(uint32_t addr) const;

        void externalWrite8(uint32_t addr, uint8_t value);
        void externalWrite16(uint32_t addr, uint16_t value);
        void externalWrite32(uint32_t addr, uint32_t value);

        uint8_t internalRead8(uint32_t addr) const;
        uint16_t internalRead16(uint32_t addr) const;
        uint32_t internalRead32(uint32_t addr) const;

        void internalWrite8(uint32_t addr, uint8_t value);
        void internalWrite16(uint32_t addr, uint16_t value);
        void internalWrite32(uint32_t addr, uint32_t value);
    };
} // namespace gbaemu

#endif