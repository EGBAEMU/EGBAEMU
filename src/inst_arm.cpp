#include "inst_arm.hpp"

#include "cpu_state.hpp"
#include "swi.hpp"
#include "util.hpp"
#include <iostream>
#include <sstream>

namespace gbaemu
{
    namespace arm
    {

        const char *instructionIDToString(ARMInstructionID id)
        {
            switch (id) {
                STRINGIFY_CASE_ID(ADC);
                STRINGIFY_CASE_ID(ADD);
                STRINGIFY_CASE_ID(AND);
                STRINGIFY_CASE_ID(B);
                STRINGIFY_CASE_ID(BIC);
                STRINGIFY_CASE_ID(BX);
                STRINGIFY_CASE_ID(CMN);
                STRINGIFY_CASE_ID(CMP);
                STRINGIFY_CASE_ID(EOR);
                STRINGIFY_CASE_ID(LDM);
                STRINGIFY_CASE_ID(LDR);
                STRINGIFY_CASE_ID(LDRB);
                STRINGIFY_CASE_ID(LDRH);
                STRINGIFY_CASE_ID(LDRSB);
                STRINGIFY_CASE_ID(LDRSH);
                STRINGIFY_CASE_ID(LDRD);
                STRINGIFY_CASE_ID(MLA);
                STRINGIFY_CASE_ID(MOV);
                STRINGIFY_CASE_ID(MRS);
                STRINGIFY_CASE_ID(MSR);
                STRINGIFY_CASE_ID(MUL);
                STRINGIFY_CASE_ID(MVN);
                STRINGIFY_CASE_ID(ORR);
                STRINGIFY_CASE_ID(RSB);
                STRINGIFY_CASE_ID(RSC);
                STRINGIFY_CASE_ID(SBC);
                STRINGIFY_CASE_ID(SMLAL);
                STRINGIFY_CASE_ID(SMULL);
                STRINGIFY_CASE_ID(STM);
                STRINGIFY_CASE_ID(STR);
                STRINGIFY_CASE_ID(STRB);
                STRINGIFY_CASE_ID(STRH);
                STRINGIFY_CASE_ID(STRD);
                STRINGIFY_CASE_ID(SUB);
                STRINGIFY_CASE_ID(SWI);
                STRINGIFY_CASE_ID(SWP);
                STRINGIFY_CASE_ID(SWPB);
                STRINGIFY_CASE_ID(TEQ);
                STRINGIFY_CASE_ID(TST);
                STRINGIFY_CASE_ID(UMLAL);
                STRINGIFY_CASE_ID(UMULL);
                STRINGIFY_CASE_ID(INVALID);
            }

            return "NULL";
        }

