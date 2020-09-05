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
        void registerIOMappedDevice(IO_Mapped &&device);

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

      private:
    };
} // namespace gbaemu

#endif