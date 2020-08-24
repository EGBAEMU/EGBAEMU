#ifndef MEMORY_HPP
#define MEMORY_HPP

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
#define GBA_MEM_CLEAR(arr, x) std::fill_n(arr, x##_LIMIT - x##_OFFSET, 0)

    class Memory
    {
      private:
        uint8_t *bios;
        uint8_t *wram;
        uint8_t *iwram;
        uint8_t *io_regs;
        uint8_t *bg_obj_ram;
        uint8_t *vram;
        uint8_t *oam;

        //TODO is external sram needed?
        uint8_t *ext_sram;
        uint8_t *rom;
        size_t romSize = 0;

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
            EXT_SRAM = 0x0E
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
            VRAM_LIMIT = 0x06017FFF,
            OAM_LIMIT = 0x070003FF,
            EXT_ROM1_LIMIT = 0x09FFFFFF,
            EXT_ROM2_LIMIT = 0x0BFFFFFF,
            EXT_ROM3_LIMIT = 0x0DFFFFFF,
            EXT_SRAM_LIMIT = 0x0E00FFFF
        };

      public:
        //TODO are there conventions about inital memory values?
        Memory()
        {
            bios = GBA_ALLOC_MEM_REG(BIOS);
            GBA_MEM_CLEAR(bios, BIOS);
            wram = GBA_ALLOC_MEM_REG(WRAM);
            GBA_MEM_CLEAR(wram, WRAM);
            iwram = GBA_ALLOC_MEM_REG(IWRAM);
            GBA_MEM_CLEAR(iwram, IWRAM);
            io_regs = GBA_ALLOC_MEM_REG(IO_REGS);
            GBA_MEM_CLEAR(io_regs, IO_REGS);
            bg_obj_ram = GBA_ALLOC_MEM_REG(BG_OBJ_RAM);
            GBA_MEM_CLEAR(bg_obj_ram, BG_OBJ_RAM);
            vram = GBA_ALLOC_MEM_REG(VRAM);
            GBA_MEM_CLEAR(vram, VRAM);
            oam = GBA_ALLOC_MEM_REG(OAM);
            GBA_MEM_CLEAR(oam, OAM);
            ext_sram = GBA_ALLOC_MEM_REG(EXT_SRAM);
            GBA_MEM_CLEAR(ext_sram, EXT_SRAM);
            rom = nullptr;
            romSize = 0;
        }

        ~Memory()
        {
            delete[] bios;
            delete[] wram;
            delete[] iwram;
            delete[] io_regs;
            delete[] bg_obj_ram;
            delete[] vram;
            delete[] oam;
            delete[] ext_sram;
            if (romSize)
                delete[] rom;
        }

        //TODO this would be too simple to work :D
        void loadROM(uint8_t *rom, size_t romSize)
        {
            if (this->romSize) {
                delete[] this->rom;
            }
            this->romSize = romSize;
            this->rom = new uint8_t[romSize];
            std::copy_n(rom, romSize, this->rom);
        }

        uint8_t read8(uint32_t addr, uint32_t *cycles) const;
        uint16_t read16(uint32_t addr, uint32_t *cycles) const;
        uint32_t read32(uint32_t addr, uint32_t *cycles) const;
        void write8(uint32_t addr, uint8_t value, uint32_t *cycles);
        void write16(uint32_t addr, uint16_t value, uint32_t *cycles);
        void write32(uint32_t addr, uint32_t value, uint32_t *cycles);

        // This is needed to handle memory mirroring
        const uint8_t *resolveAddr(uint32_t addr) const;
        uint8_t *resolveAddr(uint32_t addr);

        uint8_t nonSeqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const;
        uint8_t seqWaitCyclesForVirtualAddr(uint32_t address, uint8_t bytesToRead) const;
    };
} // namespace gbaemu

#endif /* MEMORY_HPP */
