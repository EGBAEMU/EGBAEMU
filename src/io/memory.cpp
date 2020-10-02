#include "memory.hpp"
#include "decode/inst.hpp"
#include "lcd/lcd-controller.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

namespace gbaemu
{

#ifdef DEBUG_CLI
    void MemWatch::registerTrigger(const std::function<void(address_t, Condition, uint32_t, bool, uint32_t)> &trig)
    {
        trigger = trig;
    }

    void MemWatch::watchAddress(address_t addr, const Condition &cond)
    {
        addresses.insert(addr);
        conditions[addr] = cond;
    }

    void MemWatch::unwatchAddress(address_t addr)
    {
        addresses.erase(addr);
        conditions.erase(addr);
    }

    bool MemWatch::isAddressWatched(address_t addr) const noexcept
    {
        return trigger && (addresses.find(addr) != addresses.end());
    }

    void MemWatch::addressCheckTrigger(address_t addr, uint32_t currValue) const
    {
        auto cond = conditions.find(addr)->second;

        if (!cond.onRead)
            return;

        if (!cond.onValueEqual || cond.value == currValue)
            trigger(addr, cond, currValue, false, 0);
    }

    void MemWatch::addressCheckTrigger(address_t addr, uint32_t currValue, uint32_t newValue) const
    {
        auto cond = conditions.find(addr)->second;

        if (!cond.onWrite)
            return;

        if ((!cond.onValueEqual || cond.value == currValue) ||
            (!cond.onValueChanged || currValue != newValue))
            trigger(addr, cond, currValue, true, newValue);
    }

    std::string MemWatch::getWatchPointInfo() const
    {
        std::stringstream ss;

        for (address_t addr : addresses) {
            auto cond = conditions.find(addr)->second;

            ss << std::hex << addr << " ["
               << (cond.onRead ? "r" : "")
               << (cond.onRead ? "w" : "")
               << "] " << '\n';
        }

        return ss.str();
    }
#endif

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

    template <uint8_t bytes>
    uint8_t *Memory::resolveAddrRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const
    {

        if (execInfo.resolvedAddr && execInfo.lowerBound <= addr && execInfo.upperBound >= (addr + bytes)) {
            // We can reuse our previous efforts -> nothing to do here
        } else {
            // Memory region changed or was not yet resolved
            normalizeAddressRef(addr, execInfo);
        }

        uint8_t *dst = execInfo.writeBaseAddr;

        // We need to calculate the target address by considering the offset caused by this address
        if (!execInfo.noOffset) {
            dst += addr - execInfo.lowerBound;
        }

        return dst;
    }

    template <bool read, uint8_t bytes>
    const uint8_t *Memory::resolveAddrRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const
    {
        static_assert(read == true);

        if (execInfo.resolvedAddr && execInfo.lowerBound <= addr && execInfo.upperBound >= (addr + bytes)) {
            // We can reuse our previous efforts -> nothing to do here
        } else {
            // Memory region changed or was not yet resolved
            normalizeAddressRef(addr, execInfo);
        }

        const uint8_t *dst = execInfo.readBaseAddr;

        // We need to calculate the target address by considering the offset caused by this address
        if (!execInfo.noOffset) {
            dst += addr - execInfo.lowerBound;
        }

        return dst;
    }

    uint8_t Memory::read8(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq, bool readInstruction) const
    {
        uint32_t unchangedAddr = addr;
        execInfo.cycleCount += memCycles16(extractMemoryRegion(unchangedAddr), seq);

        const uint8_t *src = resolveAddrRef<true, sizeof(uint8_t)>(addr, execInfo);

        if (readInstruction && execInfo.memReg == BIOS && addr < getBiosSize()) {
            // For instructions we are allowed to read from bios
            src = bios + addr;
        }

        uint8_t currValue;

        if (execInfo.memReg == OUT_OF_ROM) {
            currValue = readOutOfROM(unchangedAddr);
        } else if (execInfo.memReg == IO_REGS) {
            currValue = ioHandler.externalRead8(addr);
        } else if (execInfo.memReg == EEPROM_REGION) {
            currValue = eeprom->read();
        } else if (execInfo.memReg == FLASH_REGION) {
            currValue = flash->read(addr);
        } else if (execInfo.memReg == SRAM_REGION) {
            ext_sram->read(addr & 0x00007FFF, reinterpret_cast<char *>(&currValue), 1);
        } else {
            currValue = src[0];
        }

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr))
            memWatch.addressCheckTrigger(addr, currValue);
