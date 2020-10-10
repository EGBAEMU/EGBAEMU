#ifndef ROM_HPP
#define ROM_HPP

#include <cstdint>

#include "memory_defs.hpp"
#include "save/eeprom.hpp"
#include "save/flash.hpp"
#include "save/sram.hpp"

namespace gbaemu
{
    class ROM
    {
      public:
        enum BackupID : uint8_t {
            NO_BACKUP = 0, // Not sure if this is allowed?
            EEPROM_V,      // 512 bytes or 8 KiB
            SRAM_V,        // 32 KiB
            // Order is important: no non flash type shall be below this line!
            FLASH_V,    // 64 KiB
            FLASH512_V, // 64 KiB
            FLASH1M_V   // 128 KiB
        };

        static const constexpr uint32_t backupSizes[] = {
            0,                               // NO BACKUP TYPE
            /*EEPROM_V_SIZE = */ 8 << 10,    // 512 bytes or 8 KiB
            /*SRAM_V_SIZE = */ 32 << 10,     // 32 KiB
            /*FLASH_V_SIZE = */ 64 << 10,    // 64 KiB
            /*FLASH512_V_SIZE = */ 64 << 10, // 64 KiB
            /*FLASH1M_V_SIZE =*/128 << 10    // 128 KiB
        };

      private:
        const uint8_t *rom = nullptr;
        size_t romSize = 0;
        BackupID backupType = NO_BACKUP;

        save::EEPROM *eeprom = nullptr;
        save::FLASH *flash = nullptr;
        save::SRAM *ext_sram = nullptr;

      public:
        ~ROM();

        BackupID getBackupType() const
        {
            return backupType;
        }
        size_t getRomSize() const
        {
            return romSize;
        }

        void reset();

        bool loadROM(const char *saveFilePath, const uint8_t *rom, size_t romSize);

        uint8_t read8(uint32_t addr) const;
        uint16_t read16(uint32_t addr) const;
        uint32_t read32(uint32_t addr) const;
        uint8_t read8ROM3_(uint32_t addr) const;
        uint16_t read16ROM3_(uint32_t addr, bool dma) const;
        uint32_t read32ROM3_(uint32_t addr, bool dma) const;
        uint8_t read8SRAM(uint32_t addr) const;
        uint16_t read16SRAM(uint32_t addr) const;
        uint32_t read32SRAM(uint32_t addr) const;

        // Needed for backup media
        void writeROM3_(uint32_t addr, uint8_t value) const;
        void write8SRAM(uint32_t addr, uint8_t value) const;
        void write16SRAM(uint32_t addr, uint16_t value) const;
        void write32SRAM(uint32_t addr, uint32_t value) const;

      private:
        void scanROMForBackupID();

        static uint32_t readOutOfROM(uint32_t addr);
    };
} // namespace gbaemu

#endif