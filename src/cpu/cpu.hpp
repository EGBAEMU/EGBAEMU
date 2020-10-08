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

    enum CPUExecutionInfoType {
        NORMAL,
        WARNING,
        EXCEPTION
    };

    struct CPUExecutionInfo {
        CPUExecutionInfoType infoType;
        std::string message;

        CPUExecutionInfo() : infoType(NORMAL), message("Everything's good")
        {
        }

        CPUExecutionInfo(CPUExecutionInfoType info, const std::string &msg) : infoType(info), message(msg)
        {
        }
    };

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
        static arm::ArmExecutor armExecutor;
        static thumb::ThumbExecutor thumbExecutor;
        static InstructionDecoder armDecoder;
        static InstructionDecoder thumbDecoder;

        CPUState state;
        InstructionDecoder decoder;

        DMAGroup dmaGroup;

        TimerGroup timerGroup;

        InterruptHandler irqHandler;
        Keypad keypad;

        /* If an error has occured more information can be found here. */
        CPUExecutionInfo executionInfo;

        int32_t cyclesLeft;

        CPU();

        void setLCDController(lcd::LCDController* lcdController);

        void reset();

        void initPipeline();
        void execute(uint32_t inst, uint32_t pc);
        CPUExecutionInfoType step(uint32_t cycles);

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

            if (nFlag)
                state.setFlag<cpsr_flags::N_FLAG>(negative);

            if (zFlag)
                state.setFlag<cpsr_flags::Z_FLAG>(zero);

            if (vFlag)
                state.setFlag<cpsr_flags::V_FLAG>(overflow);

            if (cFlag) {
                state.setFlag<cpsr_flags::C_FLAG>(carry != invertCarry);
            }
        }

        // ARM instructions execution helpers
        template <InstructionID id>
        void handleMultAcc(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm);
        template <InstructionID id>
        void handleMultAccLong(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);
        template <InstructionID id>
        void handleDataSwp(uint8_t rn, uint8_t rd, uint8_t rm);
        // Executes instructions belonging to the branch subsection
        void handleBranch(bool link, int32_t offset);
        // Executes instructions belonging to the branch and execute subsection
        void handleBranchAndExchange(uint8_t rn);
        /* ALU functions */
        template <InstructionID id, bool thumb = false>
        void execDataProc(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
        template <InstructionID id, bool thumb = false>
        void execDataBlockTransfer(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList);
        template <InstructionID id, bool thumb = false>
        void execLoadStoreRegUByte(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
        template <InstructionID id, bool thumb = false>
        void execHalfwordDataTransferImmRegSignedTransfer(bool pre, bool up, bool writeback,
                                                          uint8_t rn, uint8_t rd, uint32_t offset);

        // THUMB instruction execution helpers
        void handleThumbLongBranchWithLink(bool h, uint16_t offset);
        void handleThumbUnconditionalBranch(int16_t offset);
        void handleThumbConditionalBranch(uint8_t cond, int8_t offset);
        void handleThumbAddOffsetToStackPtr(bool s, uint8_t offset);
        void handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd);

        template <InstructionID id>
        void handleThumbMoveShiftedReg(uint8_t rs, uint8_t rd, uint8_t offset);
        template <InstructionID id>
        void handleThumbBranchXCHG(uint8_t rd, uint8_t rs);
        template <InstructionID id, InstructionID origID>
        void handleThumbALUops(uint8_t rs, uint8_t rd);
    };

} // namespace gbaemu

#endif /* CPU_HPP */