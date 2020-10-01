#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "cpu/cpu.hpp"
#include "cpu/cpu_state.hpp"
#include "lcd/lcd-controller.hpp"
#include "logging.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

namespace gbaemu::debugger
{

    class ExecutionHistory
    {
      public:
        ExecutionHistory(uint32_t historySize) : historySize(historySize)
        {
            entries.reserve(historySize + 1);
        }

        void collect(CPU *cpu, uint32_t address);
        void dumpHistory() const;

      private:
        uint32_t historySize;
        std::vector<std::string> entries;
    };

    class Trap
    {
      public:
        virtual void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) = 0;
        virtual bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) = 0;
    };

    /* logs all branches, excluding loops */
    class JumpTrap : public Trap
    {
      public:
        struct JumpInfo {
            Instruction inst;
            uint32_t from, to;
        };

      private:
        std::vector<JumpInfo> jumps;

        bool isLoop(uint32_t from, uint32_t to) const;

      public:
        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        std::string toString() const;
    };

    class AddressTrap : public Trap
    {
      private:
        uint32_t address;
        bool *setStepMode;

      public:
        AddressTrap(uint32_t addr, bool *stepMode) : address(addr), setStepMode(stepMode) {}

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    class AddressTrapTimesX : public Trap
    {
      private:
        uint32_t address;
        uint32_t triggersNeeded;
        bool *setStepMode;

      public:
        AddressTrapTimesX(uint32_t addr, uint32_t triggersNeeded, bool *stepMode) : address(addr), triggersNeeded(triggersNeeded), setStepMode(stepMode) {}

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    class ExecutionRegionTrap : public Trap
    {
      private:
        Memory::MemoryRegion trapRegion;
        bool *setStepMode;

      public:
        ExecutionRegionTrap(Memory::MemoryRegion trapRegion, bool *stepMode) : trapRegion(trapRegion), setStepMode(stepMode) {}

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    class CPUModeTrap : public Trap
    {
      private:
        CPUState::CPUMode trapMode;
        bool *stepMode;

      public:
        CPUModeTrap(CPUState::CPUMode trapMode, bool *stepMode) : trapMode(trapMode), stepMode(stepMode) {}

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    class RegisterNonZeroTrap : public Trap
    {
      private:
        uint8_t targetReg;
        bool *stepMode;
        uint32_t minPcOffset;

      public:
        RegisterNonZeroTrap(uint8_t targetReg, uint32_t minPcOffset, bool *stepMode) : targetReg(targetReg), stepMode(stepMode), minPcOffset(minPcOffset) {}

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    class MemoryChangeTrap : public Trap
    {
      private:
        uint32_t memAddr;
        bool *stepMode;
        uint32_t minPcOffset;
        uint32_t prevMemValue;
        size_t bitSize;

      public:
        MemoryChangeTrap(uint32_t memAddr, uint32_t minPcOffset, bool *stepMode, uint32_t initialMemValue = 0, size_t _bitSize = 32) : memAddr(memAddr), stepMode(stepMode), minPcOffset(minPcOffset), prevMemValue(initialMemValue), bitSize(_bitSize)
        {
            assert(bitSize == 8 || bitSize == 16 || bitSize == 32);
        }

        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state) override;
    };

    /*
        A Watchdog checks at every instruction/CPU-state if any of the traps can trigger
        and calls their trigger function.
     */
    class Watchdog
    {
      private:
        std::vector<Trap *> traps;

      public:
        void registerTrap(Trap &t);
        void check(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state);
    };

#ifdef DEBUG_CLI
    class DebugCLI
    {
      public:
        enum State {
            RUNNING,
            STOPPED,
            HALTED
        };

        struct WatchEvent {
            address_t address;
            MemWatch::Condition condition;
            uint32_t oldValue;
            bool isWrite;
            uint32_t newValue;
        };

        struct WatchEventCounter {
            std::map<address_t, uint32_t> reads;
            std::map<address_t, uint32_t> writes;

            void incRead(address_t addr)
            {
                if (reads.find(addr) == reads.end())
                    reads[addr] = 0;

                ++reads[addr];
            }

            void incWrite(address_t addr)
            {
                if (writes.find(addr) == writes.end())
                    writes[addr] = 0;

                ++writes[addr];
            }
        };

      private:
        CPU &cpu;
        lcd::LCDController &lcdController;
        State state;

        bool exe1Step = false;

        address_t prevPC;

        std::mutex cpuExecutionMutex;
        uint64_t stepCount;
        uint64_t cpuStepCount;

        WatchEvent watchEvent;
        std::map<address_t, WatchEventCounter> watchEvents;

        /* for code */
        std::set<address_t> breakpoints;

        void executeInput(const std::string &line);

      public:
        DebugCLI(CPU &cpuRef, lcd::LCDController &lcdRef);
        bool step();
        /* These can be called from external threads. */
        State getState() const;
        void passCommand(const std::string &line);

        /* Use these when accessing memory, they don't trigger watchpoints. */
        uint8_t safeRead8(address_t addr);
        uint16_t safeRead16(address_t addr);
        uint32_t safeRead32(address_t addr);
        void safeWrite8(address_t addr, uint8_t value);
        void safeWrite16(address_t addr, uint16_t value);
        void safeWrite32(address_t addr, uint32_t value);

        std::string getBreakpointInfo() const;
        std::string getWatchEventsInfo() const;
    };
#endif
} // namespace gbaemu::debugger

#endif /* DEBUGGER_HPP */
