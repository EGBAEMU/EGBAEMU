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

    Memory::Memory(std::function<uint32_t()> readUnusedHandle) : readUnusedHandle(readUnusedHandle)
    {
        wram = GBA_ALLOC_MEM_REG(memory::WRAM);
        iwram = GBA_ALLOC_MEM_REG(memory::IWRAM);
        bg_obj_ram = GBA_ALLOC_MEM_REG(memory::BG_OBJ_RAM);
        oam = GBA_ALLOC_MEM_REG(memory::OAM);
        reset();
    }

    void Memory::reset()
    {
        GBA_MEM_CLEAR(oam, memory::OAM);
        GBA_MEM_CLEAR(bg_obj_ram, memory::BG_OBJ_RAM);
        GBA_MEM_CLEAR(iwram, memory::IWRAM);
        GBA_MEM_CLEAR(wram, memory::WRAM);

        rom.reset();
        vram.reset();

        setBiosState(Bios::BIOS_AFTER_STARTUP);
        updateWaitCycles(0);
    }

    void Memory::updateWaitCycles(uint16_t WAITCNT)
    {
        // See: https://problemkaputt.de/gbatek.htm#gbasystemcontrol
        uint8_t sramWaitCnt = WAITCNT & 3;
        WAITCNT >>= 2;
        uint8_t wait0_n = WAITCNT & 3;
        WAITCNT >>= 2;
        uint8_t wait0_s = WAITCNT & 1;
        WAITCNT >>= 1;
        uint8_t wait1_n = WAITCNT & 3;
        WAITCNT >>= 2;
        uint8_t wait1_s = WAITCNT & 1;
        WAITCNT >>= 1;
        uint8_t wait2_n = WAITCNT & 3;
        WAITCNT >>= 2;
        uint8_t wait2_s = WAITCNT & 1;

        const constexpr uint8_t waitCycles_n[] = {
            4, 3, 2, 8};

        const constexpr uint8_t waitCycles0_s[] = {2, 1};
        const constexpr uint8_t waitCycles1_s[] = {4, 1};
        const constexpr uint8_t waitCycles2_s[] = {8, 1};

        cycles16Bit[0][memory::EXT_SRAM] = cycles16Bit[0][memory::EXT_SRAM_] =
            cycles16Bit[1][memory::EXT_SRAM] = cycles16Bit[1][memory::EXT_SRAM_] =
                cycles32Bit[0][memory::EXT_SRAM] = cycles32Bit[0][memory::EXT_SRAM_] =
                    cycles32Bit[1][memory::EXT_SRAM] = cycles32Bit[1][memory::EXT_SRAM_] = 1 + waitCycles_n[sramWaitCnt];

        // First Access (Non-sequential) and Second Access (Sequential) define the waitstates for N and S cycles
        // the actual access time is 1 clock cycle PLUS the number of waitstates.
        cycles16Bit[0][memory::EXT_ROM1] = cycles16Bit[0][memory::EXT_ROM1_] = 1 + waitCycles_n[wait0_n];
        cycles16Bit[0][memory::EXT_ROM2] = cycles16Bit[0][memory::EXT_ROM2_] = 1 + waitCycles_n[wait1_n];
        cycles16Bit[0][memory::EXT_ROM3] = cycles16Bit[0][memory::EXT_ROM3_] = 1 + waitCycles_n[wait2_n];

        cycles16Bit[1][memory::EXT_ROM1] = cycles16Bit[1][memory::EXT_ROM1_] = 1 + waitCycles0_s[wait0_s];
        cycles16Bit[1][memory::EXT_ROM2] = cycles16Bit[1][memory::EXT_ROM2_] = 1 + waitCycles1_s[wait1_s];
        cycles16Bit[1][memory::EXT_ROM3] = cycles16Bit[1][memory::EXT_ROM3_] = 1 + waitCycles2_s[wait2_s];

        // GamePak uses 16bit data bus, so that a 32bit access is split into TWO 16bit accesses
        // (of which, the second fragment is always sequential, even if the first fragment was non-sequential)
        cycles32Bit[0][memory::EXT_ROM1] = cycles32Bit[0][memory::EXT_ROM1_] = cycles16Bit[0][memory::EXT_ROM1] + cycles16Bit[1][memory::EXT_ROM1];
        cycles32Bit[0][memory::EXT_ROM2] = cycles32Bit[0][memory::EXT_ROM2_] = cycles16Bit[0][memory::EXT_ROM2] + cycles16Bit[1][memory::EXT_ROM2];
        cycles32Bit[0][memory::EXT_ROM3] = cycles32Bit[0][memory::EXT_ROM3_] = cycles16Bit[0][memory::EXT_ROM3] + cycles16Bit[1][memory::EXT_ROM3];
        cycles32Bit[1][memory::EXT_ROM1] = cycles32Bit[1][memory::EXT_ROM1_] = 2 * cycles16Bit[1][memory::EXT_ROM1];
        cycles32Bit[1][memory::EXT_ROM2] = cycles32Bit[1][memory::EXT_ROM2_] = 2 * cycles16Bit[1][memory::EXT_ROM2];
        cycles32Bit[1][memory::EXT_ROM3] = cycles32Bit[1][memory::EXT_ROM3_] = 2 * cycles16Bit[1][memory::EXT_ROM3];
    }

    Memory::~Memory()
    {
        delete[] wram;
        delete[] iwram;
        delete[] bg_obj_ram;
        delete[] oam;

        wram = nullptr;
        iwram = nullptr;
        bg_obj_ram = nullptr;
        oam = nullptr;
    }

    uint8_t Memory::read8(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq, bool readInstruction) const
    {

        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles16(execInfo.memReg, seq);

        uint8_t currValue;

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                currValue = wram[(addr & memory::WRAM_LIMIT) - memory::WRAM_OFFSET];
                break;
            case memory::IWRAM:
                // Trivial mirroring
                currValue = iwram[(addr & memory::IWRAM_LIMIT) - memory::IWRAM_OFFSET];
                break;
            case memory::IO_REGS:
                currValue = ioHandler.externalRead8(addr);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                currValue = bg_obj_ram[(addr & memory::BG_OBJ_RAM_LIMIT) - memory::BG_OBJ_RAM_OFFSET];
                break;
            case memory::VRAM:
                currValue = vram.read8(addr);
                break;
            case memory::OAM:
                // Trivial mirroring
                currValue = oam[(addr & memory::OAM_LIMIT) - memory::OAM_OFFSET];
                break;

            case memory::EXT_ROM1:
            case memory::EXT_ROM1_:
            case memory::EXT_ROM2:
            case memory::EXT_ROM2_:
            case memory::EXT_ROM3:
                currValue = rom.read8(addr);
                break;
            case memory::EXT_ROM3_:
                currValue = rom.read8ROM3_(addr);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                currValue = rom.read8SRAM(addr);
                break;

            case memory::BIOS:
                if (addr < memory::BIOS_LIMIT) {
                    currValue = bios.read8(addr, readInstruction);
                    break;
                }
                // Fall through if not in bios area, as this is unused memory!
            default:
                // Unused Memory
                currValue = readUnusedHandle();
                // get the selected byte
                currValue >>= ((addr & 3) << 3);
                break;
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
        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles16(execInfo.memReg, seq);

        uint16_t currValue;

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint16_t *>(wram + (addr & memory::WRAM_LIMIT & ~1) - memory::WRAM_OFFSET));
                break;
            case memory::IWRAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint16_t *>(iwram + (addr & memory::IWRAM_LIMIT & ~1) - memory::IWRAM_OFFSET));
                break;
            case memory::IO_REGS:
                currValue = ioHandler.externalRead16(addr & ~1);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint16_t *>(bg_obj_ram + (addr & memory::BG_OBJ_RAM_LIMIT & ~1) - memory::BG_OBJ_RAM_OFFSET));
                break;
            case memory::VRAM:
                currValue = vram.read16(addr);
                break;
            case memory::OAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint16_t *>(oam + (addr & memory::OAM_LIMIT & ~1) - memory::OAM_OFFSET));
                break;

            case memory::EXT_ROM1:
            case memory::EXT_ROM1_:
            case memory::EXT_ROM2:
            case memory::EXT_ROM2_:
            case memory::EXT_ROM3:
                currValue = rom.read16(addr);
                break;
            case memory::EXT_ROM3_:
                currValue = rom.read16ROM3_(addr, dmaRequest);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                currValue = rom.read16SRAM(addr);
                break;

            case memory::BIOS:
                if (addr < memory::BIOS_LIMIT) {
                    currValue = bios.read16(addr, readInstruction);
                    break;
                }
                // Fall through if not in bios area, as this is unused memory!
            default:
                // Unused Memory
                currValue = readUnusedHandle();
                // get the selected HW
                currValue >>= ((addr & 2) << 3);
                break;
        }

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr))
            memWatch.addressCheckTrigger(addr, currValue);
