#include "inst_thumb.hpp"

#include <iostream>

namespace gbaemu
{

    namespace thumb
    {

        Instruction ThumbInstructionDecoder::decode(uint32_t lastInst) const
        {
            ThumbInstruction instruction;
            instruction.id = ThumbInstructionID::INVALID;

            if ((lastInst & MASK_THUMB_ADD_SUB) == VAL_THUMB_ADD_SUB) {

                instruction.cat = ThumbInstructionCategory::ADD_SUB;

                uint8_t opCode = (lastInst >> 9) & 0x3;

                instruction.params.add_sub.rd = lastInst & 0x7;
                instruction.params.add_sub.rs = (lastInst >> 3) & 0x7;
                instruction.params.add_sub.rn_offset = (lastInst >> 6) & 0x7;

                switch (opCode) {
                    case 0b00:
                        instruction.id = ThumbInstructionID::ADD;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::SUB;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::ADD_SHORT_IMM;
                        break;
                    case 0b11:
                        instruction.id = ThumbInstructionID::SUB_SHORT_IMM;
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
                        instruction.id = ThumbInstructionID::LSL;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::LSR;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::ASR;
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
                        instruction.id = ThumbInstructionID::MOV;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::CMP;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::ADD;
                        break;
                    case 0b11:
                        instruction.id = ThumbInstructionID::SUB;
                        break;
                }
            } else if ((lastInst & MASK_THUMB_ALU_OP) == VAL_THUMB_ALU_OP) {

                instruction.cat = ThumbInstructionCategory::ALU_OP;

                uint8_t opCode = (lastInst >> 6) & 0x0F;
                instruction.params.alu_op.rd = lastInst & 0x7;
                instruction.params.alu_op.rs = (lastInst >> 3) & 0x7;

                switch (opCode) {
                    case 0b0000:
                        instruction.id = ThumbInstructionID::AND;
                        break;
                    case 0b0001:
                        instruction.id = ThumbInstructionID::EOR;
                        break;
                    case 0b0010:
                        instruction.id = ThumbInstructionID::LSL;
                        break;
                    case 0b0011:
                        instruction.id = ThumbInstructionID::LSR;
                        break;
                    case 0b0100:
                        instruction.id = ThumbInstructionID::ASR;
                        break;
                    case 0b0101:
                        instruction.id = ThumbInstructionID::ADC;
                        break;
                    case 0b0110:
                        instruction.id = ThumbInstructionID::SBC;
                        break;
                    case 0b0111:
                        instruction.id = ThumbInstructionID::ROR;
                        break;
                    case 0b1000:
                        instruction.id = ThumbInstructionID::TST;
                        break;
                    case 0b1001:
                        instruction.id = ThumbInstructionID::NEG;
                        break;
                    case 0b1010:
                        instruction.id = ThumbInstructionID::CMP;
                        break;
                    case 0b1011:
                        instruction.id = ThumbInstructionID::CMN;
                        break;
                    case 0b1100:
                        instruction.id = ThumbInstructionID::ORR;
                        break;
                    case 0b1101:
                        instruction.id = ThumbInstructionID::MUL;
                        break;
                    case 0b1110:
                        instruction.id = ThumbInstructionID::BIC;
                        break;
                    case 0b1111:
                        instruction.id = ThumbInstructionID::MVN;
                        break;
                }
            } else if ((lastInst & MASK_THUMB_BR_XCHG) == VAL_THUMB_BR_XCHG) {

                instruction.cat = ThumbInstructionCategory::BR_XCHG;

                uint8_t opCode = (lastInst >> 8) & 0x3;
                // Destination Register most significant bit (or BL/BLX flag)
                uint8_t msbDst = (lastInst >> 7) & 1;
                // Source Register most significant bit
                uint8_t msbSrc = (lastInst >> 6) & 1;
                uint8_t rd = (lastInst & 0x7) | (msbDst << 4);
                uint8_t rs = ((lastInst >> 3) & 0x7) | (msbSrc << 4);
                instruction.params.br_xchg.rd = rd;
                instruction.params.br_xchg.rs = rs;

                switch (opCode) {
                    case 0b00:
                        instruction.id = ThumbInstructionID::ADD;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::CMP;
                        break;
                    case 0b10:
                        //Assemblers/Disassemblers should use MOV R8,R8 as NOP (in THUMB mode)
                        if (rd == rs && rd == regs::R8_OFFSET) {
                            instruction.id = ThumbInstructionID::NOP;
                        } else {
                            instruction.id = ThumbInstructionID::MOV;
                        }
                        break;
                    case 0b11:
                        if (msbDst) {
                            instruction.id = ThumbInstructionID::BLX;
                        } else {
                            instruction.id = ThumbInstructionID::BX;
                        }
                        break;
                }
            } else if ((lastInst & MASK_THUMB_PC_LD) == VAL_THUMB_PC_LD) {

                instruction.cat = ThumbInstructionCategory::PC_LD;
                instruction.id = ThumbInstructionID::LDR;

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
                        instruction.id = ThumbInstructionID::STR;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::STRB;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::LDR;
                        break;
                    case 0b11:
                        instruction.id = ThumbInstructionID::LDRB;
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
                        instruction.id = ThumbInstructionID::STRH;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::LDSB;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::LDRH;
                        break;
                    case 0b11:
                        instruction.id = ThumbInstructionID::LDSH;
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
                        instruction.id = ThumbInstructionID::STR;
                        break;
                    case 0b01:
                        instruction.id = ThumbInstructionID::LDR;
                        break;
                    case 0b10:
                        instruction.id = ThumbInstructionID::STRB;
                        break;
                    case 0b11:
                        instruction.id = ThumbInstructionID::LDRB;
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
                    instruction.id = ThumbInstructionID::LDRH;
                } else {
                    instruction.id = ThumbInstructionID::STRH;
                }
            } else if ((lastInst & MASK_THUMB_LD_ST_REL_SP) == VAL_THUMB_LD_ST_REL_SP) {

                instruction.cat = ThumbInstructionCategory::LD_ST_REL_SP;

                bool l = (lastInst >> 11) & 0x1;
                instruction.params.ld_st_rel_sp.l = l;
                instruction.params.ld_st_rel_sp.offset = lastInst & 0x0FF;
                instruction.params.ld_st_rel_sp.rd = (lastInst >> 8) & 0x7;

                if (l) {
                    instruction.id = ThumbInstructionID::LDR;
                } else {
                    instruction.id = ThumbInstructionID::STR;
                }
            } else if ((lastInst & MASK_THUMB_LOAD_ADDR) == VAL_THUMB_LOAD_ADDR) {

                instruction.cat = ThumbInstructionCategory::LOAD_ADDR;

                bool sp = (lastInst >> 11) & 0x1;
                instruction.params.load_addr.sp = sp;
                instruction.params.load_addr.offset = lastInst & 0x0FF;
                instruction.params.load_addr.rd = (lastInst >> 8) & 0x7;

                instruction.id = ThumbInstructionID::ADD;
            } else if ((lastInst & MASK_THUMB_ADD_OFFSET_TO_STACK_PTR) == VAL_THUMB_ADD_OFFSET_TO_STACK_PTR) {

                instruction.cat = ThumbInstructionCategory::ADD_OFFSET_TO_STACK_PTR;

                bool s = (lastInst >> 7) & 0x1;
                instruction.params.add_offset_to_stack_ptr.s = s;
                instruction.params.add_offset_to_stack_ptr.offset = lastInst & 0x7F;

                instruction.id = ThumbInstructionID::ADD;
            } else if ((lastInst & MASK_THUMB_PUSH_POP_REG) == VAL_THUMB_PUSH_POP_REG) {

                instruction.cat = ThumbInstructionCategory::PUSH_POP_REG;

                bool l = lastInst & (1 << 11);
                bool r = lastInst & (1 << 8);
                instruction.params.push_pop_reg.l = l;
                instruction.params.push_pop_reg.r = r;
                instruction.params.push_pop_reg.rlist = lastInst & 0x0FF;

                if (l) {
                    instruction.id = ThumbInstructionID::POP;
                } else {
                    instruction.id = ThumbInstructionID::PUSH;
                }
            } else if ((lastInst & MASK_THUMB_MULT_LOAD_STORE) == VAL_THUMB_MULT_LOAD_STORE) {

                instruction.cat = ThumbInstructionCategory::MULT_LOAD_STORE;

                bool l = lastInst & (1 << 11);
                instruction.params.mult_load_store.l = l;
                instruction.params.mult_load_store.rb = (lastInst >> 8) & 0x7;
                instruction.params.mult_load_store.rlist = lastInst & 0x0FF;

                if (l) {
                    instruction.id = ThumbInstructionID::LDMIA;
                } else {
                    instruction.id = ThumbInstructionID::STMIA;
                }
            } else if ((lastInst & MASK_THUMB_SOFTWARE_INTERRUPT) == VAL_THUMB_SOFTWARE_INTERRUPT) {

                instruction.cat = ThumbInstructionCategory::SOFTWARE_INTERRUPT;

                instruction.id = ThumbInstructionID::SWI;
                instruction.params.software_interrupt.comment = lastInst & 0x0FF;
            } else if ((lastInst & MASK_THUMB_COND_BRANCH) == VAL_THUMB_COND_BRANCH) {

                instruction.cat = ThumbInstructionCategory::COND_BRANCH;

                uint8_t cond = (lastInst >> 8) & 0x0F;
                instruction.params.cond_branch.cond = cond;
                instruction.params.cond_branch.offset = static_cast<int8_t>(lastInst & 0x0FF);

                switch (cond) {
                    case 0b0000:
                        instruction.id = ThumbInstructionID::BEQ;
                        break;
                    case 0b0001:
                        instruction.id = ThumbInstructionID::BNE;
                        break;
                    case 0b0010:
                        instruction.id = ThumbInstructionID::BCS_BHS;
                        break;
                    case 0b0011:
                        instruction.id = ThumbInstructionID::BCC_BLO;
                        break;
                    case 0b0100:
                        instruction.id = ThumbInstructionID::BMI;
                        break;
                    case 0b0101:
                        instruction.id = ThumbInstructionID::BPL;
                        break;
                    case 0b0110:
                        instruction.id = ThumbInstructionID::BVS;
                        break;
                    case 0b0111:
                        instruction.id = ThumbInstructionID::BVC;
                        break;
                    case 0b1000:
                        instruction.id = ThumbInstructionID::BHI;
                        break;
                    case 0b1001:
                        instruction.id = ThumbInstructionID::BLS;
                        break;
                    case 0b1010:
                        instruction.id = ThumbInstructionID::BGE;
                        break;
                    case 0b1011:
                        instruction.id = ThumbInstructionID::BLT;
                        break;
                    case 0b1100:
                        instruction.id = ThumbInstructionID::BGT;
                        break;
                    case 0b1101:
                        instruction.id = ThumbInstructionID::BLE;
                        break;
                        // Undefined
                    case 0b1110:
                        std::cerr << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
                        break;

                        // This case belongs to SOFTWARE_INTERRUPT
                    case 0b1111:
                        break;
                }
            } else if ((lastInst & MASK_THUMB_UNCONDITIONAL_BRANCH) == VAL_THUMB_UNCONDITIONAL_BRANCH) {

                instruction.cat = ThumbInstructionCategory::UNCONDITIONAL_BRANCH;

                instruction.id = ThumbInstructionID::B;

                // first we extract the 11 bit offset, then we place it at the MSB so we can automatically sign extend it after casting to a signed type
                // finally shift back where it came from but this time with correct sign
                instruction.params.unconditional_branch.offset = static_cast<int16_t>((lastInst & 0x07FF) << 5) >> 5;
            } else if ((lastInst & MASK_THUMB_LONG_BRANCH_WITH_LINK) == VAL_THUMB_LONG_BRANCH_WITH_LINK) {

                instruction.cat = ThumbInstructionCategory::LONG_BRANCH_WITH_LINK;

                bool h = lastInst & (1 << 11);
                instruction.params.long_branch_with_link.h = h;
                instruction.params.long_branch_with_link.offset = lastInst & 0x07FF;

                //TODO I do not understand this instruction...
                /*
Unlike all other THUMB mode instructions, this instruction occupies 32bit of memory which are split into two 16bit THUMB opcodes.
 First Instruction - LR = PC+4+(nn SHL 12)
  15-11  Must be 11110b for BL/BLX type of instructions
  10-0   nn - Upper 11 bits of Target Address
 Second Instruction - PC = LR + (nn SHL 1), and LR = PC+2 OR 1 (and BLX: T=0)
  15-11  Opcode
          11111b: BL label   ;branch long with link
          11101b: BLX label  ;branch long with link switch to ARM mode (ARM9)
  10-0   nn - Lower 11 bits of Target Address (BLX: Bit0 Must be zero)
The destination address range is (PC+4)-400000h..+3FFFFEh, ie. PC+/-4M.
Target must be halfword-aligned. As Bit 0 in LR is set, it may be used to return by a BX LR instruction (keeping CPU in THUMB mode).
Return: No flags affected, PC adjusted, return address in LR.
Execution Time: 3S+1N (first opcode 1S, second opcode 2S+1N).
Note: Exceptions may or may not occur between first and second opcode, this is "implementation defined" (unknown how this is implemented in GBA and NDS).
Using only the 2nd half of BL as "BL LR+imm" is possible (for example, Mario Golf Advance Tour for GBA uses opcode F800h as "BL LR+0").
            */
            }

            return Instruction::fromThumb(instruction);
        }

    } // namespace thumb
} // namespace gbaemu
