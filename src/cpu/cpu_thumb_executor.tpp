#include "cpu.hpp"
#include "decode/inst.hpp"

namespace gbaemu::thumb
{

    // Fallback if not implemented
    template <ThumbInstructionCategory, InstructionID id, typename T, typename... Args>
    void ThumbExecutor::operator()(T t, Args... args)
    {
        static_assert(id == INVALID);
        std::cout << "ERROR: Thumb executor: trying to execute invalid instruction!" << std::endl;
        cpu->cpuInfo.hasCausedException = true;
    }

    template <>
    void ThumbExecutor::operator()<ADD_SUB, ADD, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, ADD_SHORT_IMM, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB_SHORT_IMM, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSL, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, ASR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, MOV, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, CMP, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, ADD, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, SUB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<ALU_OP, AND, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, EOR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSL, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ASR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ADC, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, SBC, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ROR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, TST, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, NEG, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMP, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMN, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ORR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, MUL, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, BIC, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, MVN, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<BR_XCHG, ADD, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, CMP, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, NOP, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, MOV, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, BX, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STRB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDRB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, STRH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STRB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDRB, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_HW, LDRH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_HW, STRH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, LDR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, STR, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LOAD_ADDR, ADD, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<ADD_OFFSET_TO_STACK_PTR, ADD, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, POP, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, PUSH, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, LDMIA, ThumbInstruction &>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, STMIA, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<SOFTWARE_INTERRUPT, SWI, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<COND_BRANCH, B, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<UNCONDITIONAL_BRANCH, B, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LONG_BRANCH_WITH_LINK, B, ThumbInstruction &>(ThumbInstruction &thumbInst) {}

} // namespace gbaemu::thumb