#endif

        return currValue;
    }

    uint32_t Memory::read32(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq, bool readInstruction, bool dmaRequest) const
    {
        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles32(execInfo.memReg, seq);

        uint32_t currValue;

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint32_t *>(wram + (addr & memory::WRAM_LIMIT & ~3) - memory::WRAM_OFFSET));
                break;
            case memory::IWRAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint32_t *>(iwram + (addr & memory::IWRAM_LIMIT & ~3) - memory::IWRAM_OFFSET));
                break;
            case memory::IO_REGS:
                currValue = ioHandler.externalRead32(addr & ~3);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint32_t *>(bg_obj_ram + (addr & memory::BG_OBJ_RAM_LIMIT & ~3) - memory::BG_OBJ_RAM_OFFSET));
                break;
            case memory::VRAM:
                currValue = vram.read32(addr);
                break;
            case memory::OAM:
                // Trivial mirroring
                currValue = le(*reinterpret_cast<uint32_t *>(oam + (addr & memory::OAM_LIMIT & ~3) - memory::OAM_OFFSET));
                break;

            case memory::EXT_ROM1:
            case memory::EXT_ROM1_:
            case memory::EXT_ROM2:
            case memory::EXT_ROM2_:
            case memory::EXT_ROM3:
                currValue = rom.read32(addr);
                break;
            case memory::EXT_ROM3_:
                currValue = rom.read32ROM3_(addr, dmaRequest);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                currValue = rom.read32SRAM(addr);
                break;

            case memory::BIOS:
                if (addr < memory::BIOS_LIMIT) {
                    currValue = bios.read32(addr, readInstruction);
                    break;
                }
                // Fall through if not in bios area, as this is unused memory!
            default:
                // Unused Memory
                currValue = readUnusedHandle();
                break;
        }

