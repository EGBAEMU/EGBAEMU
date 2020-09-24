#include "cpu_state.hpp"

#include "decode/inst.hpp"
#include "regs.hpp"
#include "util.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace gbaemu
{

    CPUState::CPUState()
    {
        reset();
    }

    void CPUState::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        std::fill_n(reinterpret_cast<char *>(&pipeline), sizeof(pipeline), 0);

        // Ensure that system mode is also set in CPSR register!
        regs.CPSR = 0b11111;
        updateCPUMode();

        memory.reset();
    }

    const char *CPUState::cpuModeToString() const
    {
        switch (mode) {
            STRINGIFY_CASE_ID(UserMode);
            STRINGIFY_CASE_ID(FIQ);
            STRINGIFY_CASE_ID(IRQ);
            STRINGIFY_CASE_ID(SupervisorMode);
            STRINGIFY_CASE_ID(AbortMode);
            STRINGIFY_CASE_ID(UndefinedMode);
            STRINGIFY_CASE_ID(SystemMode);
        }
        return "UNKNOWN";
    }

    CPUState::~CPUState()
    {
    }

    uint32_t CPUState::getCurrentPC() const
    {
        // PC register is not banked!
        return regs.rx[regs::PC_OFFSET];
    }

    uint32_t *const *const CPUState::getModeRegs(CPUMode cpuMode)
    {
        return regsHacks[cpuMode];
    }

    const uint32_t *const *const CPUState::getModeRegs(CPUMode cpuMode) const
    {
        return regsHacks[cpuMode];
    }

    uint32_t &CPUState::accessReg(uint8_t offset)
    {
        return *(getCurrentRegs()[offset]);
    }

    uint32_t CPUState::accessReg(uint8_t offset) const
    {
        return *(getCurrentRegs()[offset]);
    }

    void CPUState::setFlag(size_t flag, bool value)
    {
        if (value)
            accessReg(regs::CPSR_OFFSET) |= (1 << flag);
        else
            accessReg(regs::CPSR_OFFSET) &= ~(1 << flag);
    }

    bool CPUState::getFlag(size_t flag) const
    {
        return accessReg(regs::CPSR_OFFSET) & (1 << flag);
    }

    bool CPUState::updateCPUMode()
    {
        /*
            The Mode Bits M4-M0 contain the current operating mode.
                    Binary Hex Dec  Expl.
                    0xx00b 00h 0  - Old User       ;\26bit Backward Compatibility modes
                    0xx01b 01h 1  - Old FIQ        ; (supported only on ARMv3, except ARMv3G,
                    0xx10b 02h 2  - Old IRQ        ; and on some non-T variants of ARMv4)
                    0xx11b 03h 3  - Old Supervisor ;/
                    10000b 10h 16 - User (non-privileged)
                    10001b 11h 17 - FIQ
                    10010b 12h 18 - IRQ
                    10011b 13h 19 - Supervisor (SWI)
                    10111b 17h 23 - Abort
                    11011b 1Bh 27 - Undefined
                    11111b 1Fh 31 - System (privileged 'User' mode) (ARMv4 and up)
            Writing any other values into the Mode bits is not allowed. 
            */
        uint8_t modeBits = regs.CPSR & cpsr_flags::MODE_BIT_MASK & 0xF;
        bool error = false;
        switch (modeBits) {
            case 0b0000:
                mode = CPUState::UserMode;
                break;
            case 0b0001:
                mode = CPUState::FIQ;
                break;
            case 0b0010:
                mode = CPUState::IRQ;
                break;
            case 0b0011:
                mode = CPUState::SupervisorMode;
                break;
            case 0b0111:
                mode = CPUState::AbortMode;
                break;
            case 0b1011:
                mode = CPUState::UndefinedMode;
                break;
            case 0b1111:
                mode = CPUState::SystemMode;
                break;

            default:
                error = true;
                break;
        }

        currentRegs = regsHacks[mode];

        return error;
    }

    std::string CPUState::toString() const
    {
        std::stringstream ss;

        /* general purpose registers */
        for (uint32_t i = 0; i < 18; ++i) {
            /* name */
            ss << "r" << std::dec << i << ' ';

            if (i == regs::PC_OFFSET)
                ss << "(PC) ";
            else if (i == regs::LR_OFFSET)
                ss << "(LR) ";
            else if (i == regs::SP_OFFSET)
                ss << "(SP) ";
            else if (i == regs::CPSR_OFFSET)
                ss << "(CPSR) ";
            else if (i == regs::SPSR_OFFSET)
                ss << "(SPSR) ";

            ss << "    ";

            /* value */
            uint32_t value = accessReg(i);
            /* TODO: show fixed point */
            ss << std::dec << value << " = 0x" << std::hex << value << '\n';
        }

        /* flag registers */
        ss << "N=" << getFlag(cpsr_flags::N_FLAG) << ' ' << "Z=" << getFlag(cpsr_flags::Z_FLAG) << ' ' << "C=" << getFlag(cpsr_flags::C_FLAG) << ' ' << "V=" << getFlag(cpsr_flags::V_FLAG) << '\n';
        // Cpu Mode
        ss << "CPU Mode: " << cpuModeToString() << '\n';
        ss << "IRQ Req Reg: 0x" << std::hex << memory.ioHandler.internalRead16(Memory::IO_REGS_OFFSET + 0x202) << '\n';

        return ss.str();
    }

    std::string CPUState::printStack(uint32_t words) const
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::hex;

        ss << "Stack:\n";
        for (uint32_t stackAddr = accessReg(regs::SP_OFFSET); words > 0; --words) {
            /* address, pad hex numbers with 0 */
            ss << "0x" << std::setw(8) << stackAddr << ":    "
               << "0x" << std::setw(8) << memory.read32(stackAddr, nullptr) << '\n';

            stackAddr += 4;
        }

        return ss.str();
    }

    std::string CPUState::disas(uint32_t addr, uint32_t cmds) const
    {
        std::stringstream ss;

        ss << std::setfill('0') << std::hex;

        uint32_t startAddr = addr - (cmds / 2) * (getFlag(cpsr_flags::THUMB_STATE) ? 2 : 4);
        //if (startAddr < Memory::MemoryRegionOffset::EXT_ROM_OFFSET) {
        //    startAddr = Memory::MemoryRegionOffset::EXT_ROM_OFFSET;
        //}

        for (uint32_t i = startAddr; cmds > 0; --cmds) {

            // indicate executed instruction
            //if (i == addr)
            //    ss << "<- ";

            // indicate current instruction
            if (i == accessReg(regs::PC_OFFSET))
                ss << "=> ";

            // address, pad hex numbers with 0
            ss << "0x" << std::setw(8) << i << "    ";

            if (getFlag(cpsr_flags::THUMB_STATE)) {
                uint32_t bytes = memory.read16(i, nullptr, false, true);

                uint32_t b0 = bytes & 0x0FF;
                uint32_t b1 = (bytes >> 8) & 0x0FF;

                Instruction inst;
                inst.inst = bytes;
                inst.isArm = false;

                // bytes
                ss << std::setw(2) << b0 << ' ' << std::setw(2) << b1 << ' ' << " [" << std::setw(4) << bytes << ']';

                // code
                ss << "    " << inst.toString() << '\n';

                i += 2;
            } else {
                uint32_t bytes = memory.read32(i, nullptr, false, true);

                uint32_t b0 = bytes & 0x0FF;
                uint32_t b1 = (bytes >> 8) & 0x0FF;
                uint32_t b2 = (bytes >> 16) & 0x0FF;
                uint32_t b3 = (bytes >> 24) & 0x0FF;

                Instruction inst;
                inst.inst = bytes;
                inst.isArm = true;

                // bytes
                ss << std::setw(2) << b0 << ' ' << std::setw(2) << b1 << ' ' << std::setw(2) << b2 << ' ' << std::setw(2) << b3 << " [" << std::setw(8) << bytes << ']';

                // code
                ss << "    " << inst.toString() << '\n';

                i += 4;
            }
        }

        return ss.str();
    }

} // namespace gbaemu
