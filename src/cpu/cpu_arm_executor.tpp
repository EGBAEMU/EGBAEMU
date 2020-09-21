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
    void ArmExecutor::operator()<MUL_ACC, MLA>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC, MUL>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMLAL>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMULL>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMLAL>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMULL>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BRANCH_XCHG, BX>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<DATA_SWP, SWP>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_SWP, SWPB>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, LDRH>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, STRH>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, LDRH>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, STRH>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSB>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSH>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, AND>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, EOR>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SUB>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSB>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADD>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADC>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SBC>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSC>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TST>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MRS>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TEQ>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MSR>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMP>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMN>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ORR>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MOV>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, BIC>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MVN>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDR>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDRB>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STR>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STRB>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, LDM>(ARMInstruction &armInst) {}
    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, STM>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<BRANCH, B>(ARMInstruction &armInst) {}

    template <>
    void ArmExecutor::operator()<SOFTWARE_INTERRUPT, SWI>(ARMInstruction &armInst) {}

} // namespace gbaemu::arm