#include "inst_arm.hpp"
#include "cpu/cpu_state.hpp"
#include "cpu/swi.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu
{
    namespace arm
    {

        std::string ARMInstruction::toString() const
        {
            std::stringstream ss;

            ss << '(' << conditionCodeToString(condition) << ") " << instructionIDToString(id);

            switch (cat) {

                case ARMInstructionCategory::DATA_PROC_PSR_TRANSF: {
                    /* TODO: probably not done */
                    bool hasRN = !(id == InstructionID::MOV || id == InstructionID::MVN);
                    bool hasRD = !(id == InstructionID::TST || id == InstructionID::TEQ ||
                                   id == InstructionID::CMP || id == InstructionID::CMN);

                    uint8_t rd = params.data_proc_psr_transf.rd;
                    uint8_t rn = params.data_proc_psr_transf.rn;
                    shifts::ShiftType shiftType;
                    uint8_t shiftAmount;
                    uint8_t rm;
                    uint8_t rs;
                    uint8_t imm;
                    bool shiftByReg = params.data_proc_psr_transf.extractOperand2(shiftType, shiftAmount, rm, rs, imm);

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
                            uint32_t roredImm = static_cast<uint32_t>(shift(imm, shiftType, shiftAmount, false, false));
                            ss << ", #" << roredImm;
                        } else {
                            ss << ", r" << std::dec << static_cast<uint32_t>(rm);
                        }
                    } else {
                        if (params.data_proc_psr_transf.s)
                            ss << "{S}";

                        if (hasRD)
                            ss << " r" << static_cast<uint32_t>(rd);

                        if (hasRN)
                            ss << " r" << static_cast<uint32_t>(rn);

                        if (params.data_proc_psr_transf.i) {
                            uint32_t roredImm = static_cast<uint32_t>(shift(imm, shiftType, shiftAmount, false, false));
                            ss << ", #" << roredImm;
                        } else {
                            ss << " r" << static_cast<uint32_t>(rm);

                            if (shiftByReg)
                                ss << "<<r" << std::dec << static_cast<uint32_t>(rs);
                            else if (shiftAmount > 0)
                                ss << "<<" << std::dec << static_cast<uint32_t>(shiftAmount);
                        }
                    }
                    break;
                }
                case ARMInstructionCategory::MUL_ACC: {
                    if (params.mul_acc.s)
                        ss << "{S}";
                    ss << " r" << static_cast<uint32_t>(params.mul_acc.rd) << " r" << static_cast<uint32_t>(params.mul_acc.rm) << " r" << static_cast<uint32_t>(params.mul_acc.rs);
                    if (params.mul_acc.a)
                        ss << " +r" << static_cast<uint32_t>(params.mul_acc.rn);
                    break;
                }
                case ARMInstructionCategory::MUL_ACC_LONG: {
                    if (params.mul_acc_long.s)
                        ss << "{S}";
                    ss << " r" << static_cast<uint32_t>(params.mul_acc_long.rd_msw) << ":r" << static_cast<uint32_t>(params.mul_acc_long.rd_lsw) << " r" << static_cast<uint32_t>(params.mul_acc_long.rs) << " r" << static_cast<uint32_t>(params.mul_acc_long.rm);
                    break;
                }
                case ARMInstructionCategory::HW_TRANSF_REG_OFF: {
                    /* No immediate in this category! */
                    ss << " r" << static_cast<uint32_t>(params.hw_transf_reg_off.rd);

                    /* TODO: does p mean pre? */
                    if (params.hw_transf_reg_off.p) {
                        ss << " [r" << static_cast<uint32_t>(params.hw_transf_reg_off.rn) << "+r" << static_cast<uint32_t>(params.hw_transf_reg_off.rm) << ']';
                    } else {
                        ss << " [r" << static_cast<uint32_t>(params.hw_transf_reg_off.rn) << "]+r" << static_cast<uint32_t>(params.hw_transf_reg_off.rm);
                    }
                    break;
                }
                case ARMInstructionCategory::HW_TRANSF_IMM_OFF: {
                    /* Immediate in this category! */
                    ss << " r" << static_cast<uint32_t>(params.hw_transf_imm_off.rd);

                    if (params.hw_transf_reg_off.p) {
                        ss << " [r" << static_cast<uint32_t>(params.hw_transf_imm_off.rn) << "+0x" << std::hex << params.hw_transf_imm_off.offset << ']';
                    } else {
                        ss << " [[r" << static_cast<uint32_t>(params.hw_transf_imm_off.rn) << "]+0x" << std::hex << params.hw_transf_imm_off.offset << ']';
                    }
                    break;
                }
                case ARMInstructionCategory::SIGN_TRANSF: {
                    /* Immediate in this category! */
                    ss << " r" << params.sign_transf.rd;

                    if (params.hw_transf_reg_off.p) {
                        ss << " [r" << static_cast<uint32_t>(params.sign_transf.rn);
                    } else {
                        ss << " [[r" << static_cast<uint32_t>(params.sign_transf.rn) << "]";
                    }
                    if (params.sign_transf.b) {
                        ss << "+0x" << std::hex << params.sign_transf.addrMode << ']';
                    } else {
                        ss << ", r" << std::dec << (params.sign_transf.addrMode & 0x0F) << ']';
                    }
                    break;
                }
                case ARMInstructionCategory::LS_REG_UBYTE: {
                    bool pre = params.ls_reg_ubyte.p;
                    char upDown = params.ls_reg_ubyte.u ? '+' : '-';

                    ss << " r" << static_cast<uint32_t>(params.ls_reg_ubyte.rd);

                    if (!params.ls_reg_ubyte.i) {
                        uint32_t immOff = params.ls_reg_ubyte.addrMode & 0xFFF;

                        if (pre)
                            ss << " [r" << static_cast<uint32_t>(params.ls_reg_ubyte.rn);
                        else
                            ss << " [[r" << static_cast<uint32_t>(params.ls_reg_ubyte.rn) << ']';

                        ss << upDown << "0x" << std::hex << immOff << ']';
                    } else {
                        uint32_t shiftAmount = (params.ls_reg_ubyte.addrMode >> 7) & 0xF;
                        uint32_t rm = params.ls_reg_ubyte.addrMode & 0xF;

                        if (pre)
                            ss << " [r" << static_cast<uint32_t>(params.ls_reg_ubyte.rn);
                        else
                            ss << " [[r" << static_cast<uint32_t>(params.ls_reg_ubyte.rn) << ']';

                        ss << upDown << "(r" << rm << "<<" << shiftAmount << ")]";
                    }
                    break;
                }
                case ARMInstructionCategory::BLOCK_DATA_TRANSF: {
                    ss << " r" << static_cast<uint32_t>(params.block_data_transf.rn) << " { ";

                    for (uint32_t i = 0; i < 16; ++i)
                        if (params.block_data_transf.rList & (1 << i))
                            ss << "r" << i << ' ';

                    ss << '}';
                    break;
                }
                case ARMInstructionCategory::BRANCH: {
                    /*  */
                    int32_t off = params.branch.offset * 4;

                    ss << (params.branch.l ? "L" : "") << " "
                       << "PC" << (off < 0 ? '-' : '+') << "0x" << std::hex << std::abs(off);
                    break;
                }
                case ARMInstructionCategory::SOFTWARE_INTERRUPT: {
                    ss << " " << swi::swiToString(params.software_interrupt.comment >> 16);
                    break;
                }
                case ARMInstructionCategory::BRANCH_XCHG: {
                    ss << " r" << static_cast<uint32_t>(params.branch_xchg.rn);
                    break;
                }

                case ARMInstructionCategory::DATA_SWP:
                case ARMInstructionCategory::INVALID_CAT:
                default: {
                    ss << "?";
                    break;
                }
            }

            return ss.str();
        }

        Instruction ARMInstructionDecoder::decode(uint32_t lastInst) const
        {
            ARMInstruction instruction;

            // Default the instruction id to invalid
            instruction.id = InstructionID::INVALID;
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
                    instruction.id = InstructionID::MLA;
                } else {
                    instruction.id = InstructionID::MUL;
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
                    instruction.id = InstructionID::SMLAL;
                } else if (u && !a) {
                    instruction.id = InstructionID::SMULL;
                } else if (!u && a) {
                    instruction.id = InstructionID::UMLAL;
                } else {
                    instruction.id = InstructionID::UMULL;
                }

            } else if ((lastInst & MASK_BRANCH_XCHG) == VAL_BRANCH_XCHG) {

                instruction.cat = ARMInstructionCategory::BRANCH_XCHG;
                instruction.id = InstructionID::BX;
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
                    instruction.id = InstructionID::SWP;
                } else {
                    instruction.id = InstructionID::SWPB;
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
                    instruction.id = InstructionID::LDRH;
                } else {
                    instruction.id = InstructionID::STRH;
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
                    instruction.id = InstructionID::LDRH;
                } else {
                    instruction.id = InstructionID::STRH;
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
                    instruction.id = InstructionID::LDRSB;
                } else if (l && h) {
                    instruction.id = InstructionID::LDRSH;
                } /*else if (!l && h) { // supported arm5 and up
                    instruction.id = InstructionID::STRD;
                }
                else if (!l && !h) {
                    instruction.id = InstructionID::LDRD;
                }*/

            } else if ((lastInst & MASK_DATA_PROC_PSR_TRANSF) == VAL_DATA_PROC_PSR_TRANSF) {

                instruction.cat = ARMInstructionCategory::DATA_PROC_PSR_TRANSF;

                uint8_t opCode = (lastInst >> 21) & 0x0F;
                bool i = (lastInst >> 25) & 1;
                bool s = lastInst & (1 << 20);
                bool r = (lastInst >> 22) & 1;

                //instruction.params.data_proc_psr_transf.opCode = opCode;
                instruction.params.data_proc_psr_transf.i = i;
                instruction.params.data_proc_psr_transf.s = s;
                instruction.params.data_proc_psr_transf.r = r;

                instruction.params.data_proc_psr_transf.rn = (lastInst >> 16) & 0x0F;
                instruction.params.data_proc_psr_transf.rd = (lastInst >> 12) & 0x0F;
                /* often shifter */
                instruction.params.data_proc_psr_transf.operand2 = lastInst & 0x0FFF;

                switch (opCode) {
                    case 0b0000:
                        instruction.id = InstructionID::AND;
                        break;
                    case 0b0001:
                        instruction.id = InstructionID::EOR;
                        break;
                    case 0b0010:
                        instruction.id = InstructionID::SUB;
                        break;
                    case 0b0011:
                        instruction.id = InstructionID::RSB;
                        break;
                    case 0b0100:
                        instruction.id = InstructionID::ADD;
                        break;
                    case 0b0101:
                        instruction.id = InstructionID::ADC;
                        break;
                    case 0b0110:
                        instruction.id = InstructionID::SBC;
                        break;
                    case 0b0111:
                        instruction.id = InstructionID::RSC;
                        break;
                    case 0b1000:
                        if (s) {
                            instruction.id = InstructionID::TST;
                        } else {
                            instruction.id = InstructionID::MRS;
                        }
                        break;
                    case 0b1001:
                        if (s) {
                            instruction.id = InstructionID::TEQ;
                        } else {
                            instruction.id = InstructionID::MSR;
                        }
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
                    case 0b1100:
                        instruction.id = InstructionID::ORR;
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
                    instruction.id = InstructionID::LDR;
                } else if (b && l) {
                    instruction.id = InstructionID::LDRB;
                } else if (!b && !l) {
                    instruction.id = InstructionID::STR;
                } else {
                    instruction.id = InstructionID::STRB;
                }

            }/* else if ((lastInst & MASK_UNDEFINED) == VAL_UNDEFINED) {
                std::cout << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
            }*/ else if ((lastInst & MASK_BLOCK_DATA_TRANSF) == VAL_BLOCK_DATA_TRANSF) {

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
                    instruction.id = InstructionID::LDM;
                } else {
                    instruction.id = InstructionID::STM;
                }

            } else if ((lastInst & MASK_BRANCH) == VAL_BRANCH) {

                instruction.cat = ARMInstructionCategory::BRANCH;

                bool l = (lastInst >> 24) & 1;

                instruction.params.branch.l = l;
                /* convert signed 24 to signed 32 */
                uint32_t si = lastInst & 0x00FFFFFF;
                instruction.params.branch.offset = signExt<int32_t, uint32_t, 24>(si);

                instruction.id = InstructionID::B;

            }/* else if ((lastInst & MASK_COPROC_DATA_TRANSF) == VAL_COPROC_DATA_TRANSF) {
                // Unused
            } else if ((lastInst & MASK_COPROC_OP) == VAL_COPROC_OP) {
                // Unused
            } else if ((lastInst & MASK_COPROC_REG_TRANSF) == VAL_COPROC_REG_TRANSF) {
                // Unused
            }*/ else if ((lastInst & MASK_SOFTWARE_INTERRUPT) == VAL_SOFTWARE_INTERRUPT) {

                instruction.cat = ARMInstructionCategory::SOFTWARE_INTERRUPT;

                instruction.id = InstructionID::SWI;
                instruction.params.software_interrupt.comment = lastInst & 0x00FFFFFF;
            }

            return Instruction::fromARM(instruction);
        }

    } // namespace arm
} // namespace gbaemu