#ifdef DEBUG_CLI
        if (memWatch.isAddressWatched(addr))
            memWatch.addressCheckTrigger(addr, currValue);
#endif

        return currValue;
    }

    void Memory::write8(uint32_t addr, uint8_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles16(execInfo.memReg, seq);

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                wram[(addr & memory::WRAM_LIMIT) - memory::WRAM_OFFSET] = value;
                break;
            case memory::IWRAM:
                // Trivial mirroring
                iwram[(addr & memory::IWRAM_LIMIT) - memory::IWRAM_OFFSET] = value;
                break;
            case memory::IO_REGS:
                ioHandler.externalWrite8(addr, value);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                // Edge cases write8 becomes write16 with repeated byte
                *reinterpret_cast<uint16_t *>(bg_obj_ram + (addr & memory::BG_OBJ_RAM_LIMIT & ~1) - memory::BG_OBJ_RAM_OFFSET) = (static_cast<uint16_t>(value) << 8) | value;
                break;
            case memory::VRAM:
                vram.write8(addr, value);
                break;

            case memory::EXT_ROM3_:
                rom.writeROM3_(addr, value);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                rom.write8SRAM(addr, value);
                break;

            case memory::OAM:
                // Always ignored, only 16 bit & 32 bit allowed
            default:
                // Ignore writes
                break;
        }
    }

    void Memory::write16(uint32_t addr, uint16_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles16(execInfo.memReg, seq);

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                *reinterpret_cast<uint16_t *>(wram + (addr & memory::WRAM_LIMIT & ~1) - memory::WRAM_OFFSET) = le(value);
                break;
            case memory::IWRAM:
                // Trivial mirroring
                *reinterpret_cast<uint16_t *>(iwram + (addr & memory::IWRAM_LIMIT & ~1) - memory::IWRAM_OFFSET) = le(value);
                break;
            case memory::IO_REGS:
                ioHandler.externalWrite16(addr, value);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                *reinterpret_cast<uint16_t *>(bg_obj_ram + (addr & memory::BG_OBJ_RAM_LIMIT & ~1) - memory::BG_OBJ_RAM_OFFSET) = le(value);
                break;
            case memory::VRAM:
                vram.write16(addr, value);
                break;
            case memory::OAM:
                // Trivial mirroring
                *reinterpret_cast<uint16_t *>(oam + (addr & memory::OAM_LIMIT & ~1) - memory::OAM_OFFSET) = le(value);
                break;

            case memory::EXT_ROM3_:
                rom.writeROM3_(addr, value);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                rom.write16SRAM(addr, value);
                break;

            default:
                // Ignore writes
                break;
        }

