#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "inst.hpp"
#include "io/io_regs.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace gbaemu
{
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
    */
#define GBA_ALLOC_MEM_REG(x) new uint8_t[x##_LIMIT - x##_OFFSET + 1]
#define GBA_MEM_CLEAR(arr, x) std::fill_n(arr, x##_LIMIT - x##_OFFSET + 1, 0)

    class Memory
    {

      public:
        enum BiosReadState : uint8_t {
            BIOS_AFTER_STARTUP = 0,
            BIOS_AFTER_SWI,
            BIOS_DURING_IRQ,
            BIOS_AFTER_IRQ
        };

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
            EXT_SRAM_ = 0x0F,
            OUT_OF_ROM = 0x42 // Virtual memory region to indicate access outside of the ROM!
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

        enum BackupID : uint8_t {
            EEPROM_V = 0, // 512 bytes or 8 KiB
            SRAM_V,       // 32 KiB
            FLASH_V,      // 64 KiB
            FLASH512_V,   // 64 KiB
            FLASH1M_V,    // 128 KiB
            NO_BACKUP     // Not sure if this is allowed?
        };

      // the offset within bios code
      static const uint32_t BIOS_IRQ_HANDLER_OFFSET = 4;

      private:
        //uint8_t *bios;
        uint8_t *wram;
        uint8_t *iwram;
        //uint8_t *io_regs;
        uint8_t *bg_obj_ram;
        uint8_t *vram;
        uint8_t *oam;

        uint8_t *ext_sram = nullptr;
        uint8_t *rom = nullptr;
        size_t romSize = 0;

        static const constexpr uint8_t BIOS_READ_AFTER_STARTUP[] = {0x00, 0xF0, 0x29, 0xE1};
        static const constexpr uint8_t BIOS_READ_AFTER_SWI[] = {0x04, 0x20, 0xA0, 0xE3};
        static const constexpr uint8_t BIOS_READ_DURING_IRQ[] = {0x04, 0xF0, 0x5E, 0xE2};
        static const constexpr uint8_t BIOS_READ_AFTER_IRQ[] = {0x02, 0xC0, 0x5E, 0xE5};

        static const constexpr uint8_t *const BIOS_READ[] = {
            BIOS_READ_AFTER_STARTUP,
            BIOS_READ_AFTER_SWI,
            BIOS_READ_DURING_IRQ,
            BIOS_READ_AFTER_IRQ};

        static const constexpr uint32_t backupSizes[] = {
            //TODO were do we know the exact size from???
            /*EEPROM_V_SIZE = */ 8 << 10,    // 512 bytes or 8 KiB
            /*SRAM_V_SIZE = */ 32 << 10,     // 32 KiB
            /*FLASH_V_SIZE = */ 64 << 10,    // 64 KiB
            /*FLASH512_V_SIZE = */ 64 << 10, // 64 KiB
                                             //TODO this exceeds the normal expected memory area???
            /*FLASH1M_V_SIZE =*/128 << 10    // 128 KiB
        };

        static const constexpr uint8_t customBiosCode[] = {

            // Protection such that execution does not run into the interrupt handler by accident
            0xFD, 0xFF, 0xFF, 0xEA, // b      -4h

            // Interrupt handler entry code
            0x0F, 0x50, 0x2D, 0xE9, // stmfd  r13!,r0-r3,r12,r14  ;save registers to SP_irq
            0x02, 0x00, 0xA0, 0xE3, // mov    r0, BIOS_DURING_IRQ (= 2)
            0x00, 0x00, 0x2B, 0xEF, // svc    0x2B
            0x01, 0x03, 0xA0, 0xE3, // mov    r0,4000000h         ;ptr+4 to 03FFFFFC (mirror of 03007FFC)
            0x00, 0xE0, 0x8F, 0xE2, // add    r14,r15,0h          ;retadr for USER handler $+8=138h
            0x04, 0xF0, 0x10, 0xE5, // ldr    r15,[r0,-4h]        ;jump to [03FFFFFC] USER handler

            // Interrupt handler exit code
            0x03, 0x00, 0xA0, 0xE3, // mov    r0, BIOS_AFTER_IRQ (= 3)
            0x00, 0x00, 0x2B, 0xEF, // svc    0x2B
            0x0F, 0x50, 0xBD, 0xE8, // ldmfd  r13!,r0-r3,r12,r14  ;restore registers from SP_irq
            0x00, 0xF0, 0x5E, 0xE2, // subs   r15,r14,0h          ;return from IRQ (PC=LR, CPSR=SPSR)

            //TODO maybe SWI implementations?
        };

        BackupID backupType;
        BiosReadState biosReadState;

      public:
        IO_Handler ioHandler;

        //TODO are there conventions about inital memory values?
        Memory()
        {
            //bios = GBA_ALLOC_MEM_REG(BIOS);
            //GBA_MEM_CLEAR(bios, BIOS);
            wram = GBA_ALLOC_MEM_REG(WRAM);
            GBA_MEM_CLEAR(wram, WRAM);
            iwram = GBA_ALLOC_MEM_REG(IWRAM);
            GBA_MEM_CLEAR(iwram, IWRAM);
            //io_regs = GBA_ALLOC_MEM_REG(IO_REGS);
            //GBA_MEM_CLEAR(io_regs, IO_REGS);
            bg_obj_ram = GBA_ALLOC_MEM_REG(BG_OBJ_RAM);
            GBA_MEM_CLEAR(bg_obj_ram, BG_OBJ_RAM);
            vram = GBA_ALLOC_MEM_REG(VRAM);
            GBA_MEM_CLEAR(vram, VRAM);
            oam = GBA_ALLOC_MEM_REG(OAM);
            GBA_MEM_CLEAR(oam, OAM);
            rom = nullptr;
            romSize = 0;
        }

        ~Memory()
        {
            //delete[] bios;
            delete[] wram;
            delete[] iwram;
            //delete[] io_regs;
            delete[] bg_obj_ram;
            delete[] vram;
            delete[] oam;
            if (ext_sram)
                delete[] ext_sram;
            if (romSize)
                delete[] rom;

            //bios = nullptr;
            wram = nullptr;
            iwram = nullptr;
            //io_regs = nullptr;
            bg_obj_ram = nullptr;
            vram = nullptr;
            oam = nullptr;
            ext_sram = nullptr;
            rom = nullptr;
        }

        Memory(const Memory &) = delete;
        Memory &operator=(const Memory &) = delete;

        void loadROM(const uint8_t *rom, size_t romSize)
        {
            if (this->romSize) {
                delete[] this->rom;
            }
            this->romSize = romSize;
            this->rom = new uint8_t[this->romSize];
            std::copy_n(rom, romSize, this->rom);

            // TODO reset stats
            scanROMForBackupID();
            biosReadState = BIOS_AFTER_STARTUP;
        }

        constexpr size_t getBiosSize() const {
            return sizeof(customBiosCode)/sizeof(customBiosCode[0]);
        }

        size_t getRomSize() const
        {
            return romSize;
        }

        uint8_t read8(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false) const;
        uint16_t read16(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false, bool readInstruction = false) const;
        uint32_t read32(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false, bool readInstruction = false) const;
        void write8(uint32_t addr, uint8_t value, InstructionExecutionInfo *execInfo, bool seq = false);
        void write16(uint32_t addr, uint16_t value, InstructionExecutionInfo *execInfo, bool seq = false);
        void write32(uint32_t addr, uint32_t value, InstructionExecutionInfo *execInfo, bool seq = false);

        // This is needed to handle memory mirroring
        const uint8_t *resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg) const;
        uint8_t *resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg);
        uint32_t normalizeAddress(uint32_t addr, MemoryRegion &memReg) const;

        uint8_t cyclesForVirtualAddrNonSeq(uint32_t address, uint8_t bytesToRead) const;
        uint8_t cyclesForVirtualAddrSeq(uint32_t address, uint8_t bytesToRead) const;

        void setBiosReadState(BiosReadState readState)
        {
            biosReadState = readState;
        }

      private:
        void scanROMForBackupID();
        uint32_t readOutOfROM(uint32_t addr) const;
    };
} // namespace gbaemu

#endif /* MEMORY_HPP */
