#include "memory.hpp"

#include <iostream>

namespace gbaemu
{
    uint8_t Memory::read8(uint32_t addr, uint32_t *cycles) const
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint8_t));
        }

        auto src = resolveAddr(addr);
        return src[0];
    }

    uint16_t Memory::read16(uint32_t addr, uint32_t *cycles) const
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint16_t));
        }

        auto src = resolveAddr(addr);

        return (static_cast<uint16_t>(src[0]) << 0) |
               (static_cast<uint16_t>(src[1]) << 8);
    }

    uint32_t Memory::read32(uint32_t addr, uint32_t *cycles) const
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint32_t));
        }

        auto src = resolveAddr(addr);

        return (static_cast<uint32_t>(src[0]) << 0) |
               (static_cast<uint32_t>(src[1]) << 8) |
               (static_cast<uint32_t>(src[2]) << 16) |
               (static_cast<uint32_t>(src[3]) << 24);
    }

    void Memory::write8(uint32_t addr, uint8_t value, uint32_t *cycles)
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

        auto dst = resolveAddr(addr);

        dst[0] = value;
    }

    void Memory::write16(uint32_t addr, uint16_t value, uint32_t *cycles)
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

        auto dst = resolveAddr(addr);

        dst[0] = value & 0x00FF;
        dst[1] = value & 0xFF00;
    }

    void Memory::write32(uint32_t addr, uint32_t value, uint32_t *cycles)
    {
        if (cycles != nullptr) {
            *cycles += nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

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
        std::cout << "ERROR: trying to access invalid memory address: " << std::hex << addr << std::endl;

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
        std::cout << "ERROR: trying to access invalid memory address: " << std::hex << addr << std::endl;

        return nullptr;
    }

    uint8_t Memory::nonSeqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const
    {
        MemoryRegion memoryRegion = static_cast<MemoryRegion>(address >> 24);

        switch (memoryRegion) {
            case WRAM: {
                // bytesToRead + 1 for ceiling
                // divided by 2 to get the access count on the 16 bit bus (2 bytes)
                uint8_t accessTimes = (bytesToRead + 1) / 2;
                // times 2 because there are always 2 wait cycles(regardless of N or S read)
                // plus access count - 1 because we need to add the read cycles between the attempts except the always expected first read
                return accessTimes * 2 + accessTimes - 1;
            }
            // No wait states here
            case BIOS:
            case IWRAM:
            case IO_REGS:
            case BG_OBJ_RAM:
                return 0;
            case VRAM:
            case OAM: {
                // 16 bit bus else no additional wait times
                return (bytesToRead + 1) / 2 - 1;
            }

                //TODO those are configurable and different for N and S cycles (WAITCNT register)
                /*
            ROM Waitstates
            The GBA starts the cartridge with 4,2 waitstates (N,S) and prefetch disabled.
            The program may change these settings by writing to WAITCNT, 
            the games F-ZERO and Super Mario Advance use 3,1 waitstates (N,S) each, with prefetch enabled.
            Third-party flashcards are reportedly running unstable with these settings.
            Also, prefetch and shorter waitstates are allowing to read more data and opcodes from ROM is less time,
            the downside is that it increases the power consumption.

            Game Pak ROM (regions 8–13) contains up to 32MiB of memory that resides on the cartridge.
                The cartridge bus is also 16-bits, but N cycles and S cycles have different timing on this bus—they are configurable.
                N cycles can be configured to have either 2, 3, 4, or 8 wait states in these regions.
                S cycles actually depend on which specific address is used.
                Region 8/9 has either 1 or 2 wait states, region 10/11 has either 1 or 4, and region 12/13 has either 1 or 8.
                Regions 10-13 are mostly unused, however, so most of what’s important is region 8/9.
            Game Pak RAM (region 14) contains rewritable memory on the cartridge. 
                The bus is only 8 bits wide, and wait states are configurable to be either 2, 3, 4, or 8 cycles.
                There are no special S cycles on this bus.
            */

            case EXT_ROM1_:
            case EXT_ROM2:
            case EXT_ROM2_:
            case EXT_ROM3:
            case EXT_ROM3_:
            case EXT_ROM1: {
                // Initial waitstate (N,S) = (4,2)
                uint8_t accessTimes = (bytesToRead + 1) / 2;
                return accessTimes * 4 + accessTimes - 1;
            }
            case EXT_SRAM:
                // Only 8 bit bus -> accesTimes = bytesToRead
                return bytesToRead * 2 + bytesToRead - 1;
        }

        return 0;
    }

    uint8_t Memory::seqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const
    {
        MemoryRegion memoryRegion = static_cast<MemoryRegion>(address >> 24);

        switch (memoryRegion) {
            case WRAM: {
                // bytesToRead + 1 for ceiling
                // divided by 2 to get the access count on the 16 bit bus (2 bytes)
                uint8_t accessTimes = (bytesToRead + 1) / 2;
                // times 2 because there are always 2 wait cycles(regardless of N or S read)
                // plus access count - 1 because we need to add the read cycles between the attempts except the always expected first read
                return accessTimes * 2 + accessTimes - 1;
            }
            // No wait states here
            case BIOS:
            case IWRAM:
            case IO_REGS:
            case BG_OBJ_RAM:
                return 0;
            case VRAM:
            case OAM: {
                // 16 bit bus else no additional wait times
                return (bytesToRead + 1) / 2 - 1;
            }

                //TODO those are configurable and different for N and S cycles
                /*
            ROM Waitstates
            The GBA starts the cartridge with 4,2 waitstates (N,S) and prefetch disabled.
            The program may change these settings by writing to WAITCNT, 
            the games F-ZERO and Super Mario Advance use 3,1 waitstates (N,S) each, with prefetch enabled.
            Third-party flashcards are reportedly running unstable with these settings.
            Also, prefetch and shorter waitstates are allowing to read more data and opcodes from ROM is less time,
            the downside is that it increases the power consumption.

            Game Pak ROM (regions 8–13) contains up to 32MiB of memory that resides on the cartridge.
                The cartridge bus is also 16-bits, but N cycles and S cycles have different timing on this bus—they are configurable.
                N cycles can be configured to have either 2, 3, 4, or 8 wait states in these regions.
                S cycles actually depend on which specific address is used.
                Region 8/9 has either 1 or 2 wait states, region 10/11 has either 1 or 4, and region 12/13 has either 1 or 8.
                Regions 10-13 are mostly unused, however, so most of what’s important is region 8/9.
            Game Pak RAM (region 14) contains rewritable memory on the cartridge. 
                The bus is only 8 bits wide, and wait states are configurable to be either 2, 3, 4, or 8 cycles.
                There are no special S cycles on this bus.
            */

            case EXT_ROM1_:
            case EXT_ROM2:
            case EXT_ROM2_:
            case EXT_ROM3:
            case EXT_ROM3_:
            case EXT_ROM1: {
                // Initial waitstate (N,S) = (4,2)
                uint8_t accessTimes = (bytesToRead + 1) / 2;
                return accessTimes * 2 + accessTimes - 1;
            }
            case EXT_SRAM:
                // Only 8 bit bus -> accesTimes = bytesToRead
                return bytesToRead * 2 + bytesToRead - 1;
        }

        return 0;
    }

} // namespace gbaemu