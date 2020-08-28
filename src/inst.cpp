#include "inst.hpp"
#include "cpu_state.hpp"
#include "regs.hpp"

namespace gbaemu
{

    const char *conditionCodeToString(ConditionOPCode condition)
    {
        switch (condition) {
            STRINGIFY_CASE_ID(EQ);
            STRINGIFY_CASE_ID(NE);
            STRINGIFY_CASE_ID(CS_HS);
            STRINGIFY_CASE_ID(CC_LO);
            STRINGIFY_CASE_ID(MI);
            STRINGIFY_CASE_ID(PL);
            STRINGIFY_CASE_ID(VS);
            STRINGIFY_CASE_ID(VC);
            STRINGIFY_CASE_ID(HI);
            STRINGIFY_CASE_ID(LS);
            STRINGIFY_CASE_ID(GE);
            STRINGIFY_CASE_ID(LT);
            STRINGIFY_CASE_ID(GT);
            STRINGIFY_CASE_ID(LE);
            STRINGIFY_CASE_ID(AL);
            STRINGIFY_CASE_ID(NV);
        }
        return "";
    }

    uint64_t arm::shift(uint32_t value, ShiftType type, uint8_t amount, bool oldCarry, bool shiftByImm)
    {
        // Only the least significant byte of the contents of Rs is used to determine the shift amount. -> uint8_t rules!

        bool initialZeroAmount = false;

        // Edge cases for shifts with immediates: Assembler uses value 0 for special cases
        if (shiftByImm && type != LSL && amount == 0) {
            if (type != ROR) {
                amount = 32;
            } else {
                amount = 1;
            }
            initialZeroAmount = true;
        }

        // Edge cases for shifts by register value: Value of 0 does nothing & keeps old carry!
        if (!shiftByImm && amount == 0) {
            return static_cast<uint64_t>(value) | (oldCarry ? (static_cast<uint64_t>(1) << 32) : 0x0);
        }

        switch (type) {
            case LSL:
                return static_cast<uint64_t>(value) << amount;
            case LSR: {
                /*
                Carry flag is the MSB of the out shifted values! -> bit amount - 1
                */
                uint64_t carry = (amount > 32 ? 0 : (static_cast<uint64_t>((value >> (amount - 1)) & 0x1) << 32));
                uint64_t res = (value >> amount) | carry;

                // LSR#0: Interpreted as LSR#32, ie. Op2 becomes zero, C becomes Bit 31 of Rm
                if (initialZeroAmount) {
                    res |= static_cast<uint64_t>(value & (1 << 31)) << 1;
                }

                return res;
            }
            case ASR: {

                /*
                Carry flag is the MSB of the out shifted values! -> bit amount - 1
                */
                // ensure a value in range of [0, 32]
                uint8_t multipleOf32 = amount / 32;
                amount = amount - (multipleOf32 > 1 ? (multipleOf32 - 1) * 32 : 0);
                uint64_t carry = (static_cast<uint64_t>((value >> (amount - 1)) & 0x1) << 32);
                return static_cast<uint64_t>(static_cast<int64_t>(static_cast<uint64_t>(value) << 32) / (static_cast<uint64_t>(1) << (32 + amount))) | carry;
            }
            case ROR: {
                // ensure a value in range of [0, 32]
                uint8_t multipleOf32 = amount / 32;
                amount = amount - (multipleOf32 > 1 ? (multipleOf32 - 1) * 32 : 0);
                uint32_t res = (value >> amount) | (value << (32 - amount));

                // ROR#0: Interpreted as RRX#1 (RCR), like ROR#1, but Op2 Bit 31 set to old C.
                if (initialZeroAmount) {
                    res = (res & ~(1 << 31)) | (oldCarry ? (1 << 31) : 0);
                }

                /*
                Carry flag is the MSB of the out shifted values! -> bit amount - 1
                */
                uint64_t extendedRes = static_cast<uint64_t>(res) | (static_cast<uint64_t>((value >> (amount - 1)) & 0x1) << 32);

                return extendedRes;
            }
            default:
                std::cout << "WARNING: Unknown shift type!";
                return value;
        }
    }

    void Instruction::setArmInstruction(arm::ARMInstruction &armInstruction)
    {
        arm = armInstruction;
        isArm = true;
    }
    void Instruction::setThumbInstruction(thumb::ThumbInstruction &thumbInstruction)
    {
        thumb = thumbInstruction;
        isArm = false;
    }

    bool Instruction::isArmInstruction() const
    {
        return isArm;
    }

    std::string Instruction::toString() const {
        if (isArm)
            return arm.toString();
        else
            return thumb.toString();
    }

    Instruction Instruction::fromARM(arm::ARMInstruction &armInst)
    {
        Instruction result;
        result.arm = armInst;
        result.isArm = true;
        return result;
    }

    Instruction Instruction::fromThumb(thumb::ThumbInstruction &thumbInst)
    {
        Instruction result;
        result.thumb = thumbInst;
        result.isArm = false;
        return result;
    }

    bool conditionSatisfied(ConditionOPCode condition, const CPUState &state)
    {
        switch (condition) {
            // Equal Z==1
            case EQ:
                return state.getFlag(cpsr_flags::Z_FLAG);
                break;

            // Not equal Z==0
            case NE:
                return !state.getFlag(cpsr_flags::Z_FLAG);
                break;

            // Carry set / unsigned higher or same C==1
            case CS_HS:
                return state.getFlag(cpsr_flags::C_FLAG);
                break;

            // Carry clear / unsigned lower C==0
            case CC_LO:
                return !state.getFlag(cpsr_flags::C_FLAG);
                break;

            // Minus / negative N==1
            case MI:
                return state.getFlag(cpsr_flags::N_FLAG);
                break;

            // Plus / positive or zero N==0
            case PL:
                return !state.getFlag(cpsr_flags::N_FLAG);
                break;

            // Overflow V==1
            case VS:
                return state.getFlag(cpsr_flags::V_FLAG);
                break;

            // No overflow V==0
            case VC:
                return !state.getFlag(cpsr_flags::V_FLAG);
                break;

            // Unsigned higher (C==1) AND (Z==0)
            case HI:
                return state.getFlag(cpsr_flags::C_FLAG) && !state.getFlag(cpsr_flags::Z_FLAG);
                break;

            // Unsigned lower or same (C==0) OR (Z==1)
            case LS:
                return !state.getFlag(cpsr_flags::C_FLAG) || state.getFlag(cpsr_flags::Z_FLAG);
                break;

            // Signed greater than or equal N == V
            case GE:
                return state.getFlag(cpsr_flags::N_FLAG) == state.getFlag(cpsr_flags::V_FLAG);
                break;

            // Signed less than N != V
            case LT:
                return state.getFlag(cpsr_flags::N_FLAG) != state.getFlag(cpsr_flags::V_FLAG);
                break;

            // Signed greater than (Z==0) AND (N==V)
            case GT:
                return !state.getFlag(cpsr_flags::Z_FLAG) && state.getFlag(cpsr_flags::N_FLAG) == state.getFlag(cpsr_flags::V_FLAG);
                break;

            // Signed less than or equal (Z==1) OR (N!=V)
            case LE:
                return state.getFlag(cpsr_flags::Z_FLAG) || state.getFlag(cpsr_flags::N_FLAG) != state.getFlag(cpsr_flags::V_FLAG);
                break;

            // Always (unconditional) Not applicable
            case AL:
                return true;
                break;

            // Never Obsolete, unpredictable in ARM7TDMI
            case NV:
            default:
                return false;
                break;
        }
    }

} // namespace gbaemu
