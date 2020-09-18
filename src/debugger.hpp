#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "cpu/cpu.hpp"
#include "cpu/cpu_state.hpp"
#include <cassert>
#include <cstring>
#include <vector>
#include <memory>
#include <set>
#include <mutex>
#include <optional>

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
        void dumpHistory(CPU *cpu) const;

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
        MemoryChangeTrap(uint32_t memAddr, uint32_t minPcOffset, bool *stepMode, uint32_t initialMemValue = 0, size_t _bitSize = 32) :
            memAddr(memAddr), stepMode(stepMode), minPcOffset(minPcOffset), prevMemValue(initialMemValue), bitSize(_bitSize)
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

    typedef uint32_t address_t;
    static const constexpr address_t INVALID_ADDRESS = 0xFFFFFFFF;

    class DebugCLI
    {
    public:
        enum State {
            RUNNING,
            STOPPED,
            HALTED
        };
    private:
        CPU& cpu;
        State state;
        /* if true, step() waits for input */
        bool inputBlocking;
        bool acceptNewInput = true;

        address_t prevPC;

        std::mutex cpuExecutionMutex;
        uint64_t stepCount;

        std::optional<std::tuple<address_t, MemWatch::Condition, uint32_t, bool, uint32_t>> triggeredWatchpoint;

        void executeInput(const std::string& line);

        /* for code */
        std::set<address_t> breakpoints;
    public:
        DebugCLI(CPU& cpuRef);
        void step();
        /* These can be called from external threads. */
        State getState() const;
        void passCommand(const std::string& line);

        std::string getBreakpointInfo() const;
    };
} // namespace gbaemu::debugger

#endif /* DEBUGGER_HPP */
