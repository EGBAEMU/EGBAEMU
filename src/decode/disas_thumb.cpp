#include "cpu/swi.hpp"
#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::thumb
{

    std::string ThumbInstruction::toString() const
    {
        std::stringstream ss;

        ss << instructionIDToString(id);

        switch (cat) {
            case ThumbInstructionCategory::MOV_SHIFT:
                ss << " r" << static_cast<uint32_t>(params.mov_shift.rd) << ", r" << static_cast<uint32_t>(params.mov_shift.rs) << ", #" << static_cast<uint32_t>(params.mov_shift.offset == 0 && id != LSL ? 32 : params.mov_shift.offset);
                break;
            case ThumbInstructionCategory::ADD_SUB: {
                uint32_t rd = params.add_sub.rd;
                uint32_t rs = params.add_sub.rs;
                uint32_t rn_off = params.add_sub.rn_offset;

                ss << " r" << rd << ", r" << rs;

                if (id == InstructionID::ADD_SHORT_IMM || id == InstructionID::SUB_SHORT_IMM)
                    ss << " 0x" << std::hex << rn_off;
                else
                    ss << " r" << rn_off;
            }

            break;
            case ThumbInstructionCategory::MOV_CMP_ADD_SUB_IMM:
                ss << " r" << static_cast<uint32_t>(params.mov_cmp_add_sub_imm.rd) << ", 0x" << std::hex << static_cast<uint32_t>(params.mov_cmp_add_sub_imm.offset);
                break;
            case ThumbInstructionCategory::ALU_OP:
                ss << " r" << static_cast<uint32_t>(params.alu_op.rd) << ", r" << static_cast<uint32_t>(params.alu_op.rs);
                break;
            case ThumbInstructionCategory::BR_XCHG: {
                ss << " r";
                if (id != BX)
                    ss << static_cast<uint32_t>(params.br_xchg.rd) << ", r";

                ss << static_cast<uint32_t>(params.br_xchg.rs);
                break;
            }
            case ThumbInstructionCategory::PC_LD:
                ss << " r" << static_cast<uint32_t>(params.pc_ld.rd) << ", [((PC + 4) & ~2) + " << static_cast<uint32_t>(params.pc_ld.offset * 4) << "]";
                break;
            case ThumbInstructionCategory::LD_ST_REL_OFF:
                ss << " r" << static_cast<uint32_t>(params.ld_st_rel_off.rd) << ", [r" << static_cast<uint32_t>(params.ld_st_rel_off.rb) << " + r" << static_cast<uint32_t>(params.ld_st_rel_off.ro) << "]";
                break;
            case ThumbInstructionCategory::LD_ST_SIGN_EXT:
                ss << " r" << static_cast<uint32_t>(params.ld_st_sign_ext.rd) << ", [r" << static_cast<uint32_t>(params.ld_st_sign_ext.rb) << " + r" << static_cast<uint32_t>(params.ld_st_sign_ext.ro) << "]";
                break;
            case ThumbInstructionCategory::LD_ST_IMM_OFF:
                ss << " r" << static_cast<uint32_t>(params.ld_st_imm_off.rd) << ", [r" << static_cast<uint32_t>(params.ld_st_imm_off.rb) << " + #" << static_cast<uint32_t>(params.ld_st_imm_off.offset) << "]";
                break;
            case ThumbInstructionCategory::LD_ST_HW:
                ss << " r" << static_cast<uint32_t>(params.ld_st_hw.rd) << ", [r" << static_cast<uint32_t>(params.ld_st_hw.rb) << " + #" << static_cast<uint32_t>(params.ld_st_hw.offset * 2) << "]";
                break;
            case ThumbInstructionCategory::LD_ST_REL_SP:
                ss << " r" << static_cast<uint32_t>(params.ld_st_rel_sp.rd) << ", [SP + #" << static_cast<uint32_t>(params.ld_st_rel_sp.offset * 4) << "]";
                break;
            case ThumbInstructionCategory::LOAD_ADDR:
                ss << " r" << static_cast<uint32_t>(params.load_addr.rd) << ", [" << (params.load_addr.sp ? "SP" : "((PC + 4) & ~2)") << " + #" << static_cast<uint32_t>(params.load_addr.offset * 4) << "]";
                break;
            case ThumbInstructionCategory::ADD_OFFSET_TO_STACK_PTR:
                ss << " SP, #" << (params.add_offset_to_stack_ptr.s ? "-" : "") << static_cast<uint32_t>(params.add_offset_to_stack_ptr.offset * 4);
                break;
            case ThumbInstructionCategory::PUSH_POP_REG: {
                ss << " { ";

                for (uint32_t i = 0; i < 8; ++i)
                    if (params.push_pop_reg.rlist & (1 << i))
                        ss << "r" << i << ' ';

                ss << '}' << '{' << (params.push_pop_reg.r ? (params.push_pop_reg.l ? "PC" : "LR") : "") << '}';
                break;
            }
            case ThumbInstructionCategory::MULT_LOAD_STORE: {
                ss << " r" << static_cast<uint32_t>(params.mult_load_store.rb) << " { ";

                for (uint32_t i = 0; i < 8; ++i)
                    if (params.mult_load_store.rlist & (1 << i))
                        ss << "r" << i << ' ';

                ss << '}';
                break;
            }
            case ThumbInstructionCategory::COND_BRANCH:
                ss << conditionCodeToString(static_cast<ConditionOPCode>(params.cond_branch.cond)) << " PC + 4 + " << (static_cast<int32_t>(params.cond_branch.offset) * 2);
                break;
            case ThumbInstructionCategory::SOFTWARE_INTERRUPT:
                ss << " " << swi::swiToString(params.software_interrupt.comment);
                break;
            case ThumbInstructionCategory::UNCONDITIONAL_BRANCH:
                ss << " PC + 4 + " << (static_cast<int32_t>(params.unconditional_branch.offset) * 2);
                break;
            case ThumbInstructionCategory::LONG_BRANCH_WITH_LINK: {
                ss << ' ';
                if (params.long_branch_with_link.h) {
                    ss << "PC = LR + 0x" << std::hex << (static_cast<uint32_t>(params.long_branch_with_link.offset) << 1) << ", LR = (PC + 2) | 1";
                } else {
                    ss << "LR = PC + 4 + 0x" << std::hex << (static_cast<uint32_t>(params.long_branch_with_link.offset) << 12);
                }
                break;
            }
            case ThumbInstructionCategory::INVALID_CAT:
                ss << " INVALID";
                break;
        }

        return ss.str();
    }

} // namespace gbaemu::thumb