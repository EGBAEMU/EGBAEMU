#include "cpu.hpp"
#include "decode/inst.hpp"
#include "logging.hpp"
#include "swi.hpp"

#include <iostream>

namespace gbaemu::arm
{

    // Fallback if not implemented
    template <ARMInstructionCategory, InstructionID id, typename... Args>
    void ArmExecutor::operator()(Args... args)
    {
        static_assert(id == INVALID);
        std::cout << "ERROR: Arm executor: trying to execute invalid instruction!" << std::endl;
        cpu->state.cpuInfo.hasCausedException = true;
    }

    template <>
    void ArmExecutor::operator()<MUL_ACC, MLA>(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAcc<MLA>(s, rd, rn, rs, rm);
    }
    template <>
    void ArmExecutor::operator()<MUL_ACC, MUL>(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAcc<MUL>(s, rd, rn, rs, rm);
    }

    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMLAL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAccLong<SMLAL>(s, rd_msw, rd_lsw, rs, rm);
    }
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, SMULL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAccLong<SMULL>(s, rd_msw, rd_lsw, rs, rm);
    }
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMLAL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAccLong<UMLAL>(s, rd_msw, rd_lsw, rs, rm);
    }
    template <>
    void ArmExecutor::operator()<MUL_ACC_LONG, UMULL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        cpu->handleMultAccLong<UMULL>(s, rd_msw, rd_lsw, rs, rm);
    }

    template <>
    void ArmExecutor::operator()<BRANCH_XCHG, BX>(uint8_t rn)
    {
        cpu->handleBranchAndExchange(rn);
    }

    template <>
    void ArmExecutor::operator()<DATA_SWP, SWP>(uint8_t rn, uint8_t rd, uint8_t rm)
    {
        cpu->handleDataSwp<SWP>(rn, rd, rm);
    }
    template <>
    void ArmExecutor::operator()<DATA_SWP, SWPB>(uint8_t rn, uint8_t rd, uint8_t rm)
    {
        cpu->handleDataSwp<SWPB>(rn, rd, rm);
    }

    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, LDRH>(bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t rm)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRH>(p, u, w, rn, rd, cpu->state.accessReg(rm));
    }
    template <>
    void ArmExecutor::operator()<HW_TRANSF_REG_OFF, STRH>(bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t rm)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<STRH>(p, u, w, rn, rd, cpu->state.accessReg(rm));
    }

    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, LDRH>(bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t offset)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRH>(p, u, w, rn, rd, offset);
    }
    template <>
    void ArmExecutor::operator()<HW_TRANSF_IMM_OFF, STRH>(bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t offset)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<STRH>(p, u, w, rn, rd, offset);
    }

    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSB>(bool b, bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t addrMode)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRSB>(p, u, w, rn, rd, b ? addrMode : cpu->state.accessReg(addrMode & 0x0F));
    }
    template <>
    void ArmExecutor::operator()<SIGN_TRANSF, LDRSH>(bool b, bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t addrMode)
    {
        cpu->execHalfwordDataTransferImmRegSignedTransfer<LDRSH>(p, u, w, rn, rd, b ? addrMode : cpu->state.accessReg(addrMode & 0x0F));
    }

    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, AND>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<AND>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, EOR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<EOR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SUB>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<SUB>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSB>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<RSB>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADD>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<ADD>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ADC>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<ADC>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, SBC>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<SBC>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, RSC>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<RSC>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TST>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<TST>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MRS_CPSR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MRS_CPSR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MRS_SPSR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MRS_SPSR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, TEQ>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<TEQ>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MSR_CPSR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MSR_CPSR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MSR_SPSR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MSR_SPSR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMP>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<CMP>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, CMN>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<CMN>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, ORR>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<ORR>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MOV>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MOV>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, BIC>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<BIC>(i, s, rn, rd, operand2);
    }
    template <>
    void ArmExecutor::operator()<DATA_PROC_PSR_TRANSF, MVN>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        cpu->execDataProc<MVN>(i, s, rn, rd, operand2);
    }

    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDR>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
    {
        cpu->execLoadStoreRegUByte<LDR>(pre, up, i, writeback, rn, rd, addrMode);
    }
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, LDRB>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
    {
        cpu->execLoadStoreRegUByte<LDRB>(pre, up, i, writeback, rn, rd, addrMode);
    }
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STR>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
    {
        cpu->execLoadStoreRegUByte<STR>(pre, up, i, writeback, rn, rd, addrMode);
    }
    template <>
    void ArmExecutor::operator()<LS_REG_UBYTE, STRB>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
    {
        cpu->execLoadStoreRegUByte<STRB>(pre, up, i, writeback, rn, rd, addrMode);
    }

    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, LDM>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList)
    {
        cpu->execDataBlockTransfer<LDM>(pre, up, writeback, forceUserRegisters, rn, rList);
    }
    template <>
    void ArmExecutor::operator()<BLOCK_DATA_TRANSF, STM>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList)
    {
        cpu->execDataBlockTransfer<STM>(pre, up, writeback, forceUserRegisters, rn, rList);
    }

    template <>
    void ArmExecutor::operator()<BRANCH, B>(bool link, int32_t offset)
    {
        cpu->handleBranch(link, offset);
    }

    template <>
    void ArmExecutor::operator()<SOFTWARE_INTERRUPT, SWI>(uint8_t index)
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

} // namespace gbaemu::arm