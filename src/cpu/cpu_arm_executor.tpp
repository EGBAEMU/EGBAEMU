#include "cpu.hpp"
#include "decode/inst.hpp"

#include <iostream>

namespace gbaemu::arm
{

    // Fallback if not implemented
    template <ARMInstructionCategory, InstructionID id, typename T, typename... Args>
    void ArmExecutor::operator()(T t, Args... args)
    {
        static_assert(id == INVALID);
        std::cout << "ERROR: Arm executor: trying to execute invalid instruction!" << std::endl;
        cpu->cpuInfo.hasCausedException = true;
    }

    template <>
    void ArmExecutor::operator()<MUL_ACC, MLA, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC, MUL, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMLAL, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMULL, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMLAL, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMULL, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BRANCH_XCHG, BX, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<DATA_SWP, SWP, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_SWP, SWPB, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, LDRH, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, STRH, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, LDRH, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, STRH, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSB, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSH, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, AND, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, EOR, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SUB, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSB, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADD, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADC, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SBC, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSC, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TST, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MRS, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TEQ, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MSR, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMP, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMN, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ORR, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MOV, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, BIC, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MVN, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDR, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDRB, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STR, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STRB, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, LDM, ARMInstruction &>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, STM, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BRANCH, B, ARMInstruction &>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<SOFTWARE_INTERRUPT, SWI, ARMInstruction &>(ARMInstruction &armInst) {}

} // namespace gbaemu::arm