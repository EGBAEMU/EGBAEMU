#include "inst_thumb.hpp"
#include "cpu/cpu.hpp"
#include "util.hpp"

namespace gbaemu
{

    namespace thumb
    {
        template <typename Executor>
        template <Executor &exec>
        void ThumbInstructionDecoder<Executor>::decode(uint32_t lastInst)
        {
            ThumbInstruction instruction;
            instruction.id = InstructionID::INVALID;
            instruction.cat = ThumbInstructionCategory::INVALID_CAT;

            if ((lastInst & MASK_THUMB_ADD_SUB) == VAL_THUMB_ADD_SUB) {

                instruction.cat = ThumbInstructionCategory::ADD_SUB;

                uint8_t opCode = (lastInst >> 9) & 0x3;

                instruction.params.add_sub.rd = lastInst & 0x7;
                instruction.params.add_sub.rs = (lastInst >> 3) & 0x7;
                instruction.params.add_sub.rn_offset = (lastInst >> 6) & 0x7;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::ADD;
                        exec.Executor::template operator()<ADD_SUB, ADD, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::SUB;
                        exec.Executor::template operator()<ADD_SUB, SUB, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::ADD_SHORT_IMM;
                        exec.Executor::template operator()<ADD_SUB, ADD_SHORT_IMM, ThumbInstruction &>(instruction);
                        break;
                    case 0b11:
                        instruction.id = InstructionID::SUB_SHORT_IMM;
                        exec.Executor::template operator()<ADD_SUB, SUB_SHORT_IMM, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_MOV_SHIFT) == VAL_THUMB_MOV_SHIFT) {

                instruction.cat = ThumbInstructionCategory::MOV_SHIFT;

                uint8_t opCode = (lastInst >> 11) & 0x3;

                instruction.params.mov_shift.rs = (lastInst >> 3) & 0x7;
                instruction.params.mov_shift.rd = lastInst & 0x7;
                instruction.params.mov_shift.offset = (lastInst >> 6) & 0x1F;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::LSL;
                        exec.Executor::template operator()<MOV_SHIFT, LSL, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::LSR;
                        exec.Executor::template operator()<MOV_SHIFT, LSR, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::ASR;
                        exec.Executor::template operator()<MOV_SHIFT, ASR, ThumbInstruction &>(instruction);
                        break;

                    // This case belongs to ADD_SUB
                    case 0b11:
                        break;
                }
            } else if ((lastInst & MASK_THUMB_MOV_CMP_ADD_SUB_IMM) == VAL_THUMB_MOV_CMP_ADD_SUB_IMM) {

                instruction.cat = ThumbInstructionCategory::MOV_CMP_ADD_SUB_IMM;

                uint8_t opCode = (lastInst >> 11) & 0x3;

                instruction.params.mov_cmp_add_sub_imm.rd = (lastInst >> 8) & 0x7;
                instruction.params.mov_cmp_add_sub_imm.offset = lastInst & 0x0FF;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::MOV;
                        exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, MOV, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::CMP;
                        exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, CMP, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::ADD;
                        exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, ADD, ThumbInstruction &>(instruction);
                        break;
                    case 0b11:
                        instruction.id = InstructionID::SUB;
                        exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, SUB, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_ALU_OP) == VAL_THUMB_ALU_OP) {

                instruction.cat = ThumbInstructionCategory::ALU_OP;

                uint8_t opCode = (lastInst >> 6) & 0x0F;
                instruction.params.alu_op.rd = lastInst & 0x7;
                instruction.params.alu_op.rs = (lastInst >> 3) & 0x7;

                switch (opCode) {
                    case 0b0000:
                        instruction.id = InstructionID::AND;
                        exec.Executor::template operator()<ALU_OP, AND, ThumbInstruction &>(instruction);
                        break;
                    case 0b0001:
                        instruction.id = InstructionID::EOR;
                        exec.Executor::template operator()<ALU_OP, EOR, ThumbInstruction &>(instruction);
                        break;
                    case 0b0010:
                        instruction.id = InstructionID::LSL;
                        exec.Executor::template operator()<ALU_OP, LSL, ThumbInstruction &>(instruction);
                        break;
                    case 0b0011:
                        instruction.id = InstructionID::LSR;
                        exec.Executor::template operator()<ALU_OP, LSR, ThumbInstruction &>(instruction);
                        break;
                    case 0b0100:
                        instruction.id = InstructionID::ASR;
                        exec.Executor::template operator()<ALU_OP, ASR, ThumbInstruction &>(instruction);
                        break;
                    case 0b0101:
                        instruction.id = InstructionID::ADC;
                        exec.Executor::template operator()<ALU_OP, ADC, ThumbInstruction &>(instruction);
                        break;
                    case 0b0110:
                        instruction.id = InstructionID::SBC;
                        exec.Executor::template operator()<ALU_OP, SBC, ThumbInstruction &>(instruction);
                        break;
                    case 0b0111:
                        instruction.id = InstructionID::ROR;
                        exec.Executor::template operator()<ALU_OP, ROR, ThumbInstruction &>(instruction);
                        break;
                    case 0b1000:
                        instruction.id = InstructionID::TST;
                        exec.Executor::template operator()<ALU_OP, TST, ThumbInstruction &>(instruction);
                        break;
                    case 0b1001:
                        instruction.id = InstructionID::NEG;
                        exec.Executor::template operator()<ALU_OP, NEG, ThumbInstruction &>(instruction);
                        break;
                    case 0b1010:
                        instruction.id = InstructionID::CMP;
                        exec.Executor::template operator()<ALU_OP, CMP, ThumbInstruction &>(instruction);
                        break;
                    case 0b1011:
                        instruction.id = InstructionID::CMN;
                        exec.Executor::template operator()<ALU_OP, CMN, ThumbInstruction &>(instruction);
                        break;
                    case 0b1100:
                        instruction.id = InstructionID::ORR;
                        exec.Executor::template operator()<ALU_OP, ORR, ThumbInstruction &>(instruction);
                        break;
                    case 0b1101:
                        instruction.id = InstructionID::MUL;
                        exec.Executor::template operator()<ALU_OP, MUL, ThumbInstruction &>(instruction);
                        break;
                    case 0b1110:
                        instruction.id = InstructionID::BIC;
                        exec.Executor::template operator()<ALU_OP, BIC, ThumbInstruction &>(instruction);
                        break;
                    case 0b1111:
                        instruction.id = InstructionID::MVN;
                        exec.Executor::template operator()<ALU_OP, MVN, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_BR_XCHG) == VAL_THUMB_BR_XCHG) {

                instruction.cat = ThumbInstructionCategory::BR_XCHG;

                uint8_t opCode = (lastInst >> 8) & 0x3;
                // Destination Register most significant bit (or BL/BLX flag)
                uint8_t msbDst = (lastInst >> 7) & 1;
                // Source Register most significant bit
                uint8_t msbSrc = (lastInst >> 6) & 1;
                uint8_t rd = (lastInst & 0x7) | (msbDst << 3);
                uint8_t rs = ((lastInst >> 3) & 0x7) | (msbSrc << 3);
                instruction.params.br_xchg.rd = rd;
                instruction.params.br_xchg.rs = rs;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::ADD;
                        exec.Executor::template operator()<BR_XCHG, ADD, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::CMP;
                        exec.Executor::template operator()<BR_XCHG, CMP, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        //Assemblers/Disassemblers should use MOV R8,R8 as NOP (in THUMB mode)
                        if (rd == rs && rd == regs::R8_OFFSET) {
                            instruction.id = InstructionID::NOP;
                            exec.Executor::template operator()<BR_XCHG, NOP, ThumbInstruction &>(instruction);
                        } else {
                            instruction.id = InstructionID::MOV;
                            exec.Executor::template operator()<BR_XCHG, MOV, ThumbInstruction &>(instruction);
                        }
                        break;
                    case 0b11:
                        if (msbDst) {
                            //instruction.id = InstructionID::BLX;
                        } else {
                            instruction.id = InstructionID::BX;
                            exec.Executor::template operator()<BR_XCHG, BX, ThumbInstruction &>(instruction);
                        }
                        break;
                }
            } else if ((lastInst & MASK_THUMB_PC_LD) == VAL_THUMB_PC_LD) {

                instruction.cat = ThumbInstructionCategory::PC_LD;
                instruction.id = InstructionID::LDR;

                instruction.params.pc_ld.offset = lastInst & 0x0FF;
                instruction.params.pc_ld.rd = (lastInst >> 8) & 0x7;
            } else if ((lastInst & MASK_THUMB_LD_ST_REL_OFF) == VAL_THUMB_LD_ST_REL_OFF) {

                instruction.cat = ThumbInstructionCategory::LD_ST_REL_OFF;

                uint8_t opCode = (lastInst >> 10) & 3;
                instruction.params.ld_st_rel_off.l = opCode & 0x2;
                instruction.params.ld_st_rel_off.b = opCode & 0x1;
                instruction.params.ld_st_rel_off.ro = (lastInst >> 6) & 0x7;
                instruction.params.ld_st_rel_off.rb = (lastInst >> 3) & 0x7;
                instruction.params.ld_st_rel_off.rd = lastInst & 0x7;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::STR;
                        exec.Executor::template operator()<LD_ST_REL_OFF, STR, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::STRB;
                        exec.Executor::template operator()<LD_ST_REL_OFF, STRB, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::LDR;
                        exec.Executor::template operator()<LD_ST_REL_OFF, LDR, ThumbInstruction &>(instruction);
                        break;
                    case 0b11:
                        instruction.id = InstructionID::LDRB;
                        exec.Executor::template operator()<LD_ST_REL_OFF, LDRB, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_LD_ST_SIGN_EXT) == VAL_THUMB_LD_ST_SIGN_EXT) {

                instruction.cat = ThumbInstructionCategory::LD_ST_SIGN_EXT;

                uint8_t opCode = (lastInst >> 10) & 3;
                instruction.params.ld_st_sign_ext.h = opCode & 0x2;
                instruction.params.ld_st_sign_ext.s = opCode & 0x1;
                instruction.params.ld_st_sign_ext.ro = (lastInst >> 6) & 0x7;
                instruction.params.ld_st_sign_ext.rb = (lastInst >> 3) & 0x7;
                instruction.params.ld_st_sign_ext.rd = lastInst & 0x7;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::STRH;
                        exec.Executor::template operator()<LD_ST_SIGN_EXT, STRH, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::LDRSB;
                        exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRSB, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::LDRH;
                        exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRH, ThumbInstruction &>(instruction);
                        break;
                    case 0b11:
                        instruction.id = InstructionID::LDRSH;
                        exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRSH, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_LD_ST_IMM_OFF) == VAL_THUMB_LD_ST_IMM_OFF) {

                instruction.cat = ThumbInstructionCategory::LD_ST_IMM_OFF;

                uint8_t opCode = (lastInst >> 11) & 3;
                instruction.params.ld_st_imm_off.l = opCode & 0x1;
                instruction.params.ld_st_imm_off.b = opCode & 0x2;
                instruction.params.ld_st_imm_off.offset = (lastInst >> 6) & 0x1F;
                instruction.params.ld_st_imm_off.rb = (lastInst >> 3) & 0x7;
                instruction.params.ld_st_imm_off.rd = lastInst & 0x7;

                switch (opCode) {
                    case 0b00:
                        instruction.id = InstructionID::STR;
                        exec.Executor::template operator()<LD_ST_IMM_OFF, STR, ThumbInstruction &>(instruction);
                        break;
                    case 0b01:
                        instruction.id = InstructionID::LDR;
                        exec.Executor::template operator()<LD_ST_IMM_OFF, LDR, ThumbInstruction &>(instruction);
                        break;
                    case 0b10:
                        instruction.id = InstructionID::STRB;
                        exec.Executor::template operator()<LD_ST_IMM_OFF, STRB, ThumbInstruction &>(instruction);
                        break;
                    case 0b11:
                        instruction.id = InstructionID::LDRB;
                        exec.Executor::template operator()<LD_ST_IMM_OFF, LDRB, ThumbInstruction &>(instruction);
                        break;
                }
            } else if ((lastInst & MASK_THUMB_LD_ST_HW) == VAL_THUMB_LD_ST_HW) {

                instruction.cat = ThumbInstructionCategory::LD_ST_HW;

                bool l = (lastInst >> 11) & 0x1;
                instruction.params.ld_st_hw.l = l;
                instruction.params.ld_st_hw.offset = (lastInst >> 6) & 0x1F;
                instruction.params.ld_st_hw.rb = (lastInst >> 3) & 0x7;
                instruction.params.ld_st_hw.rd = lastInst & 0x7;

                if (l) {
                    instruction.id = InstructionID::LDRH;
                    exec.Executor::template operator()<LD_ST_HW, LDRH, ThumbInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STRH;
                    exec.Executor::template operator()<LD_ST_HW, STRH, ThumbInstruction &>(instruction);
                }
            } else if ((lastInst & MASK_THUMB_LD_ST_REL_SP) == VAL_THUMB_LD_ST_REL_SP) {

                instruction.cat = ThumbInstructionCategory::LD_ST_REL_SP;

                bool l = (lastInst >> 11) & 0x1;
                instruction.params.ld_st_rel_sp.l = l;
                instruction.params.ld_st_rel_sp.offset = lastInst & 0x0FF;
                instruction.params.ld_st_rel_sp.rd = (lastInst >> 8) & 0x7;

                if (l) {
                    instruction.id = InstructionID::LDR;
                    exec.Executor::template operator()<LD_ST_REL_SP, LDR, ThumbInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STR;
                    exec.Executor::template operator()<LD_ST_REL_SP, STR, ThumbInstruction &>(instruction);
                }
            } else if ((lastInst & MASK_THUMB_LOAD_ADDR) == VAL_THUMB_LOAD_ADDR) {

                instruction.cat = ThumbInstructionCategory::LOAD_ADDR;

                bool sp = (lastInst >> 11) & 0x1;
                instruction.params.load_addr.sp = sp;
                instruction.params.load_addr.offset = lastInst & 0x0FF;
                instruction.params.load_addr.rd = (lastInst >> 8) & 0x7;

                instruction.id = InstructionID::ADD;
                exec.Executor::template operator()<LOAD_ADDR, ADD, ThumbInstruction &>(instruction);
            } else if ((lastInst & MASK_THUMB_ADD_OFFSET_TO_STACK_PTR) == VAL_THUMB_ADD_OFFSET_TO_STACK_PTR) {

                instruction.cat = ThumbInstructionCategory::ADD_OFFSET_TO_STACK_PTR;

                bool s = (lastInst >> 7) & 0x1;
                instruction.params.add_offset_to_stack_ptr.s = s;
                instruction.params.add_offset_to_stack_ptr.offset = lastInst & 0x7F;

                instruction.id = InstructionID::ADD;
                exec.Executor::template operator()<ADD_OFFSET_TO_STACK_PTR, ADD, ThumbInstruction &>(instruction);
            } else if ((lastInst & MASK_THUMB_PUSH_POP_REG) == VAL_THUMB_PUSH_POP_REG) {

                instruction.cat = ThumbInstructionCategory::PUSH_POP_REG;

                bool l = lastInst & (1 << 11);
                bool r = lastInst & (1 << 8);
                instruction.params.push_pop_reg.l = l;
                instruction.params.push_pop_reg.r = r;
                instruction.params.push_pop_reg.rlist = lastInst & 0x0FF;

                if (l) {
                    instruction.id = InstructionID::POP;
                    exec.Executor::template operator()<PUSH_POP_REG, POP, ThumbInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::PUSH;
                    exec.Executor::template operator()<PUSH_POP_REG, PUSH, ThumbInstruction &>(instruction);
                }
            } else if ((lastInst & MASK_THUMB_MULT_LOAD_STORE) == VAL_THUMB_MULT_LOAD_STORE) {

                instruction.cat = ThumbInstructionCategory::MULT_LOAD_STORE;

                bool l = lastInst & (1 << 11);
                instruction.params.mult_load_store.l = l;
                instruction.params.mult_load_store.rb = (lastInst >> 8) & 0x7;
                instruction.params.mult_load_store.rlist = lastInst & 0x0FF;

                if (l) {
                    instruction.id = InstructionID::LDMIA;
                    exec.Executor::template operator()<MULT_LOAD_STORE, LDMIA, ThumbInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STMIA;
                    exec.Executor::template operator()<MULT_LOAD_STORE, STMIA, ThumbInstruction &>(instruction);
                }
            } else if ((lastInst & MASK_THUMB_SOFTWARE_INTERRUPT) == VAL_THUMB_SOFTWARE_INTERRUPT) {

                instruction.cat = ThumbInstructionCategory::SOFTWARE_INTERRUPT;

                instruction.id = InstructionID::SWI;
                instruction.params.software_interrupt.comment = lastInst & 0x0FF;
                exec.Executor::template operator()<SOFTWARE_INTERRUPT, SWI, ThumbInstruction &>(instruction);
            } else if ((lastInst & MASK_THUMB_COND_BRANCH) == VAL_THUMB_COND_BRANCH) {

                instruction.cat = ThumbInstructionCategory::COND_BRANCH;

                uint8_t cond = (lastInst >> 8) & 0x0F;
                instruction.params.cond_branch.cond = cond;
                instruction.params.cond_branch.offset = static_cast<int8_t>(lastInst & 0x0FF);

                instruction.id = InstructionID::B;
                exec.Executor::template operator()<COND_BRANCH, B, ThumbInstruction &>(instruction);

            } else if ((lastInst & MASK_THUMB_UNCONDITIONAL_BRANCH) == VAL_THUMB_UNCONDITIONAL_BRANCH) {

                instruction.cat = ThumbInstructionCategory::UNCONDITIONAL_BRANCH;

                instruction.id = InstructionID::B;

                // first we extract the 11 bit offset, then we place it at the MSB so we can automatically sign extend it after casting to a signed type
                // finally shift back where it came from but this time with correct sign
                instruction.params.unconditional_branch.offset = signExt<int16_t, uint16_t, 11>(static_cast<uint16_t>(lastInst & 0x07FF));

                exec.Executor::template operator()<UNCONDITIONAL_BRANCH, B, ThumbInstruction &>(instruction);
            } else if ((lastInst & MASK_THUMB_LONG_BRANCH_WITH_LINK) == VAL_THUMB_LONG_BRANCH_WITH_LINK) {

                instruction.cat = ThumbInstructionCategory::LONG_BRANCH_WITH_LINK;
                instruction.id = InstructionID::B;

                bool h = lastInst & (1 << 11);
                instruction.params.long_branch_with_link.h = h;
                instruction.params.long_branch_with_link.offset = lastInst & 0x07FF;

                exec.Executor::template operator()<LONG_BRANCH_WITH_LINK, B, ThumbInstruction &>(instruction);
            } else {
                exec.Executor::template operator()<INVALID_CAT, INVALID, ThumbInstruction &>(instruction);
            }
        }

    } // namespace thumb
} // namespace gbaemu
