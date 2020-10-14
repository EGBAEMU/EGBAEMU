#include "rom.hpp"
#include "memory.hpp"
#include "util.hpp"

#include <iostream>

namespace gbaemu
{
    ROM::~ROM()
    {
        if (rom) {
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
        romSize = 0;
        rom = nullptr;
        ext_sram = nullptr;
        eeprom = nullptr;
        flash = nullptr;
    }

    void ROM::reset()
    {
        if (eeprom) {
            eeprom->reset();
        }
        if (flash) {
            flash->reset();
        }
    }

    bool ROM::loadROM(const char *saveFilePath, const uint8_t *rom, size_t romSize)
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
        uint8_t *newRom = new uint8_t[this->romSize];
        std::copy_n(rom, romSize, newRom);
        this->rom = newRom;

        BackupID backupType = scanROMForBackupID();

        bool loadSuccessful = true;
        if (backupType == SRAM_V) {
            // Allocate needed memory
            this->ext_sram = new save::SRAM(saveFilePath, loadSuccessful, backupSizes[backupType]);
        } else if (backupType == EEPROM_V) {
            // EEPROM size is determinable via the size of the dma requests
            this->eeprom = new save::EEPROM(saveFilePath, loadSuccessful);
        } else if (backupType >= FLASH_V) {
            this->flash = new save::FLASH(saveFilePath, loadSuccessful, backupSizes[backupType]);
        }

        return loadSuccessful;
    }

    ROM::BackupID ROM::scanROMForBackupID()
    {
        // reset backup type
        BackupID backupType = NO_BACKUP;

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
        std::copy_n(parsingStrs, sizeof(parsingStrs) / sizeof(parsingStrs[0]), currentParsingState);

        for (const uint8_t *romIt = rom; romIt + 3 < rom + romSize;) {
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
                            return backupType;
                        }
                    } else {
                        // reset
                        currentParsingState[k] = parsingStrs[k];
                    }
                }
            }
        }

        std::cout << "INFO: No backup id was found!" << std::endl;
        return backupType;
    }

    uint32_t ROM::readOutOfROM(uint32_t addr)
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

    uint8_t ROM::read8SRAM(uint32_t addr) const
    {
        // The 64K SRAM area is mirrored across the whole 32MB area at E000000h-FFFFFFFh,
        //also, inside of the 64K SRAM field, 32K SRAM chips are repeated twice
        // First handle 64K mirroring! & ensure we start with EXT_SRAM offset
        addr = (addr & ((static_cast<uint32_t>(64) << 10) - 1)) | memory::EXT_SRAM_OFFSET;

        if (ext_sram) {
            return ext_sram->read8(addr);
        } else if (flash) {
            return flash->read(addr);
        }
        return 0xFF;
    }
    uint16_t ROM::read16SRAM(uint32_t addr) const
    {
        uint16_t data = read8SRAM(addr);
        return data | (data << 8);
    }

    uint32_t ROM::read32SRAM(uint32_t addr) const
    {
        uint32_t data = read8SRAM(addr);
        return data | (data << 8) | (data << 16) | (data << 24);
    }

    uint8_t ROM::read8(uint32_t addr) const
    {
        uint32_t romOffset = addr & 0x00FFFFFF;
        if (romOffset < getRomSize()) {
            return rom[romOffset];
        } else {
            return readOutOfROM(addr) >> ((addr & 3) << 3);
        }
    }
    uint16_t ROM::read16(uint32_t addr) const
    {
        uint32_t romOffset = addr & 0x00FFFFFE;
        if (romOffset + 1 < getRomSize()) {
            return le(*reinterpret_cast<const uint16_t *>(rom + romOffset));
        } else {
            return readOutOfROM(addr) >> ((addr & 2) << 3);
        }
    }
    uint32_t ROM::read32(uint32_t addr) const
    {
        uint32_t romOffset = addr & 0x00FFFFFC;
        if (romOffset + 3 < getRomSize()) {
            return le(*reinterpret_cast<const uint32_t *>(rom + romOffset));
        } else {
            return readOutOfROM(addr);
        }
    }

    uint8_t ROM::read8ROM3_(uint32_t addr) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            return 0x01;
        } else {
            return read8(addr);
        }
    }
    uint16_t ROM::read16ROM3_(uint32_t addr) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            return 0x01;
        } else {
            return read16(addr);
        }
    }
    uint16_t ROM::read16ROM3_DMA(uint32_t addr) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            return eeprom->read();
        } else {
            return read16(addr);
        }
    }
    uint32_t ROM::read32ROM3_(uint32_t addr) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            return 0x01;
        } else {
            return read32(addr);
        }
    }
    uint32_t ROM::read32ROM3_DMA(uint32_t addr) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            return eeprom->read();
        } else {
            return read32(addr);
        }
    }

    // Needed for backup media
    void ROM::writeROM3_(uint32_t addr, uint8_t value) const
    {
        if (eeprom && isAddrEEPROM(addr)) {
            eeprom->write(value);
        }
    }

    void ROM::write8SRAM(uint32_t addr, uint8_t value) const
    {
        if (ext_sram) {
            ext_sram->write8(addr, value);
        } else if (flash) {
            flash->write(addr, value);
        }
    }
    void ROM::write16SRAM(uint32_t addr, uint16_t value) const
    {
        write8SRAM(addr, value >> ((addr & 1) << 3));
    }
    void ROM::write32SRAM(uint32_t addr, uint32_t value) const
    {
        write8SRAM(addr, value >> ((addr & 3) << 3));
    }

    bool ROM::eepromNeedsInit() const
    {
        return eeprom && !eeprom->knowsBitWidth();
    }

    // Should only be used if its known that addr is in EXT_ROM3_ address space or isRegEEPROM returned true
    bool ROM::isAddrEEPROM(uint32_t addr) const
    {
        // The address internal to EXT3
        uint32_t internalAddress = addr & 0x00FFFFFF;
        return (romSize <= 0x01000000 || internalAddress >= 0x00FFFF00);
    }
    bool ROM::isRegEEPROM(uint32_t addr)
    {
        return (addr >> 24) == memory::EXT_ROM3_;
    }

    void ROM::initEEPROM(uint32_t srcAddr, uint32_t destAddr, uint32_t count) const
    {
        if (isRegEEPROM(srcAddr) && isAddrEEPROM(srcAddr)) {
            const constexpr uint32_t bus14BitReadExpectedCount = 17;
            const constexpr uint32_t bus6BitReadExpectedCount = 9;
            if (count == bus14BitReadExpectedCount) {
                eeprom->expand(14);
                return;
            } else if (count == bus6BitReadExpectedCount) {
                eeprom->expand(6);
                return;
            }
        }

        if (isRegEEPROM(destAddr) && isAddrEEPROM(destAddr)) {
            const constexpr uint32_t bus14BitWriteExpectedCount = 81;
            const constexpr uint32_t bus6BitWriteExpectedCount = 73;
            if (count == bus14BitWriteExpectedCount) {
                eeprom->expand(14);
            } else if (count == bus6BitWriteExpectedCount) {
                eeprom->expand(6);
            }
        }
    }
} // namespace gbaemu