        std::string ARMInstruction::toString() const
        {
            std::stringstream ss;

            ss << '(' << conditionCodeToString(condition) << ") ";

            if (cat == ARMInstructionCategory::DATA_PROC_PSR_TRANSF) {
                /* TODO: probably not done */
                bool hasRN = !(id == ARMInstructionID::MOV || id == ARMInstructionID::MVN);
                bool hasRD = !(id == ARMInstructionID::TST || id == ARMInstructionID::TEQ ||
                               id == ARMInstructionID::CMP || id == ARMInstructionID::CMN);

                auto idStr = instructionIDToString(id);
                uint32_t rd = params.data_proc_psr_transf.rd;
                uint32_t rn = params.data_proc_psr_transf.rn;
                ShiftType shiftType;
                uint8_t shiftAmount;
                uint32_t rm;
                uint32_t rs;
                uint32_t imm;
                bool shiftByReg = params.data_proc_psr_transf.extractOperand2(shiftType, shiftAmount, rm, rs, imm);

                ss << idStr;

                if (id == MSR) {
                    // true iff write to flag field is allowed 31-24
                    bool f = params.data_proc_psr_transf.rn & 0x08;
                    // true iff write to status field is allowed 23-16
                    bool s = params.data_proc_psr_transf.rn & 0x04;
                    // true iff write to extension field is allowed 15-8
                    bool x = params.data_proc_psr_transf.rn & 0x02;
                    // true iff write to control field is allowed 7-0
                    bool c = params.data_proc_psr_transf.rn & 0x01;
                    ss << (params.data_proc_psr_transf.r ? " SPSR_" : " CPSR_");
                    ss << (f ? "f" : "");
                    ss << (s ? "s" : "");
                    ss << (x ? "x" : "");
                    ss << (c ? "c" : "");
                    if (params.data_proc_psr_transf.i) {
                        uint32_t roredImm = shift(imm, shiftType, shiftAmount, false, false);
                        ss << ", #" << roredImm;
                    } else {
                        ss << ", r" << std::dec << rm;
                    }
                } else {
                    if (params.data_proc_psr_transf.s)
                        ss << "{S}";

                    if (hasRD)
                        ss << " r" << rd;

                    if (hasRN)
                        ss << " r" << rn;

                    if (params.data_proc_psr_transf.i) {
                        uint32_t roredImm = shift(imm, shiftType, shiftAmount, false, false);
                        ss << ", #" << roredImm;
                    } else {
                        ss << " r" << rm;

                        if (shiftByReg)
                            ss << "<<r" << std::dec << rs;
                        else if (shiftAmount > 0)
                            ss << "<<" << std::dec << static_cast<uint32_t>(shiftAmount);
                    }
                }
            } else if (cat == ARMInstructionCategory::MUL_ACC) {
                ss << instructionIDToString(id);
                if (params.mul_acc.s)
                    ss << "{S}";
                ss << " r" << params.mul_acc.rd << " r" << params.mul_acc.rm << " r" << params.mul_acc.rs;
                if (params.mul_acc.a)
                    ss << " +r" << params.mul_acc.rn;
            } else if (cat == ARMInstructionCategory::MUL_ACC_LONG) {
                ss << instructionIDToString(id);
                if (params.mul_acc_long.s)
                    ss << "{S}";
                ss << " r" << params.mul_acc_long.rd_msw << ":r" << params.mul_acc_long.rd_lsw << " r" << params.mul_acc_long.rs << " r" << params.mul_acc_long.rm;
            } else if (cat == ARMInstructionCategory::HW_TRANSF_REG_OFF) {
                /* No immediate in this category! */
                ss << instructionIDToString(id) << " r" << params.hw_transf_reg_off.rd;

                /* TODO: does p mean pre? */
                if (params.hw_transf_reg_off.p) {
                    ss << " [r" << params.hw_transf_reg_off.rn << "+r" << params.hw_transf_reg_off.rm << ']';
                } else {
                    ss << " [r" << params.hw_transf_reg_off.rn << "]+r" << params.hw_transf_reg_off.rm;
                }
            } else if (cat == ARMInstructionCategory::HW_TRANSF_IMM_OFF) {
                /* Immediate in this category! */
                ss << instructionIDToString(id) << " r" << params.hw_transf_imm_off.rd;

                if (params.hw_transf_reg_off.p) {
                    ss << " [r" << params.hw_transf_imm_off.rn << "+0x" << std::hex << params.hw_transf_imm_off.offset << ']';
                } else {
                    ss << " [[r" << params.hw_transf_imm_off.rn << "]+0x" << std::hex << params.hw_transf_imm_off.offset << ']';
                }
            } else if (cat == ARMInstructionCategory::LS_REG_UBYTE) {
                bool pre = params.ls_reg_ubyte.p;
                char upDown = params.ls_reg_ubyte.u ? '+' : '-';

                ss << instructionIDToString(id) << " r" << params.ls_reg_ubyte.rd;

                if (!params.ls_reg_ubyte.i) {
                    uint32_t immOff = params.ls_reg_ubyte.addrMode & 0xFFF;

                    if (pre)
                        ss << " [r" << params.ls_reg_ubyte.rn;
                    else
                        ss << " [[r" << params.ls_reg_ubyte.rn << ']';

                    ss << upDown << "0x" << std::hex << immOff << ']';
                } else {
                    uint32_t shiftAmount = (params.ls_reg_ubyte.addrMode >> 7) & 0xF;
                    uint32_t rm = params.ls_reg_ubyte.addrMode & 0xF;

                    if (pre)
                        ss << " [r" << params.ls_reg_ubyte.rn;
                    else
                        ss << " [[r" << params.ls_reg_ubyte.rn << ']';

                    ss << upDown << "(r" << rm << "<<" << shiftAmount << ")]";
                }
            } else if (cat == ARMInstructionCategory::BLOCK_DATA_TRANSF) {
                ss << instructionIDToString(id) << " r" << params.block_data_transf.rn << " { ";

                for (uint32_t i = 0; i < 16; ++i)
                    if (params.block_data_transf.rList & (1 << i))
                        ss << "r" << i << ' ';

                ss << '}';
            } else if (id == ARMInstructionID::B) {
                /*  */
                int32_t off = params.branch.offset * 4;

                ss << "B" << (params.branch.l ? "L" : "") << " "
                   << "PC" << (off < 0 ? '-' : '+') << "0x" << std::hex << std::abs(off);
            } else if (cat == ARMInstructionCategory::SOFTWARE_INTERRUPT) {
                ss << instructionIDToString(id) << " " << swi::swiToString(params.software_interrupt.comment >> 16);
            } else {
                ss << instructionIDToString(id) << "?";
            }

            return ss.str();
        }

