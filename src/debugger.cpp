#include "debugger.hpp"
#include <sstream>

namespace gbaemu::debugger
{
    bool JumpTrap::isLoop(uint32_t from, uint32_t to) const
    {
        if (jumps.size() == 0)
            return false;

        const auto &last = jumps.back();
        return (last.from == from) && (last.to == to);
    }

    void JumpTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {

        if (!isLoop(prevPC, postPC))
            jumps.push_back({inst, prevPC, postPC});
    }

    bool JumpTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return (prevPC != postPC) && (prevPC + 2 != postPC) && (prevPC + 4 != postPC);

        /*
        if (inst.isArm) {
            return (inst.arm.cat == arm::BRANCH) ||
                (inst.arm.cat == arm::BRANCH_XCHG);
        } else {
            return (inst.thumb.cat == thumb::COND_BRANCH) ||
                (inst.thumb.cat == thumb::LONG_BRANCH_WITH_LINK) ||
                (inst.thumb.cat == thumb::UNCONDITIONAL_BRANCH) ||
                (inst.thumb.cat == thumb::BR_XCHG);
        }
         */
    }

    std::string JumpTrap::toString() const
    {
        std::stringstream ss;
        ss << std::hex;

        for (auto e : jumps) {
            ss << e.inst.toString() << '\n';
            ss << e.from << " ----> " << e.to << '\n';
        }

        return ss.str();
    }

    void RegisterNonZeroTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *stepMode = true;
    }

    bool RegisterNonZeroTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return minPcOffset < postPC && state.accessReg(targetReg) != 0;
    }

    void AddressTrapTimesX::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *setStepMode = triggersNeeded == 0 || --triggersNeeded == 0;
    }

    bool AddressTrapTimesX::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return postPC == address;
    }

    void AddressTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *setStepMode = true;
    }

    bool AddressTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return postPC == address;
    }

    void CPUModeTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *stepMode = true;
    }

    bool CPUModeTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return state.mode == trapMode;
    }

    void Watchdog::registerTrap(Trap &t)
    {
        traps.push_back(&t);
    }

    void Watchdog::check(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        for (auto trap : traps)
            if (trap->satisfied(prevPC, postPC, inst, state))
                trap->trigger(prevPC, postPC, inst, state);
    }
} // namespace gbaemu::debugger