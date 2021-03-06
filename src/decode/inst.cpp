#include "inst.hpp"
#include "cpu/cpu_state.hpp"
#include "cpu/regs.hpp"
#include "util.hpp"

#include "decode/disas_arm.hpp"
#include "decode/disas_thumb.hpp"
#include "decode/inst_arm.hpp"
#include "decode/inst_thumb.hpp"

#include <iostream>

namespace gbaemu
{

    bool extractOperand2(shifts::ShiftType &shiftType, uint8_t &shiftAmount, uint8_t &rm, uint8_t &rs, uint8_t &imm, uint16_t operand2, bool i)
    {
        bool shiftAmountFromReg = false;

        if (i) {
            /* ROR */
            shiftType = shifts::ShiftType::ROR;
            imm = operand2 & 0x0FF;
            shiftAmount = ((operand2 >> 8) & 0x0F) * 2;
        } else {
            shiftType = static_cast<shifts::ShiftType>((operand2 >> 5) & 0b11);
            rm = operand2 & 0xF;
            shiftAmountFromReg = (operand2 >> 4) & 1;

            if (shiftAmountFromReg)
                rs = (operand2 >> 8) & 0x0F;
            else
                shiftAmount = (operand2 >> 7) & 0b11111;
        }

        return shiftAmountFromReg;
    }

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

    const char *instructionIDToString(InstructionID id)
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
            // supported arm5 and up
            //STRINGIFY_CASE_ID(LDRD);
            STRINGIFY_CASE_ID(MLA);
            STRINGIFY_CASE_ID(MOV);
            STRINGIFY_CASE_ID(MRS_CPSR);
            STRINGIFY_CASE_ID(MRS_SPSR);
            STRINGIFY_CASE_ID(MSR_CPSR);
            STRINGIFY_CASE_ID(MSR_SPSR);
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
            // supported arm5 and up
            //STRINGIFY_CASE_ID(STRD);
            STRINGIFY_CASE_ID(SUB);
            STRINGIFY_CASE_ID(SWI);
            STRINGIFY_CASE_ID(SWP);
            STRINGIFY_CASE_ID(SWPB);
            STRINGIFY_CASE_ID(TEQ);
            STRINGIFY_CASE_ID(TST);
            STRINGIFY_CASE_ID(UMLAL);
            STRINGIFY_CASE_ID(UMULL);

            STRINGIFY_CASE_ID(LSL);
            STRINGIFY_CASE_ID(LSR);
            STRINGIFY_CASE_ID(ASR);
            STRINGIFY_CASE_ID(ROR);
            STRINGIFY_CASE_ID(NOP);
            STRINGIFY_CASE_ID(ADD_SHORT_IMM);
            STRINGIFY_CASE_ID(SUB_SHORT_IMM);
            STRINGIFY_CASE_ID(NEG);
            // This one is ARM9
            //STRINGIFY_CASE_ID(BLX);
            STRINGIFY_CASE_ID(POP);
            STRINGIFY_CASE_ID(PUSH);
            STRINGIFY_CASE_ID(STMIA);
            STRINGIFY_CASE_ID(LDMIA);

