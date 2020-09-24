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
            if ((lastInst & MASK_MUL_ACC) == VAL_MUL_ACC) {

                bool a = (lastInst >> 21) & 1;
                bool s = (lastInst >> 20) & 1;

                uint8_t rd = (lastInst >> 16) & 0x0F;
                uint8_t rn = (lastInst >> 12) & 0x0F;
                uint8_t rs = (lastInst >> 8) & 0x0F;
                uint8_t rm = lastInst & 0x0F;

                if (a) {
                    exec.Executor::template operator()<MUL_ACC, MLA>(s, rd, rn, rs, rm);
                } else {
                    exec.Executor::template operator()<MUL_ACC, MUL>(s, rd, rn, rs, rm);
                }

            } else if ((lastInst & MASK_MUL_ACC_LONG) == VAL_MUL_ACC_LONG) {
                bool u = (lastInst >> 22) & 1;
                bool a = (lastInst >> 21) & 1;
                bool s = (lastInst >> 20) & 1;

                uint8_t rd_msw = (lastInst >> 16) & 0x0F;
                uint8_t rd_lsw = (lastInst >> 12) & 0x0F;
                uint8_t rs = (lastInst >> 8) & 0x0F;
                uint8_t rm = lastInst & 0x0F;

                if (u && a) {
                    exec.Executor::template operator()<MUL_ACC_LONG, SMLAL>(s, rd_msw, rd_lsw, rs, rm);
                } else if (u && !a) {
                    exec.Executor::template operator()<MUL_ACC_LONG, SMULL>(s, rd_msw, rd_lsw, rs, rm);
                } else if (!u && a) {
                    exec.Executor::template operator()<MUL_ACC_LONG, UMLAL>(s, rd_msw, rd_lsw, rs, rm);
                } else {
                    exec.Executor::template operator()<MUL_ACC_LONG, UMULL>(s, rd_msw, rd_lsw, rs, rm);
                }

            } else if ((lastInst & MASK_BRANCH_XCHG) == VAL_BRANCH_XCHG) {
                exec.Executor::template operator()<BRANCH_XCHG, BX>(static_cast<uint8_t>(lastInst & 0x0F));
            } else if ((lastInst & MASK_DATA_SWP) == VAL_DATA_SWP) {

                /* also called b */
                bool b = (lastInst >> 22) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;
                uint8_t rm = lastInst & 0x0F;

                if (!b) {
                    exec.Executor::template operator()<DATA_SWP, SWP>(rn, rd, rm);
                } else {
                    exec.Executor::template operator()<DATA_SWP, SWPB>(rn, rd, rm);
                }

            } else if ((lastInst & MASK_HW_TRANSF_REG_OFF) == VAL_HW_TRANSF_REG_OFF) {
                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;
                uint8_t rm = lastInst & 0x0F;

                // register offset variants
                if (l) {
                    exec.Executor::template operator()<HW_TRANSF_REG_OFF, LDRH>(p, u, w, rn, rd, rm);
                } else {
                    exec.Executor::template operator()<HW_TRANSF_REG_OFF, STRH>(p, u, w, rn, rd, rm);
                }

            } else if ((lastInst & MASK_HW_TRANSF_IMM_OFF) == VAL_HW_TRANSF_IMM_OFF) {

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;

                /* called addr_mode in detailed doc but is really offset because immediate flag I is 1 */
                uint8_t offset = (((lastInst >> 8) & 0x0F) << 4) | (lastInst & 0x0F);

                if (l) {
                    exec.Executor::template operator()<HW_TRANSF_IMM_OFF, LDRH>(p, u, w, rn, rd, offset);
                } else {
                    exec.Executor::template operator()<HW_TRANSF_IMM_OFF, STRH>(p, u, w, rn, rd, offset);
                }

            } else if ((lastInst & MASK_SIGN_TRANSF) == VAL_SIGN_TRANSF) {

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool b = (lastInst >> 22) & 1;
                bool w = (lastInst >> 21) & 1;

                bool l = (lastInst >> 20) & 1;
                bool h = (lastInst >> 5) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;

                uint8_t addrMode = (((lastInst >> 8) & 0x0F) << 4) | (lastInst & 0x0F);

                if (l && !h) {
                    exec.Executor::template operator()<SIGN_TRANSF, LDRSB>(b, p, u, w, rn, rd, addrMode);
                } else if (l && h) {
                    exec.Executor::template operator()<SIGN_TRANSF, LDRSH>(b, p, u, w, rn, rd, addrMode);
                } else {
                    exec.Executor::template operator()<INVALID_CAT, INVALID>(/*b, p, u, w, rn, rd, addrMode*/);
                } /*else if (!l && h) { // supported arm5 and up
                    exec.Executor::template operator()<SIGN_TRANSF, STRD>(b, p, u, w, rn, rd, addrMode);
                }
                else if (!l && !h) {
                    exec.Executor::template operator()<SIGN_TRANSF, LDRD>(b, p, u, w, rn, rd, addrMode);
                }*/

            } else if ((lastInst & MASK_DATA_PROC_PSR_TRANSF) == VAL_DATA_PROC_PSR_TRANSF) {

                uint8_t opCode = (lastInst >> 21) & 0x0F;
                bool i = (lastInst >> 25) & 1;
                bool s = lastInst & (1 << 20);

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;
                /* often shifter */
                uint16_t operand2 = lastInst & 0x0FFF;

                switch (opCode) {
                    case 0b0000:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, AND>(i, s, rn, rd, operand2);
                        break;
                    case 0b0001:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, EOR>(i, s, rn, rd, operand2);
                        break;
                    case 0b0010:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, SUB>(i, s, rn, rd, operand2);
                        break;
                    case 0b0011:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, RSB>(i, s, rn, rd, operand2);
                        break;
                    case 0b0100:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ADD>(i, s, rn, rd, operand2);
                        break;
                    case 0b0101:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ADC>(i, s, rn, rd, operand2);
                        break;
                    case 0b0110:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, SBC>(i, s, rn, rd, operand2);
                        break;
                    case 0b0111:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, RSC>(i, s, rn, rd, operand2);
                        break;
                    case 0b1000:
                        if (s) {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, TST>(i, s, rn, rd, operand2);
                        } else {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MRS_CPSR>(i, s, rn, rd, operand2);
                        }
                        break;
                    case 0b1001:
                        if (s) {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, TEQ>(i, s, rn, rd, operand2);
                        } else {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MSR_CPSR>(i, s, rn, rd, operand2);
                        }
                        break;
                    case 0b1010:
                        if (s) {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, CMP>(i, s, rn, rd, operand2);
                        } else {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MRS_SPSR>(i, s, rn, rd, operand2);
                        }
                        break;
                    case 0b1011:
                        if (s) {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, CMN>(i, s, rn, rd, operand2);
                        } else {
                            exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MSR_SPSR>(i, s, rn, rd, operand2);
                        }
                        break;
                    case 0b1100:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, ORR>(i, s, rn, rd, operand2);
                        break;
                    case 0b1101:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MOV>(i, s, rn, rd, operand2);
                        break;
                    case 0b1110:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, BIC>(i, s, rn, rd, operand2);
                        break;
                    case 0b1111:
                        exec.Executor::template operator()<DATA_PROC_PSR_TRANSF, MVN>(i, s, rn, rd, operand2);
                        break;
                }
            } else if ((lastInst & MASK_LS_REG_UBYTE) == VAL_LS_REG_UBYTE) {

                bool i = (lastInst >> 25) & 1;
                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool b = (lastInst >> 22) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint8_t rd = (lastInst >> 12) & 0x0F;
                uint16_t addrMode = lastInst & 0x0FFF;

                if (!b && l) {
                    exec.Executor::template operator()<LS_REG_UBYTE, LDR>(p, u, i, w, rn, rd, addrMode);
                } else if (b && l) {
                    exec.Executor::template operator()<LS_REG_UBYTE, LDRB>(p, u, i, w, rn, rd, addrMode);
                } else if (!b && !l) {
                    exec.Executor::template operator()<LS_REG_UBYTE, STR>(p, u, i, w, rn, rd, addrMode);
                } else {
                    exec.Executor::template operator()<LS_REG_UBYTE, STRB>(p, u, i, w, rn, rd, addrMode);
                }

            } /* else if ((lastInst & MASK_UNDEFINED) == VAL_UNDEFINED) {
                std::cout << "WARNING: Undefined instruction: " << std::hex << lastInst << std::endl;
            }*/
            else if ((lastInst & MASK_BLOCK_DATA_TRANSF) == VAL_BLOCK_DATA_TRANSF) {

                bool p = (lastInst >> 24) & 1;
                bool u = (lastInst >> 23) & 1;
                bool s = (lastInst >> 22) & 1;
                bool w = (lastInst >> 21) & 1;
                bool l = (lastInst >> 20) & 1;

                uint8_t rn = (lastInst >> 16) & 0x0F;
                uint16_t rList = lastInst & 0x0FFFF;

                /* docs say there are two more distinct instructions in this category */
                if (l) {
                    exec.Executor::template operator()<BLOCK_DATA_TRANSF, LDM>(p, u, w, s, rn, rList);
                } else {
                    exec.Executor::template operator()<BLOCK_DATA_TRANSF, STM>(p, u, w, s, rn, rList);
                }

            } else if ((lastInst & MASK_BRANCH) == VAL_BRANCH) {

                bool l = (lastInst >> 24) & 1;

                exec.Executor::template operator()<BRANCH, B>(l, signExt<int32_t, uint32_t, 24>(lastInst & 0x00FFFFFF));

            } /* else if ((lastInst & MASK_COPROC_DATA_TRANSF) == VAL_COPROC_DATA_TRANSF) {
                // Unused
            } else if ((lastInst & MASK_COPROC_OP) == VAL_COPROC_OP) {
                // Unused
            } else if ((lastInst & MASK_COPROC_REG_TRANSF) == VAL_COPROC_REG_TRANSF) {
                // Unused
            }*/
            else if ((lastInst & MASK_SOFTWARE_INTERRUPT) == VAL_SOFTWARE_INTERRUPT) {
                uint32_t comment = lastInst & 0x00FFFFFF;
                exec.Executor::template operator()<SOFTWARE_INTERRUPT, SWI>(static_cast<uint8_t>(comment >> 16));
            } else {
                exec.Executor::template operator()<INVALID_CAT, INVALID>();
            }
        }

    } // namespace arm
} // namespace gbaemu
