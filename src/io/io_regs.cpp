#include "io_regs.hpp"
#include "logging.hpp"

#include <iostream>

namespace gbaemu
{
    bool operator<(const IO_Mapped &a, const IO_Mapped &b)
    {
        return a.upperBound < b.lowerBound;
    }

    bool operator<(const IO_Mapped &a, const uint32_t b)
    {
        return a.upperBound < b;
    }

    bool operator<(const uint32_t a, const IO_Mapped &b)
    {
        return a < b.lowerBound;
    }

    void IO_Handler::registerIOMappedDevice(IO_Mapped &&device)
    {
        mappedDevices.insert(device);
    }

    uint8_t IO_Handler::externalRead8(uint32_t addr) const
    {
        const auto devIt = mappedDevices.find(addr);
        if (devIt != mappedDevices.end()) {
            return devIt->externalRead8(addr - devIt->lowerBound);
        } else {
            //TODO how to handle not found?
            LOG_MEM(std::cout << "WARNING: externalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
            return 0x0000;
        }
    }
    uint16_t IO_Handler::externalRead16(uint32_t addr) const
    {
        return externalRead8(addr) | (static_cast<uint16_t>(externalRead8(addr + 1)) << 8);
    }
    uint32_t IO_Handler::externalRead32(uint32_t addr) const
    {
        return externalRead8(addr) | (static_cast<uint32_t>(externalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(externalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(externalRead8(addr + 3)) << 24);
    }

    void IO_Handler::externalWrite8(uint32_t addr, uint8_t value)
    {
        auto devIt = mappedDevices.find(addr);
        if (devIt != mappedDevices.end()) {
            devIt->externalWrite8(addr - devIt->lowerBound, value);
        } else {
            //TODO how to handle not found? probably just ignore...
            LOG_MEM(std::cout << "WARNING: externalWrite: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        }
    }
    void IO_Handler::externalWrite16(uint32_t addr, uint16_t value)
    {
        externalWrite8(addr, value & 0xFF);
        externalWrite8(addr + 1, (value >> 8) & 0xFF);
    }
    void IO_Handler::externalWrite32(uint32_t addr, uint32_t value)
    {
        externalWrite8(addr, value & 0xFF);
        externalWrite8(addr + 1, (value >> 8) & 0xFF);
        externalWrite8(addr + 2, (value >> 16) & 0xFF);
        externalWrite8(addr + 3, (value >> 24) & 0xFF);
    }

    uint8_t IO_Handler::internalRead8(uint32_t addr) const
    {
        const auto devIt = mappedDevices.find(addr);
        if (devIt != mappedDevices.end()) {
            return devIt->internalRead8(addr - devIt->lowerBound);
        } else {
            //TODO how to handle not found?
            LOG_MEM(std::cout << "WARNING: internalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
            return 0x0000;
        }
    }
    uint16_t IO_Handler::internalRead16(uint32_t addr) const
    {
        return internalRead8(addr) | (static_cast<uint16_t>(internalRead8(addr + 1)) << 8);
    }
    uint32_t IO_Handler::internalRead32(uint32_t addr) const
    {
        return internalRead8(addr) | (static_cast<uint32_t>(internalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(internalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(internalRead8(addr + 3)) << 24);
    }

    void IO_Handler::internalWrite8(uint32_t addr, uint8_t value)
    {
        auto devIt = mappedDevices.find(addr);
        if (devIt != mappedDevices.end()) {
            devIt->internalWrite8(addr - devIt->lowerBound, value);
        } else {
            //TODO how to handle not found? probably just ignore...
            LOG_MEM(std::cout << "WARNING: internalWrite: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        }
    }
    void IO_Handler::internalWrite16(uint32_t addr, uint16_t value)
    {
        internalWrite8(addr, value & 0xFF);
        internalWrite8(addr + 1, (value >> 8) & 0xFF);
    }
    void IO_Handler::internalWrite32(uint32_t addr, uint32_t value)
    {
        internalWrite8(addr, value & 0xFF);
        internalWrite8(addr + 1, (value >> 8) & 0xFF);
        internalWrite8(addr + 2, (value >> 16) & 0xFF);
        internalWrite8(addr + 3, (value >> 24) & 0xFF);
    }
} // namespace gbaemu