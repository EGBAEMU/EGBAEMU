#include "memory.hpp"

namespace gbaemu
{
    uint8_t Memory::read8(uint32_t addr) const
    {
        auto src = resolveAddr(addr);
        return src[0];
    }

    uint16_t Memory::read16(uint32_t addr) const
    {
        auto src = resolveAddr(addr);

        return (static_cast<uint16_t>(src[0]) << 0) |
               (static_cast<uint16_t>(src[1]) << 8);
    }

    uint32_t Memory::read32(uint32_t addr) const
    {
        auto src = resolveAddr(addr);

        return (static_cast<uint32_t>(src[0]) << 0) |
               (static_cast<uint32_t>(src[1]) << 8) |
               (static_cast<uint32_t>(src[2]) << 16) |
               (static_cast<uint32_t>(src[3]) << 24);
    }

    void Memory::write8(uint32_t addr, uint8_t value)
    {
        auto dst = resolveAddr(addr);

        dst[0] = value;
    }

    void Memory::write16(uint32_t addr, uint16_t value)
    {
        auto dst = resolveAddr(addr);

        dst[0] = value & 0x00FF;
        dst[1] = value & 0xFF00;
    }

    void Memory::write32(uint32_t addr, uint32_t value)
    {
        auto dst = resolveAddr(addr);

        dst[0] = value & 0x000000FF;
        dst[1] = value & 0x0000FF00;
        dst[2] = value & 0x00FF0000;
        dst[3] = value & 0xFF000000;
    }

    /*
    General Internal Memory
      00000000-00003FFF   BIOS - System ROM         (16 KBytes)
      00004000-01FFFFFF   Not used
      02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes) 2 Wait
      02040000-02FFFFFF   Not used
      03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
      03008000-03FFFFFF   Not used
      04000000-040003FE   I/O Registers
      04000400-04FFFFFF   Not used
    Internal Display Memory
      05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
      05000400-05FFFFFF   Not used
      06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
      06018000-06FFFFFF   Not used
      07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
      07000400-07FFFFFF   Not used
    External Memory (Game Pak)
      08000000-09FFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0
      0A000000-0BFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1
      0C000000-0DFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2
      0E000000-0E00FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width
      0E010000-0FFFFFFF   Not used
    Unused Memory Area
      10000000-FFFFFFFF   Not used (upper 4bits of address bus unused)

    We need to ignore certain bits because of memory mirroring, see:
    https://mgba.io/2014/12/28/classic-nes/
    */

#define PATCH_MEM_REG(addr, x, storage) \
    case x:                             \
        return (storage + ((addr & x##_LIMIT) - x##_OFFSET))
#define PATCH_MEM_REG_(addr, lim, off, storage) \
    case lim:                                   \
        return (storage + ((addr & lim##_LIMIT) - off##_OFFSET))

    const uint8_t *Memory::resolveAddr(uint32_t addr) const
    {
        MemoryRegion memoryRegion = static_cast<MemoryRegion>(addr >> 24);

        switch (memoryRegion) {
            PATCH_MEM_REG(addr, BIOS, bios);
            PATCH_MEM_REG(addr, WRAM, wram);
            PATCH_MEM_REG(addr, IWRAM, iwram);
            PATCH_MEM_REG(addr, IO_REGS, io_regs);
            PATCH_MEM_REG(addr, BG_OBJ_RAM, bg_obj_ram);
            PATCH_MEM_REG(addr, VRAM, vram);
            PATCH_MEM_REG(addr, OAM, oam);
            PATCH_MEM_REG(addr, EXT_SRAM, ext_sram);
            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2:
            case EXT_ROM3_:
                PATCH_MEM_REG_(addr, EXT_ROM3, EXT_ROM, rom);
        }

        //TODO abort?
        // invalid address!
        return nullptr;
    }

    uint8_t *Memory::resolveAddr(uint32_t addr)
    {
        MemoryRegion memoryRegion = static_cast<MemoryRegion>(addr >> 24);

        switch (memoryRegion) {
            PATCH_MEM_REG(addr, BIOS, bios);
            PATCH_MEM_REG(addr, WRAM, wram);
            PATCH_MEM_REG(addr, IWRAM, iwram);
            PATCH_MEM_REG(addr, IO_REGS, io_regs);
            PATCH_MEM_REG(addr, BG_OBJ_RAM, bg_obj_ram);
            PATCH_MEM_REG(addr, VRAM, vram);
            PATCH_MEM_REG(addr, OAM, oam);
            PATCH_MEM_REG(addr, EXT_SRAM, ext_sram);
            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2:
            case EXT_ROM3_:
                PATCH_MEM_REG_(addr, EXT_ROM3, EXT_ROM, rom);
        }

        //TODO abort?
        // invalid address!
        return nullptr;
    }

} // namespace gbaemu