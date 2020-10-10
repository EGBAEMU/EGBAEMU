#ifndef MEMORY_DEFS_HPP
#define MEMORY_DEFS_HPP

#include <cstdint>

namespace gbaemu::memory
{
    enum MemoryRegion : uint8_t {
        BIOS = 0x00,
        WRAM = 0x02,
        IWRAM = 0x03,
        IO_REGS = 0x04,
        BG_OBJ_RAM = 0x05,
        VRAM = 0x06,
        OAM = 0x07,
        EXT_ROM1 = 0x08,
        EXT_ROM1_ = 0x09,
        EXT_ROM2 = 0x0A,
        EXT_ROM2_ = 0x0B,
        EXT_ROM3 = 0x0C,
        EXT_ROM3_ = 0x0D,
        EXT_SRAM = 0x0E,
        EXT_SRAM_ = 0x0F
    };

    enum MemoryRegionOffset : uint32_t {
        BIOS_OFFSET = 0x00000000,
        WRAM_OFFSET = 0x02000000,
        IWRAM_OFFSET = 0x03000000,
        IO_REGS_OFFSET = 0x04000000,
        BG_OBJ_RAM_OFFSET = 0x05000000,
        VRAM_OFFSET = 0x06000000,
        OAM_OFFSET = 0x07000000,
        EXT_ROM_OFFSET = 0x08000000,
        EXT_SRAM_OFFSET = 0x0E000000
    };

    enum MemoryRegionLimit : uint32_t {
        BIOS_LIMIT = 0x00003FFF,
        WRAM_LIMIT = 0x0203FFFF,
        IWRAM_LIMIT = 0x03007FFF,
        IO_REGS_LIMIT = 0x040003FE,
        BG_OBJ_RAM_LIMIT = 0x050003FF,
        VRAM_LIMIT = 0x06017FFF, // unsafe to use as mask!
        VRAM_LIMIT_MASK = 0x0601FFFF,
        OAM_LIMIT = 0x070003FF,
        EXT_ROM1_LIMIT = 0x09FFFFFF,
        EXT_ROM2_LIMIT = 0x0BFFFFFF,
        EXT_ROM3_LIMIT = 0x0DFFFFFF,
        EXT_SRAM_LIMIT = 0x0E00FFFF
    };
} // namespace gbaemu

#endif