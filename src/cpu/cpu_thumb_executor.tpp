#include "cpu.hpp"
#include "cpu/swi.hpp"
#include "decode/inst.hpp"

namespace gbaemu::thumb
{

    // Fallback if not implemented
    template <ThumbInstructionCategory, InstructionID id, typename... Args>
    void ThumbExecutor::operator()(Args... args)
    {
        static_assert(id == INVALID);
        std::cout << "ERROR: Thumb executor: trying to execute invalid instruction!" << std::endl;
        cpu->state.cpuInfo.hasCausedException = true;
    }

    template <>
    void ThumbExecutor::operator()<ADD_SUB, ADD>(uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        cpu->execDataProc<ADD, true>(false, true, rs, rd, rn_offset);
    }
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB>(uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        cpu->execDataProc<SUB, true>(false, true, rs, rd, rn_offset);
    }
    template <>
    void ThumbExecutor::operator()<ADD_SUB, ADD_SHORT_IMM>(uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        cpu->execDataProc<ADD_SHORT_IMM, true>(true, true, rs, rd, rn_offset);
    }
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB_SHORT_IMM>(uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        cpu->execDataProc<SUB_SHORT_IMM, true>(true, true, rs, rd, rn_offset);
    }

    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSL>(uint8_t rs, uint8_t rd, uint8_t offset)
    {
        cpu->handleThumbMoveShiftedReg<LSL>(rs, rd, offset);
    }
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSR>(uint8_t rs, uint8_t rd, uint8_t offset)
    {
        cpu->handleThumbMoveShiftedReg<LSR>(rs, rd, offset);
    }
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, ASR>(uint8_t rs, uint8_t rd, uint8_t offset)
    {
        cpu->handleThumbMoveShiftedReg<ASR>(rs, rd, offset);
    }

    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, MOV>(uint8_t rd, uint8_t offset)
    {
        cpu->execDataProc<MOV, true>(true, true, rd, rd, offset);
    }
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, CMP>(uint8_t rd, uint8_t offset)
    {
        cpu->execDataProc<CMP, true>(true, true, rd, rd, offset);
    }
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, ADD>(uint8_t rd, uint8_t offset)
    {
        cpu->execDataProc<ADD, true>(true, true, rd, rd, offset);
    }
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, SUB>(uint8_t rd, uint8_t offset)
    {
        cpu->execDataProc<SUB, true>(true, true, rd, rd, offset);
    }

