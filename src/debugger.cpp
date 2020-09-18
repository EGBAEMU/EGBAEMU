#include "debugger.hpp"
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

namespace gbaemu::debugger
{

    void ExecutionHistory::collect(CPU *cpu, uint32_t address)
    {
        entries.push_back(cpu->state.disas(address, 1));
        // How many elements to remove?
        int32_t overhang = entries.size() - historySize;

        if (overhang > 0) {
            entries.erase(entries.begin(), entries.begin() + overhang);
        }
    }

    void ExecutionHistory::dumpHistory(CPU *cpu) const
    {
        // Just dump every stuff
        for (const auto &inst : entries) {
            std::cout << inst;
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

        uint32_t value;

        switch (bitSize) {
            case 8:
                value = state.memory.read8(memAddr, nullptr);
            case 16:
                value = state.memory.read16(memAddr, nullptr);
            case 32:
                value = state.memory.read32(memAddr, nullptr);
        }

        prevMemValue = value;
        std::cout << "INFO memory trap triggered of addr: 0x" << std::hex << memAddr << " new Value: 0x" << std::hex << prevMemValue << " at PC: 0x" << std::hex << prevPC << std::endl;
    }

    bool MemoryChangeTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        uint32_t value;

        switch (bitSize) {
            case 8:
                value = state.memory.read8(memAddr, nullptr);
            case 16:
                value = state.memory.read16(memAddr, nullptr);
            case 32:
                value = state.memory.read32(memAddr, nullptr);
            default:
                return false;
        }

        return minPcOffset < postPC && value != prevMemValue;
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

    /* DebugCLI */

    void DebugCLI::executeInput(const std::string &line)
    {
        std::istringstream iss(line);
        std::vector<std::string> words((std::istream_iterator<std::string>(iss)),
                                       std::istream_iterator<std::string>());

        if (words.size() == 0)
            return;

        if (words[0] == "continue" || words[0] == "con" || words[0] == "c") {
            if (state == HALTED) {
                std::cout << "DebugCLI: CPU is an unrecoverable state and cannot continue running." << std::endl;
                return;
            }

            std::cout << "DebugCLI: continuing..." << std::endl;
            state = RUNNING;

            return;
        }

        if (words[0] == "break" || words[0] == "b") {
            if (words.size() == 1) {
                state = STOPPED;
                return;
            }

            address_t where = std::stoul(words[1], nullptr, 16);

            breakpoints.insert(where);
            std::cout << "DebugCLI: Added breakpoint at 0x" << std::hex << where << std::endl;

            return;
        }

        if (words[0] == "listbreak" || words[0] == "lb") {
            std::cout << "DebugCLI: Breakpoints: " << std::endl;

            for (const auto& bp : breakpoints) {
                std::cout << "    0x" << std::hex << bp << std::endl;
            }

            return;
        }

        if (words[0] == "unbreak") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing address of breakpoint to remove." << std::endl;
                return;
            }

            address_t where = std::stoul(words[1], nullptr, 16);

            auto result = breakpoints.find(where);

            if (result != breakpoints.end()) {
                breakpoints.erase(*result);
                std::cout << "DebugCLI: Removed breakpoint 0x" << std::hex << where << std::endl;
            } else {
                std::cout << "DebugCLI: No such breakpoint." << std::endl;
            }

            return;
        }

        if (words[0] == "watch") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing address for watchpoint." << std::endl;
                return;
            }

            address_t where = std::stoul(words[1], nullptr, 16);

            watchpoints.insert(where);
            std::cout << "DebugCLI: Added watchpoint 0x" << std::hex << where << std::endl;

            return;
        }

        if (words[0] == "unwatch") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing address for watchpoint to remove." << std::endl;
                return;
            }

            address_t where = std::stoul(words[1], nullptr, 16);
            auto result = watchpoints.find(where);

            if (result == watchpoints.end()) {
                std::cout << "DebugCLI: No such watchpoint." << std::endl;
            } else {
                watchpoints.erase(result);
                std::cout << "DebugCLI: Watchpoint 0x" << std::hex << where << " removed." << std::endl;
            }

            return;
        }

        if (words[0] == "disas") {
            address_t where = cpu.state.accessReg(regs::PC_OFFSET);
            uint32_t howMuch = 16;

            if (words.size() >= 2)
                where = std::stoul(words[1], nullptr, 16);

            if (words.size() >= 3)
                howMuch = std::stoul(words[1]);

            std::cout << cpu.state.disas(where, howMuch) << std::endl;

            return;
        }

        if (words[0] == "regs") {
            std::cout << cpu.state.toString() << std::endl;
            return;
        }

        if (words[0] == "step" || words[0] == "s") {
            exe1Step = true;
            state = RUNNING;
            return;
        }

        if (words[0] == "help" || words[0] == "h") {
        }

        std::cout << "DebugCLI: Invalid command!" << std::endl;
    }

    DebugCLI::DebugCLI(CPU& cpuRef) : cpu(cpuRef)
    {
        state = RUNNING;
    }

    void DebugCLI::step()
    {
        if (state == STOPPED)
            return;

        /* The CPU is stopped so we need the user something to do. */
        if (state == RUNNING) {
            cpuExecutionMutex.lock();
            cpu.step();
            cpuExecutionMutex.unlock();
        }

        address_t pc = cpu.state.accessReg(regs::PC_OFFSET);

        if (pc != prevPC) {
            if (state == RUNNING) {
                if (exe1Step) {
                    cpuExecutionMutex.lock();
                    std::cout << "DebugCLI: step executed" << std::endl;
                    state = STOPPED;
                    exe1Step = false;
                    cpuExecutionMutex.unlock();
                }
                if (breakpoints.find(pc) != breakpoints.end()) {
                    cpuExecutionMutex.lock();
                    std::cout << "DebugCLI: breakpoint 0x" << std::hex << pc << " reached" << std::endl;
                    state = STOPPED;
                    cpuExecutionMutex.unlock();
                }
            }
        }

        prevPC = pc;
    }

    DebugCLI::State DebugCLI::getState() const
    {
        return state;
    }

    void DebugCLI::passCommand(const std::string &line)
    {
        cpuExecutionMutex.lock();
        executeInput(line);
        cpuExecutionMutex.unlock();
    }
} // namespace gbaemu::debugger