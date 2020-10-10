#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "bios.hpp"
#include "io_regs.hpp"
#include "logging.hpp"
#include "save/eeprom.hpp"
#include "save/flash.hpp"
#include "util.hpp"
#include <cstdint>
#include <functional>
#ifdef DEBUG_CLI
#include <map>
#endif

namespace gbaemu
{
    struct InstructionExecutionInfo;

    typedef uint32_t address_t;

#ifdef DEBUG_CLI
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
#endif

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
            UNUSED_MEMORY = 0x46  // Virtual memory region to signal the address points to unmapped & unused memory!
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
        uint8_t cycles16Bit[2][16] = {
            // Non seq times
            {1, 1, 3, 1, 1, 1, 1, 1},
            // seq times
            {1, 1, 3, 1, 1, 1, 1, 1},
        };
        uint8_t cycles32Bit[2][16] = {
            // Non seq times
            {1, 1, 6, 1, 1, 2, 1, 1},
            // seq times
            {1, 1, 6, 1, 1, 2, 1, 1},
        };

        uint8_t *wram;
        uint8_t *iwram;

      public:
        uint8_t *bg_obj_ram;
        uint8_t *vram;
        uint8_t *oam;

      private:
        uint8_t *rom;
        size_t romSize;

        BackupID backupType;

        static const constexpr uint32_t backupSizes[] = {
            0,                               // NO BACKUP TYPE
            /*EEPROM_V_SIZE = */ 8 << 10,    // 512 bytes or 8 KiB
            /*SRAM_V_SIZE = */ 32 << 10,     // 32 KiB
            /*FLASH_V_SIZE = */ 64 << 10,    // 64 KiB
            /*FLASH512_V_SIZE = */ 64 << 10, // 64 KiB
            /*FLASH1M_V_SIZE =*/128 << 10    // 128 KiB
        };

      public:
        Bios bios;
        IO_Handler ioHandler;
        save::EEPROM *eeprom;
        save::FLASH *flash;
        save::SaveFile *ext_sram;

        // needed to handle read from unused memory regions
        const std::function<uint32_t()> readUnusedHandle;

#ifdef DEBUG_CLI
        /* We are going to expose this directly becaus this cannot be in an invalid state and I don't have time. */
        MemWatch memWatch;
#endif

        Memory(std::function<uint32_t()> readUnusedHandle) : readUnusedHandle(readUnusedHandle)
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
            romSize = 0;
            backupType = NO_BACKUP;

