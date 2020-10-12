#ifndef CPU_STATE_HPP
#define CPU_STATE_HPP

#include "decode/inst.hpp"
#include "io/memory.hpp"
#include "regs.hpp"
#include <cstdint>
#include <sstream>
#include <string>

namespace gbaemu
{

    enum CPUExecutionInfoType {
        NORMAL,
        WARNING,
        EXCEPTION
    };

    struct CPUExecutionInfo {
        CPUExecutionInfoType infoType = NORMAL;
        std::stringstream message;
    };

    struct CPUState {
      public:
        enum CPUMode : uint8_t {
            UserMode,
            FIQ,
            IRQ,
            SupervisorMode,
            AbortMode,
            UndefinedMode,
            SystemMode
        };

        enum ExecutionState : uint8_t {
            EXEC_THUMB = 1 << 0,
            EXEC_DMA = 1 << 1,
            EXEC_TIMER = 1 << 2,
            EXEC_IRQ = 1 << 3,
            EXEC_HALT = 1 << 4,
            // Indicate that an exception occurred -> abort
            EXEC_ERROR = 1 << 5,
        };

        uint8_t execState;

        /* If an error has occured more information can be found here. */
        CPUExecutionInfo executionInfo;

      private:
        struct Regs {
            uint32_t rx[16];
            uint32_t r8_14_fiq[7];
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
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.r8_14_fiq, regs.r8_14_fiq + 1, regs.r8_14_fiq + 2, regs.r8_14_fiq + 3, regs.r8_14_fiq + 4, regs.r8_14_fiq + 5, regs.r8_14_fiq + 6, regs.rx + 15, &regs.CPSR, &regs.SPSR_fiq},
            // IRQ mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_irq, regs.r13_14_irq + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_irq},
            // Supervisor Mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_svc, regs.r13_14_svc + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_svc},
            // Abort mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_abt, regs.r13_14_abt + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_abt},
            // Undefined Mode
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_und, regs.r13_14_und + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_und},
            // System / User mode regs
            {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.rx + 13, regs.rx + 14, regs.rx + 15, &regs.CPSR, &regs.CPSR}};

        uint32_t *const *currentRegs;

      public:
        /* pipeline */
        uint32_t pipeline[2];

        Memory memory;

        InstructionExecutionInfo cpuInfo;

        // CPU halting
        uint32_t haltCondition;

        uint8_t seqCycles;
        uint8_t nonSeqCycles;

        struct CPSR_Flags {
            bool thumbMode;
            bool negative;
            bool zero;
            bool carry;
            bool overflow;

            bool irqDisable;
            CPUMode mode;
        } cpsr;

      private:
        uint32_t handleReadUnused();

      public:
        CPUState();

        void reset();

        uint32_t normalizePC();

        const char *cpuModeToString() const;

        uint32_t getCurrentPC() const;
        uint32_t &getPC();

        uint32_t *const *const getCurrentRegs() { return currentRegs; }

        const uint32_t *const *const getCurrentRegs() const { return currentRegs; }

        uint32_t *const *const getModeRegs(CPUMode cpuMode);

        const uint32_t *const *const getModeRegs(CPUMode cpuMode) const;

        uint32_t &accessReg(uint8_t offset);

        uint32_t accessReg(uint8_t offset) const;

        template <cpsr_flags::CPSR_FLAGS flag>
        inline void setFlag(bool value = true)
        {
            static_assert(flag != cpsr_flags::FIQ_DISABLE);

            constexpr uint32_t mask = static_cast<uint32_t>(1) << flag;
            if (value) {
                regs.CPSR |= mask;
                if (flag == cpsr_flags::THUMB_STATE)
                    execState |= EXEC_THUMB;
            } else {
                regs.CPSR &= ~mask;
                if (flag == cpsr_flags::THUMB_STATE)
                    execState &= ~EXEC_THUMB;
            }

            (flag == cpsr_flags::N_FLAG ? cpsr.negative : (flag == cpsr_flags::Z_FLAG ? cpsr.zero : (flag == cpsr_flags::C_FLAG ? cpsr.carry : (flag == cpsr_flags::V_FLAG ? cpsr.overflow : (flag == cpsr_flags::IRQ_DISABLE ? cpsr.irqDisable : cpsr.thumbMode))))) = value;
        }

        template <cpsr_flags::CPSR_FLAGS flag>
        inline bool getFlag() const
        {
            static_assert(flag != cpsr_flags::FIQ_DISABLE);

            return (flag == cpsr_flags::N_FLAG ? cpsr.negative : (flag == cpsr_flags::Z_FLAG ? cpsr.zero : (flag == cpsr_flags::C_FLAG ? cpsr.carry : (flag == cpsr_flags::V_FLAG ? cpsr.overflow : (flag == cpsr_flags::IRQ_DISABLE ? cpsr.irqDisable : cpsr.thumbMode)))));
        }

        void clearFlags();

        void updateCPSR(uint32_t value);
        uint32_t getCurrentCPSR()
        {
            return regs.CPSR;
        }

        CPUMode getCPUMode() const { return cpsr.mode; }
        void updateCPUMode();
        void setCPUMode(uint8_t modeBits);

      private:
        void updateCPUMode(uint8_t modeBits);

      public:
        std::string toString() const;
        std::string printStack(uint32_t words) const;

        std::string disas(uint32_t addr, uint32_t cmds);
    };

} // namespace gbaemu

#endif
