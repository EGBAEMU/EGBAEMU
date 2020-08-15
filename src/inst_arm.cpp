#include "inst_arm.hpp"

#include <iostream>


namespace gbaemu {

const char *instructionIDToString(InstructionID id)
{
    switch (id) {
        case ADC:
            return "ADC";
        case ADD:
            return "ADD";
        case AND:
            return "AND";
        case B:
            return "B";
        case BIC:
            return "BIC";
        case BX:
            return "BX";
        case CMN:
            return "CMN";
        case CMP:
            return "CMP";
        case EOR:
            return "EOR";
        case LDM:
            return "LDM";
        case LDR:
            return "LDR";
        case LDRB:
            return "LDRB";
        case LDRH:
            return "LDRH";
        case LDRSB:
            return "LDRSB";
        case LDRSH:
            return "LDRSH";
        case LDRD:
            return "LDRD";
        case MLA:
            return "MLA";
        case MOV:
            return "MOV";
        case MRS:
            return "MRS";
        case MSR:
            return "MSR";
        case MUL:
            return "MUL";
        case MVN:
            return "MVN";
        case ORR:
            return "ORR";
        case RSB:
            return "RSB";
        case RSC:
            return "RSC";
        case SBC:
            return "SBC";
        case SMLAL:
            return "SMLAL";
        case SMULL:
            return "SMULL";
        case STM:
            return "STM";
        case STR:
            return "STR";
        case STRB:
            return "STRB";
        case STRH:
            return "STRH";
        case STRD:
            return "STRD";
        case SUB:
            return "SUB";
        case SWI:
            return "SWI";
        case SWP:
            return "SWP";
        case SWPB:
            return "SWPB";
        case TEQ:
            return "TEQ";
        case TST:
            return "TST";
        case UMLAL:
            return "UMLAL";
        case UMULL:
            return "UMULL";
        case INVALID:
            return "INVALID";
    }

    return "NULL";
}

std::string ARMInstruction::toString() const {
    return std::string(name);
}


ARMInstruction ARMInstructionDecoder::decode(uint32_t lastInst) const {

    //uint32_t lastInst = state.pipeline.fetch.lastInstruction;
    ConditionOPCode executeCondition = static_cast<ConditionOPCode>(lastInst >> 28);
    
    ARMInstruction instruction;
    // Default the instruction id to invalid
    instruction.id = InstructionID::INVALID;

    if ((lastInst & MASK_MUL_ACC) == VAL_MUL_ACC) {

        bool a = (lastInst >> 21) & 1;
        bool s = (lastInst >> 20) & 1;
        instruction.params.mul_acc.a = a;
        instruction.params.mul_acc.s = s;

        instruction.params.mul_acc.rd = (lastInst >> 16) & 0x0F;
        instruction.params.mul_acc.rn = (lastInst >> 12) & 0x0F;
        instruction.params.mul_acc.rs = (lastInst >> 8) & 0x0F;
        instruction.params.mul_acc.rm = lastInst & 0x0F;

        if (a) {
            instruction.id = InstructionID::MLA;
        } else {
            instruction.id = InstructionID::MUL;
        }

    } else if ((lastInst & MASK_MUL_ACC_LONG) == VAL_MUL_ACC_LONG) {
        
        bool u = (lastInst >> 22) & 1;
        bool a = (lastInst >> 21) & 1;
        bool s = (lastInst >> 20) & 1;

        instruction.params.mul_acc_long.u = u;
        instruction.params.mul_acc_long.a = a;
        instruction.params.mul_acc_long.s = s;

        instruction.params.mul_acc_long.rd_msw = (lastInst >> 16) & 0x0F;
        instruction.params.mul_acc_long.rd_lsw = (lastInst >> 12) & 0x0F;
        instruction.params.mul_acc_long.rn = (lastInst >> 8) & 0x0F;
        instruction.params.mul_acc_long.rm = lastInst & 0x0F;

        if (u && a) {
            instruction.id = InstructionID::SMLAL;
        } else if (u && !a) {
            instruction.id = InstructionID::SMULL;
        } else if (!u && a) {
            instruction.id = InstructionID::UMLAL;
        } else {
            instruction.id = InstructionID::UMULL;
        }
        
    } else if ((lastInst & MASK_BRANCH_XCHG) == VAL_BRANCH_XCHG) {
        instruction.id = InstructionID::BX;
    } else if ((lastInst & MASK_DATA_SWP) == VAL_DATA_SWP) {
        
        /* also called b */
        bool b = (lastInst >> 22) & 1;

        instruction.params.data_swp.b = b;

        instruction.params.data_swp.rn = (lastInst >> 16) & 0x0F;
        instruction.params.data_swp.rd = (lastInst >> 12) & 0x0F;
        instruction.params.data_swp.rm = lastInst & 0x0FF;

        if (!b) {
            instruction.id = InstructionID::SWP;
        } else {
            instruction.id = InstructionID::SWPB;
        }

    } else if ((lastInst & MASK_HW_TRANSF_REG_OFF) == VAL_HW_TRANSF_REG_OFF) {
        
        bool p = (lastInst >> 24) & 1;
        bool u = (lastInst >> 23) & 1;
        bool w = (lastInst >> 21) & 1;
        bool l = (lastInst >> 20) & 1;

        instruction.params.hw_trans_reg_off.p = p;
        instruction.params.hw_trans_reg_off.u = u;
        instruction.params.hw_trans_reg_off.w = w;
        instruction.params.hw_trans_reg_off.l = l;

        instruction.params.hw_trans_reg_off.rm = lastInst & 0x0FF;

        // register offset variants
        if (l) {
            instruction.id = InstructionID::LDRH;
        } else {
            instruction.id = InstructionID::STRH;
        }

    } else if ((lastInst & MASK_HW_TRANSF_IMM_OFF) == VAL_HW_TRANSF_IMM_OFF) {
        
        bool p = (lastInst >> 24) & 1;
        bool u = (lastInst >> 23) & 1;
        bool w = (lastInst >> 21) & 1;
        bool l = (lastInst >> 20) & 1;

        instruction.params.hw_trans_imm_off.p = p;
        instruction.params.hw_trans_imm_off.u = u;
        instruction.params.hw_trans_imm_off.w = w;
        instruction.params.hw_trans_imm_off.l = l;

        instruction.params.hw_trans_imm_off.rn = (lastInst >> 16) & 0x0F;
        instruction.params.hw_trans_imm_off.rd = (lastInst >> 12) & 0x0F;

        /* called addr_mode in detailed doc but is really offset because immediate flag I is 1 */
        instruction.params.hw_trans_imm_off.offset = ((lastInst >> 8) & 0x0F) | (lastInst & 0x0F);

        if (l) {
            instruction.id = InstructionID::LDRH;
        } else {
            instruction.id = InstructionID::STRH;
        }

    } else if ((lastInst & MASK_SIGN_TRANSF) == VAL_SIGN_TRANSF) {
        
        bool p = (lastInst >> 24) & 1;
        bool u = (lastInst >> 23) & 1;
        bool b = (lastInst >> 22) & 1;
        bool w = (lastInst >> 21) & 1;

        bool l = (lastInst >> 20) & 1;
        bool h = (lastInst >> 5) & 1;

        instruction.params.sign_transf.p = p;
        instruction.params.sign_transf.u = u;
        instruction.params.sign_transf.b = b;
        instruction.params.sign_transf.w = w;
        instruction.params.sign_transf.l = l;
        instruction.params.sign_transf.h = h;

        instruction.params.sign_transf.rn = (lastInst >> 16) & 0x0F;
        instruction.params.sign_transf.rd = (lastInst >> 12) & 0x0F;

        if (l && !h) {
            instruction.id = InstructionID::LDRSB;
        } else if (l && h) {
            instruction.id = InstructionID::LDRSH;
        } else if (!l && h) {
            instruction.id = InstructionID::STRD;
        } else if (!l && !h) {
            instruction.id = InstructionID::LDRD;
        }

    } else if ((lastInst & MASK_DATA_PROC_PSR_TRANSF) == VAL_DATA_PROC_PSR_TRANSF) {
        
        uint32_t opCode = (lastInst >> 21) & 0x0F;
        bool i = lastInst & (1 << 25);
        bool s = lastInst & (1 << 20);

        instruction.params.data_proc_psr_transf.opCode = opCode;
        instruction.params.data_proc_psr_transf.i = i;
        instruction.params.data_proc_psr_transf.s = s;

        instruction.params.data_proc_psr_transf.rn = (lastInst >> 16) & 0x0F;
        instruction.params.data_proc_psr_transf.rd = (lastInst >> 12) & 0x0F;
        /* often shifter */
        instruction.params.data_proc_psr_transf.operand2 = lastInst & 0x0FFF;
        

        //TODO take a second look
        switch (opCode) {
            case 0b0101:
                instruction.id = InstructionID::ADC;
                break;
            case 0b0100:
                instruction.id = InstructionID::ADD;
                break;
            case 0b0000:
                instruction.id = InstructionID::AND;
                break;
            case 0b1010:
                if (s) {
                    instruction.id = InstructionID::CMP;
                } else {
                    instruction.id = InstructionID::MRS;
                }
                break;
            case 0b1011:
                if (s) {
                    instruction.id = InstructionID::CMN;
                } else {
                    instruction.id = InstructionID::MSR;
                }
                break;
            case 0b0001:
                instruction.id = InstructionID::EOR;
                break;
            case 0b1101:
                instruction.id = InstructionID::MOV;
                break;
            case 0b1110:
                instruction.id = InstructionID::BIC;
                break;
            case 0b1111:
                instruction.id = InstructionID::MVN;
                break;
            case 0b1100:
                instruction.id = InstructionID::ORR;
                break;
            case 0b0011:
                instruction.id = InstructionID::RSB;
                break;
            case 0b0111:
                instruction.id = InstructionID::RSC;
                break;
            case 0b0110:
                instruction.id = InstructionID::SBC;
                break;
            case 0b0010:
                instruction.id = InstructionID::SUB;
                break;
            case 0b1001:
                if (s) {
                    instruction.id = InstructionID::TEQ;
                } else {
                    instruction.id = InstructionID::MSR;
                }
                break;
            case 0b1000:
                if (s) {
                    instruction.id = InstructionID::TST;
                } else {
                    instruction.id = InstructionID::MRS;
                }
                break;
        }
    } else if ((lastInst & MASK_LS_REG_UBYTE) == VAL_LS_REG_UBYTE) {
        
        bool p = (lastInst >> 24) & 1;
        bool u = (lastInst >> 23) & 1;
        bool b = (lastInst >> 22) & 1;
        bool w = (lastInst >> 21) & 1;
        bool l = (lastInst >> 20) & 1;

        instruction.params.ls_reg_ubyte.p = p;
        instruction.params.ls_reg_ubyte.u = u;
        instruction.params.ls_reg_ubyte.b = b;
        instruction.params.ls_reg_ubyte.w = w;
        instruction.params.ls_reg_ubyte.l = l;

        instruction.params.ls_reg_ubyte.rn = (lastInst >> 16) & 0x0F;
        instruction.params.ls_reg_ubyte.rd = (lastInst >> 12) & 0x0F;
        instruction.params.ls_reg_ubyte.addrMode = lastInst & 0x0FF;

        if (!b && l) {
            instruction.id = InstructionID::LDR;
        } else if (b && l) {
            instruction.id = InstructionID::LDRB;
        } else if (!b && !l) {
            instruction.id = InstructionID::STR;
        } else {
            instruction.id = InstructionID::STRB;
        }

    } else if ((lastInst & MASK_UNDEFINED) == VAL_UNDEFINED) {
        std::cerr << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
    } else if ((lastInst & MASK_BLOCK_DATA_TRANSF) == VAL_BLOCK_DATA_TRANSF) {

        bool p = (lastInst >> 24) & 1;
        bool u = (lastInst >> 23) & 1;
        bool w = (lastInst >> 21) & 1;
        bool l = (lastInst >> 20) & 1;

        instruction.params.block_data_transf.p = p;
        instruction.params.block_data_transf.u = u;
        instruction.params.block_data_transf.w = w;
        instruction.params.block_data_transf.l = l;

        instruction.params.block_data_transf.rn = (lastInst >> 16) & 0x0F;
        instruction.params.block_data_transf.rList = lastInst & 0x0FF;

        /* docs say there are two more distinct instructions in this category */
        if (l) {
            instruction.id = InstructionID::LDM;
        } else {
            instruction.id = InstructionID::STM;
        }

    } else if ((lastInst & MASK_BRANCH) == VAL_BRANCH) {
        
        bool l = (lastInst >> 24) & 1;
        
        instruction.params.branch.l = l;
        instruction.params.branch.offset = lastInst & 0x00FFFFFF;

        instruction.id = InstructionID::B;

    } else if ((lastInst & MASK_COPROC_DATA_TRANSF) == VAL_COPROC_DATA_TRANSF) {
        //TODO
    } else if ((lastInst & MASK_COPROC_OP) == VAL_COPROC_OP) {
        //TODO
    } else if ((lastInst & MASK_COPROC_REG_TRANSF) == VAL_COPROC_REG_TRANSF) {
        //TODO
    } else if ((lastInst & MASK_SOFTWARE_INTERRUPT) == VAL_SOFTWARE_INTERRUPT) {
        //TODO
        instruction.id = InstructionID::SWI;
    } else {
        std::cerr << "ERROR: Could not decode instruction: " << std::hex << lastInst << std::endl;
    }

    if (instruction.id != InstructionID::SWI && instruction.id != InstructionID::INVALID)
        std::cout << instructionIDToString(instruction.id) << std::endl;

}

bool ARMInstructionDecoder::conditionSatisfied(const ARMInstruction& inst, uint32_t CPSR) {
    ConditionOPCode executeCondition = static_cast<ConditionOPCode>(inst.cond);
    
    switch (executeCondition) {
        // Equal Z==1
        case EQ:
            return CPSR & CPSR_Z_FLAG_BITMASK;
            break;

        // Not equal Z==0
        case NE:
            return (CPSR & CPSR_Z_FLAG_BITMASK) == 0;
            break;

        // Carry set / unsigned higher or same C==1
        case CS_HS:
            return CPSR & CPSR_C_FLAG_BITMASK;
            break;

        // Carry clear / unsigned lower C==0
        case CC_LO:
            return (CPSR & CPSR_C_FLAG_BITMASK) == 0;
            break;

        // Minus / negative N==1
        case MI:
            return CPSR & CPSR_N_FLAG_BITMASK;
            break;

        // Plus / positive or zero N==0
        case PL:
            return (CPSR & CPSR_N_FLAG_BITMASK) == 0;
            break;

        // Overflow V==1
        case VS:
            return CPSR & CPSR_V_FLAG_BITMASK;
            break;

        // No overflow V==0
        case VC:
            return (CPSR & CPSR_V_FLAG_BITMASK) == 0;
            break;

        // Unsigned higher (C==1) AND (Z==0)
        case HI:
            return (CPSR & CPSR_C_FLAG_BITMASK) && (CPSR & CPSR_Z_FLAG_BITMASK);
            break;

        // Unsigned lower or same (C==0) OR (Z==1)
        case LS:
            return (CPSR & CPSR_C_FLAG_BITMASK) == 0 || (CPSR & CPSR_Z_FLAG_BITMASK);
            break;

        // Signed greater than or equal N == V
        case GE:
            return (bool)(CPSR & CPSR_N_FLAG_BITMASK) == (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed less than N != V
        case LT:
            return (bool)(CPSR & CPSR_N_FLAG_BITMASK) != (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed greater than (Z==0) AND (N==V)
        case GT:
            return (CPSR & CPSR_Z_FLAG_BITMASK) == 0 && (bool)(CPSR & CPSR_Z_FLAG_BITMASK) == (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed less than or equal (Z==1) OR (N!=V)
        case LE:
            return (CPSR & CPSR_Z_FLAG_BITMASK) || (bool)(CPSR & CPSR_Z_FLAG_BITMASK) != (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Always (unconditional) Not applicable
        case AL:
            return true;
            break;

        // Never Obsolete, unpredictable in ARM7TDMI
        case NV:
        default:
            return false;
            break;
}

}

