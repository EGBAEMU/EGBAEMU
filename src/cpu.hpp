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
        void step();

        void setFlags(uint64_t resultValue, bool nFlag, bool zFlag, bool vFlag, bool cFlag);

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
        InstructionExecutionInfo execLoadStoreRegUByte(const arm::ARMInstruction &inst);
        InstructionExecutionInfo execDataBlockTransfer(arm::ARMInstruction &inst);
        InstructionExecutionInfo execHalfwordDataTransferImmRegSignedTransfer(const arm::ARMInstruction &inst);

        // THUMB instruction execution helpers
        InstructionExecutionInfo handleThumbLongBranchWithLink(bool h, uint16_t offset);
        InstructionExecutionInfo handleThumbUnconditionalBranch(int16_t offset);
        InstructionExecutionInfo handleThumbConditionalBranch(uint8_t cond, int8_t offset);
        InstructionExecutionInfo handleThumbMultLoadStore(bool load, uint8_t rb, uint8_t rlist);
        InstructionExecutionInfo handleThumbPushPopRegister(bool load, bool r, uint8_t rlist);
        InstructionExecutionInfo handleThumbAddOffsetToStackPtr(bool s, uint8_t offset);
        InstructionExecutionInfo handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd);
        InstructionExecutionInfo handleThumbLoadStoreSPRelative(bool l, uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbLoadStoreHalfword(bool l, uint8_t offset, uint8_t rb, uint8_t rd);
        InstructionExecutionInfo handleThumbLoadStoreImmOff(bool l, bool b, uint8_t offset, uint8_t rb, uint8_t rd);
        InstructionExecutionInfo handleThumbLoadStoreSignExt(bool h, bool s, uint8_t ro, uint8_t rb, uint8_t rd);
        InstructionExecutionInfo handleThumbLoadStoreRegisterOffset(bool l, bool b, uint8_t ro, uint8_t rb, uint8_t rd);
        InstructionExecutionInfo handleThumbLoadPCRelative(uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbAddSubtract(thumb::ThumbInstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset);
        InstructionExecutionInfo handleThumbMovCmpAddSubImm(thumb::ThumbInstructionID ins, uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbMoveShiftedReg(thumb::ThumbInstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset);
        InstructionExecutionInfo handleThumbBranchXCHG(thumb::ThumbInstructionID id, uint8_t rd, uint8_t rs);
        InstructionExecutionInfo handleThumbALUops(thumb::ThumbInstructionID instID, uint8_t rs, uint8_t rd);
    };

} // namespace gbaemu

#endif /* CPU_HPP */