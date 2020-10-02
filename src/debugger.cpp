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

    void ExecutionHistory::dumpHistory() const
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

        uint32_t value = 0;

        InstructionExecutionInfo execInfo{0};
        switch (bitSize) {
            case 8:
                value = state.memory.read8(memAddr, execInfo);
            case 16:
                value = state.memory.read16(memAddr, execInfo);
            case 32:
                value = state.memory.read32(memAddr, execInfo);
        }

        prevMemValue = value;
        std::cout << "INFO memory trap triggered of addr: 0x" << std::hex << memAddr << " new Value: 0x" << std::hex << prevMemValue << " at PC: 0x" << std::hex << prevPC << std::endl;
    }

    bool MemoryChangeTrap::satisfied(uint32_t prevPC, uint32_t postPC, const Instruction &inst, const CPUState &state)
    {
        uint32_t value;

        InstructionExecutionInfo execInfo{0};
        switch (bitSize) {
            case 8:
                value = state.memory.read8(memAddr, execInfo);
            case 16:
                value = state.memory.read16(memAddr, execInfo);
            case 32:
                value = state.memory.read32(memAddr, execInfo);
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
            InstructionExecutionInfo execInfo{0};
            state.memory.normalizeAddressRef(postPC, execInfo);
            *setStepMode = execInfo.memReg == trapRegion;
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
        return state.getCPUMode() == trapMode;
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

#ifdef DEBUG_CLI
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
            std::cout << cpu.state.toString() << std::endl;
            std::cout << std::dec << cpuStepCount << " cpu steps" << std::endl;
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

            for (const auto &bp : breakpoints) {
                std::cout << "    0x" << std::hex << bp << std::endl;
            }

            return;
        }

        if (words[0] == "unbreak") {
            address_t where = cpu.state.accessReg(regs::PC_OFFSET);

            if (words.size() == 2)
                where = std::stoul(words[1], nullptr, 16);

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
            cpu.state.memory.memWatch.watchAddress(where, MemWatch::Condition{0, true, true, false, false});
            std::cout << "DebugCLI: Added watchpoint 0x" << std::hex << where << std::endl;

            return;
        }

        if (words[0] == "unwatch") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing address for watchpoint to remove." << std::endl;
                return;
            }

            address_t where = std::stoul(words[1], nullptr, 16);
            cpu.state.memory.memWatch.unwatchAddress(where);
            std::cout << "DebugCLI: Watchpoint 0x" << std::hex << where << " removed." << std::endl;

            return;
        }

        if (words[0] == "disas" || words[0] == "dis") {
            address_t where = cpu.state.accessReg(regs::PC_OFFSET);
            uint32_t howMuch = 16;

            if (words.size() >= 2)
                where = std::stoul(words[1], nullptr, 16);

            if (words.size() >= 3)
                howMuch = std::stoul(words[2]);

            std::cout << cpu.state.disas(where, howMuch) << std::endl;

            return;
        }

        if (words[0] == "regs" || words[0] == "r") {
            std::cout << cpu.state.toString() << std::endl;
            return;
        }

        if (words[0] == "breakpoints" || words[0] == "bps") {
            std::cout << getBreakpointInfo() << std::endl;
            return;
        }

        if (words[0] == "watchpoints" || words[0] == "wps") {
            std::cout << cpu.state.memory.memWatch.getWatchPointInfo() << std::endl;
            return;
        }

        if (words[0] == "step" || words[0] == "s") {
            exe1Step = true;
            state = RUNNING;
            return;
        }

        if (words[0] == "reset") {
            cpu.reset();
            cpu.initPipeline();
            return;
        }

        if (words[0] == "help" || words[0] == "h") {
            std::cout
                << "continue/con\nbreak/b [address] (defaults to PC)\nlistbreak/lb\nunbreak address\n"
                << "watch address\nunwatch address\ndisas/dis [address] [length] (defaults to PC)\n"
                << "regs/r\nbreakpoints/bps\nwatchpoints/wps\nstep/s\nreset\n"
                << "watchevents\nmem address [1/2/4] [count]" << std::endl;

            return;
        }

        if (words[0] == "watchevents") {
            std::cout << getWatchEventsInfo() << std::endl;
            return;
        }

        if (words[0] == "mem") {
            uint32_t unitSize = 4;

            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing address parameter." << std::endl;
                return;
            }

            if (words.size() >= 3) {
                if (words[2] == "1")
                    unitSize = 1;
                else if (words[2] == "2")
                    unitSize = 2;
                else if (words[2] == "4")
                    unitSize = 4;
            }

            size_t count = 1;
            address_t addr = std::stoul(words[1], nullptr, 16);

            if (words.size() >= 4)
                count = std::stoul(words[3], nullptr, 16);

            for (size_t i = 0; i < count; ++i) {
                uint32_t value = 0;

                switch (unitSize) {
                    case 1:
                        value = safeRead8(addr);
                        break;
                    case 2:
                        value = safeRead16(addr);
                        break;
                    case 4:
                        value = safeRead32(addr);
                        break;
                }

                std::cout << std::hex << "0x" << addr << "    0x" << value << "    " << std::dec << value << '\n';
                addr += unitSize;
            }

            return;
        }

        if (words[0] == "lcd") {
            std::cout << lcdController.getLayerStatusString() << std::endl;
            return;
        }

        if (words[0] == "obj") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing obj index parameter!" << std::endl;
                return;
            }

            uint32_t objIndex = std::stol(words[1]);
            //std::cout << lcdController.getOBJLayerString(objIndex) << std::endl;
            return;
        }

        if (words[0] == "bg") {
            if (words.size() < 2) {
                std::cout << "DebugCLI: Missing bg index parameter!" << std::endl;
                return;
            }

            uint32_t bgIndex = std::stol(words[1]);
            //std::cout << lcdController.getBGLayerString(bgIndex) << std::endl;
            return;
        }

        std::cout << "DebugCLI: Invalid command!" << std::endl;
    }

    DebugCLI::DebugCLI(CPU &cpuRef, lcd::LCDController &lcdRef) : cpu(cpuRef), lcdController(lcdRef), stepCount(0), cpuStepCount(0)
    {
        state = RUNNING;
        watchEvent.address = INVALID_ADDRESS;

        cpu.state.memory.memWatch.registerTrigger([&](address_t addr, const MemWatch::Condition &cond,
                                                      uint32_t oldValue, bool onWrite, uint32_t newValue) {
            watchEvent.address = addr;
            watchEvent.condition = cond;
            watchEvent.isWrite = onWrite;
            watchEvent.oldValue = oldValue;
            watchEvent.newValue = newValue;
        });
    }

    bool DebugCLI::step()
    {
        if (state == STOPPED)
            return false;

        if (state == RUNNING) {
            cpuExecutionMutex.lock();
            ++cpuStepCount;
            CPUExecutionInfoType executionInfo = cpu.step(1);
            if (executionInfo != CPUExecutionInfoType::NORMAL) {
                state = HALTED;
                std::cout << "CPU error occurred: " << cpu.executionInfo.message << std::endl;
            }
            cpuExecutionMutex.unlock();
        }

        address_t pc = cpu.state.accessReg(regs::PC_OFFSET);

        if (pc != prevPC) {
            if (state == RUNNING) {
                if (watchEvent.address != INVALID_ADDRESS) {
                    address_t addr = watchEvent.address;

                    if (watchEvent.isWrite)
                        watchEvents[watchEvent.address].incWrite(pc);
                    else
                        watchEvents[watchEvent.address].incRead(pc);

                    // clear event
                    watchEvent.address = INVALID_ADDRESS;
                }

                /*
                if (watchEvent.address != INVALID_ADDRESS) {
                    address_t addr = watchEvent.address;

                    cpuExecutionMutex.lock();
                    std::cout << "DebugCLI: watchpoint " << std::hex << addr << " reached" << '\n' <<
                        "old value: " << std::dec << watchEvent.oldValue << "    0x" << std::hex << watchEvent.oldValue << '\n';

                    if (watchEvent.isWrite)
                        std::cout << "new value: " << std::dec << watchEvent.newValue << "    0x" << std::hex << watchEvent.newValue << '\n';

                    std::cout << "acesses from " << std::hex << pc << std::endl;

                    state = STOPPED;
                    cpuExecutionMutex.unlock();

                    // clear event
                    watchEvent.address = INVALID_ADDRESS;
                }
                 */

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
        ++stepCount;

        return state == HALTED;
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

    uint8_t DebugCLI::safeRead8(address_t addr)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        auto value = cpu.state.memory.read8(addr, execInfo);
        watchEvent = backup;
        return value;
    }

    uint16_t DebugCLI::safeRead16(address_t addr)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        auto value = cpu.state.memory.read16(addr, execInfo);
        watchEvent = backup;
        return value;
    }

    uint32_t DebugCLI::safeRead32(address_t addr)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        auto value = cpu.state.memory.read32(addr, execInfo);
        watchEvent = backup;
        return value;
    }

    void DebugCLI::safeWrite8(address_t addr, uint8_t value)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        cpu.state.memory.write8(addr, value, execInfo);
        watchEvent = backup;
    }

    void DebugCLI::safeWrite16(address_t addr, uint16_t value)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        cpu.state.memory.write16(addr, value, execInfo);
        watchEvent = backup;
    }

    void DebugCLI::safeWrite32(address_t addr, uint32_t value)
    {
        WatchEvent backup = watchEvent;
        InstructionExecutionInfo execInfo{0};
        cpu.state.memory.write32(addr, value, execInfo);
        watchEvent = backup;
    }

    std::string DebugCLI::getBreakpointInfo() const
    {
        std::stringstream ss;
        ss << std::hex;

        for (address_t addr : breakpoints)
            ss << addr << '\n';

        return ss.str();
    }

    std::string DebugCLI::getWatchEventsInfo() const
    {
        std::stringstream ss;

        for (const auto &eventCounter : watchEvents) {
            ss << std::hex << "0x" << eventCounter.first << '\n';

            for (const auto &read : eventCounter.second.reads)
                ss << " read by " << std::hex << "0x" << read.first << " " << std::dec << read.second << " times\n";

            for (const auto &write : eventCounter.second.writes)
                ss << " written by " << std::hex << "0x" << write.first << " " << std::dec << write.second << " times\n";
        }

        return ss.str();
    }

#endif
} // namespace gbaemu::debugger