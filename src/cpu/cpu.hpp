#ifndef CPU_HPP
#define CPU_HPP

#include "cpu_state.hpp"
#include "decode/inst.hpp"
#include "io/dma.hpp"
#include "io/interrupts.hpp"
#include "io/keypad.hpp"
#include "io/timer.hpp"
#include "regs.hpp"

#include <cstdint>

namespace gbaemu
{

    // forward declarations
    namespace arm
    {
        class ArmExecutor;
    }
    namespace thumb
    {
        class ThumbExecutor;
    }
    namespace lcd
    {
        class LCDController;
    }

    class CPU
    {

      public:
        // using InstExecutor = void (CPU::*)(uint32_t);
        typedef void(CPU::*InstExecutor)(uint32_t);

      private:
        static const InstExecutor thumbExeLUT[1024];
        static const InstExecutor armExeLUT[4096];

      public:
        CPUState state;

        DMAGroup dmaGroup;

        TimerGroup timerGroup;

        InterruptHandler irqHandler;
        Keypad keypad;

        int32_t cyclesLeft;

        CPU();

        void setLCDController(lcd::LCDController *lcdController);

        void reset();

        CPUExecutionInfoType step(uint32_t cycles);

        void patchFetchToNCycle();
        template <bool thumbMode>
        void refillPipelineAfterBranch();

        void refillPipeline();

      private:
        template <uint8_t execState>
        void execStep(uint32_t &prevPC);

      public:
        template <bool nFlag, bool zFlag, bool vFlag, bool cFlag, bool invertCarry>
        void setFlags(uint64_t resultValue, bool msbOp1, bool msbOp2)
        {
            /*
            The arithmetic operations (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN) treat each
            operand as a 32 bit integer (either unsigned or 2’s complement signed, the two are equivalent).
            the V flag in the CPSR will be set if
            an overflow occurs into bit 31 of the result; this may be ignored if the operands were
            considered unsigned, but warns of a possible error if the operands were 2’s
            complement signed. The C flag will be set to the carry out of bit 31 of the ALU, the Z
            flag will be set if and only if the result was zero, and the N flag will be set to the value
            of bit 31 of the result (indicating a negative result if the operands are considered to be
            2’s complement signed).
            */
            bool negative = (resultValue) & (static_cast<uint64_t>(1) << 31);
            bool zero = (resultValue & 0x0FFFFFFFF) == 0;
            bool overflow = msbOp1 == msbOp2 && (negative != msbOp1);
            bool carry = resultValue & (static_cast<uint64_t>(1) << 32);

            if (nFlag) {
                state.setFlag<cpsr_flags::N_FLAG>(negative);
            }

            if (zFlag) {
                state.setFlag<cpsr_flags::Z_FLAG>(zero);
            }

            if (vFlag) {
                state.setFlag<cpsr_flags::V_FLAG>(overflow);
            }

            if (cFlag) {
                state.setFlag<cpsr_flags::C_FLAG>(carry != invertCarry);
            }
        }

        // ARM instructions execution helpers
        template <bool a, bool s, bool thumb = false>
        void handleMultAcc(uint32_t inst);
        template <bool a, bool s, bool signMul>
        void handleMultAccLong(uint32_t inst);
        template <bool b>
        void handleDataSwp(uint32_t inst);
        // Executes instructions belonging to the branch subsection
        template <bool link>
        void handleBranch(uint32_t inst);
        // Executes instructions belonging to the branch and execute subsection
        void handleBranchAndExchange(uint32_t inst);
        /* ALU functions */
        template <InstructionID id, bool i, bool s, bool thumb, thumb::ThumbInstructionCategory cat = thumb::INVALID_CAT, InstructionID origID = INVALID>
        void execDataProc(uint32_t inst);
        template <bool thumb, bool pre, bool up, bool writeback, bool forceUserRegisters, bool load, bool patchRlist = false, bool useSP = false>
        void execDataBlockTransfer(uint32_t inst);
        /* bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode */
        template <InstructionID id, bool thumb, bool pre, bool up, bool i, bool writeback, thumb::ThumbInstructionCategory thumbCat>
        void execLoadStoreRegUByte(uint32_t inst);
        template <bool b, InstructionID id, bool thumb, bool pre, bool up, bool writeback, arm::ARMInstructionCategory armCat, thumb::ThumbInstructionCategory thumbCat>
        void execHalfwordDataTransferImmRegSignedTransfer(uint32_t inst);
        template <bool thumb>
        void softwareInterrupt(uint32_t inst);

        void handleInvalid(uint32_t);

        // THUMB instruction execution helpers
        template <bool link>
        void handleThumbLongBranchWithLink(uint32_t inst);
        void handleThumbUnconditionalBranch(uint32_t inst);
        void handleThumbConditionalBranch(uint32_t inst);
        template <bool s>
        void handleThumbAddOffsetToStackPtr(uint32_t inst);
        template <bool sp>
        void handleThumbRelAddr(uint32_t inst);

        template <InstructionID id>
        void handleThumbMoveShiftedReg(uint32_t inst);
        template <InstructionID id>
        void handleThumbBranchXCHG(uint32_t inst);

        template <uint16_t hash>
        static constexpr InstExecutor resolveThumbHashHandler();

        template <uint16_t hash>
        static constexpr InstExecutor resolveArmHashHandler();
    };

} // namespace gbaemu

// #include "cpu_arm.tpp"
// #include "cpu_thumb.tpp"

#endif /* CPU_HPP */