#endif

        return currValue;
    }

    /*
    ARM CPU Memory Alignments

    The CPU does NOT support accessing mis-aligned addresses (which would be rather slow because it'd have to merge/split that data into two accesses).
    When reading/writing code/data to/from memory, Words and Halfwords must be located at well-aligned memory address, ie. 32bit words aligned by 4, and 16bit halfwords aligned by 2.

    Mis-aligned STR,STRH,STM,LDM,LDRD,STRD,PUSH,POP (forced align)
    The mis-aligned low bit(s) are ignored, the memory access goes to a forcibly aligned (rounded-down) memory address.
    For LDRD/STRD, it isn't clearly defined if the address must be aligned by 8 (on the NDS, align-4 seems to be okay) (align-8 may be required on other CPUs with 64bit databus).

    Mis-aligned LDR,SWP (rotated read)
    Reads from forcibly aligned address "addr AND (NOT 3)", and does then rotate the data as "ROR (addr AND 3)*8". That effect is internally used by LDRB and LDRH opcodes (which do then mask-out the unused bits).
    The SWP opcode works like a combination of LDR and STR, that means, it does read-rotated, but does write-unrotated.
    */
    uint16_t Memory::read16(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq, bool readInstruction, bool dmaRequest) const
    {
        uint32_t alignedAddr = addr & ~static_cast<uint32_t>(1);
        execInfo.cycleCount += memCycles16(extractMemoryRegion(addr), seq);

        const uint8_t *src = resolveAddrRef<true, sizeof(uint16_t)>(alignedAddr, execInfo);

        if (readInstruction && execInfo.memReg == BIOS && alignedAddr < getBiosSize()) {
            // For instructions we are allowed to read from bios
            src = bios + alignedAddr;
        }

        uint16_t currValue;

        if (execInfo.memReg == OUT_OF_ROM) {
            currValue = readOutOfROM(addr);
        } else if (execInfo.memReg == IO_REGS) {
            currValue = ioHandler.externalRead16(alignedAddr);
        } else if (execInfo.memReg == EEPROM_REGION) {
            if (dmaRequest) {
                currValue = static_cast<uint16_t>(eeprom->read());
            } else {
                currValue = 0x1;
            }
        } else if (execInfo.memReg == FLASH_REGION) {
            currValue = flash->read(addr);
            currValue |= (currValue << 8);
        } else if (execInfo.memReg == SRAM_REGION) {
            uint8_t data;
            ext_sram->read(addr & 0x00007FFF, reinterpret_cast<char *>(&data), 1);
            currValue = data | (data << 8);
        } else {
            currValue = (static_cast<uint16_t>(src[0]) << 0) |
                        (static_cast<uint16_t>(src[1]) << 8);
        }

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr))
            memWatch.addressCheckTrigger(addr, currValue);
#endif

        return currValue;
    }

    uint32_t Memory::read32(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq, bool readInstruction, bool dmaRequest) const
    {
        uint32_t alignedAddr = addr & ~static_cast<uint32_t>(3);
        execInfo.cycleCount += memCycles32(extractMemoryRegion(addr), seq);

        const uint8_t *src = resolveAddrRef<true, sizeof(uint32_t)>(alignedAddr, execInfo);

        if (readInstruction && execInfo.memReg == BIOS && alignedAddr < getBiosSize()) {
            // For instructions we are allowed to read from bios
            src = bios + alignedAddr;
        }

        uint32_t currValue;

        if (execInfo.memReg == OUT_OF_ROM) {
            currValue = readOutOfROM(addr);
        } else if (execInfo.memReg == IO_REGS) {
            currValue = ioHandler.externalRead32(alignedAddr);
        } else if (execInfo.memReg == EEPROM_REGION) {
            if (dmaRequest) {
                currValue = static_cast<uint32_t>(eeprom->read());
            } else {
                currValue = 0x1;
            }
        } else if (execInfo.memReg == FLASH_REGION) {
            currValue = flash->read(addr);
            currValue |= (currValue << 8) | (currValue << 16) | (currValue << 24);
        } else if (execInfo.memReg == SRAM_REGION) {
            uint8_t data;
            ext_sram->read(addr & 0x00007FFF, reinterpret_cast<char *>(&data), 1);
            currValue = data | (data << 8) | (data << 16) | (data << 24);
        } else {
            currValue = (static_cast<uint32_t>(src[0]) << 0) |
                        (static_cast<uint32_t>(src[1]) << 8) |
                        (static_cast<uint32_t>(src[2]) << 16) |
                        (static_cast<uint32_t>(src[3]) << 24);
        }

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr))
            memWatch.addressCheckTrigger(addr, currValue);
