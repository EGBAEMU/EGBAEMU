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
    void ThumbExecutor::operator()<ADD_SUB, ADD>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, ADD_SHORT_IMM>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ADD_SUB, SUB_SHORT_IMM>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSL>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, LSR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_SHIFT, ASR>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, MOV>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, CMP>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, ADD>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MOV_CMP_ADD_SUB_IMM, SUB>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<ALU_OP, AND>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, EOR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSL>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, LSR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ASR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ADC>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, SBC>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ROR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, TST>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, NEG>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMP>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, CMN>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, ORR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, MUL>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, BIC>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<ALU_OP, MVN>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<BR_XCHG, ADD>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, CMP>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, NOP>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, MOV>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<BR_XCHG, BX>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, STRB>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_OFF, LDRB>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, STRH>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSB>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRH>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_SIGN_EXT, LDRSH>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, STRB>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_IMM_OFF, LDRB>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_HW, LDRH>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_HW, STRH>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, LDR>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<LD_ST_REL_SP, STR>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LOAD_ADDR, ADD>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<ADD_OFFSET_TO_STACK_PTR, ADD>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, POP>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<PUSH_POP_REG, PUSH>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, LDMIA>(ThumbInstruction &thumbInst) {}
    template <>
    void ThumbExecutor::operator()<MULT_LOAD_STORE, STMIA>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<SOFTWARE_INTERRUPT, SWI>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<COND_BRANCH, B>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<UNCONDITIONAL_BRANCH, B>(ThumbInstruction &thumbInst) {}

    template <>
    void ThumbExecutor::operator()<LONG_BRANCH_WITH_LINK, B>(ThumbInstruction &thumbInst) {}

} // namespace gbaemu::thumb