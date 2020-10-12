#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "bios.hpp"
#include "io_regs.hpp"
#include "logging.hpp"
#include "memory_defs.hpp"
#include "rom.hpp"
#include "util.hpp"
#include "vram.hpp"
#include <cstdint>
#include <functional>
#ifdef DEBUG_CLI
#include <map>
#include <set>
#endif

namespace gbaemu
{
    struct InstructionExecutionInfo;
    class ROM;

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
        uint8_t *oam;

        ROM rom;
        Bios bios;
        VRAM vram;
        IO_Handler ioHandler;

        // needed to handle read from unused memory regions
        const std::function<uint32_t()> readUnusedHandle;

#ifdef DEBUG_CLI
        /* We are going to expose this directly becaus this cannot be in an invalid state and I don't have time. */
        MemWatch memWatch;
#endif

        Memory(std::function<uint32_t()> readUnusedHandle);

        void reset();

        void updateWaitCycles(uint16_t WAITCNT);

        ~Memory();

        Memory(const Memory &) = delete;
        Memory &operator=(const Memory &) = delete;

        bool loadROM(const char *saveFilePath, const uint8_t *rom, size_t romSize)
        {
            reset();
            return this->rom.loadROM(saveFilePath, rom, romSize);
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
            return rom.getRomSize();
        }

        uint8_t read8(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false) const;
        uint16_t read16(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        uint32_t read32(uint32_t addr, InstructionExecutionInfo &execInfo, bool seq = false, bool readInstruction = false, bool dmaRequest = false) const;
        void write8(uint32_t addr, uint8_t value, InstructionExecutionInfo &execInfo, bool seq = false);
        void write16(uint32_t addr, uint16_t value, InstructionExecutionInfo &execInfo, bool seq = false);
        void write32(uint32_t addr, uint32_t value, InstructionExecutionInfo &execInfo, bool seq = false);

        uint8_t memCycles32(memory::MemoryRegion reg, bool seq) const
        {
            return cycles32Bit[bmap<uint8_t>(seq)][reg & 0xF];
        }
        uint8_t memCycles16(memory::MemoryRegion reg, bool seq) const
        {
            return cycles16Bit[bmap<uint8_t>(seq)][reg & 0xF];
        }

        void setBiosState(uint32_t inst)
        {
            bios.forceBiosState(inst);
        }

        static memory::MemoryRegion extractMemoryRegion(uint32_t addr);
    };
} // namespace gbaemu

#endif /* MEMORY_HPP */