#endif

        return currValue;
    }

    void Memory::write8(uint32_t addr, uint8_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        uint8_t cycles = memCycles16(extractMemoryRegion(addr), seq);

        auto dst = resolveAddrRef<sizeof(uint8_t)>(addr, execInfo);

        execInfo.cycleCount += cycles;

        if (execInfo.memReg == OUT_OF_ROM) {
            LOG_MEM(std::cout << "CRITICAL ERROR: trying to write8 ROM + outside of its bounds!" << std::endl;);
            execInfo.hasCausedException = true;
        } else if (execInfo.memReg == IO_REGS) {
            ioHandler.externalWrite8(addr, value);
        } else if (execInfo.memReg == EEPROM_REGION) {
            eeprom->write(value);
        } else if (execInfo.memReg == FLASH_REGION) {
            flash->write(addr, value);
        } else if (execInfo.memReg == SRAM_REGION) {
            ext_sram->write(addr & 0x00007FFF, reinterpret_cast<const char *>(&value), 1);
        } else {

            //TODO is this legit?
            bool bitMapMode = (vram[0] & lcd::DISPCTL::BG_MODE_MASK) >= 4;

            switch (execInfo.memReg) {
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
                    if (addr <= static_cast<uint32_t>(bitMapMode ? 0x0613FFFF : 0x0600FFFF)) {
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
                    // Undo cycle count increment
                    execInfo.cycleCount -= cycles;
                    // Written as halfword, same byte twice
                    write16(addr & ~1, (static_cast<uint16_t>(value) << 8) | value, execInfo);
                    break;
                }

                default:
                    dst[0] = value;
                    break;
            }
        }
    }

    void Memory::write16(uint32_t addr, uint16_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        uint32_t unalignedPart = addr & 1;

        addr = addr & ~static_cast<uint32_t>(1);

        execInfo.cycleCount += memCycles16(extractMemoryRegion(addr), seq);

        auto dst = resolveAddrRef<sizeof(uint16_t)>(addr, execInfo);

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr)) {
            uint32_t currValue = le<uint16_t>(*reinterpret_cast<const uint16_t *>(dst));
            memWatch.addressCheckTrigger(addr, currValue, value);
        }
#endif

        if (execInfo.memReg == OUT_OF_ROM) {
            LOG_MEM(std::cout << "CRITICAL ERROR: trying to write16 ROM + outside of its bounds!" << std::endl;);
            execInfo.hasCausedException = true;
        } else if (execInfo.memReg == IO_REGS) {
            ioHandler.externalWrite16(addr, value);
        } else if (execInfo.memReg == EEPROM_REGION) {
            eeprom->write(value);
        } else if (execInfo.memReg == FLASH_REGION) {
            flash->write(addr | unalignedPart, value >> (unalignedPart << 3));
        } else if (execInfo.memReg == SRAM_REGION) {
            const uint8_t data = value >> (unalignedPart << 3);
            ext_sram->write((addr | unalignedPart) & 0x00007FFF, reinterpret_cast<const char *>(&data), 1);
        } else {
            dst[0] = value & 0x0FF;
            dst[1] = (value >> 8) & 0x0FF;
        }
    }

    void Memory::write32(uint32_t addr, uint32_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        uint32_t unalignedPart = addr & 3;
        addr = addr & ~static_cast<uint32_t>(3);
        execInfo.cycleCount += memCycles32(extractMemoryRegion(addr), seq);

        auto dst = resolveAddrRef<sizeof(uint32_t)>(addr, execInfo);

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr)) {
            uint32_t currValue = le<uint32_t>(*reinterpret_cast<const uint32_t *>(dst));
            memWatch.addressCheckTrigger(addr, currValue, value);
        }
