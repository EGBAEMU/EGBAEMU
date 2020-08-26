#ifndef CPU_STATE_HPP
#define CPU_STATE_HPP

#include "inst.hpp"
#include "memory.hpp"
#include "regs.hpp"
#include "util.hpp"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace gbaemu
{

    struct CPUState {
        /*
        https://problemkaputt.de/gbatek.htm#armcpuregisterset
        https://static.docs.arm.com/dvi0027/b/DVI_0027A_ARM7TDMI_PO.pdf
     */

        /*
        enum OperationState : uint8_t {
            ARMState,
            ThumbState
        } operationState = ARMState;
        */

        enum CPUMode : uint8_t {
            UserMode,
            FIQ,
            IRQ,
            SupervisorMode,
            AbortMode,
            UndefinedMode,
            SystemMode
        } mode = UserMode;

      private:
        //TODO are there conventions about inital reg values?
        struct Regs {
            uint32_t rx[16];
            uint32_t r8_14_fig[7];
            uint32_t r13_14_svc[2];
            uint32_t r13_14_abt[2];
            uint32_t r13_14_irq[2];
            uint32_t r13_14_und[2];
            uint32_t CPSR;
            uint32_t SPSR_fiq;
            uint32_t SPSR_svc;
            uint32_t SPSR_abt;
            uint32_t SPSR_irq;
            uint32_t SPSR_und;
        } regs = {0};

        // Complain to: tammo.muermann@stud.tu-darmstadt.de
        uint32_t *const regsHacks[7][18] = {
            // System / User mode regs
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.rx + 13, regs.rx + 14, regs.rx + 15, &regs.CPSR, &regs.CPSR},
            // FIQ mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.r8_14_fig, regs.r8_14_fig + 1, regs.r8_14_fig + 2, regs.r8_14_fig + 3, regs.r8_14_fig + 4, regs.r8_14_fig + 5, regs.r8_14_fig + 6, regs.rx + 15, &regs.CPSR, &regs.SPSR_fiq},
            // IRQ mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_irq, regs.r13_14_irq + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_irq},
            // Supervisor Mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_svc, regs.r13_14_svc + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_svc},
            // Abort mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_abt, regs.r13_14_abt + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_abt},
            // Undefined Mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_und, regs.r13_14_und + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_abt},
            // System / User mode regs
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.rx + 13, regs.rx + 14, regs.rx + 15, &regs.CPSR, &regs.CPSR}};

        const char *cpuModeToString() const
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

      public:
        /* pipeline */
        struct {
            struct {
                uint32_t lastInstruction;
                uint32_t lastReadData;

                uint32_t instruction;
                uint32_t readData;
            } fetch;

            struct {
                Instruction instruction;
                Instruction lastInstruction;
            } decode;
            /*
        struct {
            uint32_t result;
        } execute;
        */
        } pipeline = {0};

        Memory memory;

        const InstructionDecoder *decoder;

        uint32_t getCurrentPC()
        {
            // TODO: This is somewhat finshy as there are 3 active pc's due to pipelining. As the regs
            // only get modified by the EXECUTE stage, this will return the pc for the exec stage.
            // Fetch will be at +8 and decode at +4. Maybe encode this as option or so.
            return accessReg(regs::PC_OFFSET);
        }

        uint32_t *const *const getCurrentRegs()
        {
            return regsHacks[mode];
        }

        const uint32_t *const *const getCurrentRegs() const
        {
            return regsHacks[mode];
        }

        uint32_t *const *const getModeRegs(CPUMode mode)
        {
            return regsHacks[mode];
        }

        const uint32_t *const *const getModeRegs(CPUMode mode) const
        {
            return regsHacks[mode];
        }

        uint32_t &accessReg(uint8_t offset)
        {
            return *(getCurrentRegs()[offset]);
        }

        uint32_t accessReg(uint8_t offset) const
        {
            return *(getCurrentRegs()[offset]);
        }

        void setFlag(size_t flag, bool value = true)
        {
            if (value)
                accessReg(regs::CPSR_OFFSET) |= (1 << flag);
            else
                accessReg(regs::CPSR_OFFSET) &= ~(1 << flag);
        }

        bool getFlag(size_t flag) const
        {
            return accessReg(regs::CPSR_OFFSET) & (1 << flag);
        }

        std::string toString() const
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
            ss << "N=" << getFlag(cpsr_flags::N_FLAG) << ' ' << "Z=" << getFlag(cpsr_flags::Z_FLAG) << ' ' << "C=" << getFlag(cpsr_flags::C_FLAG) << ' ' << "V=" << getFlag(cpsr_flags::V_FLAG) << ' ' << "Q=" << getFlag(cpsr_flags::Q_FLAG) << '\n';
            // Cpu Mode
            ss << "CPU Mode: " << cpuModeToString() << '\n';

            return ss.str();
        }

        std::string disas(uint32_t addr, uint32_t cmds) const
        {
            std::stringstream ss;
            ss << std::setfill('0') << std::hex;

            uint32_t startAddr = addr - (cmds / 2) * (getFlag(cpsr_flags::THUMB_STATE) ? 2 : 4);
            if (startAddr < Memory::MemoryRegionOffset::EXT_ROM_OFFSET) {
                startAddr = Memory::MemoryRegionOffset::EXT_ROM_OFFSET;
            }

            for (uint32_t i = startAddr; cmds > 0; --cmds) {

                /* indicate executed instruction */
                if (i == addr)
                    ss << "<- ";

                /* indicate current instruction */
                if (i == accessReg(regs::PC_OFFSET))
                    ss << "=> ";

                /* address, pad hex numbers with 0 */
                ss << "0x" << std::setw(8) << i << "    ";

                if (getFlag(cpsr_flags::THUMB_STATE)) {
                    uint32_t bytes = memory.read16(i, nullptr);

                    uint32_t b0 = memory.read8(i, nullptr);
                    uint32_t b1 = memory.read8(i + 1, nullptr);

                    auto inst = decoder->decode(bytes).thumb;

                    /* bytes */
                    ss << std::setw(2) << b0 << ' ' << std::setw(2) << b1 << ' ' << " [" << std::setw(4) << bytes << ']';

                    /* code */
                    ss << "    " << inst.toString() << '\n';

                    i += 2;
                } else {
                    uint32_t bytes = memory.read32(i, nullptr);

                    uint32_t b0 = memory.read8(i, nullptr);
                    uint32_t b1 = memory.read8(i + 1, nullptr);
                    uint32_t b2 = memory.read8(i + 2, nullptr);
                    uint32_t b3 = memory.read8(i + 3, nullptr);

                    auto inst = decoder->decode(bytes).arm;

                    /* bytes */
                    ss << std::setw(2) << b0 << ' ' << std::setw(2) << b1 << ' ' << std::setw(2) << b2 << ' ' << std::setw(2) << b3 << " [" << std::setw(8) << bytes << ']';

                    /* code */
                    ss << "    " << inst.toString() << '\n';

                    i += 4;
                }
            }

            return ss.str();
        }
    };

} // namespace gbaemu

#endif