            STRINGIFY_CASE_ID(INVALID);
        }

        return "NULL";
    }

    namespace shifts
    {
        uint32_t rorShiftValueUnalignedAddr(uint32_t value, uint8_t amount) {
            if (amount == 0)
                return value;

            return (value >> amount) | (value << (32 - amount));
        }

        template <bool shiftByImm>
        uint64_t shift(uint32_t value, ShiftType type, uint8_t amount, bool oldCarry)
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

            const uint64_t extendedVal = static_cast<uint64_t>(value);

            // Edge cases for shifts by register value: Value of 0 does nothing & keeps old carry!
            if (!shiftByImm && amount == 0) {
                return extendedVal | (bmap<uint64_t>(oldCarry) << 32);
            }

            switch (type) {
                case LSL:
                    return extendedVal << amount;
                case LSR: {
                    /*
                    Carry flag is the MSB of the out shifted values! -> bit amount - 1
                    */
                    uint64_t carry = (amount > 32 ? 0 : (static_cast<uint64_t>((extendedVal >> (amount - 1)) & 0x1) << 32));
                    uint64_t res = (extendedVal >> amount) | carry;

                    // LSR#0: Interpreted as LSR#32, ie. Op2 becomes zero, C becomes Bit 31 of Rm
                    if (initialZeroAmount) {
                        res |= static_cast<uint64_t>(value & (static_cast<uint32_t>(1) << 31)) << 1;
                    }

                    return res;
                }
                case ASR: {

                    /*
                    Carry flag is the MSB of the out shifted values! -> bit amount - 1
                    */
                    // ensure a value in range of [0, 32]
                    amount = amount > 32 ? 32 : amount;
                    uint64_t carry = (static_cast<uint64_t>((extendedVal >> (amount - 1)) & 0x1) << 32);
                    return (static_cast<uint64_t>(static_cast<int64_t>(extendedVal << 32) / (static_cast<int64_t>(1) << amount)) >> 32) | carry;
                }
                case ROR: {
                    // ensure a value in range of [0, 32]
                    uint8_t restOf32 = amount % 32;
                    amount = (amount > 32 ? (restOf32 ? restOf32 : 32) : amount);
                    uint32_t res = static_cast<uint32_t>((extendedVal >> amount) | (extendedVal << (32 - amount)));

                    // ROR#0: Interpreted as RRX#1 (RCR), like ROR#1, but Op2 Bit 31 set to old C.
                    if (initialZeroAmount) {
                        res = (res & ~(static_cast<uint32_t>(1) << 31)) | (bmap<uint64_t>(oldCarry) << 31);
                    }

                    /*
                    Carry flag is the MSB of the out shifted values! -> bit amount - 1
                    */
                    uint64_t extendedRes = static_cast<uint64_t>(res) | (static_cast<uint64_t>((extendedVal >> (amount - 1)) & 0x1) << 32);

                    return extendedRes;
                }
                default:
                    std::cout << "WARNING: Unknown shift type!";
                    return value;
            }
        }

        template uint64_t shift<true>(uint32_t value, ShiftType type, uint8_t amount, bool oldCarry);
        template uint64_t shift<false>(uint32_t value, ShiftType type, uint8_t amount, bool oldCarry);
    } // namespace shifts

    static thumb::ThumbDisas thumbDisas;
    static arm::ArmDisas armDisas;

    std::string Instruction::toString() const
    {
        std::string result;
        if (isArm) {
            arm::ARMInstructionDecoder<arm::ArmDisas>::decode<armDisas>(inst);
            result = armDisas.ss.str();
            armDisas.ss.str(std::string());
        } else {
            thumb::ThumbInstructionDecoder<thumb::ThumbDisas>::decode<thumbDisas>(inst);
            result = thumbDisas.ss.str();
            thumbDisas.ss.str(std::string());
        }

        return result;
    }

    bool conditionSatisfied(ConditionOPCode condition, const CPUState &state)
    {
        switch (condition) {
            // Equal Z==1
            case EQ:
                return state.getFlag<cpsr_flags::Z_FLAG>();
                break;

            // Not equal Z==0
            case NE:
                return !state.getFlag<cpsr_flags::Z_FLAG>();
                break;

            // Carry set / unsigned higher or same C==1
            case CS_HS:
                return state.getFlag<cpsr_flags::C_FLAG>();
                break;

            // Carry clear / unsigned lower C==0
            case CC_LO:
                return !state.getFlag<cpsr_flags::C_FLAG>();
                break;

            // Minus / negative N==1
            case MI:
                return state.getFlag<cpsr_flags::N_FLAG>();
                break;

            // Plus / positive or zero N==0
            case PL:
                return !state.getFlag<cpsr_flags::N_FLAG>();
                break;

            // Overflow V==1
            case VS:
                return state.getFlag<cpsr_flags::V_FLAG>();
                break;

            // No overflow V==0
            case VC:
                return !state.getFlag<cpsr_flags::V_FLAG>();
                break;

            // Unsigned higher (C==1) AND (Z==0)
            case HI:
                return state.getFlag<cpsr_flags::C_FLAG>() && !state.getFlag<cpsr_flags::Z_FLAG>();
                break;

            // Unsigned lower or same (C==0) OR (Z==1)
            case LS:
                return !state.getFlag<cpsr_flags::C_FLAG>() || state.getFlag<cpsr_flags::Z_FLAG>();
                break;

            // Signed greater than or equal N == V
            case GE:
                return state.getFlag<cpsr_flags::N_FLAG>() == state.getFlag<cpsr_flags::V_FLAG>();
                break;

            // Signed less than N != V
            case LT:
                return state.getFlag<cpsr_flags::N_FLAG>() != state.getFlag<cpsr_flags::V_FLAG>();
                break;

            // Signed greater than (Z==0) AND (N==V)
            case GT:
                return !state.getFlag<cpsr_flags::Z_FLAG>() && state.getFlag<cpsr_flags::N_FLAG>() == state.getFlag<cpsr_flags::V_FLAG>();
                break;

            // Signed less than or equal (Z==1) OR (N!=V)
            case LE:
                return state.getFlag<cpsr_flags::Z_FLAG>() || state.getFlag<cpsr_flags::N_FLAG>() != state.getFlag<cpsr_flags::V_FLAG>();
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

    uint16_t hashArm(uint32_t inst)
    {
        return ((inst >> 16) & 0xFF0) | ((inst >> 4) & 0xF);
    }
    uint16_t hashThumb(uint16_t inst)
    {
        return inst >> 6;
    }

} // namespace gbaemu
