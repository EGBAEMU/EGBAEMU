#include "cpu/swi.hpp"
#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::arm {

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
}