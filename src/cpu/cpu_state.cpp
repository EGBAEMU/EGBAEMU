#include "cpu_state.hpp"

#include "decode/inst.hpp"
#include "regs.hpp"
#include "util.hpp"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>

namespace gbaemu
{

    CPUState::CPUState() : memory(std::bind(&CPUState::handleReadUnused, this))
    {
        reset();
    }

    void CPUState::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        std::fill_n(reinterpret_cast<char *>(&pipeline), sizeof(pipeline), 0);

        // Ensure that system mode is also set in CPSR register!
        thumbMode = false;
        regs.CPSR = 0b11111;
        updateCPUMode();

        memory.reset();

        /*
        Default memory usage at 03007FXX (and mirrored to 03FFFFXX)
          Addr.    Size Expl.
          3007FFCh 4    Pointer to user IRQ handler (32bit ARM code)
          3007FF8h 2    Interrupt Check Flag (for IntrWait/VBlankIntrWait functions)
          3007FF4h 4    Allocated Area
          3007FF0h 4    Pointer to Sound Buffer
          3007FE0h 16   Allocated Area
          3007FA0h 64   Default area for SP_svc Supervisor Stack (4 words/time)
          3007F00h 160  Default area for SP_irq Interrupt Stack (6 words/time)
        Memory below 7F00h is free for User Stack and user data. The three stack pointers are initially initialized at the TOP of the respective areas:
          SP_svc=03007FE0h
          SP_irq=03007FA0h
          SP_usr=03007F00h
        The user may redefine these addresses and move stacks into other locations, however, the addresses for system data at 7FE0h-7FFFh are fixed.
        */
        // Set default SP values
        *getModeRegs(CPUState::CPUMode::UserMode)[regs::SP_OFFSET] = 0x03007F00;
        *getModeRegs(CPUState::CPUMode::FIQ)[regs::SP_OFFSET] = 0x03007F00;
        *getModeRegs(CPUState::CPUMode::AbortMode)[regs::SP_OFFSET] = 0x03007F00;
        *getModeRegs(CPUState::CPUMode::UndefinedMode)[regs::SP_OFFSET] = 0x03007F00;
        *getModeRegs(CPUState::CPUMode::SupervisorMode)[regs::SP_OFFSET] = 0x03007FE0;
        *getModeRegs(CPUState::CPUMode::IRQ)[regs::SP_OFFSET] = 0x3007FA0;

        std::fill_n(reinterpret_cast<char *>(&cpuInfo), sizeof(cpuInfo), 0);
        std::fill_n(reinterpret_cast<char *>(&dmaInfo), sizeof(dmaInfo), 0);
        std::fill_n(reinterpret_cast<char *>(&fetchInfo), sizeof(fetchInfo), 0);

        accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
        fetchInfo.memReg = Memory::MemoryRegion::EXT_ROM1;
    }

    uint32_t CPUState::handleReadUnused() const
    {
        uint32_t value = pipeline[0];

        // See: http://problemkaputt.de/gbatek.htm#gbaunpredictablethings reading from unused memory
        if (getFlag<cpsr_flags::THUMB_STATE>()) {
            // Ugly case for THUMB: depends on alignment of PC & its memory region
            uint32_t currentPC = getCurrentPC();
            switch (Memory::extractMemoryRegion(currentPC)) {
                case Memory::BIOS:
                case Memory::OAM:
                    if (currentPC & 3) {
                        // Not word aligned pc
                        value = (value << 16) | pipeline[1];
                    } else {
                        // word aligned pc
                        InstructionExecutionInfo _waste = fetchInfo;
                        value |= memory.read16(currentPC + 6, _waste, true, true) << 16;
                    }
                    break;
                case Memory::IWRAM:
                    if (currentPC & 3) {
                        // Not word aligned pc
                        value = (value << 16) | pipeline[1];
                    } else {
                        // word aligned pc
                        value |= pipeline[1] << 16;
                    }
                    break;
                default:
                    value |= value << 16;
                    break;
            }

        } //else {
        // Trivial case for ARM: return latest fetched instruction (here pipeline[0]) -> nothing to do here
        //}
        return value;
    }

    uint32_t CPUState::propagatePipeline(uint32_t pc)
    {
        // propagate pipeline
        uint32_t currentInst = pipeline[1];
        pipeline[1] = pipeline[0];

        if (thumbMode) {
            pc += 4;
            pipeline[0] = memory.read16(pc, fetchInfo, true, true);
        } else {
            pc += 8;
            pipeline[0] = memory.read32(pc, fetchInfo, true, true);

            // auto update bios state if we are currently executing inside bios!
            if (pc < memory.getBiosSize()) {
                memory.setBiosState(pipeline[0]);
            }
        }

        return currentInst;
    }

    uint32_t CPUState::normalizePC()
    {
        return accessReg(regs::PC_OFFSET) = memory.normalizeAddress(accessReg(regs::PC_OFFSET) & (thumbMode ? 0xFFFFFFFE : 0xFFFFFFFC), fetchInfo);
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
        ss << "N=" << getFlag<cpsr_flags::N_FLAG>() << ' ' << "Z=" << getFlag<cpsr_flags::Z_FLAG>() << ' ' << "C=" << getFlag<cpsr_flags::C_FLAG>() << ' ' << "V=" << getFlag<cpsr_flags::V_FLAG>() << '\n';
        // Cpu Mode
        ss << "CPU Mode: " << cpuModeToString() << '\n';
        ss << "IRQ Req Reg: 0x" << std::hex << memory.ioHandler.internalRead16(Memory::IO_REGS_OFFSET + 0x202) << '\n';
        ss << "IRQ IE Reg: 0x" << std::hex << memory.ioHandler.internalRead16(Memory::IO_REGS_OFFSET + 0x200) << '\n';
        ss << "IRQ EN CPSR: " << ((accessReg(regs::CPSR_OFFSET) & (1 << 7)) == 0) << std::endl;
        ss << "IRQ EN MASTER: 0x" << std::hex << memory.ioHandler.internalRead16(Memory::IO_REGS_OFFSET + 0x208) << std::endl;

        return ss.str();
    }

    std::string CPUState::printStack(uint32_t words) const
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::hex;

        ss << "Stack:\n";

        InstructionExecutionInfo info;
        info.resolvedAddr = false;
        for (uint32_t stackAddr = accessReg(regs::SP_OFFSET); words > 0; --words) {
            /* address, pad hex numbers with 0 */
            ss << "0x" << std::setw(8) << stackAddr << ":    "
               << "0x" << std::setw(8) << memory.read32(stackAddr, info) << '\n';

            stackAddr += 4;
        }

        return ss.str();
    }

    std::string CPUState::disas(uint32_t addr, uint32_t cmds) const
    {
        std::stringstream ss;

        ss << std::setfill('0') << std::hex;

        uint32_t startAddr = addr - (cmds / 2) * (getFlag<cpsr_flags::THUMB_STATE>() ? 2 : 4);

        InstructionExecutionInfo info;
        info.resolvedAddr = false;
        for (uint32_t i = startAddr; cmds > 0; --cmds) {

            // indicate executed instruction
            //if (i == addr)
            //    ss << "<- ";

            // indicate current instruction
            if (i == accessReg(regs::PC_OFFSET))
                ss << "=> ";

            // address, pad hex numbers with 0
            ss << "0x" << std::setw(8) << i << "    ";

            if (getFlag<cpsr_flags::THUMB_STATE>()) {
                uint32_t bytes = memory.read16(i, info, false, true);

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
                uint32_t bytes = memory.read32(i, info, false, true);

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
                ss << "    " << '(' << conditionCodeToString(static_cast<ConditionOPCode>(b3 >> 4)) << ") " << inst.toString() << '\n';

                i += 4;
            }
        }

        return ss.str();
    }

} // namespace gbaemu