            reset();
        }

        BackupID getBackupType() const
        {
            return backupType;
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

            setBiosState(Bios::BIOS_AFTER_STARTUP);
            updateWaitCycles(0);
        }

        void updateWaitCycles(uint16_t WAITCNT)
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

            cycles16Bit[0][EXT_SRAM] = cycles16Bit[0][EXT_SRAM_] =
                cycles16Bit[1][EXT_SRAM] = cycles16Bit[1][EXT_SRAM_] =
                    cycles32Bit[0][EXT_SRAM] = cycles32Bit[0][EXT_SRAM_] =
                        cycles32Bit[1][EXT_SRAM] = cycles32Bit[1][EXT_SRAM_] = waitCycles_n[sramWaitCnt];

            // First Access (Non-sequential) and Second Access (Sequential) define the waitstates for N and S cycles
            // the actual access time is 1 clock cycle PLUS the number of waitstates.
            cycles16Bit[0][EXT_ROM1] = cycles16Bit[0][EXT_ROM1_] = 1 + waitCycles_n[wait0_n];
            cycles16Bit[0][EXT_ROM2] = cycles16Bit[0][EXT_ROM2_] = 1 + waitCycles_n[wait1_n];
            cycles16Bit[0][EXT_ROM3] = cycles16Bit[0][EXT_ROM3_] = 1 + waitCycles_n[wait2_n];

            cycles16Bit[1][EXT_ROM1] = cycles16Bit[1][EXT_ROM1_] = 1 + waitCycles0_s[wait0_s];
            cycles16Bit[1][EXT_ROM2] = cycles16Bit[1][EXT_ROM2_] = 1 + waitCycles1_s[wait1_s];
            cycles16Bit[1][EXT_ROM3] = cycles16Bit[1][EXT_ROM3_] = 1 + waitCycles2_s[wait2_s];

            // GamePak uses 16bit data bus, so that a 32bit access is split into TWO 16bit accesses
            // (of which, the second fragment is always sequential, even if the first fragment was non-sequential)
            cycles32Bit[0][EXT_ROM1] = cycles32Bit[0][EXT_ROM1_] = cycles16Bit[0][EXT_ROM1] + cycles16Bit[1][EXT_ROM1];
            cycles32Bit[0][EXT_ROM2] = cycles32Bit[0][EXT_ROM2_] = cycles16Bit[0][EXT_ROM2] + cycles16Bit[1][EXT_ROM2];
            cycles32Bit[0][EXT_ROM3] = cycles32Bit[0][EXT_ROM3_] = cycles16Bit[0][EXT_ROM3] + cycles16Bit[1][EXT_ROM3];
            cycles32Bit[1][EXT_ROM1] = cycles32Bit[1][EXT_ROM1_] = 2 * cycles16Bit[1][EXT_ROM1];
            cycles32Bit[1][EXT_ROM2] = cycles32Bit[1][EXT_ROM2_] = 2 * cycles16Bit[1][EXT_ROM2];
            cycles32Bit[1][EXT_ROM3] = cycles32Bit[1][EXT_ROM3_] = 2 * cycles16Bit[1][EXT_ROM3];
        }

        ~Memory()
        {
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
                // EEPROM size is determinable via the size of the dma requests
                this->eeprom = new save::EEPROM(saveFilePath, loadSuccessful);
            } else if (backupType >= FLASH_V) {
                this->flash = new save::FLASH(saveFilePath, loadSuccessful, backupSizes[backupType]);
            }

            return loadSuccessful;
        }

        void loadExternalBios(const uint8_t *externalBios, size_t biosSize)
        {
            bios.setExternalBios(externalBios, biosSize);
        }

        size_t getBiosSize() const
        {
            return bios.getBiosSize();
        }

        bool usesExternalBios() const
        {
            return bios.usesExternalBios();
        }

        size_t getRomSize() const
        {
            return romSize;
        }

        uint8_t read8(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false) const;
        uint16_t read16(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        uint32_t read32(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        void write8(uint32_t addr, uint8_t value, InstructionExecutionInfo &execInfo, bool seq = false);
        void write16(uint32_t addr, uint16_t value, InstructionExecutionInfo &execInfo, bool seq = false);
        void write32(uint32_t addr, uint32_t value, InstructionExecutionInfo &execInfo, bool seq = false);

        // This is needed to handle memory mirroring
        void normalizeAddressRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const;
        uint32_t normalizeAddress(uint32_t addr, InstructionExecutionInfo &execInfo) const;

        uint8_t memCycles32(uint8_t reg, bool seq) const
        {
            return cycles32Bit[bmap<uint8_t>(seq)][reg & 0xF];
        }
        uint8_t memCycles16(uint8_t reg, bool seq) const
        {
            return cycles16Bit[bmap<uint8_t>(seq)][reg & 0xF];
        }

        void setBiosState(uint32_t inst)
        {
            bios.forceBiosState(inst);
        }

        static MemoryRegion extractMemoryRegion(uint32_t addr);

      private:
        void scanROMForBackupID();
        uint32_t readOutOfROM(uint32_t addr) const;

        template <uint8_t bytes>
        uint8_t *resolveAddrRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const;
        template <bool read, uint8_t bytes>
        const uint8_t *resolveAddrRef(uint32_t &addr, InstructionExecutionInfo &execInfo) const;
    }; // namespace gbaemu
} // namespace gbaemu

#endif /* MEMORY_HPP */
