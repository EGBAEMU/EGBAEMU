#ifndef CPU_HPP
#define CPU_HPP

#include "cpu_state.hpp"
#include "inst_arm.hpp"
#include "inst_thumb.hpp"
#include "regs.hpp"
#include <cstdint>

namespace gbaemu
{

    class CPU
    {
      private:
        arm::ARMInstructionDecoder armDecoder;
        thumb::ThumbInstructionDecoder thumbDecoder;

      public:
        CPUState state;

        CPU();

        void initPipeline();
        void fetch();
        void decode();
        InstructionExecutionInfo execute();
        bool step();

        void setFlags(uint64_t resultValue, bool msbOp1, bool msbOp2, bool nFlag, bool zFlag, bool vFlag, bool cFlag, bool invertCarry);

        // ARM instructions execution helpers
        InstructionExecutionInfo handleMultAcc(bool a, bool s, uint32_t rd, uint32_t rn, uint32_t rs, uint32_t rm);
        InstructionExecutionInfo handleMultAccLong(bool signMul, bool a, bool s, uint32_t rd_msw, uint32_t rd_lsw, uint32_t rs, uint32_t rm);
        InstructionExecutionInfo handleDataSwp(bool b, uint32_t rn, uint32_t rd, uint32_t rm);
        // Executes instructions belonging to the branch subsection
        InstructionExecutionInfo handleBranch(bool link, int32_t offset);
        // Executes instructions belonging to the branch and execute subsection
        InstructionExecutionInfo handleBranchAndExchange(uint32_t rn);
        /* ALU functions */
        InstructionExecutionInfo execDataProc(arm::ARMInstruction &inst);
        InstructionExecutionInfo execDataBlockTransfer(arm::ARMInstruction &inst, bool thumb = false);
        InstructionExecutionInfo execLoadStoreRegUByte(const arm::ARMInstruction &inst, bool thumb = false);
        InstructionExecutionInfo execHalfwordDataTransferImmRegSignedTransfer(bool pre, bool up, bool load, bool writeback, bool sign,
                                                                              uint8_t rn, uint8_t rd, uint32_t offset, uint8_t transferSize, bool thumb = false);

        // THUMB instruction execution helpers
        InstructionExecutionInfo handleThumbLongBranchWithLink(bool h, uint16_t offset);
        InstructionExecutionInfo handleThumbUnconditionalBranch(int16_t offset);
        InstructionExecutionInfo handleThumbConditionalBranch(uint8_t cond, int8_t offset);
        InstructionExecutionInfo handleThumbMultLoadStore(bool load, uint8_t rb, uint8_t rlist);
        InstructionExecutionInfo handleThumbPushPopRegister(bool load, bool r, uint8_t rlist);
        InstructionExecutionInfo handleThumbAddOffsetToStackPtr(bool s, uint8_t offset);
        InstructionExecutionInfo handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd);

        InstructionExecutionInfo handleThumbLoadStore(const thumb::ThumbInstruction &inst);
        InstructionExecutionInfo handleThumbLoadStoreSignHalfword(const thumb::ThumbInstruction &inst);

        InstructionExecutionInfo handleThumbAddSubtract(thumb::ThumbInstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset);
        InstructionExecutionInfo handleThumbMovCmpAddSubImm(thumb::ThumbInstructionID ins, uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbMoveShiftedReg(thumb::ThumbInstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbBranchXCHG(thumb::ThumbInstructionID id, uint8_t rd, uint8_t rs);
        InstructionExecutionInfo handleThumbALUops(thumb::ThumbInstructionID instID, uint8_t rs, uint8_t rd);
    };

} // namespace gbaemu

#endif /* CPU_HPP */