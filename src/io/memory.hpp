#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "decode/inst.hpp"
#include "io/io_regs.hpp"
#include "save/eeprom.hpp"
#include "save/flash.hpp"
#include <cstdint>
#include <functional>
#include <map>

namespace gbaemu
{
    typedef uint32_t address_t;

    /*
        A pointer to this class can be handed to Memory, which triggers a callback
        if the condition is satisfied.
     */
    class MemWatch
    {
      public:
        struct Condition {
            uint32_t value;
            bool onRead;
            bool onWrite;
            bool onValueEqual;
            bool onValueChanged;
        };

      private:
        std::set<address_t> addresses;
        std::map<address_t, Condition> conditions;

        /* address, condition, currValue, on write?, newValue */
        std::function<void(address_t, Condition, uint32_t, bool, uint32_t)> trigger;

      public:
        void registerTrigger(const std::function<void(address_t, Condition, uint32_t, bool, uint32_t)> &trig);

        void watchAddress(address_t addr, const Condition &cond);
        void unwatchAddress(address_t addr);

        /* Watch and trigger are split up for performance reasons. */
        bool isAddressWatched(address_t addr) const noexcept;
        /* These functions crash if the function above does not return true. */
        /* only for reading */
        void addressCheckTrigger(address_t addr, uint32_t currValue) const;
        /* only for writing */
        void addressCheckTrigger(address_t addr, uint32_t currValue, uint32_t newValue) const;

