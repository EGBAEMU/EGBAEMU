#include "sram.hpp"
#include "io/memory_defs.hpp"

namespace gbaemu::save
{
    SRAM::SRAM(const char *path, bool &success, uint32_t fallBackSize) : saveFile(path, success, fallBackSize)
    {
    }

    static inline uint32_t handleMirroring(uint32_t addr)
    {
        // Handle 32K SRAM chips mirroring: (subtract 32K if >= 32K (last 32K block))
        return (addr - (addr >= (memory::EXT_SRAM_OFFSET | (static_cast<uint32_t>(32) << 10)) ? (static_cast<uint32_t>(32) << 10) : 0)) & 0x00007FFF;
    }

    uint8_t SRAM::read8(uint32_t addr)
    {
        addr = handleMirroring(addr);
        uint8_t result;
        saveFile.read(addr, reinterpret_cast<char *>(&result), 1);
        return result;
    }
    void SRAM::write8(uint32_t addr, uint8_t value)
    {
        addr = handleMirroring(addr);
        saveFile.write(addr, reinterpret_cast<char *>(&value), 1);
    }
} // namespace gbaemu::save
