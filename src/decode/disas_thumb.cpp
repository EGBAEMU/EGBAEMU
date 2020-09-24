#include "disas_thumb.hpp"
#include "cpu/swi.hpp"
#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::thumb
{

    template <>
    void ThumbDisas::disas<MOV_SHIFT>(InstructionID id, uint8_t rs, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", r" << static_cast<uint32_t>(rs) << ", #" << static_cast<uint32_t>(offset == 0 && id != LSL ? 32 : offset);
    }

    template <>
    void ThumbDisas::disas<ADD_SUB>(InstructionID id, uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {

        ss << " r" << static_cast<uint32_t>(rd) << ", r" << static_cast<uint32_t>(rs);

        if (id == InstructionID::ADD_SHORT_IMM || id == InstructionID::SUB_SHORT_IMM)
            ss << " 0x" << std::hex << static_cast<uint32_t>(rn_offset);
        else
            ss << " r" << static_cast<uint32_t>(rn_offset);
    }

    template <>
    void ThumbDisas::disas<MOV_CMP_ADD_SUB_IMM>(InstructionID id, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", 0x" << std::hex << static_cast<uint32_t>(offset);
    }

    template <>
    void ThumbDisas::disas<ALU_OP>(InstructionID id, uint8_t rs, uint8_t rd)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", r" << static_cast<uint32_t>(rs);
    }

    template <>
    void ThumbDisas::disas<BR_XCHG>(InstructionID id, uint8_t rd, uint8_t rs)
    {
        ss << " r";
        if (id != BX)
            ss << static_cast<uint32_t>(rd) << ", r";

        ss << static_cast<uint32_t>(rs);
    }

    template <>
    void ThumbDisas::disas<PC_LD>(InstructionID id, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [((PC + 4) & ~2) + " << static_cast<uint32_t>(offset * 4) << "]";
    }

    template <>
    void ThumbDisas::disas<LD_ST_REL_OFF>(InstructionID id, uint8_t ro, uint8_t rb, uint8_t rd)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [r" << static_cast<uint32_t>(rb) << " + r" << static_cast<uint32_t>(ro) << "]";
    }

    template <>
    void ThumbDisas::disas<LD_ST_SIGN_EXT>(InstructionID id, uint8_t ro, uint8_t rb, uint8_t rd)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [r" << static_cast<uint32_t>(rb) << " + r" << static_cast<uint32_t>(ro) << "]";
    }

    template <>
    void ThumbDisas::disas<LD_ST_IMM_OFF>(InstructionID id, uint8_t rb, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [r" << static_cast<uint32_t>(rb) << " + #" << static_cast<uint32_t>(offset) << "]";
    }

    template <>
    void ThumbDisas::disas<LD_ST_HW>(InstructionID id, uint8_t rb, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [r" << static_cast<uint32_t>(rb) << " + #" << static_cast<uint32_t>(offset * 2) << "]";
    }

    template <>
    void ThumbDisas::disas<LD_ST_REL_SP>(InstructionID id, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [SP + #" << static_cast<uint32_t>(offset * 4) << "]";
    }

    template <>
    void ThumbDisas::disas<LOAD_ADDR>(InstructionID id, bool sp, uint8_t rd, uint8_t offset)
    {
        ss << " r" << static_cast<uint32_t>(rd) << ", [" << (sp ? "SP" : "((PC + 4) & ~2)") << " + #" << static_cast<uint32_t>(offset * 4) << "]";
    }

    template <>
    void ThumbDisas::disas<ADD_OFFSET_TO_STACK_PTR>(InstructionID id, bool s, uint8_t offset)
    {
        ss << " SP, #" << (s ? "-" : "") << static_cast<uint32_t>(offset * 4);
    }

    template <>
    void ThumbDisas::disas<PUSH_POP_REG>(InstructionID id, bool r, uint8_t rlist)
    {
        ss << " { ";

        for (uint32_t i = 0; i < 8; ++i)
            if (rlist & (1 << i))
                ss << "r" << i << ' ';

        ss << '}' << '{' << (r ? (id == POP ? "PC" : "LR") : "") << '}';
    }

    template <>
    void ThumbDisas::disas<MULT_LOAD_STORE>(InstructionID id, uint8_t rb, uint8_t rlist)
    {
        ss << " r" << static_cast<uint32_t>(rb) << " { ";

        for (uint32_t i = 0; i < 8; ++i)
            if (rlist & (1 << i))
                ss << "r" << i << ' ';

        ss << '}';
    }

    template <>
    void ThumbDisas::disas<COND_BRANCH>(InstructionID id, uint8_t cond, int8_t offset)
    {
        ss << conditionCodeToString(static_cast<ConditionOPCode>(cond)) << " PC + 4 + " << (static_cast<int32_t>(offset) * 2);
    }

    template <>
    void ThumbDisas::disas<SOFTWARE_INTERRUPT>(InstructionID id, uint8_t index)
    {
        ss << " " << swi::swiToString(index);
    }

    template <>
    void ThumbDisas::disas<UNCONDITIONAL_BRANCH>(InstructionID id, int16_t offset)
    {
        ss << " PC + 4 + " << (static_cast<int32_t>(offset) * 2);
    }

    template <>
    void ThumbDisas::disas<LONG_BRANCH_WITH_LINK>(InstructionID id, bool h, uint16_t offset)
    {
        ss << ' ';
        if (h) {
            ss << "PC = LR + 0x" << std::hex << (static_cast<uint32_t>(offset) << 1) << ", LR = (PC + 2) | 1";
        } else {
            ss << "LR = PC + 4 + 0x" << std::hex << (static_cast<uint32_t>(offset) << 12);
        }
    }

    template <>
    void ThumbDisas::disas<INVALID_CAT>(InstructionID id)
    {
    }
} // namespace gbaemu::thumb