#include "inst_thumb.hpp"
#include "swi.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu
{

    namespace thumb
    {

        const char *instructionIDToString(ThumbInstructionID id)
        {
            switch (id) {
                STRINGIFY_CASE_ID(MVN);
                STRINGIFY_CASE_ID(AND);
                STRINGIFY_CASE_ID(TST);
                STRINGIFY_CASE_ID(BIC);
                STRINGIFY_CASE_ID(ORR);
                STRINGIFY_CASE_ID(EOR);
                STRINGIFY_CASE_ID(LSL);
                STRINGIFY_CASE_ID(LSR);
                STRINGIFY_CASE_ID(ASR);
                STRINGIFY_CASE_ID(ROR);
                STRINGIFY_CASE_ID(NOP);
                STRINGIFY_CASE_ID(ADC);
                STRINGIFY_CASE_ID(ADD);
                STRINGIFY_CASE_ID(ADD_SHORT_IMM);
                STRINGIFY_CASE_ID(SUB);
                STRINGIFY_CASE_ID(SUB_SHORT_IMM);
                STRINGIFY_CASE_ID(MOV);
                STRINGIFY_CASE_ID(CMP);
                STRINGIFY_CASE_ID(SBC);
                STRINGIFY_CASE_ID(NEG);
                STRINGIFY_CASE_ID(CMN);
                STRINGIFY_CASE_ID(MUL);
                STRINGIFY_CASE_ID(BX);
                // This one is ARM9
                //STRINGIFY_CASE_ID(BLX);
                STRINGIFY_CASE_ID(POP);
                STRINGIFY_CASE_ID(LDR);
                STRINGIFY_CASE_ID(LDRB);
                STRINGIFY_CASE_ID(LDRH);
                STRINGIFY_CASE_ID(LDSB);
                STRINGIFY_CASE_ID(LDSH);
                STRINGIFY_CASE_ID(STR);
                STRINGIFY_CASE_ID(STRB);
                STRINGIFY_CASE_ID(STRH);
                STRINGIFY_CASE_ID(PUSH);
                STRINGIFY_CASE_ID(STMIA);
                STRINGIFY_CASE_ID(LDMIA);
                /*
                STRINGIFY_CASE_ID(BEQ);
                STRINGIFY_CASE_ID(BNE);
                STRINGIFY_CASE_ID(BCS_BHS);
                STRINGIFY_CASE_ID(BCC_BLO);
                STRINGIFY_CASE_ID(BMI);
                STRINGIFY_CASE_ID(BPL);
                STRINGIFY_CASE_ID(BVS);
                STRINGIFY_CASE_ID(BVC);
                STRINGIFY_CASE_ID(BHI);
                STRINGIFY_CASE_ID(BLS);
                STRINGIFY_CASE_ID(BGE);
                STRINGIFY_CASE_ID(BLT);
                STRINGIFY_CASE_ID(BGT);
                STRINGIFY_CASE_ID(BLE);
                */
                STRINGIFY_CASE_ID(SWI);
                STRINGIFY_CASE_ID(B);
                STRINGIFY_CASE_ID(INVALID);
            }

            return "NULL";
        }

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

                    if (id == ThumbInstructionID::ADD_SHORT_IMM || id == ThumbInstructionID::SUB_SHORT_IMM)
                        ss << " 0x" << std::hex << rn_off;
                    else
                        ss << " r" << rn_off;
                }

                break;
                case ThumbInstructionCategory::MOV_CMP_ADD_SUB_IMM:
                    ss << " r" << static_cast<uint32_t>(params.mov_cmp_add_sub_imm.rd) << ", 0x" << std::hex << params.mov_cmp_add_sub_imm.offset;
                    break;
                case ThumbInstructionCategory::ALU_OP:
                    ss << " r" << static_cast<uint32_t>(params.alu_op.rd) << ", r" << static_cast<uint32_t>(params.alu_op.rs);
                    break;
                case ThumbInstructionCategory::BR_XCHG:
                    ss << " r" << static_cast<uint32_t>(params.br_xchg.rd) << ", r" << static_cast<uint32_t>(params.br_xchg.rs);
                    break;
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

        Instruction ThumbInstructionDecoder::decode(uint32_t lastInst) const
        {
            ThumbInstruction instruction;
            instruction.id = ThumbInstructionID::INVALID;
            instruction.cat = ThumbInstructionCategory::INVALID_CAT;

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
                uint8_t rd = (lastInst & 0x7) | (msbDst << 3);
                uint8_t rs = ((lastInst >> 3) & 0x7) | (msbSrc << 3);
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
                            //instruction.id = ThumbInstructionID::BLX;
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

                /*
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
                */
                instruction.id = ThumbInstructionID::B;

            } else if ((lastInst & MASK_THUMB_UNCONDITIONAL_BRANCH) == VAL_THUMB_UNCONDITIONAL_BRANCH) {

                instruction.cat = ThumbInstructionCategory::UNCONDITIONAL_BRANCH;

                instruction.id = ThumbInstructionID::B;

                // first we extract the 11 bit offset, then we place it at the MSB so we can automatically sign extend it after casting to a signed type
                // finally shift back where it came from but this time with correct sign
                instruction.params.unconditional_branch.offset = static_cast<int16_t>((lastInst & 0x07FF) << 5) / (1 << 5) /*>> 5*/;
            } else if ((lastInst & MASK_THUMB_LONG_BRANCH_WITH_LINK) == VAL_THUMB_LONG_BRANCH_WITH_LINK) {

                instruction.cat = ThumbInstructionCategory::LONG_BRANCH_WITH_LINK;
                instruction.id = ThumbInstructionID::B;

                bool h = lastInst & (1 << 11);
                instruction.params.long_branch_with_link.h = h;
                instruction.params.long_branch_with_link.offset = lastInst & 0x07FF;
            }

            return Instruction::fromThumb(instruction);
        }

    } // namespace thumb
} // namespace gbaemu
