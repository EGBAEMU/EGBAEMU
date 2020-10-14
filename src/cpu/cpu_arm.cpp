#include "cpu.hpp"
#include "decode/inst.hpp"
#include "logging.hpp"
#include "swi.hpp"
#include "util.hpp"

#include "decode/inst_arm.hpp"

#include <iostream>
#include <set>

namespace gbaemu
{
    constexpr shifts::ShiftType getShiftType(InstructionID id)
    {
        switch (id) {
            case LSL:
                return shifts::ShiftType::LSL;
            case LSR:
                return shifts::ShiftType::LSR;
            case ASR:
                return shifts::ShiftType::ASR;
            case ROR:
                return shifts::ShiftType::ROR;
            default:
                return shifts::ShiftType::LSL;
        }
    }

    // shout-outs to https://smolka.dev/eggvance/progress-3/ & https://smolka.dev/eggvance/progress-5/
    // for his insane optimization ideas
    constexpr uint16_t constexprHashArm(uint32_t inst)
    {
        return ((inst >> 16) & 0xFF0) | ((inst >> 4) & 0xF);
    }
    template <uint16_t hash>
    constexpr uint32_t dehashArm()
    {
        return (static_cast<uint32_t>(hash & 0xFF0) << 16) | ((hash & 0xF) << 4);
    }
    constexpr uint16_t constexprHashThumb(uint16_t inst)
    {
        return inst >> 6;
    }
    template <uint16_t hash>
    constexpr uint16_t dehashThumb()
    {
        return hash << 6;
    }

    template <bool a, bool s, bool thumb>
    void CPU::handleMultAcc(uint32_t instruction)
    {
        /*
        void handleMultAcc(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm);
        */
        uint8_t rd = thumb ? (instruction & 0x7) : ((instruction >> 16) & 0x0F);
        uint8_t rn = thumb ? (0) : ((instruction >> 12) & 0x0F);
        uint8_t rs = thumb ? ((instruction >> 3) & 0x7) : ((instruction >> 8) & 0x0F);
        uint8_t rm = thumb ? (rd) : (instruction & 0x0F);

#ifdef DEBUG_CLI
        // Check given restrictions
        if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: MUL/MLA PC register may not be involved in calculations!" << std::endl;
        }
#endif

        auto currentRegs = state.getCurrentRegs();
        const uint32_t rmVal = *currentRegs[rm];
        const uint32_t rsVal = *currentRegs[rs];
        const uint32_t rnVal = *currentRegs[rn];

        uint32_t mulRes = rmVal * rsVal;

        if (a) { // MLA: add RN
            mulRes += rnVal;
        }

        *currentRegs[rd] = static_cast<uint32_t>(mulRes & 0x0FFFFFFFF);

        if (s) {
            // update zero flag & signed flags
            // the rest is unaffected
            setFlags<true,
                     true,
                     false,
                     false,
                     false>(
                mulRes,
                false,
                false);
        }

        /*
        Execution Time: 1S+mI for MUL, and 1S+(m+1)I for MLA.
        Whereas 'm' depends on whether/how many most significant bits of Rs are all zero or all one.
        That is m=1 for Bit 31-8, m=2 for Bit 31-16, m=3 for Bit 31-24, and m=4 otherwise.
        */
        // bool a decides if it is a MLAL instruction or MULL
        state.cpuInfo.cycleCount += (a ? 1 : 0);