        Instruction ARMInstructionDecoder::decode(uint32_t lastInst) const
        {
            ARMInstruction instruction;

            // Default the instruction id to invalid
            instruction.id = ARMInstructionID::INVALID;
            instruction.cat = ARMInstructionCategory::INVALID_CAT;
            instruction.condition = static_cast<ConditionOPCode>(lastInst >> 28);

            if ((lastInst & MASK_MUL_ACC) == VAL_MUL_ACC) {

                instruction.cat = ARMInstructionCategory::MUL_ACC;

                bool a = (lastInst >> 21) & 1;
                bool s = (lastInst >> 20) & 1;
                instruction.params.mul_acc.a = a;
                instruction.params.mul_acc.s = s;

                instruction.params.mul_acc.rd = (lastInst >> 16) & 0x0F;
                instruction.params.mul_acc.rn = (lastInst >> 12) & 0x0F;
                instruction.params.mul_acc.rs = (lastInst >> 8) & 0x0F;
                instruction.params.mul_acc.rm = lastInst & 0x0F;

                if (a) {
                    instruction.id = ARMInstructionID::MLA;
                } else {
                    instruction.id = ARMInstructionID::MUL;
                }

            } else if ((lastInst & MASK_MUL_ACC_LONG) == VAL_MUL_ACC_LONG) {

                instruction.cat = ARMInstructionCategory::MUL_ACC_LONG;

                bool u = (lastInst >> 22) & 1;
                bool a = (lastInst >> 21) & 1;
                bool s = (lastInst >> 20) & 1;

                instruction.params.mul_acc_long.u = u;
                instruction.params.mul_acc_long.a = a;
                instruction.params.mul_acc_long.s = s;

                instruction.params.mul_acc_long.rd_msw = (lastInst >> 16) & 0x0F;
                instruction.params.mul_acc_long.rd_lsw = (lastInst >> 12) & 0x0F;
                instruction.params.mul_acc_long.rs = (lastInst >> 8) & 0x0F;
                instruction.params.mul_acc_long.rm = lastInst & 0x0F;

                if (u && a) {
                    instruction.id = ARMInstructionID::SMLAL;
                } else if (u && !a) {
                    instruction.id = ARMInstructionID::SMULL;
                } else if (!u && a) {
                    instruction.id = ARMInstructionID::UMLAL;
                } else {
                    instruction.id = ARMInstructionID::UMULL;
                }

            } else if ((lastInst & MASK_BRANCH_XCHG) == VAL_BRANCH_XCHG) {

                instruction.cat = ARMInstructionCategory::BRANCH_XCHG;
                instruction.id = ARMInstructionID::BX;
                instruction.params.branch_xchg.rn = lastInst & 0x0F;

            } else if ((lastInst & MASK_DATA_SWP) == VAL_DATA_SWP) {

                instruction.cat = ARMInstructionCategory::DATA_SWP;

                /* also called b */
                bool b = (lastInst >> 22) & 1;

                instruction.params.data_swp.b = b;

                instruction.params.data_swp.rn = (lastInst >> 16) & 0x0F;
                instruction.params.data_swp.rd = (lastInst >> 12) & 0x0F;
                instruction.params.data_swp.rm = lastInst & 0x0F;

                if (!b) {
                    instruction.id = ARMInstructionID::SWP;
                } else {
                    instruction.id = ARMInstructionID::SWPB;
                }

            } else if ((lastInst & MASK_HW_TRANSF_REG_OFF) == VAL_HW_TRANSF_REG_OFF) {
                instruction.cat = ARMInstructionCategory::HW_TRANSF_REG_OFF;

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                instruction.params.hw_transf_reg_off.p = p;
                instruction.params.hw_transf_reg_off.u = u;
                instruction.params.hw_transf_reg_off.w = w;
                instruction.params.hw_transf_reg_off.l = l;

                instruction.params.hw_transf_reg_off.rn = (lastInst >> 16) & 0x0F;
                instruction.params.hw_transf_reg_off.rd = (lastInst >> 12) & 0x0F;
                instruction.params.hw_transf_reg_off.rm = lastInst & 0x0F;

                // register offset variants
                if (l) {
                    instruction.id = ARMInstructionID::LDRH;
                } else {
                    instruction.id = ARMInstructionID::STRH;
                }

            } else if ((lastInst & MASK_HW_TRANSF_IMM_OFF) == VAL_HW_TRANSF_IMM_OFF) {

                instruction.cat = ARMInstructionCategory::HW_TRANSF_IMM_OFF;

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                instruction.params.hw_transf_imm_off.p = p;
                instruction.params.hw_transf_imm_off.u = u;
                instruction.params.hw_transf_imm_off.w = w;
                instruction.params.hw_transf_imm_off.l = l;

                instruction.params.hw_transf_imm_off.rn = (lastInst >> 16) & 0x0F;
                instruction.params.hw_transf_imm_off.rd = (lastInst >> 12) & 0x0F;

                /* called addr_mode in detailed doc but is really offset because immediate flag I is 1 */
                instruction.params.hw_transf_imm_off.offset = (((lastInst >> 8) & 0x0F) << 4) | (lastInst & 0x0F);

                if (l) {
                    instruction.id = ARMInstructionID::LDRH;
                } else {
                    instruction.id = ARMInstructionID::STRH;
                }

            } else if ((lastInst & MASK_SIGN_TRANSF) == VAL_SIGN_TRANSF) {

                instruction.cat = ARMInstructionCategory::SIGN_TRANSF;

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

                instruction.params.sign_transf.addrMode = (((lastInst >> 8) & 0x0F) << 4) | (lastInst & 0x0F);

                if (l && !h) {
                    instruction.id = ARMInstructionID::LDRSB;
                } else if (l && h) {
                    instruction.id = ARMInstructionID::LDRSH;
                } else if (!l && h) {
                    instruction.id = ARMInstructionID::STRD;
                } else if (!l && !h) {
                    instruction.id = ARMInstructionID::LDRD;
                }

            } else if ((lastInst & MASK_DATA_PROC_PSR_TRANSF) == VAL_DATA_PROC_PSR_TRANSF) {

                instruction.cat = ARMInstructionCategory::DATA_PROC_PSR_TRANSF;

                uint32_t opCode = (lastInst >> 21) & 0x0F;
                bool i = (lastInst >> 25) & 1;
                bool s = lastInst & (1 << 20);
                bool r = (lastInst >> 2) & 1;

                instruction.params.data_proc_psr_transf.opCode = opCode;
                instruction.params.data_proc_psr_transf.i = i;
                instruction.params.data_proc_psr_transf.s = s;
                instruction.params.data_proc_psr_transf.r = r;

                instruction.params.data_proc_psr_transf.rn = (lastInst >> 16) & 0x0F;
                instruction.params.data_proc_psr_transf.rd = (lastInst >> 12) & 0x0F;
                /* often shifter */
                instruction.params.data_proc_psr_transf.operand2 = lastInst & 0x0FFF;

                switch (opCode) {
                    case 0b0101:
                        instruction.id = ARMInstructionID::ADC;
                        break;
                    case 0b0100:
                        instruction.id = ARMInstructionID::ADD;
                        break;
                    case 0b0000:
                        instruction.id = ARMInstructionID::AND;
                        break;
                    case 0b1010:
                        if (s) {
                            instruction.id = ARMInstructionID::CMP;
                        } else {
                            instruction.id = ARMInstructionID::MRS;
                        }
                        break;
                    case 0b1011:
                        if (s) {
                            instruction.id = ARMInstructionID::CMN;
                        } else {
                            instruction.id = ARMInstructionID::MSR;
                        }
                        break;
                    case 0b0001:
                        instruction.id = ARMInstructionID::EOR;
                        break;
                    case 0b1101:
                        instruction.id = ARMInstructionID::MOV;
                        break;
                    case 0b1110:
                        instruction.id = ARMInstructionID::BIC;
                        break;
                    case 0b1111:
                        instruction.id = ARMInstructionID::MVN;
                        break;
                    case 0b1100:
                        instruction.id = ARMInstructionID::ORR;
                        break;
                    case 0b0011:
                        instruction.id = ARMInstructionID::RSB;
                        break;
                    case 0b0111:
                        instruction.id = ARMInstructionID::RSC;
                        break;
                    case 0b0110:
                        instruction.id = ARMInstructionID::SBC;
                        break;
                    case 0b0010:
                        instruction.id = ARMInstructionID::SUB;
                        break;
                    case 0b1001:
                        if (s) {
                            instruction.id = ARMInstructionID::TEQ;
                        } else {
                            instruction.id = ARMInstructionID::MSR;
                        }
                        break;
                    case 0b1000:
                        if (s) {
                            instruction.id = ARMInstructionID::TST;
                        } else {
                            instruction.id = ARMInstructionID::MRS;
                        }
                        break;
                }
            } else if ((lastInst & MASK_LS_REG_UBYTE) == VAL_LS_REG_UBYTE) {

                instruction.cat = ARMInstructionCategory::LS_REG_UBYTE;

                bool i = (lastInst >> 25) & 1;
                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool b = (lastInst >> 22) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                instruction.params.ls_reg_ubyte.i = i;
                instruction.params.ls_reg_ubyte.p = p;
                instruction.params.ls_reg_ubyte.u = u;
                instruction.params.ls_reg_ubyte.b = b;
                instruction.params.ls_reg_ubyte.w = w;
                instruction.params.ls_reg_ubyte.l = l;

                instruction.params.ls_reg_ubyte.rn = (lastInst >> 16) & 0x0F;
                instruction.params.ls_reg_ubyte.rd = (lastInst >> 12) & 0x0F;
                instruction.params.ls_reg_ubyte.addrMode = lastInst & 0x0FFF;

                if (!b && l) {
                    instruction.id = ARMInstructionID::LDR;
                } else if (b && l) {
                    instruction.id = ARMInstructionID::LDRB;
                } else if (!b && !l) {
                    instruction.id = ARMInstructionID::STR;
                } else {
                    instruction.id = ARMInstructionID::STRB;
                }

            } else if ((lastInst & MASK_UNDEFINED) == VAL_UNDEFINED) {
                std::cerr << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
            } else if ((lastInst & MASK_BLOCK_DATA_TRANSF) == VAL_BLOCK_DATA_TRANSF) {

                instruction.cat = ARMInstructionCategory::BLOCK_DATA_TRANSF;

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool s = (lastInst >> 22) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                instruction.params.block_data_transf.p = p;
                instruction.params.block_data_transf.s = s;
                instruction.params.block_data_transf.u = u;
                instruction.params.block_data_transf.w = w;
                instruction.params.block_data_transf.l = l;

                instruction.params.block_data_transf.rn = (lastInst >> 16) & 0x0F;
                instruction.params.block_data_transf.rList = lastInst & 0x0FFFF;

                /* docs say there are two more distinct instructions in this category */
                if (l) {
                    instruction.id = ARMInstructionID::LDM;
                } else {
                    instruction.id = ARMInstructionID::STM;
                }

            } else if ((lastInst & MASK_BRANCH) == VAL_BRANCH) {

                instruction.cat = ARMInstructionCategory::BRANCH;

                bool l = (lastInst >> 24) & 1;

                //std::cout << std::hex << lastInst << std::endl;

                instruction.params.branch.l = l;
                /* convert signed 24 to signed 32 */
                uint32_t si = lastInst & 0x00FFFFFF;
                instruction.params.branch.offset = static_cast<int32_t>(si << 8) / (1 << 8);

                instruction.id = ARMInstructionID::B;

            } else if ((lastInst & MASK_COPROC_DATA_TRANSF) == VAL_COPROC_DATA_TRANSF) {
                //TODO
            } else if ((lastInst & MASK_COPROC_OP) == VAL_COPROC_OP) {
                //TODO
            } else if ((lastInst & MASK_COPROC_REG_TRANSF) == VAL_COPROC_REG_TRANSF) {
                //TODO
            } else if ((lastInst & MASK_SOFTWARE_INTERRUPT) == VAL_SOFTWARE_INTERRUPT) {

                instruction.cat = ARMInstructionCategory::SOFTWARE_INTERRUPT;

                instruction.id = ARMInstructionID::SWI;
                instruction.params.software_interrupt.comment = lastInst & 0x00FFFFFF;
            } else {
                //std::cerr << "ERROR: Could not decode instruction: " << std::hex << lastInst << std::endl;
            }

            //if (instruction.id != ARMInstructionID::SWI && instruction.id != ARMInstructionID::INVALID)
            //    std::cout << instructionIDToString(instruction.id) << std::endl;

            //std::cout << instruction.toString() << std::endl;

            return Instruction::fromARM(instruction);
        }

    } // namespace arm
} // namespace gbaemu