#endif

        if (execInfo.memReg == OUT_OF_ROM) {
            LOG_MEM(std::cout << "CRITICAL ERROR: trying to write32 ROM + outside of its bounds!" << std::endl;);
            execInfo.hasCausedException = true;
        } else if (execInfo.memReg == IO_REGS) {
            ioHandler.externalWrite32(addr, value);
        } else if (execInfo.memReg == EEPROM_REGION) {
            eeprom->write(value);
        } else if (execInfo.memReg == FLASH_REGION) {
            flash->write(addr | unalignedPart, value >> (unalignedPart << 3));
        } else if (execInfo.memReg == SRAM_REGION) {
            const uint8_t data = value >> (unalignedPart << 3);
            ext_sram->write((addr | unalignedPart) & 0x00007FFF, reinterpret_cast<const char *>(&data), 1);
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

#define COMBINE_MEM_ADDR(addr, x, storage, info) \
    case x:                                      \
        addr = (addr & x##_LIMIT);               \
        info.lowerBound = x##_OFFSET;            \
        info.upperBound = x##_LIMIT + 1;         \
        info.readBaseAddr = storage;             \
        info.writeBaseAddr = storage;            \
        break
#define COMBINE_MEM_ADDR_(addr, lim, off, storage, info) \
    case lim:                                            \
        addr = (addr & x##_LIMIT);                       \
        info.lowerBound = off##_OFFSET;                  \
        info.upperBound = lim##_LIMIT + 1;               \
        info.readBaseAddr = storage;                     \
        info.writeBaseAddr = storage;                    \
        break

    static const uint8_t noBackupMedia[] = {0xFF, 0xFF, 0xFF, 0xFF};

    Memory::MemoryRegion Memory::extractMemoryRegion(uint32_t addr) const
    {
        return static_cast<MemoryRegion>((addr >> 24) & 0x0F);
    }

    uint32_t Memory::normalizeAddress(uint32_t addr, InstructionExecutionInfo &execInfo) const
    {
        normalizeAddressRef(addr, execInfo);
        return addr;
    }

    void Memory::normalizeAddressRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const
    {
        static uint8_t wasteMem[4];
        static const uint8_t zeroMem[4] = {0};

        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.resolvedAddr = true;
        execInfo.noOffset = false;

        switch (execInfo.memReg) {
            COMBINE_MEM_ADDR(addr, WRAM, wram, execInfo);
            COMBINE_MEM_ADDR(addr, IWRAM, iwram, execInfo);
            COMBINE_MEM_ADDR(addr, BG_OBJ_RAM, bg_obj_ram, execInfo);
            COMBINE_MEM_ADDR(addr, OAM, oam, execInfo);

            case VRAM: {
                // Even though VRAM is sized 96K (64K+32K),
                // it is repeated in steps of 128K
                // (64K+32K+32K, the two 32K blocks itself being mirrors of each other)

                // First handle 128K mirroring!
                addr &= VRAM_OFFSET | ((static_cast<uint32_t>(128) << 10) - 1);

                // Now lets handle upper 32K mirroring! (subtract 32K if >= 96K (last 32K block))
                addr -= (addr >= (VRAM_OFFSET | (static_cast<uint32_t>(96) << 10)) ? (static_cast<uint32_t>(32) << 10) : 0);

                execInfo.readBaseAddr = vram;
                execInfo.writeBaseAddr = vram;
                execInfo.lowerBound = VRAM_OFFSET;
                execInfo.upperBound = VRAM_LIMIT + 1;

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
                    execInfo.memReg = SRAM_REGION;
                    //TODO if mirroring is used this will be evaluated over and over again... maybe move mirroring to implementation!
                    execInfo.upperBound = (EXT_SRAM_OFFSET | (static_cast<uint32_t>(32) << 10));
                } else if (backupType >= FLASH_V) {
                    execInfo.memReg = FLASH_REGION;
                    execInfo.upperBound = 0x10000000;
                } else {
                    execInfo.upperBound = 0x10000000;
                }
                execInfo.readBaseAddr = noBackupMedia;
                execInfo.writeBaseAddr = wasteMem;
                execInfo.lowerBound = EXT_SRAM_OFFSET;
                execInfo.noOffset = true;
                break;
            }

            // EXT_ROM3_ maps everything from D000000h-DFFFFFFh. Here the EEPROM may reside! Thus we need special handling for that
            case EXT_ROM3_: {

                if (backupType == EEPROM_V) {
                    // The address internal to EXT3
                    uint32_t internalAddress = addr & 0x00FFFFFF;

                    // On carts with 16MB or smaller ROM, eeprom can be alternately
                    // accessed anywhere at D000000h-DFFFFFFh.
                    //  => So basically the whole EXT3 now can be used for the EEPROM
                    if (getRomSize() <= 0x01000000) {
                        execInfo.memReg = EEPROM_REGION;
                        execInfo.lowerBound = static_cast<uint32_t>(EXT_ROM3_) << 24;
                        execInfo.upperBound = EXT_ROM3_LIMIT + 1;
                        execInfo.readBaseAddr = noBackupMedia;
                        execInfo.writeBaseAddr = wasteMem;
                        execInfo.noOffset = true;
                        break;
                    } else if (internalAddress >= 0x00FFFF00) {
                        // In all other cases the EEPROM can be accessed from DFFFF00h..DFFFFFFh.
                        execInfo.memReg = EEPROM_REGION;
                        execInfo.lowerBound = 0x0DFFFF00;
                        execInfo.upperBound = EXT_ROM3_LIMIT + 1;
                        execInfo.readBaseAddr = noBackupMedia;
                        execInfo.writeBaseAddr = wasteMem;
                        execInfo.noOffset = true;
                        break;
                    }
                }

                // Else fall through and handle the address as part of the ROM
            }
            case EXT_ROM3:
            case EXT_ROM1_:
            case EXT_ROM1:
            case EXT_ROM2_:
            case EXT_ROM2: {
                uint32_t romOffset = addr & 0x00FFFFFF;
                addr = romOffset + EXT_ROM_OFFSET;
                if (romOffset >= getRomSize()) {
                    LOG_MEM(std::cout << "WARNING: trying to access rom out of bounds! Addr: 0x" << std::hex << addr << std::endl;);
                    // Indicate out of ROM!!!
                    execInfo.memReg = OUT_OF_ROM;
                    execInfo.readBaseAddr = zeroMem;
                    execInfo.writeBaseAddr = wasteMem;
                    execInfo.noOffset = true;
                    execInfo.lowerBound = EXT_ROM_OFFSET + getRomSize();
                    execInfo.upperBound = EXT_ROM2_LIMIT + 1;
                } else {
                    execInfo.readBaseAddr = rom;
                    execInfo.writeBaseAddr = rom;
                    execInfo.lowerBound = EXT_ROM_OFFSET;
                    execInfo.upperBound = EXT_ROM_OFFSET + getRomSize();
                }
                break;
            }

            case IO_REGS:
                //TODO we might want to replace ioHandler with resolving here & use std::functions for read & write instead of bare uint8_t pointers in InstructionExecutionInfo
                // IO is not mirrored
                execInfo.readBaseAddr = zeroMem;
                execInfo.writeBaseAddr = wasteMem;
                execInfo.lowerBound = IO_REGS_OFFSET;
                execInfo.upperBound = BG_OBJ_RAM_OFFSET;
                execInfo.noOffset = true;
                break;

            case BIOS:
                // Bios is not mirrored but limited!
                if (addr < BIOS_LIMIT) {
                    execInfo.readBaseAddr = biosState;
                    execInfo.writeBaseAddr = wasteMem;
                    execInfo.lowerBound = BIOS_OFFSET;
                    execInfo.upperBound = BIOS_LIMIT + 1;
                    execInfo.noOffset = true;
                    break;
                }

            default:
                // invalid address!
                LOG_MEM(std::cout << "ERROR: trying to access unused memory address: 0x" << std::hex << addr << std::endl;);
                execInfo.hasCausedException = true;
                execInfo.noOffset = true;
                execInfo.resolvedAddr = false;
                execInfo.memReg = UNUSED_MEMORY;

                execInfo.readBaseAddr = zeroMem;
                execInfo.writeBaseAddr = wasteMem;
                execInfo.lowerBound = addr;
                execInfo.upperBound = addr + 1;
                break;
        }
    }

    void Memory::scanROMForBackupID()
    {
        // reset backup type
        backupType = NO_BACKUP;

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
                            backupType = static_cast<BackupID>(k + 1);
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