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

    class CPU
    {

      public:
        static arm::ArmExecutor armExecutor;
        static thumb::ThumbExecutor thumbExecutor;

        CPUState state;
        InstructionDecoder decoder;

        DMA dma0;
        DMA dma1;
        DMA dma2;
        DMA dma3;

        TimerGroup timerGroup;

        InterruptHandler irqHandler;
        Keypad keypad;

        /* If an error has occured more information can be found here. */
        CPUExecutionInfo executionInfo;

        InstructionExecutionInfo cpuInfo;
        InstructionExecutionInfo dmaInfo;

        // Execute phase variables
        Memory::MemoryRegion executionMemReg;
        uint8_t waitStatesSeq;
        uint8_t waitStatesNonSeq;

        CPU();

        uint32_t normalizePC(bool thumbMode);

        void reset();

        void initPipeline();
        void fetch();
        void decode();
        void execute();
        CPUExecutionInfoType step();

        void setFlags(uint64_t resultValue, bool msbOp1, bool msbOp2, bool nFlag, bool zFlag, bool vFlag, bool cFlag, bool invertCarry);

        // ARM instructions execution helpers
        void handleMultAcc(bool a, bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm);
        void handleMultAccLong(bool signMul, bool a, bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);
        void handleDataSwp(bool b, uint8_t rn, uint8_t rd, uint8_t rm);
        // Executes instructions belonging to the branch subsection
        void handleBranch(bool link, int32_t offset);
        // Executes instructions belonging to the branch and execute subsection
        void handleBranchAndExchange(uint8_t rn);
        /* ALU functions */
        void execDataProc(arm::ARMInstruction &inst, bool thumb = false);
        void execDataBlockTransfer(arm::ARMInstruction &inst, bool thumb = false);
        void execLoadStoreRegUByte(const arm::ARMInstruction &inst, bool thumb = false);
        void execHalfwordDataTransferImmRegSignedTransfer(bool pre, bool up, bool load, bool writeback, bool sign,
                                                          uint8_t rn, uint8_t rd, uint32_t offset, uint8_t transferSize, bool thumb = false);

        // THUMB instruction execution helpers
        void handleThumbLongBranchWithLink(bool h, uint16_t offset);
        void handleThumbUnconditionalBranch(int16_t offset);
        void handleThumbConditionalBranch(uint8_t cond, int8_t offset);
        void handleThumbMultLoadStore(bool load, uint8_t rb, uint8_t rlist);
        void handleThumbPushPopRegister(bool load, bool r, uint8_t rlist);
        void handleThumbAddOffsetToStackPtr(bool s, uint8_t offset);
        void handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd);

        void handleThumbLoadStore(const thumb::ThumbInstruction &inst);
        void handleThumbLoadStoreSignHalfword(const thumb::ThumbInstruction &inst);

        void handleThumbAddSubtract(InstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset);
        void handleThumbMovCmpAddSubImm(InstructionID ins, uint8_t rd, uint8_t offset);
        void handleThumbMoveShiftedReg(InstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset);
        void handleThumbBranchXCHG(InstructionID id, uint8_t rd, uint8_t rs);
        void handleThumbALUops(InstructionID instID, uint8_t rs, uint8_t rd);

        // Lookup tables
        static const std::function<void(thumb::ThumbInstruction &, CPU *)> thumbExecuteHandler[];
        static const std::function<void(arm::ARMInstruction &, CPU *)> armExecuteHandler[];
    };

} // namespace gbaemu

#endif /* CPU_HPP */