#include "debugger.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

namespace gbaemu::debugger
{

    void ExecutionHistory::collect(CPU* cpu, uint32_t address) 
    {   

        entries.push_back(cpu->state.disas(address, 1));    
        // How many elements to remove?
        int32_t overhang = entries.size() - historySize;
      
        if (overhang > 0) {
            entries.erase(entries.begin(), entries.begin() + overhang);
        }

    }

    void ExecutionHistory::dumpHistory(CPU* cpu) const
    {   
        // Just dump every stuff
        for (unsigned i = 0; i < entries.size(); ++i) {
            std::cout << entries[i];
        }

    }

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

    void MemoryChangeTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *stepMode = true;
        prevMemValue = state.memory.read32(memAddr, nullptr);
        std::cout << "INFO memory trap triggered of addr: 0x" << std::hex << memAddr << " new Value: 0x" << std::hex << prevMemValue << " at PC: 0x" << std::hex << prevPC << std::endl;
    }

    bool MemoryChangeTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return minPcOffset < postPC && state.memory.read32(memAddr, nullptr) != prevMemValue;
    }

    void AddressTrapTimesX::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        *setStepMode = triggersNeeded == 0 || --triggersNeeded == 0;
    }

    bool AddressTrapTimesX::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return postPC == address;
    }

    void ExecutionRegionTrap::trigger(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        if (!*setStepMode) {
            Memory::MemoryRegion memReg;
            state.memory.normalizeAddress(postPC, memReg);
            *setStepMode = memReg == trapRegion;
        }
    }

    bool ExecutionRegionTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        return true;
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