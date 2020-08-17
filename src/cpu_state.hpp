#ifndef CPU_STATE_HPP
#define CPU_STATE_HPP

#include "inst.hpp"
#include "memory.hpp"
#include "regs.hpp"
#include <cstdint>
#include <string>

namespace gbaemu
{

    struct CPUState {
        /*
        https://problemkaputt.de/gbatek.htm#armcpuregisterset
        https://static.docs.arm.com/dvi0027/b/DVI_0027A_ARM7TDMI_PO.pdf
     */

        enum OperationState : uint8_t {
            ARMState,
            ThumbState
        } operationState = ARMState;

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
        struct {
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
        } regs;

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
        } pipeline;

        Memory memory;

        // Indicates if the execute stage processed an branch instruction and changed the pc
        bool branchOccurred = false;

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

        uint32_t &accessReg(uint8_t offset)
        {
            return *(getCurrentRegs()[offset]);
        }

        uint32_t accessReg(uint8_t offset) const
        {
            return *(getCurrentRegs()[offset]);
        }

        void setFlag(size_t flag)
        {
            accessReg(regs::CPSR_OFFSET) |= (1 << flag);
        }

        void clearFlags()
        {
            //TODO implement
        }

        std::string toString() const
        {
            return "";
        }
    };

} // namespace gbaemu

#endif