        if (((rsVal >> 8) & 0x00FFFFFF) == 0 || ((rsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF) {
            state.cpuInfo.cycleCount += 1;
        } else if (((rsVal >> 16) & 0x0000FFFF) == 0 || ((rsVal >> 16) & 0x0000FFFF) == 0x0000FFFF) {
            state.cpuInfo.cycleCount += 2;
        } else if (((rsVal >> 24) & 0x000000FF) == 0 || ((rsVal >> 24) & 0x000000FF) == 0x000000FF) {
            state.cpuInfo.cycleCount += 3;
        } else {
            state.cpuInfo.cycleCount += 4;
        }
    }

    template <bool a, bool s, bool signMul>
    void CPU::handleMultAccLong(uint32_t instruction)
    {
        uint8_t rd_msw = (instruction >> 16) & 0x0F;
        uint8_t rd_lsw = (instruction >> 12) & 0x0F;
        uint8_t rs = (instruction >> 8) & 0x0F;
        uint8_t rm = instruction & 0x0F;

#ifdef DEBUG_CLI
        if (rd_lsw == rd_msw || rd_lsw == rm || rd_msw == rm) {
            std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL lo, high & rm registers may not be the same!" << std::endl;
        }
        if (rd_lsw == regs::PC_OFFSET || rd_msw == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL PC register may not be involved in calculations!" << std::endl;
        }
#endif

        auto currentRegs = state.getCurrentRegs();

        const uint64_t rdVal = (static_cast<uint64_t>(*currentRegs[rd_msw]) << 32) | *currentRegs[rd_lsw];

        uint64_t mulRes;

        const uint32_t unExtRmVal = *currentRegs[rm];
        const uint32_t unExtRsVal = *currentRegs[rs];

        if (!signMul) {
            uint64_t rmVal = static_cast<uint64_t>(unExtRmVal);
            uint64_t rsVal = static_cast<uint64_t>(unExtRsVal);

            mulRes = rmVal * rsVal;

            if (a) { // UMLAL: add rdVal
                mulRes += rdVal;
            }
        } else {
            // Enforce sign extension
            int64_t rmVal = static_cast<int64_t>(static_cast<int32_t>(unExtRmVal));
            int64_t rsVal = static_cast<int64_t>(static_cast<int32_t>(unExtRsVal));

            int64_t signedMulRes = rmVal * rsVal;

            if (a) { // SMLAL: add rdVal
                signedMulRes += static_cast<int64_t>(rdVal);
            }

            mulRes = static_cast<uint64_t>(signedMulRes);
        }

        *currentRegs[rd_msw] = static_cast<uint32_t>((mulRes >> 32) & 0x0FFFFFFFF);
        *currentRegs[rd_lsw] = static_cast<uint32_t>(mulRes & 0x0FFFFFFFF);

        if (s) {
            // update zero flag & signed flags
            // the rest is unaffected
            // for the flags the whole 64 bit are considered as this is the result beeing returned
            // -> negative is checked at bit 63
            // -> zero flag <=> all 64 bit == 0
            bool negative = mulRes & (static_cast<uint64_t>(1) << 63);
            bool zero = mulRes == 0;

            state.setFlag<cpsr_flags::N_FLAG>(negative);

            state.setFlag<cpsr_flags::Z_FLAG>(zero);
        }

        /*
        Execution Time: 1S+(m+1)I for MULL, and 1S+(m+2)I for MLAL.
        Whereas 'm' depends on whether/how many most significant bits of Rs are "all zero" (UMULL/UMLAL)
        or "all zero or all one" (SMULL,SMLAL).
        That is m=1 for Bit31-8, m=2 for Bit31-16, m=3 for Bit31-24, and m=4 otherwise.
        */
        // bool a decides if it is a MLAL instruction or MULL
        state.cpuInfo.cycleCount += (a ? 2 : 1);

        if (((unExtRsVal >> 8) & 0x00FFFFFF) == 0 || (signMul && ((unExtRsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF)) {
            state.cpuInfo.cycleCount += 1;
        } else if (((unExtRsVal >> 16) & 0x0000FFFF) == 0 || (signMul && ((unExtRsVal >> 16) & 0x0000FFFF) == 0x0000FFFF)) {
            state.cpuInfo.cycleCount += 2;
        } else if (((unExtRsVal >> 24) & 0x000000FF) == 0 || (signMul && ((unExtRsVal >> 24) & 0x000000FF) == 0x000000FF)) {
            state.cpuInfo.cycleCount += 3;
        } else {
            state.cpuInfo.cycleCount += 4;
        }
    }

    template <bool b>
    void CPU::handleDataSwp(uint32_t instruction)
    {
        uint8_t rn = (instruction >> 16) & 0x0F;
        uint8_t rd = (instruction >> 12) & 0x0F;
        uint8_t rm = instruction & 0x0F;

#ifdef DEBUG_CLI
        if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: SWP/SWPB PC register may not be involved in calculations!" << std::endl;
        }
#endif

        auto currentRegs = state.getCurrentRegs();
        uint32_t newMemVal = *currentRegs[rm];
        uint32_t memAddr = *currentRegs[rn];

        // Execution Time: 1S+2N+1I. That is, 2N data cycles (added through Memory class), 1S code cycle, plus 1I(initial value)

        state.cpuInfo.cycleCount += 1;

        if (b) {
            uint8_t memVal = state.memory.read8(memAddr, state.cpuInfo, false);
            state.memory.write8(memAddr, static_cast<uint8_t>(newMemVal & 0x0FF), state.cpuInfo);
            *currentRegs[rd] = static_cast<uint32_t>(memVal);
        } else {
            // LDR part
            uint32_t alignedWord = state.memory.read32(memAddr, state.cpuInfo, false);
            alignedWord = shifts::shift(alignedWord, shifts::ShiftType::ROR, (memAddr & 0x03) * 8, false, false) & 0xFFFFFFFF;
            *currentRegs[rd] = alignedWord;

            // STR part
            state.memory.write32(memAddr, newMemVal, state.cpuInfo);
        }
    }

    // Executes instructions belonging to the branch subsection
    template <bool link>
    void CPU::handleBranch(uint32_t instruction)
    {
        int32_t offset = signExt<int32_t, uint32_t, 24>(instruction & 0x00FFFFFF);

        auto currentRegs = state.getCurrentRegs();

        // If link is set, R14 will receive the address of the next instruction to be executed. So if we are
        // jumping but want to remember where to return to after the subroutine finished that might be usefull.
        if (link) {
            // Note that pc is already incremented by 4
            // Next instruction should be at: PC + 4
            *currentRegs[regs::LR_OFFSET] = *currentRegs[regs::PC_OFFSET];
        }

        // Offset is given in units of 4. Thus we need to shift it first by two
        offset <<= 2;

        // Note that pc is already incremented by 4
        *currentRegs[regs::PC_OFFSET] += static_cast<uint32_t>(4 + offset);

        // Execution Time: 2S + 1N
        // This is a branch instruction so we need to refill the pipeline!
        refillPipelineAfterBranch<false>();
    }

    // Executes instructions belonging to the branch and execute subsection
    void CPU::handleBranchAndExchange(uint32_t instruction)
    {
        uint8_t rn = static_cast<uint8_t>(instruction & 0x0F);
        auto currentRegs = state.getCurrentRegs();

        // Load the content of register given by rm
        uint32_t rnValue = *currentRegs[rn];
        // If the first bit of rn is set
        bool changeToThumb = rnValue & 0x00000001;

        // Change the PC to the address given by rm. Note that we have to mask out the thumb switch bit.
        *currentRegs[regs::PC_OFFSET] = rnValue & 0xFFFFFFFE;

        // Execution Time: 2S + 1N
        // This is a branch instruction so we need to refill the pipeline!
        if (changeToThumb) {
            state.setFlag<cpsr_flags::THUMB_STATE>(true);
            refillPipelineAfterBranch<true>();
        } else {
            refillPipelineAfterBranch<false>();
        }
    }

#define CREATE_CONSTEXPR_GETTER(Name, ...)                              \
    static constexpr bool get##Name(InstructionID id)                   \
    {                                                                   \
        constexpr InstructionID Name[] = {__VA_ARGS__};                 \
        for (uint32_t i = 0; i < sizeof(Name) / sizeof(Name[0]); ++i) { \
            if (Name[i] == id)                                          \
                return true;                                            \
        }                                                               \
        return false;                                                   \
    }

    CREATE_CONSTEXPR_GETTER(UpdateNegative, ADC, ADD, AND, BIC, CMN,
                            CMP, EOR, MOV, MVN, ORR,
                            RSB, RSC, SBC, SUB, TEQ,
                            TST, ADD_SHORT_IMM, SUB_SHORT_IMM, MUL, NEG)
    CREATE_CONSTEXPR_GETTER(UpdateZero, ADC, ADD, AND, BIC, CMN,
                            CMP, EOR, MOV, MVN, ORR,
                            RSB, RSC, SBC, SUB, TEQ,
                            TST, ADD_SHORT_IMM, SUB_SHORT_IMM, MUL, NEG)
    CREATE_CONSTEXPR_GETTER(UpdateCarry,
                            ADC, ADD, CMN, CMP, RSB,
                            RSC, SBC, SUB, ADD_SHORT_IMM, SUB_SHORT_IMM, NEG)
    CREATE_CONSTEXPR_GETTER(UpdateOverflow,
                            ADC, ADD, CMN, CMP, MOV,
                            RSB, RSC, SBC, SUB, ADD_SHORT_IMM, SUB_SHORT_IMM, NEG)
    CREATE_CONSTEXPR_GETTER(UpdateCarryFromShiftOp,
                            AND, EOR, MOV, MVN, ORR,
                            BIC, TEQ, TST)
    CREATE_CONSTEXPR_GETTER(DontUpdateRD,
                            CMP, CMN, TST, TEQ, MSR_SPSR, MSR_CPSR)
    CREATE_CONSTEXPR_GETTER(InvertCarry,
                            CMP, SUB, SBC, RSB, RSC, NEG, SUB_SHORT_IMM)
    CREATE_CONSTEXPR_GETTER(MovSPSR,
                            SUB, MVN, ADC, ADD, AND,
                            BIC, EOR, MOV, ORR, RSB,
                            RSC, SBC, ADD_SHORT_IMM, SUB_SHORT_IMM)

    /* ALU functions */
    template <InstructionID id, bool i, bool s, bool shiftAmountFromReg, bool thumb, thumb::ThumbInstructionCategory cat, InstructionID origID>
    void CPU::execDataProc(uint32_t inst)
    {

        static_assert(!i || !shiftAmountFromReg);
        static_assert(id != INVALID);
        static_assert(!thumb || (cat == thumb::ADD_SUB || cat == thumb::MOV_CMP_ADD_SUB_IMM || cat == thumb::ALU_OP));

        uint8_t rn = (inst >> 16) & 0x0F;
        uint8_t rd = (inst >> 12) & 0x0F;
        uint16_t operand2 = inst & 0x0FFF;

        if (thumb && cat == thumb::MOV_CMP_ADD_SUB_IMM) {
            rn = (inst >> 8) & 0x7;
            rd = rn;
            operand2 = inst & 0x0FF;
        } else if (thumb && cat == thumb::ADD_SUB) {
            rn = (inst >> 3) & 0x7;
            rd = (inst & 0x7);
            operand2 = ((inst >> 6) & 0x7);
        } else if (thumb && cat == thumb::ALU_OP) {

            constexpr shifts::ShiftType origShiftType = getShiftType(origID);

            rd = inst & 0x7;
            rn = rd;
            uint8_t rs = (inst >> 3) & 0x7;

            if (id == MOV)
                //TODO previously we did: uint8_t shiftAmount = rsValue & 0xFF; we might need to enforce this in execDataProc as well!
                // Set bit 4 for shiftAmountFromReg flag & move regs at their positions & include the shifttype to use
                operand2 = (static_cast<uint16_t>(1) << 4) | rd | (static_cast<uint16_t>(rs) << 8) | (static_cast<uint16_t>(origShiftType) << 5);
            else
                operand2 = rs;
        }

        bool carry = state.getFlag<cpsr_flags::C_FLAG>();

        const auto currentRegs = state.getCurrentRegs();

        uint64_t shifterOperand;
        shifts::ShiftType shiftType = shifts::ShiftType::ROR;
        uint8_t shiftAmount = 0;
        if (i) {
            shifterOperand = shifts::shift(operand2 & 0x0FF, shifts::ShiftType::ROR, ((operand2 >> 8) & 0x0F) * 2, carry, false);
        } else {
            /* calculate shifter operand */
            shiftType = static_cast<shifts::ShiftType>((operand2 >> 5) & 0b11);
            uint8_t rm = operand2 & 0xF;

            if (shiftAmountFromReg) {
                uint8_t rs = (operand2 >> 8) & 0x0F;
                shiftAmount = *currentRegs[rs];
            } else {
                shiftAmount = (operand2 >> 7) & 0b11111;
            }

            uint32_t rmValue = *currentRegs[rm];

            if (rm == regs::PC_OFFSET) {
                // Note that pc is already incremented by 2/4
                // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
                // PC+12 if I=0,R=1 (shift by register),
                // otherwise PC+8 (shift by immediate).
                if (!i && shiftAmountFromReg) {
                    rmValue += thumb ? 4 : 8;
                } else {
                    rmValue += thumb ? 2 : 4;
                }
            }

            shifterOperand = shifts::shift(rmValue, shiftType, shiftAmount, carry, !shiftAmountFromReg);
        }

        bool shifterOperandCarry = shifterOperand & (static_cast<uint64_t>(1) << 32);
        shifterOperand &= 0xFFFFFFFF;

        uint64_t rnValue = *currentRegs[rn];
        if (rn == regs::PC_OFFSET) {
            // Note that pc is already incremented by 2 / 4
            // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
            // PC+12 if I=0,R=1 (shift by register),
            // otherwise PC+8 (shift by immediate).
            if (!i && shiftAmountFromReg) {
                rnValue += thumb ? 4 : 8;
            } else {
                rnValue += thumb ? 2 : 4;
            }
        }

        uint64_t resultValue = 0;

        /* Different instructions cause different flags to be changed. */
        constexpr bool updateNegative = getUpdateNegative(id);
        constexpr bool updateZero = getUpdateZero(id);
        constexpr bool updateCarry = getUpdateCarry(id);
        constexpr bool updateOverflow = getUpdateOverflow(id);
        constexpr bool updateCarryFromShiftOp = getUpdateCarryFromShiftOp(id);
        constexpr bool dontUpdateRD = getDontUpdateRD(id);
        constexpr bool invertCarry = getInvertCarry(id);
        constexpr bool movSPSR = getMovSPSR(id);

        /* execute functions */
        switch (id) {
            case ADC:
                resultValue = rnValue + shifterOperand + (carry ? 1 : 0);
                break;
            case CMN:
            case ADD:
            case ADD_SHORT_IMM:
                resultValue = rnValue + shifterOperand;
                break;
            case TST:
            case AND:
                resultValue = rnValue & shifterOperand;
                break;
            case BIC:
                resultValue = rnValue & (~shifterOperand);
                break;
            case TEQ:
            case EOR:
                resultValue = rnValue ^ shifterOperand;
                break;
            case MOV:
                resultValue = shifterOperand;
                break;
            case MRS_CPSR:
            case MRS_SPSR: {
                constexpr bool r = id == MRS_SPSR;

                if (r)
                    resultValue = *currentRegs[regs::SPSR_OFFSET];
                else
                    resultValue = state.getCurrentCPSR();
                break;
            }
            case MSR_SPSR:
            case MSR_CPSR: {
                // true iff write to flag field is allowed 31-24
                bool f_ = rn & 0x08;
                // true iff write to status field is allowed 23-16
                bool s_ = rn & 0x04;
                // true iff write to extension field is allowed 15-8
                bool x_ = rn & 0x02;
                // true iff write to control field is allowed 7-0
                bool c_ = rn & 0x01;

                uint32_t bitMask = (f_ ? 0xFF000000 : 0) | (s_ ? 0x00FF0000 : 0) | (x_ ? 0x0000FF00 : 0) | (c_ ? 0x000000FF : 0);

                constexpr bool r = id == MSR_SPSR;

                // ensure that only fields that should be written to are changed!
                resultValue = (shifterOperand & bitMask);

                // Shady trick to fix destination register because extracted rd value is not used
                if (r) {
                    rd = regs::SPSR_OFFSET;
                    // clear fields that should be written to
                    resultValue |= *currentRegs[regs::SPSR_OFFSET] & ~bitMask;
                    *currentRegs[regs::SPSR_OFFSET] = resultValue;
                } else {
                    rd = 16 /*regs::CPSR_OFFSET*/;
                    // clear fields that should be written to
                    resultValue |= state.getCurrentCPSR() & ~bitMask;
                    state.updateCPSR(resultValue);
                }

                break;
            }
            case MVN:
                resultValue = ~shifterOperand;
                break;
            case ORR:
                resultValue = rnValue | shifterOperand;
                break;
            case RSB:
                resultValue = static_cast<int64_t>(shifterOperand) - static_cast<int64_t>(rnValue);
                rnValue = (rnValue >> 31) & 1 ? 0 : static_cast<uint32_t>(1) << 31;
                break;
            case RSC:
                resultValue = static_cast<int64_t>(shifterOperand) - static_cast<int64_t>(rnValue) - (carry ? 0 : 1);
                rnValue = (rnValue >> 31) & 1 ? 0 : static_cast<uint32_t>(1) << 31;
                break;
            case SBC:
                resultValue = static_cast<int64_t>(rnValue) - static_cast<int64_t>(shifterOperand) - (carry ? 0 : 1);
                shifterOperand = (shifterOperand >> 31) & 1 ? 0 : static_cast<uint32_t>(1) << 31;
                break;
            case CMP:
            case SUB:
            case SUB_SHORT_IMM:
                resultValue = static_cast<int64_t>(rnValue) - static_cast<int64_t>(shifterOperand);
                shifterOperand = (shifterOperand >> 31) & 1 ? 0 : static_cast<uint32_t>(1) << 31;
                break;
            case NEG:
                /*
                    ALARM, ALAAARM: NEG is not an ARM instruction, it only exists in thumb, where it uses
                    only the second operand rs, rd (and therefore rn) is ignored!!!
                 */
                resultValue = -static_cast<int64_t>(shifterOperand);
                shifterOperand = (rnValue >> 31) & 1 ? 0 : static_cast<uint32_t>(1) << 31;
                rnValue = 0;
                break;

            default:
                state.executionInfo.message << "ERROR: execDataProc can not handle instruction: " << instructionIDToString(id) << std::endl;
                state.execState = CPUState::EXEC_ERROR;
                break;
        }

        bool destPC = rd == regs::PC_OFFSET;

        // Special case 9000
        if (movSPSR && s && destPC) {
            state.updateCPSR(*currentRegs[regs::SPSR_OFFSET]);
        } else if (s) {
            setFlags<updateNegative,
                     updateZero,
                     updateOverflow,
                     updateCarry,
                     invertCarry>(
                resultValue,
                (rnValue >> 31) & 1,
                (shifterOperand >> 31) & 1);

            if (updateCarryFromShiftOp &&
                (i || shiftType != shifts::ShiftType::LSL || shiftAmount != 0)) {
                state.setFlag<cpsr_flags::C_FLAG>(shifterOperandCarry);
            }
        }

        if (!dontUpdateRD)
            *currentRegs[rd] = static_cast<uint32_t>(resultValue);

        if (destPC) {
            refillPipeline();
        }
        if (!i && shiftAmountFromReg) {
            state.cpuInfo.cycleCount += 1;
        }
    }

    template <bool thumb, bool pre, bool up, bool writeback, bool forceUserRegisters, bool load, bool patchRList, bool useSP>
    void CPU::execDataBlockTransfer(uint32_t inst)
    {
        static_assert(!(!thumb && patchRList));
        static_assert(!(!thumb && useSP));
        //TODO NOTE: this instruction is reused for handleThumbMultLoadStore & handleThumbPushPopRegister

        uint8_t rn = thumb ? (useSP ? regs::SP_OFFSET : ((inst >> 8) & 0x7))
                           : ((inst >> 16) & 0x0F);
        uint16_t rList = thumb ? (inst & 0x0FF) : (inst & 0x0FFFF);
        if (patchRList) {
            if (load) {
                rList |= static_cast<uint16_t>(1) << regs::PC_OFFSET;
            } else {
                rList |= static_cast<uint16_t>(1) << regs::LR_OFFSET;
            }
        }

        auto currentRegs = state.getCurrentRegs();

        /*
        When S Bit is set (S=1)
        If instruction is LDM and R15 is in the list: (Mode Changes)
          While R15 loaded, additionally: CPSR=SPSR_<current mode>
        Otherwise: (User bank transfer)
          Rlist is referring to User Bank Registers, R0-R15 (rather than
          register related to the current mode, such like R14_svc etc.)
          Base write-back should not be used for User bank transfer.
        */
        if (forceUserRegisters && (!load || (rList & (1 << regs::PC_OFFSET)) == 0)) {
            currentRegs = state.getModeRegs(CPUState::UserMode);
        }

        uint32_t address = *currentRegs[rn];

        // Execution Time:
        // For normal LDM, nS+1N+1I. For LDM PC, (n+1)S+2N+1I.
        // For STM (n-1)S+2N. Where n is the number of words transferred.
        if (load) {
            // handle +1I
            state.cpuInfo.cycleCount += 1;
        } else {
            // same edge case as for STR
            patchFetchToNCycle();
        }

        // The first read / write is non sequential but afterwards all accesses are sequential
        // As there will always be at least one transfer we can patch the non sequential access and always add cycles as if sequential
        memory::MemoryRegion memReg = Memory::extractMemoryRegion(address);
        state.cpuInfo.cycleCount += state.memory.memCycles32(memReg, false) - state.memory.memCycles32(memReg, true);

        // Handle edge case: Empty Rlist: R15 loaded/stored (ARMv4 only)
        // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5).
        if (!patchRList && rList == 0) {
            // handle this odd case on its own!

            // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5).
            if (up)
                *currentRegs[rn] += 0x40;
            else
                *currentRegs[rn] -= 0x40;

            /*
            empty rlist edge cases:
            STMIA: str -> r0        // pre = false, up = true
                r0 = r0 + 0x40
            STMIB: str->r0 +4       // pre = true, up = true
                r0 = r0 + 0x40
            STMDB: str ->r0 - 0x40  // pre = true, up = false
                r0 = r0 - 0x40
            STMDA: str -> r0 -0x3C  // pre = false, up = false
                r0 = r0 - 0x40
            */
            if (pre && up) {
                address += 4;
            } else if (pre && !up) {
                address -= 0x40;
            } else if (!pre && !up) {
                address -= 0x3C;
            }

            if (load) {
                *currentRegs[regs::PC_OFFSET] = state.memory.read32(address, state.cpuInfo, true);
                refillPipelineAfterBranch<thumb>();
            } else {
                // Note that pc is already incremented by 2/4
                // Edge case of storing PC -> PC + 12 will be stored
                state.memory.write32(address, *currentRegs[regs::PC_OFFSET] + (thumb ? 4 : 8), state.cpuInfo, true);
            }

            // fully handled edge case
            return;
        }

        uint32_t unchangedAddr = address;

        /*
        Transfer Order
        The lowest Register in Rlist (R0 if its in the list) will be loaded/stored to/from the lowest memory address.
        Internally, the rlist register are always processed with INCREASING addresses 
        (ie. for DECREASING addressing modes, the CPU does first calculate the lowest address,
        and does then process rlist with increasing addresses; this detail can be important when accessing memory mapped I/O ports).
        */
        // get the count of bits set -> transfers that will be done!
        uint8_t bitsSet = popcnt(rList);
        if (!up) {
            // Calculate lowest address to start with
            address -= static_cast<uint32_t>(bitsSet) << 2;
        }

        // Also do the write back to the register!
        if (!up && writeback) {
            // If decrementing we already have the correct address!
            *currentRegs[rn] = address;
        } else if (up && writeback) {
            // If incrementing we have to calculate the final address first
            *currentRegs[rn] += (static_cast<uint32_t>(bitsSet) << 2);
        }

        // Edge case: writeback enabled & rn is inside rlist
        // If load then it was overwritten anyway & we should not do another writeback as writeback comes before read!
        // else on STM it depends if this register was the first written to memory
        // if so we need to write the unchanged memory address into memory (and remove rn from the rList)
        // else the address that would normally written back is stored into memory (out default behaviour)
        // Check if edge case conditions are met & there are none registers that were written first to memory
        if (!load && writeback && (rList & (1 << rn)) && (rList & ((1 << rn) - 1)) == 0) {
            // write unchanged address value to memory
            state.memory.write32(address + (up == pre ? 4 : 0), unchangedAddr, state.cpuInfo, true);
            // update the address
            address += 4;
            // remove rn from rList
            rList ^= 1 << rn;
        }

        bool loadedPC = load && (patchRList || (rList & (1 << regs::PC_OFFSET)));

        // shout-outs to https://smolka.dev/eggvance/progress-5/
        for (; rList != 0; rList &= rList - 1) {
            uint8_t currentIdx = ctz(rList);

            // See https://iitd-plos.github.io/col718/ref/arm-instructionset.pdf page 38ff.
            if (up == pre) {
                address += 4;
            }

            if (load) {
                *currentRegs[currentIdx] = state.memory.read32(address, state.cpuInfo, true);
            } else {
                // Note that pc is already incremented by 2/4
                // Edge case of storing PC -> PC + 12 will be stored
                state.memory.write32(address, *currentRegs[currentIdx] + (currentIdx == regs::PC_OFFSET ? (thumb ? 4 : 8) : 0), state.cpuInfo, true);
            }

            if (up != pre) {
                address += 4;
            }
        }

        if (loadedPC) {
            // More special cases
            /*
            When S Bit is set (S=1)
            If instruction is LDM and R15 is in the list: (Mode Changes)
            While R15 loaded, additionally: CPSR=SPSR_<current mode>
            */
            // Special case for pipeline refill
            if (forceUserRegisters) {
                state.updateCPSR(state.accessReg(regs::SPSR_OFFSET));
            }

            refillPipeline();
        }
    }

    template <InstructionID id, bool thumb, bool pre, bool up, bool i, bool writeback, thumb::ThumbInstructionCategory thumbCat>
    // rn, rd, addrMode
    void CPU::execLoadStoreRegUByte(uint32_t inst)
    {
        /*
            Opcode Format

            Bit    Expl.
            31-28  Condition (Must be 1111b for PLD)
            27-26  Must be 01b for this instruction
            25     I - Immediate Offset Flag (0=Immediate, 1=Shifted Register)
            24     P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)
            23     U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
            22     B - Byte/Word bit (0=transfer 32bit/word, 1=transfer 8bit/byte)
            When above Bit 24 P=0 (Post-indexing, write-back is ALWAYS enabled):
                21     T - Memory Management (0=Normal, 1=Force non-privileged access)
            When above Bit 24 P=1 (Pre-indexing, write-back is optional):
                21     W - Write-back bit (0=no write-back, 1=write address into base)
            20     L - Load/Store bit (0=Store to memory, 1=Load from memory)
                    0: STR{cond}{B}{T} Rd,<Address>   ;[Rn+/-<offset>]=Rd
                    1: LDR{cond}{B}{T} Rd,<Address>   ;Rd=[Rn+/-<offset>]
                    (1: PLD <Address> ;Prepare Cache for Load, see notes below)
                    Whereas, B=Byte, T=Force User Mode (only for POST-Indexing)
            19-16  Rn - Base register               (R0..R15) (including R15=PC+8)
            15-12  Rd - Source/Destination Register (R0..R15) (including R15=PC+12)
            When above I=0 (Immediate as Offset)
                11-0   Unsigned 12bit Immediate Offset (0-4095, steps of 1)
            When above I=1 (Register shifted by Immediate as Offset)
                11-7   Is - Shift amount      (1-31, 0=Special/See below)
                6-5    Shift Type             (0=LSL, 1=LSR, 2=ASR, 3=ROR)
                4      Must be 0 (Reserved, see The Undefined Instruction)
                3-0    Rm - Offset Register   (R0..R14) (not including PC=R15)
         */

        uint8_t rn, rd;
        uint16_t addrMode;

        static_assert(!thumb || (thumbCat == thumb::PC_LD || thumbCat == thumb::LD_ST_REL_OFF || thumbCat == thumb::LD_ST_IMM_OFF || thumbCat == thumb::LD_ST_REL_SP));

        if (thumb) {
            switch (thumbCat) {
                case thumb::PC_LD: {
                    rn = regs::PC_OFFSET;
                    rd = (inst >> 8) & 0x7;
                    addrMode = (static_cast<uint16_t>(inst & 0x0FF) << 2);
                    break;
                }
                case thumb::LD_ST_REL_OFF: {
                    rn = (inst >> 3) & 0x7; /* rb */
                    rd = inst & 0x7;        /* rd */
                    addrMode = static_cast<uint16_t>(shifts::ShiftType::LSL << 5) | static_cast<uint16_t>((inst >> 6) & 0x7 /* ro */);
                    break;
                }
                case thumb::LD_ST_IMM_OFF: {
                    rn = (inst >> 3) & 0x7; /* rb */
                    rd = inst & 0x7;        /* rd */
                    uint8_t offset = (inst >> 6) & 0x1F;

                    if (id == STR || id == LDR)
                        addrMode = static_cast<uint16_t>(offset) << 2;
                    else
                        addrMode = static_cast<uint16_t>(offset);

                    break;
                }
                case thumb::LD_ST_REL_SP: {
                    rn = regs::SP_OFFSET;
                    rd = (inst >> 8) & 0x7; /* rd */
                    addrMode = static_cast<uint16_t>(inst & 0x0FF /* offset */) << 2;
                    break;
                }
                default:
                    break;
            }
        } else {
            rn = (inst >> 16) & 0x0F;
            rd = (inst >> 12) & 0x0F;
            addrMode = inst & 0x0FFF;
        }

        constexpr bool load = id == LDR || id == LDRB;
        constexpr bool byte = id == LDRB || id == STRB;
        constexpr bool immediate = !i;

        auto currentRegs = state.getCurrentRegs();

        if (!pre) {
            constexpr bool forceNonPrivAccess = writeback;
            // I assume that we access user regs... because user mode is the only non-priviledged mode
            if (forceNonPrivAccess) {
                currentRegs = state.getModeRegs(CPUState::UserMode);
            }
        }

        /* these are computed in the next step */
        uint32_t memoryAddress;
        uint32_t offset;

        // Execution Time: For normal LDR: 1S+1N+1I. For LDR PC: 2S+2N+1I. For STR: 2N.

        if (load) {
            // 1 I for beeing complex
            state.cpuInfo.cycleCount += 1;
        } else {
            // same edge case as for STR
            patchFetchToNCycle();
        }

        /* offset is calculated differently, depending on the I-bit */
        if (immediate) {
            offset = addrMode;
        } else {
            uint8_t shiftAmount = (addrMode >> 7) & 0x1F;
            auto shiftType = static_cast<shifts::ShiftType>((addrMode >> 5) & 0b11);
            uint8_t rm = addrMode & 0xF;

            offset = shifts::shift(*currentRegs[rm], shiftType, shiftAmount, state.getFlag<cpsr_flags::C_FLAG>(), true) & 0xFFFFFFFF;
        }

        uint32_t rnValue = *currentRegs[rn];
        uint32_t rdValue = *currentRegs[rd];

        bool isRnPC = rn == regs::PC_OFFSET;
        bool isRdPC = rd == regs::PC_OFFSET;

        if (isRnPC) {
            // Note that pc is already incremented by 2/4
            rnValue += thumb ? 2 : 4;

            if (thumb) {
                // special case for thumb PC_LD
                //TODO check that this wont affect other thumb instructions... probably not because normally they can only address r0-r7
                rnValue &= ~2;
            }
        }

        if (!load && isRdPC) {
            // Note that pc is already incremented by 2/4
            rdValue += thumb ? 4 : 8;
        }

        offset = up ? offset : -offset;

        /* if the offset is added depends on the indexing mode */
        if (pre)
            memoryAddress = rnValue + offset;
        else
            memoryAddress = rnValue;

        /* transfer */
        if (load) {
            if (byte) {
                *currentRegs[rd] = state.memory.read8(memoryAddress, state.cpuInfo, false);
            } else {
                // More edge case:
                /*
                When reading a word from a halfword-aligned address (which is located in the middle between two word-aligned addresses),
                the lower 16bit of Rd will contain [address] ie. the addressed halfword, 
                and the upper 16bit of Rd will contain [address-2] ie. more or less unwanted garbage. 
                However, by isolating lower bits this may be used to read a halfword from memory. 
                (Above applies to little endian mode, as used in GBA.)
                */
                uint32_t alignedWord = state.memory.read32(memoryAddress, state.cpuInfo, false);
                alignedWord = shifts::shift(alignedWord, shifts::ShiftType::ROR, (memoryAddress & 0x03) * 8, false, false) & 0xFFFFFFFF;
                *currentRegs[rd] = alignedWord;
            }
        } else {
            if (byte) {
                state.memory.write8(memoryAddress, rdValue, state.cpuInfo);
            } else {
                // Mask out unaligned bits!
                state.memory.write32(memoryAddress, rdValue, state.cpuInfo);
            }
        }

        if ((!pre || writeback) && (!load || rn != rd)) {
            if (!pre) {
                memoryAddress += offset;
            }

            *currentRegs[rn] = memoryAddress;

            if (isRnPC) {
                //TODO this is a very odd edge case!
                std::cout << "WARNING very odd edge case" << std::endl;
                refillPipelineAfterBranch<thumb>();
            }
        }
        if (load && isRdPC) {
            refillPipelineAfterBranch<thumb>();
        }
    }

    /*
            ARM Instruction Description:
                Bit    Expl.
                31-28  Condition
                27-25  Must be 000b for this instruction
                24     P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)
                23     U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
                22     I - Immediate Offset Flag (0=Register Offset, 1=Immediate Offset)
                When above Bit 24 P=0 (Post-indexing, write-back is ALWAYS enabled):
                    21     Not used, must be zero (0)
                When above Bit 24 P=1 (Pre-indexing, write-back is optional):
                    21     W - Write-back bit (0=no write-back, 1=write address into base)
                20     L - Load/Store bit (0=Store to memory, 1=Load from memory)
                19-16  Rn - Base register                (R0-R15) (Including R15=PC+8)
                15-12  Rd - Source/Destination Register  (R0-R15) (Including R15=PC+12)
                11-8   When above Bit 22 I=0 (Register as Offset):
                        Not used. Must be 0000b
                        When above Bit 22 I=1 (immediate as Offset):
                        Immediate Offset (upper 4bits)
                7      Reserved, must be set (1)
                6-5    Opcode (0-3)
                        When Bit 20 L=0 (Store) (and Doubleword Load/Store):
                        0: Reserved for SWP instruction
                        1: STR{cond}H  Rd,<Address>  ;Store halfword   [a]=Rd
                        2: LDR{cond}D  Rd,<Address>  ;Load Doubleword  R(d)=[a], R(d+1)=[a+4]
                        3: STR{cond}D  Rd,<Address>  ;Store Doubleword [a]=R(d), [a+4]=R(d+1)
                        When Bit 20 L=1 (Load):
                        0: Reserved.
                        1: LDR{cond}H  Rd,<Address>  ;Load Unsigned halfword (zero-extended)
                        2: LDR{cond}SB Rd,<Address>  ;Load Signed byte (sign extended)
                        3: LDR{cond}SH Rd,<Address>  ;Load Signed halfword (sign extended)
                4      Reserved, must be set (1)
                3-0    When above Bit 22 I=0:
                        Rm - Offset Register            (R0-R14) (not including R15)
                        When above Bit 22 I=1:
                        Immediate Offset (lower 4bits)  (0-255, together with upper bits)
             */

    template <bool b, InstructionID id, bool thumb, bool pre, bool up, bool writeback, arm::ARMInstructionCategory armCat, thumb::ThumbInstructionCategory thumbCat>
    void CPU::execHalfwordDataTransferImmRegSignedTransfer(uint32_t instruction)
    {
        /*
        Previous parameter: bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset
        */

        // Sanity checks:
        static_assert(thumb || (armCat == arm::SIGN_TRANSF || armCat == arm::HW_TRANSF_IMM_OFF || armCat == arm::HW_TRANSF_REG_OFF));
        static_assert(!thumb || (thumbCat == thumb::LD_ST_SIGN_EXT || thumbCat == thumb::LD_ST_HW));

        constexpr uint8_t transferSize = (id == LDRH || id == STRH || id == LDRSH) ? 16 : 8;
        constexpr bool load = id == LDRH || id == LDRSB || id == LDRSH;
        constexpr bool sign = id == LDRSB || id == LDRSH;

        uint8_t rn;
        uint8_t rd;
        uint32_t offset;

        if (thumb) {
            switch (thumbCat) {
                case thumb::LD_ST_SIGN_EXT: {
                    rn = (instruction >> 3) & 0x7;
                    rd = instruction & 0x7;
                    offset = state.accessReg((instruction >> 6) & 0x7);
                    break;
                }
                case thumb::LD_ST_HW: {
                    rn = (instruction >> 3) & 0x7;
                    rd = instruction & 0x7;
                    offset = static_cast<uint16_t>((instruction >> 6) & 0x1F) << 1;
                    break;
                }
                default: {
                    // Cry
                }
            }
        } else {

            rn = (instruction >> 16) & 0x0F;
            rd = (instruction >> 12) & 0x0F;

            switch (armCat) {
                case arm::SIGN_TRANSF: {

                    if (b)
                        offset = (((instruction >> 8) & 0x0F) << 4) | (instruction & 0x0F);
                    else
                        offset = state.accessReg(instruction & 0x0F);

                    break;
                }
                case arm::HW_TRANSF_IMM_OFF: {
                    /* called addr_mode in detailed doc but is really offset because immediate flag I is 1 */
                    offset = (((instruction >> 8) & 0x0F) << 4) | (instruction & 0x0F);
                    break;
                }
                case arm::HW_TRANSF_REG_OFF: {
                    offset = state.accessReg(instruction & 0x0F);
                    break;
                }
                default: {
                    // Cry
                }
            }
        }

        auto currentRegs = state.getCurrentRegs();

        //Execution Time: For Normal LDR, 1S+1N+1I. For LDR PC, 2S+2N+1I. For STRH 2N

        // both instructions have a + 1I for being complex
        if (load) {
            // 1N is handled by Memory class & 1S is handled globally
            state.cpuInfo.cycleCount += 1;
        } else {
            // same edge case as for STR
            patchFetchToNCycle();
        }

        uint32_t rnValue = *currentRegs[rn];
        uint32_t rdValue = *currentRegs[rd];

        bool isRnPC = rn == regs::PC_OFFSET;
        bool isRdPC = rd == regs::PC_OFFSET;

        if (isRnPC) {
            // Note that pc is already incremented by 2/4
            rnValue += thumb ? 2 : 4;
        }
        if (!load && isRdPC) {
            // Note that pc is already incremented by 2/4
            rdValue += thumb ? 4 : 8;
        }

        offset = up ? offset : -offset;

        uint32_t memoryAddress;
        if (pre) {
            memoryAddress = rnValue + offset;
        } else {
            memoryAddress = rnValue;
        }

        if (load) {
            uint32_t readData;
            if (transferSize == 16) {
                // Handle misaligned address
                if (sign && memoryAddress & 1) {
                    // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value
                    readData = signExt<int32_t, uint32_t, 8>(state.memory.read8(memoryAddress, state.cpuInfo, false));
                } else {
                    // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
                    // LDRH with ROR (see LDR with non word aligned)
                    readData = static_cast<uint32_t>(state.memory.read16(memoryAddress, state.cpuInfo, false));
                    readData = shifts::shift(readData, shifts::ShiftType::ROR, (memoryAddress & 0x01) * 8, false, false) & 0xFFFFFFFF;

                    if (sign) {
                        readData = signExt<int32_t, uint32_t, transferSize>(readData);
                    }
                }
            } else {
                readData = static_cast<uint32_t>(state.memory.read8(memoryAddress, state.cpuInfo, false));

                if (sign) {
                    readData = signExt<int32_t, uint32_t, transferSize>(readData);
                }
            }

            *currentRegs[rd] = readData;
        } else {
            if (transferSize == 16) {
                state.memory.write16(memoryAddress, rdValue, state.cpuInfo);
            } else {
                state.memory.write8(memoryAddress, rdValue, state.cpuInfo);
            }
        }

        if ((!pre || writeback) && (!load || rn != rd)) {
            if (!pre) {
                memoryAddress += offset;
            }

            *currentRegs[rn] = memoryAddress;

            if (isRnPC) {
                //TODO this is a very odd edge case!
                std::cout << "WARNING very odd edge case" << std::endl;
                refillPipelineAfterBranch<thumb>();
            }
        }
        if (load && isRdPC) {
            // will PC be updated? if so we need an additional Prog N & S cycle
            refillPipelineAfterBranch<thumb>();
        }
    }

} // namespace gbaemu

#include "create_arm_lut.tpp"

#include "cpu_thumb.tpp"