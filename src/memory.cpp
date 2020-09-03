#include "memory.hpp"

#include "lcd/lcd-controller.hpp"
#include "util.hpp"
#include <cstring>
#include <iostream>

namespace gbaemu
{
    uint32_t Memory::readOutOfROM(uint32_t addr) const
    {
        /*
        Reading from GamePak ROM when no Cartridge is inserted
        Because Gamepak uses the same signal-lines for both 16bit data 
        and for lower 16bit halfword address, the entire gamepak 
        ROM area is effectively filled by incrementing 16bit values (Address/2 AND FFFFh).
        */
        addr = addr & ~3;
        return ((addr >> 1) & 0xFFFF) | ((((addr + 2) >> 1) & 0xFFFF) << 16);
    }

    uint8_t Memory::read8(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq) const
    {
        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(uint8_t)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint8_t));
        }

        MemoryRegion memReg;
        const auto src = resolveAddr(addr, execInfo, memReg);

        if (memReg == OUT_OF_ROM) {
            return readOutOfROM(addr);
        } else if (memReg == IO_REGS) {
            return ioHandler.externalRead8(addr);
        } else {
            return src[0];
        }
    }

    uint16_t Memory::read16(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq) const
    {
        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(uint16_t)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint16_t));
        }

        MemoryRegion memReg;

        const auto src = resolveAddr(addr, execInfo, memReg);
        if (memReg == OUT_OF_ROM) {
            return readOutOfROM(addr);
        } else if (memReg == IO_REGS) {
            return ioHandler.externalRead16(addr);
        } else {
            return (static_cast<uint16_t>(src[0]) << 0) |
                   (static_cast<uint16_t>(src[1]) << 8);
        }
    }

    uint32_t Memory::read32(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq) const
    {
        if (addr & 0x03) {
            std::cout << "WARNING: word read on non word aligned address: 0x" << std::hex << addr << '!' << std::endl;
        }

        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(uint32_t)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(uint32_t));
        }

        MemoryRegion memReg;
        const auto src = resolveAddr(addr, execInfo, memReg);

        if (memReg == OUT_OF_ROM) {
            return readOutOfROM(addr);
        } else if (memReg == IO_REGS) {
            return ioHandler.externalRead32(addr);
        } else {
            return (static_cast<uint32_t>(src[0]) << 0) |
                   (static_cast<uint32_t>(src[1]) << 8) |
                   (static_cast<uint32_t>(src[2]) << 16) |
                   (static_cast<uint32_t>(src[3]) << 24);
        }
    }

    void Memory::write8(uint32_t addr, uint8_t value, InstructionExecutionInfo *execInfo, bool seq)
    {
        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(value)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

        MemoryRegion memReg;
        auto dst = resolveAddr(addr, execInfo, memReg);

        if (memReg == OUT_OF_ROM) {
            std::cout << "CRITICAL ERROR: trying to write8 ROM + outside of its bounds!" << std::endl;
            if (execInfo != nullptr) {
                execInfo->hasCausedException = true;
            }
        } else if (memReg == IO_REGS) {
            ioHandler.externalWrite8(addr, value);
        } else {

            //TODO is this legit?
            bool bitMapMode = (vram[0] & lcd::DISPCTL::BG_MODE_MASK) >= 4;

            switch (memReg) {
                case OAM: {
                    // Always ignored, only 16 bit & 32 bit allowed
                    return;
                }

                case VRAM: {
                    // Consists of BG & PBJ
                    // In bitmap mode:
                    // 0x06014000-0x06017FFF ignored
                    // 0x06000000-0x0613FFFF as BG_OBJ_RAM

                    // Not in bitmap mode:
                    // 0x06010000-0x06017FFF ignored
                    // 0x06000000-0x0600FFFF as BG_OBJ_RAM
                    uint32_t normalizedAddr = normalizeAddress(addr, memReg);
                    if (normalizedAddr <= (bitMapMode ? 0x0613FFFF : 0x0600FFFF)) {
                        // Reuse BG_OBJ_RAM code
                    } else {
                        // Else ignored
                        return;
                    }
                }
                // Writes to BG (6000000h-600FFFFh) (or 6000000h-6013FFFh in Bitmap mode)
                // and to Palette (5000000h-50003FFh) are writing
                //the new 8bit value to BOTH upper and lower 8bits of the addressed halfword,
                // ie. "[addr AND NOT 1]=data*101h".
                case BG_OBJ_RAM: { // Palette
                    // Written as halfword, same byte twice
                    write16(addr & ~1, (static_cast<uint16_t>(value) << 8) | value, nullptr);
                    break;
                }

                default:
                    dst[0] = value;
                    break;
            }
        }
    }

    void Memory::write16(uint32_t addr, uint16_t value, InstructionExecutionInfo *execInfo, bool seq)
    {
        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(value)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

        MemoryRegion memReg;
        auto dst = resolveAddr(addr, execInfo, memReg);

        if (memReg == OUT_OF_ROM) {
            std::cout << "CRITICAL ERROR: trying to write16 ROM + outside of its bounds!" << std::endl;
            if (execInfo != nullptr) {
                execInfo->hasCausedException = true;
            }
        } else if (memReg == IO_REGS) {
            ioHandler.externalWrite16(addr, value);
        } else {
            dst[0] = value & 0x0FF;
            dst[1] = (value >> 8) & 0x0FF;
        }
    }

    void Memory::write32(uint32_t addr, uint32_t value, InstructionExecutionInfo *execInfo, bool seq)
    {
        if (addr & 0x03) {
            std::cout << "WARNING: word write on non word aligned address: 0x" << std::hex << addr << '!' << std::endl;
        }

        if (execInfo != nullptr) {
            execInfo->cycleCount += seq ? seqWaitCyclesForVirtualAddr(addr, sizeof(value)) : nonSeqWaitCyclesForVirtualAddr(addr, sizeof(value));
        }

        MemoryRegion memReg;
        auto dst = resolveAddr(addr, execInfo, memReg);
        if (memReg == OUT_OF_ROM) {
            std::cout << "CRITICAL ERROR: trying to write32 ROM + outside of its bounds!" << std::endl;
            if (execInfo != nullptr) {
                execInfo->hasCausedException = true;
            }
        } else if (memReg == IO_REGS) {
            ioHandler.externalWrite32(addr, value);
        } else {
            dst[0] = value & 0x0FF;
            dst[1] = (value >> 8) & 0x0FF;
            dst[2] = (value >> 16) & 0x0FF;
            dst[3] = (value >> 24) & 0x0FF;
        }
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

#define PATCH_MEM_ADDR(addr, x) \
    case x:                     \
        return (addr & x##_LIMIT)

    uint32_t Memory::normalizeAddress(uint32_t addr, MemoryRegion &memReg) const
    {
        memReg = static_cast<MemoryRegion>((addr >> 24) & 0x0F);

        addr = addr & 0x0FFFFFFF;

        //TODO handle memory mirroring!!!
        switch (memReg) {
            //PATCH_MEM_ADDR(addr, BIOS);
            PATCH_MEM_ADDR(addr, WRAM);
            PATCH_MEM_ADDR(addr, IWRAM);
            PATCH_MEM_ADDR(addr, BG_OBJ_RAM);
            PATCH_MEM_ADDR(addr, OAM);
            case VRAM: {
                // Even though VRAM is sized 96K (64K+32K),
                //it is repeated in steps of 128K
                //(64K+32K+32K, the two 32K blocks itself being mirrors of each other)

                // First handle 128K mirroring!
                addr &= VRAM_OFFSET | ((static_cast<uint32_t>(128) << 10) - 1);

                // Now lets handle upper 32K mirroring! (subtract 32K if >= 96K (last 32K block))
                addr -= (addr >= (VRAM_OFFSET | (static_cast<uint32_t>(96) << 10)) ? (static_cast<uint32_t>(32) << 10) : 0);
                break;
            }
            case EXT_SRAM_:
            case EXT_SRAM: {
                // The 64K SRAM area is mirrored across the whole 32MB area at E000000h-FFFFFFFh,
                //also, inside of the 64K SRAM field, 32K SRAM chips are repeated twice

                // First handle 64K mirroring! & ensure we start with EXT_SRAM offset
                addr = (addr & ((static_cast<uint32_t>(64) << 10) - 1)) | EXT_SRAM_OFFSET;

                if (backupType == SRAM_V) {
                    // Handle 32K SRAM chips mirroring: (subtract 32K if >= 32K (last 32K block))
                    addr -= (addr >= (EXT_SRAM_OFFSET | (static_cast<uint32_t>(32) << 10)) ? (static_cast<uint32_t>(32) << 10) : 0);
                }
                break;
            }

            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2:
            case EXT_ROM3:
            case EXT_ROM3_: {
                //TODO proper ROM mirroring... cause this is shady AF
                //TODO we need something similar in the memory class...
                uint32_t romSizeLog2 = getRomSize();
                // Uff: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
                romSizeLog2--;
                romSizeLog2 |= romSizeLog2 >> 1;
                romSizeLog2 |= romSizeLog2 >> 2;
                romSizeLog2 |= romSizeLog2 >> 4;
                romSizeLog2 |= romSizeLog2 >> 8;
                romSizeLog2 |= romSizeLog2 >> 16;
                romSizeLog2++;
                uint32_t romOffset = ((addr - EXT_ROM_OFFSET) & (romSizeLog2 - 1));
                if (romOffset >= getRomSize()) {
                    std::cout << "ERROR: trying to access rom out of bounds! Addr: 0x" << std::hex << addr << std::endl;
                    // Indicate out of ROM!!!
                    memReg = OUT_OF_ROM;
                }
                return romOffset + EXT_ROM_OFFSET;
            }

            // IO is not mirrored
            case IO_REGS:
            default:
                break;
        }

        return addr;
    }

#define COMBINE_MEM_ADDR(addr, x, storage) \
    case x:                             \
        return (storage + ((addr) - x##_OFFSET))
#define COMBINE_MEM_ADDR_(addr, lim, off, storage) \
    case lim:                                      \
        return (storage + ((addr) - off##_OFFSET))

    static const uint8_t noBackupMedia[] = {0xFF, 0xFF, 0xFF, 0xFF};

    const uint8_t *Memory::resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg) const
    {
        static const uint8_t zeroMem[4] = {0};

        addr = normalizeAddress(addr, memReg);

        switch (memReg) {
            //COMBINE_MEM_ADDR(addr, BIOS, bios);
            COMBINE_MEM_ADDR(addr, WRAM, wram);
            COMBINE_MEM_ADDR(addr, IWRAM, iwram);
            COMBINE_MEM_ADDR(addr, BG_OBJ_RAM, bg_obj_ram);
            COMBINE_MEM_ADDR(addr, VRAM, vram);
            COMBINE_MEM_ADDR(addr, OAM, oam);
            case EXT_SRAM:
            case EXT_SRAM_:
                if (backupType != NO_BACKUP) {
                    return (ext_sram + ((addr & EXT_SRAM_LIMIT) - EXT_SRAM_OFFSET));
                } else {
                    return noBackupMedia;
                }
            case OUT_OF_ROM:
            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2:
            case EXT_ROM3_:
                COMBINE_MEM_ADDR_(addr, EXT_ROM3, EXT_ROM, rom);
            case BIOS:
                return BIOS_READ[biosReadState];
            case IO_REGS:
                if (addr >= IO_REGS_LIMIT) {
                    std::cout << "ERROR: read invalid io reg address: 0x" << std::hex << addr << std::endl;
                    return zeroMem;
                } else {
                    return (io_regs + ((addr)-IO_REGS_OFFSET));
                }
        }

        // invalid address!
        std::cout << "ERROR: trying to access invalid memory address: " << std::hex << addr << std::endl;
        if (execInfo != nullptr) {
            execInfo->hasCausedException = true;
        } else {
            std::cout << "CRITICAL ERROR: could indicate that an exception has been caused!" << std::endl;
        }
        return zeroMem;
    }

    uint8_t *Memory::resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg)
    {
        static uint8_t wasteMem[4];

        addr = normalizeAddress(addr, memReg);

        switch (memReg) {
            //COMBINE_MEM_ADDR(addr, BIOS, bios);
            COMBINE_MEM_ADDR(addr, WRAM, wram);
            COMBINE_MEM_ADDR(addr, IWRAM, iwram);
            COMBINE_MEM_ADDR(addr, BG_OBJ_RAM, bg_obj_ram);
            COMBINE_MEM_ADDR(addr, VRAM, vram);
            COMBINE_MEM_ADDR(addr, OAM, oam);
            case EXT_SRAM:
            case EXT_SRAM_:
                if (backupType != NO_BACKUP) {
                    return (ext_sram + ((addr & EXT_SRAM_LIMIT) - EXT_SRAM_OFFSET));
                } else {
                    return wasteMem;
                }
            case OUT_OF_ROM:
            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2:
            case EXT_ROM3_:
                COMBINE_MEM_ADDR_(addr, EXT_ROM3, EXT_ROM, rom);
            case BIOS:
                std::cout << "ERROR: trying to write bios mem: 0x" << std::hex << addr << std::endl;
                // Read only!
                return wasteMem;
            case IO_REGS:
                if (addr >= IO_REGS_LIMIT) {
                    std::cout << "ERROR: write invalid io reg address: 0x" << std::hex << addr << std::endl;
                    return wasteMem;
                } else {
                    return (io_regs + ((addr)-IO_REGS_OFFSET));
                }
        }

        // invalid address!
        std::cout << "ERROR: trying to access invalid memory address: " << std::hex << addr << std::endl;
        if (execInfo != nullptr) {
            execInfo->hasCausedException = true;
        } else {
            std::cout << "CRITICAL ERROR: could indicate that an exception has been caused!" << std::endl;
        }
        return wasteMem;
    }

    uint8_t Memory::nonSeqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const
    {
        MemoryRegion memoryRegion;
        normalizeAddress(address, memoryRegion);

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
            case OUT_OF_ROM:
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
            case EXT_SRAM_:
            case EXT_SRAM:
                // Only 8 bit bus -> accesTimes = bytesToRead
                return bytesToRead * 2 + bytesToRead - 1;
        }

        return 0;
    }

    uint8_t Memory::seqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const
    {
        MemoryRegion memoryRegion;
        normalizeAddress(address, memoryRegion);

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
            case OUT_OF_ROM:
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
            case EXT_SRAM_:
            case EXT_SRAM:
                // Only 8 bit bus -> accesTimes = bytesToRead
                return bytesToRead * 2 + bytesToRead - 1;
        }

        return 0;
    }

    void Memory::scanROMForBackupID()
    {
        // reset backup type
        backupType = NO_BACKUP;

        if (ext_sram)
            delete[] ext_sram;
        ext_sram = nullptr;

        /*
        ID Strings
        The ID string must be located at a word-aligned memory location, the string length should be a multiple of 4 bytes (padded with zero's).
          EEPROM_Vnnn    EEPROM 512 bytes or 8 Kbytes (4Kbit or 64Kbit)
          SRAM_Vnnn      SRAM 32 Kbytes (256Kbit)
          FLASH_Vnnn     FLASH 64 Kbytes (512Kbit) (ID used in older files)
          FLASH512_Vnnn  FLASH 64 Kbytes (512Kbit) (ID used in newer files)
          FLASH1M_Vnnn   FLASH 128 Kbytes (1Mbit)
        For Nintendo's tools, "nnn" is a 3-digit library version number. When using other tools, best keep it set to "nnn" rather than inserting numeric digits.
        */

        static const char *const parsingStrs[] = {
            STRINGIFY(EEPROM_V),
            STRINGIFY(SRAM_V),
            STRINGIFY(FLASH_V),
            STRINGIFY(FLASH512_V),
            STRINGIFY(FLASH1M_V)};

        // Array with pointer to char to compare with next on match increment, else reset!
        const char *currentParsingState[sizeof(parsingStrs) / sizeof(parsingStrs[0])];
        std::memcpy(currentParsingState, parsingStrs, sizeof(parsingStrs));

        for (uint8_t *romIt = rom; romIt + 4 < rom + romSize;) {
            for (uint32_t i = 0; i < 4; ++i) {
                const char token = *romIt;
                ++romIt;

                for (uint32_t k = 0; k < sizeof(parsingStrs) / sizeof(parsingStrs[0]); ++k) {
                    if (token == *currentParsingState[k]) {
                        // token match increment compare string
                        ++(currentParsingState[k]);

                        // Is there a token to compare to left? if not we got a match!
                        if (*currentParsingState[k] == '\0') {
                            std::cout << "INFO: Found backup id: " << parsingStrs[k] << std::endl;
                            backupType = static_cast<BackupID>(k);

                            // Allocate needed memory
                            ext_sram = new uint8_t[backupSizes[k]];
                            std::fill_n(ext_sram, backupSizes[k], 0);
                            return;
                        }
                    } else {
                        // reset
                        currentParsingState[k] = parsingStrs[k];
                    }
                }
            }
        }

        std::cout << "INFO: No backup id was found!" << std::endl;
    }

} // namespace gbaemu