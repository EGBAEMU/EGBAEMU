#include "disas_arm.hpp"
#include "cpu/swi.hpp"
#include "decode/inst.hpp"
#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::arm
{
    template <>
    void ArmDisas::disas<MUL_ACC>(InstructionID id, bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm)
    {
        if (s)
            ss << "{S}";

        ss << " r" << static_cast<uint32_t>(rd) << " r" << static_cast<uint32_t>(rm) << " r" << static_cast<uint32_t>(rs);

        if (id == MLA)
            ss << " +r" << static_cast<uint32_t>(rn);
    }

    template <>
    void ArmDisas::disas<MUL_ACC_LONG>(InstructionID id, bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        ss << " r" << static_cast<uint32_t>(rd_msw) << ":r" << static_cast<uint32_t>(rd_lsw) << " r" << static_cast<uint32_t>(rs) << " r" << static_cast<uint32_t>(rm);
    }

    template <>
    void ArmDisas::disas<DATA_SWP>(InstructionID id, uint8_t rn, uint8_t rd, uint8_t rm)
    {
        // TODO
    }

    template <>
    void ArmDisas::disas<BRANCH>(InstructionID id, bool link, int32_t offset)
    {
        int32_t off = offset * 4;
        ss << (link ? "L" : "") << " "
           << "PC" << (off < 0 ? '-' : '+') << "0x" << std::hex << std::abs(off);
    }

    template <>
    void ArmDisas::disas<BRANCH_XCHG>(InstructionID id, uint8_t rn)
    {
        ss << " r" << static_cast<uint32_t>(rn);
    }

    template <>
    void ArmDisas::disas<SIGN_TRANSF>(InstructionID id, bool b, bool p, bool u, bool w, uint8_t rn, uint8_t rd, uint8_t addrMode)
    {
        // Immediate in this category!
        ss << " r" << rd;

        if (p) {
            ss << " [r" << static_cast<uint32_t>(rn);
        } else {
            ss << " [[r" << static_cast<uint32_t>(rn) << "]";
        }

        if (b) {
            ss << "+0x" << std::hex << addrMode << ']';
        } else {
            ss << ", r" << std::dec << (addrMode & 0x0F) << ']';
        }
    }

    template <>
    void ArmDisas::disas<DATA_PROC_PSR_TRANSF>(InstructionID id, bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {
        // TODO: probably not done
        bool hasRN = !(id == InstructionID::MOV || id == InstructionID::MVN);
        bool hasRD = !(id == InstructionID::TST || id == InstructionID::TEQ ||
                       id == InstructionID::CMP || id == InstructionID::CMN);

        shifts::ShiftType shiftType;
        uint8_t shiftAmount;
        uint8_t rm;
        uint8_t rs;
        uint8_t imm;
        bool shiftByReg = extractOperand2(shiftType, shiftAmount, rm, rs, imm, operand2, i);

        if (id == MSR_CPSR || id == MSR_SPSR) {

            // true iff write to flag field is allowed 31-24
            bool f = rn & 0x08;
            // true iff write to status field is allowed 23-16
            bool s = rn & 0x04;
            // true iff write to extension field is allowed 15-8
            bool x = rn & 0x02;
            // true iff write to control field is allowed 7-0
            bool c = rn & 0x01;

            ss << (id == MSR_SPSR ? " SPSR_" : " CPSR_");
            ss << (f ? "f" : "");
            ss << (s ? "s" : "");
            ss << (x ? "x" : "");
            ss << (c ? "c" : "");

            if (i) {
                uint32_t roredImm = static_cast<uint32_t>(shift(imm, shiftType, shiftAmount, false, false));
                ss << ", #" << roredImm;
            } else {
                ss << ", r" << std::dec << static_cast<uint32_t>(rm);
            }

        } else {

            if (s)
                ss << "{S}";

            if (hasRD)
                ss << " r" << static_cast<uint32_t>(rd);

            if (hasRN)
                ss << " r" << static_cast<uint32_t>(rn);

            if (i) {
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
    }

    template <>
    void ArmDisas::disas<BLOCK_DATA_TRANSF>(InstructionID id, bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList)
    {
        ss << " r" << static_cast<uint32_t>(rn) << " { ";

        for (uint32_t i = 0; i < 16; ++i)
            if (rList & (1 << i))
                ss << "r" << i << ' ';

        ss << '}';
    }

    template <>
    void ArmDisas::disas<LS_REG_UBYTE>(InstructionID id, bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
    {
        char upDown = up ? '+' : '-';

        ss << " r" << static_cast<uint32_t>(rd);

        if (!i) {
            uint32_t immOff = addrMode & 0xFFF;

            if (pre)
                ss << " [r" << static_cast<uint32_t>(rn);
            else
                ss << " [[r" << static_cast<uint32_t>(rn) << ']';

            ss << upDown << "0x" << std::hex << immOff << ']';

        } else {
            uint32_t shiftAmount = (addrMode >> 7) & 0xF;
            uint32_t rm = addrMode & 0xF;

            if (pre)
                ss << " [r" << static_cast<uint32_t>(rn);
            else
                ss << " [[r" << static_cast<uint32_t>(rn) << ']';

            ss << upDown << "(r" << rm << "<<" << shiftAmount << ")]";
        }
    }

    template <>
    void ArmDisas::disas<HW_TRANSF_REG_OFF>(InstructionID id, bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint8_t rm)
    {
        // No immediate in this category!
        ss << " r" << static_cast<uint32_t>(rd);

        // TODO: does p mean pre?
        if (pre) {
            ss << " [r" << static_cast<uint32_t>(rn) << "+r" << static_cast<uint32_t>(rm) << ']';
        } else {
            ss << " [r" << static_cast<uint32_t>(rn) << "]+r" << static_cast<uint32_t>(rm);
        }
    }

    template <>
    void ArmDisas::disas<HW_TRANSF_IMM_OFF>(InstructionID id, bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint8_t offset)
    {
        // Immediate in this category!
        ss << " r" << static_cast<uint32_t>(rd);

        if (pre) {
            ss << " [r" << static_cast<uint32_t>(rn) << "+0x" << std::hex << static_cast<uint32_t>(offset) << ']';
        } else {
            ss << " [[r" << static_cast<uint32_t>(rn) << "]+0x" << std::hex << static_cast<uint32_t>(offset) << ']';
        }
    }

    template <>
    void ArmDisas::disas<SOFTWARE_INTERRUPT>(InstructionID id, uint8_t index)
    {
        ss << " " << swi::swiToString(index);
    }

    template <>
    void ArmDisas::disas<INVALID_CAT>(InstructionID id)
    {
        // Nothing to do here
    }

} // namespace gbaemu::arm