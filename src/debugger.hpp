#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "cpu/cpu_state.hpp"
#include <vector>

namespace gbaemu::debugger
{
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

      public:
        MemoryChangeTrap(uint32_t memAddr, uint32_t minPcOffset, bool *stepMode) : memAddr(memAddr), stepMode(stepMode), minPcOffset(minPcOffset), prevMemValue(0) {}

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
        std::vector<Trap*> traps;

      public:
        void registerTrap(Trap &t);
        void check(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state);
    };
} // namespace gbaemu::debugger

#endif /* DEBUGGER_HPP */
