#include "cpu/cpu.hpp"
#include "cpu/cpu_state.hpp"
#include "util.hpp"

namespace gbaemu
{
    namespace arm
    {

        template <typename Executor>
        template <Executor &exec>
        void ARMInstructionDecoder<Executor>::decode(uint32_t lastInst)
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
                    exec.Executor::template operator()<MUL_ACC, MLA, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::MUL;
                    exec.Executor::template operator()<MUL_ACC, MUL, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<MUL_ACC_LONG, SMLAL, ARMInstruction &>(instruction);
                } else if (u && !a) {
                    instruction.id = InstructionID::SMULL;
                    exec.Executor::template operator()<MUL_ACC_LONG, SMULL, ARMInstruction &>(instruction);
                } else if (!u && a) {
                    instruction.id = InstructionID::UMLAL;
                    exec.Executor::template operator()<MUL_ACC_LONG, UMLAL, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::UMULL;
                    exec.Executor::template operator()<MUL_ACC_LONG, UMULL, ARMInstruction &>(instruction);
                }

            } else if ((lastInst & MASK_BRANCH_XCHG) == VAL_BRANCH_XCHG) {

                instruction.cat = ARMInstructionCategory::BRANCH_XCHG;
                instruction.id = InstructionID::BX;
                instruction.params.branch_xchg.rn = lastInst & 0x0F;

                exec.Executor::template operator()<BRANCH_XCHG, BX, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<DATA_SWP, SWP, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::SWPB;
                    exec.Executor::template operator()<DATA_SWP, SWPB, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<HW_TRANSF_REG_OFF, LDRH, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STRH;
                    exec.Executor::template operator()<HW_TRANSF_REG_OFF, STRH, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<HW_TRANSF_IMM_OFF, LDRH, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STRH;
                    exec.Executor::template operator()<HW_TRANSF_IMM_OFF, STRH, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<SIGN_TRANSF, LDRSB, ARMInstruction &>(instruction);
                } else if (l && h) {
                    instruction.id = InstructionID::LDRSH;
                    exec.Executor::template operator()<SIGN_TRANSF, LDRSH, ARMInstruction &>(instruction);
                } else {
                    exec.Executor::template operator()<INVALID_CAT, INVALID, ARMInstruction &>(instruction);
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
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, AND, ARMInstruction &>(instruction);
                        break;
                    case 0b0001:
                        instruction.id = InstructionID::EOR;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, EOR, ARMInstruction &>(instruction);
                        break;
                    case 0b0010:
                        instruction.id = InstructionID::SUB;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, SUB, ARMInstruction &>(instruction);
                        break;
                    case 0b0011:
                        instruction.id = InstructionID::RSB;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, RSB, ARMInstruction &>(instruction);
                        break;
                    case 0b0100:
                        instruction.id = InstructionID::ADD;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ADD, ARMInstruction &>(instruction);
                        break;
                    case 0b0101:
                        instruction.id = InstructionID::ADC;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ADC, ARMInstruction &>(instruction);
                        break;
                    case 0b0110:
                        instruction.id = InstructionID::SBC;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, SBC, ARMInstruction &>(instruction);
                        break;
                    case 0b0111:
                        instruction.id = InstructionID::RSC;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, RSC, ARMInstruction &>(instruction);
                        break;
                    case 0b1000:
                        if (s) {
                            instruction.id = InstructionID::TST;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, TST, ARMInstruction &>(instruction);
                        } else {
                            instruction.id = InstructionID::MRS;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MRS, ARMInstruction &>(instruction);
                        }
                        break;
                    case 0b1001:
                        if (s) {
                            instruction.id = InstructionID::TEQ;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, TEQ, ARMInstruction &>(instruction);
                        } else {
                            instruction.id = InstructionID::MSR;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MSR, ARMInstruction &>(instruction);
                        }
                        break;
                    case 0b1010:
                        if (s) {
                            instruction.id = InstructionID::CMP;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, CMP, ARMInstruction &>(instruction);
                        } else {
                            instruction.id = InstructionID::MRS;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MRS, ARMInstruction &>(instruction);
                        }
                        break;
                    case 0b1011:
                        if (s) {
                            instruction.id = InstructionID::CMN;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, CMN, ARMInstruction &>(instruction);
                        } else {
                            instruction.id = InstructionID::MSR;
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MSR, ARMInstruction &>(instruction);
                        }
                        break;
                    case 0b1100:
                        instruction.id = InstructionID::ORR;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ORR, ARMInstruction &>(instruction);
                        break;
                    case 0b1101:
                        instruction.id = InstructionID::MOV;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MOV, ARMInstruction &>(instruction);
                        break;
                    case 0b1110:
                        instruction.id = InstructionID::BIC;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, BIC, ARMInstruction &>(instruction);
                        break;
                    case 0b1111:
                        instruction.id = InstructionID::MVN;
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MVN, ARMInstruction &>(instruction);
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
                    exec.Executor::template operator()<LS_REG_UBYTE, LDR, ARMInstruction &>(instruction);
                } else if (b && l) {
                    instruction.id = InstructionID::LDRB;
                    exec.Executor::template operator()<LS_REG_UBYTE, LDRB, ARMInstruction &>(instruction);
                } else if (!b && !l) {
                    instruction.id = InstructionID::STR;
                    exec.Executor::template operator()<LS_REG_UBYTE, STR, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STRB;
                    exec.Executor::template operator()<LS_REG_UBYTE, STRB, ARMInstruction &>(instruction);
                }

            } /* else if ((lastInst & MASK_UNDEFINED) == VAL_UNDEFINED) {
                std::cout << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
            }*/
            else if ((lastInst & MASK_BLOCK_DATA_TRANSF) == VAL_BLOCK_DATA_TRANSF) {

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
                    exec.Executor::template operator()<BLOCK_DATA_TRANSF, LDM, ARMInstruction &>(instruction);
                } else {
                    instruction.id = InstructionID::STM;
                    exec.Executor::template operator()<BLOCK_DATA_TRANSF, STM, ARMInstruction &>(instruction);
                }

            } else if ((lastInst & MASK_BRANCH) == VAL_BRANCH) {

                instruction.cat = ARMInstructionCategory::BRANCH;

                bool l = (lastInst >> 24) & 1;

                instruction.params.branch.l = l;
                /* convert signed 24 to signed 32 */
                uint32_t si = lastInst & 0x00FFFFFF;
                instruction.params.branch.offset = signExt<int32_t, uint32_t, 24>(si);

                instruction.id = InstructionID::B;
                exec.Executor::template operator()<BRANCH, B, ARMInstruction &>(instruction);

            } /* else if ((lastInst & MASK_COPROC_DATA_TRANSF) == VAL_COPROC_DATA_TRANSF) {
                // Unused
            } else if ((lastInst & MASK_COPROC_OP) == VAL_COPROC_OP) {
                // Unused
            } else if ((lastInst & MASK_COPROC_REG_TRANSF) == VAL_COPROC_REG_TRANSF) {
                // Unused
            }*/
            else if ((lastInst & MASK_SOFTWARE_INTERRUPT) == VAL_SOFTWARE_INTERRUPT) {

                instruction.cat = ARMInstructionCategory::SOFTWARE_INTERRUPT;

                instruction.id = InstructionID::SWI;
                instruction.params.software_interrupt.comment = lastInst & 0x00FFFFFF;
                exec.Executor::template operator()<SOFTWARE_INTERRUPT, SWI, ARMInstruction &>(instruction);
            } else {
                exec.Executor::template operator()<INVALID_CAT, INVALID, ARMInstruction &>(instruction);
            }
        }

    } // namespace arm
} // namespace gbaemu