    template <>
    void ThumbExecutor::operator()<ALU_OP, AND>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<AND, AND>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, EOR>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<EOR, EOR>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSL>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MOV, LSL>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSR>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MOV, LSR>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, ASR>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MOV, ASR>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, ADC>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<ADC, ADC>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, SBC>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<SBC, SBC>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, ROR>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MOV, ROR>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, TST>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<TST, TST>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, NEG>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<NEG, NEG>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMP>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<CMP, CMP>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMN>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<CMN, CMN>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, ORR>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<ORR, ORR>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, MUL>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MUL, MUL>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, BIC>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<BIC, BIC>(rs, rd);
    }
    template <>
    void ThumbExecutor::operator()<ALU_OP, MVN>(uint8_t rs, uint8_t rd)
    {
        cpu->handleThumbALUops<MVN, MVN>(rs, rd);
    }

    template <>
    void ThumbExecutor::operator()<BR_XCHG, ADD>(uint8_t rd, uint8_t rs)
    {
        cpu->handleThumbBranchXCHG<ADD>(rd, rs);
    }
    template <>
    void ThumbExecutor::operator()<BR_XCHG, CMP>(uint8_t rd, uint8_t rs)
    {
        cpu->handleThumbBranchXCHG<CMP>(rd, rs);
    }
    template <>
    void ThumbExecutor::operator()<BR_XCHG, NOP>(uint8_t rd, uint8_t rs)
    {
    }
    template <>
    void ThumbExecutor::operator()<BR_XCHG, MOV>(uint8_t rd, uint8_t rs)
    {
        cpu->handleThumbBranchXCHG<MOV>(rd, rs);
    }
    template <>
    void ThumbExecutor::operator()<BR_XCHG, BX>(uint8_t rd, uint8_t rs)
    {
        cpu->handleThumbBranchXCHG<BX>(rd, rs);
    }

    template <>
    void ThumbExecutor::operator()<PC_LD, LDR>(uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<LDR, true>(true, true, false, false, regs::PC_OFFSET, rd, (static_cast<uint16_t>(offset) << 2));
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STR>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execLoadStoreRegUByte<STR, true>(true, true, true, false, rb, rd, static_cast<uint16_t>(shifts::ShiftType::LSL << 5) | static_cast<uint16_t>(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STRB>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execLoadStoreRegUByte<STRB, true>(true, true, true, false, rb, rd, static_cast<uint16_t>(shifts::ShiftType::LSL << 5) | static_cast<uint16_t>(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDR>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execLoadStoreRegUByte<LDR, true>(true, true, true, false, rb, rd, static_cast<uint16_t>(shifts::ShiftType::LSL << 5) | static_cast<uint16_t>(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDRB>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execLoadStoreRegUByte<LDRB, true>(true, true, true, false, rb, rd, static_cast<uint16_t>(shifts::ShiftType::LSL << 5) | static_cast<uint16_t>(ro));
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, STRH>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<STRH, true>(true, true, false, rb, rd, cpu->state.accessReg(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSB>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRSB, true>(true, true, false, rb, rd, cpu->state.accessReg(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRH>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRH, true>(true, true, false, rb, rd, cpu->state.accessReg(ro));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSH>(uint8_t ro, uint8_t rb, uint8_t rd)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRSH, true>(true, true, false, rb, rd, cpu->state.accessReg(ro));
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STR>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<STR, true>(true, true, false, false, rb, rd, static_cast<uint16_t>(offset) << 2);
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDR>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<LDR, true>(true, true, false, false, rb, rd, static_cast<uint16_t>(offset) << 2);
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STRB>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<STRB, true>(true, true, false, false, rb, rd, static_cast<uint16_t>(offset));
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDRB>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<LDRB, true>(true, true, false, false, rb, rd, static_cast<uint16_t>(offset));
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_HW, LDRH>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRH, true>(true, true, false, rb, rd, static_cast<uint16_t>(offset) << 1);
    }
    template <>
    void ThumbExecutor::operator()<LD_ST_HW, STRH>(uint8_t rb, uint8_t rd, uint8_t offset)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<STRH, true>(true, true, false, rb, rd, static_cast<uint16_t>(offset) << 1);
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, LDR>(uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<LDR, true>(true, true, false, false, regs::SP_OFFSET, rd, static_cast<uint16_t>(offset) << 2);
    }

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, STR>(uint8_t rd, uint8_t offset)
    {
        cpu->execLoadStoreRegUByte<STR, true>(true, true, false, false, regs::SP_OFFSET, rd, static_cast<uint16_t>(offset) << 2);
    }

    template <>
    void ThumbExecutor::operator()<LOAD_ADDR, ADD>(bool sp, uint8_t rd, uint8_t offset)
    {
        cpu->handleThumbRelAddr(sp, offset, rd);
    }

    template <>
    void ThumbExecutor::operator()<ADD_OFFSET_TO_STACK_PTR, ADD>(bool s, uint8_t offset)
    {
        cpu->handleThumbAddOffsetToStackPtr(s, offset);
    }

    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, POP>(bool r, uint8_t rlist)
    {
        uint16_t extendedRList = static_cast<uint16_t>(rlist);
        if (r) {
            extendedRList |= 1 << regs::PC_OFFSET;
        }
        cpu->execDataBlockTransfer<LDM, true>(false, true, true, false, regs::SP_OFFSET, extendedRList);
    }

    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, PUSH>(bool r, uint8_t rlist)
    {
        uint16_t extendedRList = static_cast<uint16_t>(rlist);
        if (r) {
            extendedRList |= 1 << regs::LR_OFFSET;
        }
        cpu->execDataBlockTransfer<STM, true>(true, false, true, false, regs::SP_OFFSET, extendedRList);
    }

    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, LDMIA>(uint8_t rb, uint8_t rlist)
    {
        cpu->execDataBlockTransfer<LDM, true>(false, true, true, false, rb, rlist);
    }
    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, STMIA>(uint8_t rb, uint8_t rlist)
    {
        cpu->execDataBlockTransfer<STM, true>(false, true, true, false, rb, rlist);
    }

    template <>
    void ThumbExecutor::operator()<SOFTWARE_INTERRUPT, SWI>(uint8_t index)
    {
        if (cpu->state.memory.usesExternalBios()) {
            swi::callBiosCodeSWIHandler(cpu);
        } else {
            if (index < sizeof(swi::biosCallHandler) / sizeof(swi::biosCallHandler[0])) {
                if (index != 5 && index != 0x2B) {
                    LOG_SWI(std::cout << "Info: trying to call bios handler: " << swi::biosCallHandlerStr[index] << " at PC: 0x" << std::hex << cpu->state.getCurrentPC() << std::endl;);
                }
                swi::biosCallHandler[index](cpu);
            } else {
                std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << " at PC: 0x" << std::hex << cpu->state.getCurrentPC() << std::endl;
            }
        }
    }

    template <>
    void ThumbExecutor::operator()<COND_BRANCH, B>(uint8_t cond, int8_t offset)
    {
        cpu->handleThumbConditionalBranch(cond, offset);
    }

    template <>
    void ThumbExecutor::operator()<UNCONDITIONAL_BRANCH, B>(int16_t offset)
    {
        cpu->handleThumbUnconditionalBranch(offset);
    }

    template <>
    void ThumbExecutor::operator()<LONG_BRANCH_WITH_LINK, B>(bool h, uint16_t offset)
    {
        cpu->handleThumbLongBranchWithLink(h, offset);
    }

} // namespace gbaemu::thumb