#ifdef DEBUG_CLI
            //TODO re-implement memWatch?
            /*
        if (memWatch.isAddressWatched(addr)) {
            uint32_t currValue = le<uint16_t>(*reinterpret_cast<const uint16_t *>(dst));
            memWatch.addressCheckTrigger(addr, currValue, value);
        }
        */
#endif
    }

    void Memory::write32(uint32_t addr, uint32_t value, InstructionExecutionInfo &execInfo, bool seq)
    {
        execInfo.memReg = extractMemoryRegion(addr);
        execInfo.cycleCount += memCycles32(execInfo.memReg, seq);

        switch (execInfo.memReg) {
            case memory::WRAM:
                // Trivial mirroring
                *reinterpret_cast<uint32_t *>(wram + (addr & memory::WRAM_LIMIT & ~3) - memory::WRAM_OFFSET) = le(value);
                break;
            case memory::IWRAM:
                // Trivial mirroring
                *reinterpret_cast<uint32_t *>(iwram + (addr & memory::IWRAM_LIMIT & ~3) - memory::IWRAM_OFFSET) = le(value);
                break;
            case memory::IO_REGS:
                ioHandler.externalWrite32(addr, value);
                break;
            case memory::BG_OBJ_RAM:
                // Trivial mirroring
                *reinterpret_cast<uint32_t *>(bg_obj_ram + (addr & memory::BG_OBJ_RAM_LIMIT & ~3) - memory::BG_OBJ_RAM_OFFSET) = le(value);
                break;
            case memory::VRAM:
                vram.write32(addr, value);
                break;
            case memory::OAM:
                // Trivial mirroring
                *reinterpret_cast<uint32_t *>(oam + (addr & memory::OAM_LIMIT & ~3) - memory::OAM_OFFSET) = le(value);
                break;

            case memory::EXT_ROM3_:
                rom.writeROM3_(addr, value);
                break;
            case memory::EXT_SRAM:
            case memory::EXT_SRAM_:
                rom.write32SRAM(addr, value);
                break;

            default:
                // Ignore writes
                break;
        }

#ifdef DEBUG_CLI
            //TODO re-implement memWatch?
            /*
        if (memWatch.isAddressWatched(addr)) {
            uint32_t currValue = le<uint32_t>(*reinterpret_cast<const uint32_t *>(dst));
            memWatch.addressCheckTrigger(addr, currValue, value);
        }
        */
#endif
    }

    memory::MemoryRegion Memory::extractMemoryRegion(uint32_t addr)
    {
        return static_cast<memory::MemoryRegion>((addr >> 24) & 0x0F);
    }

} // namespace gbaemu