        std::string getWatchPointInfo() const;
    };

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
#define GBA_MEM_CLEAR_VALUE(arr, x, value) std::fill_n(arr, x##_LIMIT - x##_OFFSET + 1, (value))

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
            OUT_OF_ROM = 0x42,    // Virtual memory region to indicate access outside of the ROM!
            EEPROM_REGION = 0x43, // Virtual memory region to signal EEPROM usage
            FLASH_REGION = 0x44,  // Virtual memory region to signal flash usage
            SRAM_REGION = 0x45,   // Virtual memory region to signal sram usage
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
            NO_BACKUP = 0, // Not sure if this is allowed?
            EEPROM_V,      // 512 bytes or 8 KiB
            SRAM_V,        // 32 KiB
            // Order is important: no non flash type shall be below this line!
            FLASH_V,    // 64 KiB
            FLASH512_V, // 64 KiB
            FLASH1M_V   // 128 KiB
        };

        // the offset within bios code
        static constexpr uint32_t BIOS_IRQ_HANDLER_OFFSET = 0x18;
        static constexpr uint32_t BIOS_SWI_HANDLER_OFFSET = 0x08;

      private:
        uint8_t *wram;
        uint8_t *iwram;
        uint8_t *bg_obj_ram;
        uint8_t *vram;
        uint8_t *oam;

        uint8_t *rom;
        size_t romSize;

        const uint8_t *bios;
        size_t externalBiosSize;
        uint8_t biosState[4];

        BackupID backupType;

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
            0,                               // NO BACKUP TYPE
            /*EEPROM_V_SIZE = */ 8 << 10,    // 512 bytes or 8 KiB
            /*SRAM_V_SIZE = */ 32 << 10,     // 32 KiB
            /*FLASH_V_SIZE = */ 64 << 10,    // 64 KiB
            /*FLASH512_V_SIZE = */ 64 << 10, // 64 KiB
                                             //TODO this exceeds the normal expected memory area???
            /*FLASH1M_V_SIZE =*/128 << 10    // 128 KiB
        };

        static const constexpr uint8_t customBiosCode[] = {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x0a, 0x00, 0x00, 0xea,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xea,
            0x00, 0x00, 0x00, 0x00,
            0x0f, 0x50, 0x2d, 0xe9,
            0x01, 0x03, 0xa0, 0xe3,
            0x00, 0xe0, 0x8f, 0xe2,
            0x04, 0xf0, 0x10, 0xe5,
            0x0f, 0x50, 0xbd, 0xe8,
            0x04, 0xf0, 0x5e, 0xe2,
            0x00, 0x58, 0x2d, 0xe9,
            0x02, 0xc0, 0x5e, 0xe5,
            0x00, 0xb0, 0x4f, 0xe1,
            0x04, 0xb0, 0x2d, 0xe5,
            0x80, 0xb0, 0x0b, 0xe2,
            0x1f, 0xb0, 0x8b, 0xe3,
            0x0b, 0xf0, 0x21, 0xe1,
            0x04, 0xe0, 0x2d, 0xe5,
            0x74, 0xe0, 0xa0, 0xe3,
            0x0b, 0x00, 0x5c, 0xe3,
            0xf8, 0xb0, 0xa0, 0x03,
            0x01, 0x00, 0x00, 0x0a,
            0x0c, 0x00, 0x5c, 0xe3,
            0x94, 0xb0, 0xa0, 0x03,
            0x1b, 0xff, 0x2f, 0x01,
            0x04, 0xe0, 0x9d, 0xe4,
            0x93, 0xf0, 0x21, 0xe3,
            0x04, 0xc0, 0x9d, 0xe4,
            0x0c, 0xf0, 0x69, 0xe1,
            0x00, 0x58, 0xbd, 0xe8,
            0x0e, 0xf0, 0xb0, 0xe1,
            0x00, 0x00, 0x00, 0x00,
            0x04, 0x20, 0xa0, 0xe3,
            0xf8, 0x47, 0x2d, 0xe9,
            0x02, 0x36, 0xa0, 0xe3,
            0x01, 0x30, 0x43, 0xe2,
            0x03, 0x00, 0x12, 0xe1,
            0x12, 0x00, 0x00, 0x0a,
            0x01, 0x04, 0x12, 0xe3,
            0x03, 0x20, 0x02, 0xe0,
            0x04, 0x00, 0x00, 0x1a,
            0xf8, 0x07, 0xb0, 0xe8,
            0xf8, 0x07, 0xa1, 0xe8,
            0x08, 0x20, 0x52, 0xe2,
            0x08, 0x00, 0x00, 0xca,
            0x0a, 0x00, 0x00, 0xea,
            0x00, 0x30, 0x90, 0xe5,
            0x03, 0x40, 0xa0, 0xe1,
            0x03, 0x50, 0xa0, 0xe1,
            0x03, 0x60, 0xa0, 0xe1,
            0x03, 0x70, 0xa0, 0xe1,
            0x03, 0x80, 0xa0, 0xe1,
            0x03, 0x90, 0xa0, 0xe1,
            0x03, 0xa0, 0xa0, 0xe1,
            0xf8, 0x07, 0xa1, 0xe8,
            0x08, 0x20, 0x52, 0xe2,
            0xfc, 0xff, 0xff, 0xca,
            0xf8, 0x87, 0xbd, 0xe8,
            0x0f, 0x40, 0x2d, 0xe9,
            0x02, 0x36, 0xa0, 0xe3,
            0x01, 0x30, 0x43, 0xe2,
            0x03, 0x00, 0x12, 0xe1,
            0x1a, 0x00, 0x00, 0x0a,
            0x01, 0x04, 0x12, 0xe3,
            0x0c, 0x00, 0x00, 0x1a,
            0x01, 0x03, 0x12, 0xe3,
            0x03, 0x20, 0x02, 0xe0,
            0x04, 0x00, 0x00, 0x1a,
            0xb2, 0x30, 0xd0, 0xe0,
            0xb2, 0x30, 0xc1, 0xe0,
            0x01, 0x20, 0x52, 0xe2,
            0xfb, 0xff, 0xff, 0xca,
            0x10, 0x00, 0x00, 0xea,
            0x04, 0x30, 0x90, 0xe4,
            0x04, 0x30, 0x81, 0xe4,
            0x01, 0x20, 0x52, 0xe2,
            0xfb, 0xff, 0xff, 0xca,
            0x0b, 0x00, 0x00, 0xea,
            0x01, 0x03, 0x12, 0xe3,
            0x03, 0x20, 0x02, 0xe0,
            0xb0, 0x30, 0xd0, 0x11,
            0x00, 0x30, 0x90, 0x05,
            0x03, 0x00, 0x00, 0x1a,
            0xb2, 0x30, 0xc1, 0xe0,
            0x01, 0x20, 0x52, 0xe2,
            0xfc, 0xff, 0xff, 0xca,
            0x02, 0x00, 0x00, 0xea,
            0x04, 0x30, 0x81, 0xe4,
            0x01, 0x20, 0x52, 0xe2,
            0xfc, 0xff, 0xff, 0xca,
            0x0f, 0x80, 0xbd, 0xe8};

      public:
        IO_Handler ioHandler;
        save::EEPROM *eeprom;
        save::FLASH *flash;
        save::SaveFile *ext_sram;
        /* We are going to expose this directly becaus this cannot be in an invalid state and I don't have time. */
        MemWatch memWatch;

        Memory()
        {
            wram = GBA_ALLOC_MEM_REG(WRAM);
            iwram = GBA_ALLOC_MEM_REG(IWRAM);
            bg_obj_ram = GBA_ALLOC_MEM_REG(BG_OBJ_RAM);
            vram = GBA_ALLOC_MEM_REG(VRAM);
            oam = GBA_ALLOC_MEM_REG(OAM);
            rom = nullptr;
            eeprom = nullptr;
            flash = nullptr;
            ext_sram = nullptr;
            bios = customBiosCode;
            externalBiosSize = 0;
            romSize = 0;
            backupType = NO_BACKUP;

            reset();
        }

        void reset()
        {
            GBA_MEM_CLEAR(oam, OAM);
            GBA_MEM_CLEAR(vram, VRAM);
            GBA_MEM_CLEAR(bg_obj_ram, BG_OBJ_RAM);
            GBA_MEM_CLEAR(iwram, IWRAM);
            GBA_MEM_CLEAR(wram, WRAM);

            if (eeprom) {
                eeprom->reset();
            }
            if (flash) {
                flash->reset();
            }

            setBiosReadState(BIOS_AFTER_STARTUP);
        }

        ~Memory()
        {
            if (externalBiosSize) {
                delete[] bios;
                externalBiosSize = 0;
                bios = customBiosCode;
            }
            delete[] wram;
            delete[] iwram;
            delete[] bg_obj_ram;
            delete[] vram;
            delete[] oam;
            if (romSize) {
                delete[] rom;
            }
            if (ext_sram) {
                delete ext_sram;
            }
            if (eeprom) {
                delete eeprom;
            }
            if (flash) {
                delete flash;
            }

            wram = nullptr;
            iwram = nullptr;
            bg_obj_ram = nullptr;
            vram = nullptr;
            oam = nullptr;
            ext_sram = nullptr;
            rom = nullptr;
            eeprom = nullptr;
            flash = nullptr;
        }

        Memory(const Memory &) = delete;
        Memory &operator=(const Memory &) = delete;

        bool loadROM(const char *saveFilePath, const uint8_t *rom, size_t romSize)
        {
            if (this->romSize) {
                delete[] this->rom;
            }
            if (this->eeprom) {
                delete this->eeprom;
                this->eeprom = nullptr;
            }
            if (this->flash) {
                delete this->flash;
                this->flash = nullptr;
            }
            if (this->ext_sram) {
                delete this->ext_sram;
                this->ext_sram = nullptr;
            }
            this->romSize = romSize;
            this->rom = new uint8_t[this->romSize];
            std::copy_n(rom, romSize, this->rom);

            reset();
            scanROMForBackupID();

            bool loadSuccessful = true;
            if (backupType == SRAM_V) {
                // Allocate needed memory
                ext_sram = new save::SaveFile(saveFilePath, loadSuccessful, backupSizes[backupType]);
            } else if (backupType == EEPROM_V) {
                //TODO how to find out the eeprom size???
                this->eeprom = new save::EEPROM(saveFilePath, loadSuccessful /*, romSize >= 0x01000000 ? 14 : 6*/);
            } else if (backupType >= FLASH_V) {
                this->flash = new save::FLASH(saveFilePath, loadSuccessful, backupSizes[backupType]);
            }

            return loadSuccessful;
        }

        void loadExternalBios(const uint8_t *bios, size_t biosSize)
        {
            if (biosSize) {
                if (externalBiosSize) {
                    delete[] this->bios;
                }
                externalBiosSize = biosSize;
                uint8_t *newBios = new uint8_t[biosSize];
                std::copy_n(bios, biosSize, newBios);
                this->bios = newBios;
            }
        }

        size_t getBiosSize() const
        {
            return externalBiosSize ? externalBiosSize : sizeof(customBiosCode) / sizeof(customBiosCode[0]);
        }

        bool usesExternalBios() const
        {
            return externalBiosSize;
        }

        size_t getRomSize() const
        {
            return romSize;
        }

        uint8_t read8(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false, bool readInstruction = false) const;
        uint16_t read16(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        uint32_t read32(uint32_t addr, InstructionExecutionInfo *execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        void write8(uint32_t addr, uint8_t value, InstructionExecutionInfo *execInfo, bool seq = false);
        void write16(uint32_t addr, uint16_t value, InstructionExecutionInfo *execInfo, bool seq = false);
        void write32(uint32_t addr, uint32_t value, InstructionExecutionInfo *execInfo, bool seq = false);

        // This is needed to handle memory mirroring
        const uint8_t *resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg) const;
        const uint8_t *resolveAddrRef(uint32_t &addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg) const;
        uint8_t *resolveAddr(uint32_t addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg);
        uint8_t *resolveAddrRef(uint32_t &addr, InstructionExecutionInfo *execInfo, MemoryRegion &memReg);
        void normalizeAddressRef(uint32_t &addr, MemoryRegion &memReg) const;
        uint32_t normalizeAddress(uint32_t addr, MemoryRegion &memReg) const;

        uint8_t cyclesForVirtualAddrNonSeq(MemoryRegion memReg, uint8_t bytesToRead) const;
        uint8_t cyclesForVirtualAddrSeq(MemoryRegion memReg, uint8_t bytesToRead) const;

        void setBiosReadState(BiosReadState readState)
        {
            biosState[0] = BIOS_READ[readState][0];
            biosState[1] = BIOS_READ[readState][1];
            biosState[2] = BIOS_READ[readState][2];
            biosState[3] = BIOS_READ[readState][3];
        }

        void setBiosState(uint32_t inst)
        {
            biosState[0] = inst & 0x0FF;
            biosState[1] = (inst >> 8) & 0x0FF;
            biosState[2] = (inst >> 16) & 0x0FF;
            biosState[3] = (inst >> 24) & 0x0FF;
        }

      private:
        void scanROMForBackupID();
        uint32_t readOutOfROM(uint32_t addr) const;
    }; // namespace gbaemu
} // namespace gbaemu

#endif /* MEMORY_HPP */
