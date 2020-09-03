#ifndef IO_REGS_HPP
#define IO_REGS_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>

namespace gbaemu
{
    class IO_Mapped
    {
      public:
        const uint32_t lowerBound;
        const uint32_t upperBound;
        const std::function<uint8_t(uint32_t)> externalRead8;
        const std::function<void(uint32_t, uint8_t)> externalWrite8;

        const std::function<uint8_t(uint32_t)> internalRead8;
        const std::function<void(uint32_t, uint8_t)> internalWrite8;

        IO_Mapped(uint32_t lowerBound, uint32_t upperBound, std::function<uint8_t(uint32_t)> externalRead8,
                  std::function<void(uint32_t, uint8_t)> externalWrite8, std::function<uint8_t(uint32_t)> internalRead8,
                  std::function<void(uint32_t, uint8_t)> internalWrite8) : lowerBound(lowerBound), upperBound(upperBound), externalRead8(externalRead8), externalWrite8(externalWrite8), internalRead8(internalRead8), internalWrite8(internalWrite8) {}
    };

    bool operator<(const IO_Mapped &a, const IO_Mapped &b);
    bool operator<(const IO_Mapped &a, const uint32_t b);
    bool operator<(const uint32_t a, const IO_Mapped &b);

    class IO_Handler
    {
      private:
        std::set<IO_Mapped, std::less<>> mappedDevices;

      public:
        void registerIOMappedDevice(IO_Mapped &&device)
        {
            mappedDevices.insert(device);
        }

        uint8_t externalRead8(uint32_t addr) const
        {
            const auto devIt = mappedDevices.find(addr);
            if (devIt != mappedDevices.end()) {
                return devIt->externalRead8(addr - devIt->lowerBound);
            } else {
                //TODO how to handle not found?
                return 0x0000;
            }
        }
        uint16_t externalRead16(uint32_t addr) const
        {
            return externalRead8(addr) | (static_cast<uint16_t>(externalRead8(addr + 1)) << 8);
        }
        uint32_t externalRead32(uint32_t addr) const
        {
            return externalRead8(addr) | (static_cast<uint32_t>(externalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(externalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(externalRead8(addr + 3)) << 24);
        }

        void externalWrite8(uint32_t addr, uint8_t value)
        {
            auto devIt = mappedDevices.find(addr);
            if (devIt != mappedDevices.end()) {
                devIt->externalWrite8(addr - devIt->lowerBound, value);
            } else {
                //TODO how to handle not found? probably just ignore...
            }
        }
        void externalWrite16(uint32_t addr, uint16_t value)
        {
            externalWrite8(addr, value & 0xFF);
            externalWrite8(addr + 1, (value >> 8) & 0xFF);
        }
        void externalWrite32(uint32_t addr, uint32_t value)
        {
            externalWrite8(addr, value & 0xFF);
            externalWrite8(addr + 1, (value >> 8) & 0xFF);
            externalWrite8(addr + 2, (value >> 16) & 0xFF);
            externalWrite8(addr + 3, (value >> 24) & 0xFF);
        }

        uint8_t internalRead8(uint32_t addr) const
        {
            const auto devIt = mappedDevices.find(addr);
            if (devIt != mappedDevices.end()) {
                return devIt->internalRead8(addr - devIt->lowerBound);
            } else {
                //TODO how to handle not found?
                return 0x0000;
            }
        }
        uint16_t internalRead16(uint32_t addr) const
        {
            return internalRead8(addr) | (static_cast<uint16_t>(internalRead8(addr + 1)) << 8);
        }
        uint32_t internalRead32(uint32_t addr) const
        {
            return internalRead8(addr) | (static_cast<uint32_t>(internalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(internalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(internalRead8(addr + 3)) << 24);
        }

        void internalWrite8(uint32_t addr, uint8_t value)
        {
            auto devIt = mappedDevices.find(addr);
            if (devIt != mappedDevices.end()) {
                devIt->internalWrite8(addr - devIt->lowerBound, value);
            } else {
                //TODO how to handle not found? probably just ignore...
            }
        }
        void internalWrite16(uint32_t addr, uint16_t value)
        {
            internalWrite8(addr, value & 0xFF);
            internalWrite8(addr + 1, (value >> 8) & 0xFF);
        }
        void internalWrite32(uint32_t addr, uint32_t value)
        {
            internalWrite8(addr, value & 0xFF);
            internalWrite8(addr + 1, (value >> 8) & 0xFF);
            internalWrite8(addr + 2, (value >> 16) & 0xFF);
            internalWrite8(addr + 3, (value >> 24) & 0xFF);
        }

      private:
    };
} // namespace gbaemu

#endif