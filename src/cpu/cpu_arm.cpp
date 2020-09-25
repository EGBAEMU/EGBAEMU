#include "cpu.hpp"
#include "decode/inst.hpp"
#include "logging.hpp"
#include "swi.hpp"
#include "util.hpp"

#include <iostream>
#include <set>

namespace gbaemu
{

    template <InstructionID id>
    void CPU::handleMultAcc(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm)
    {
        constexpr bool a = id == MLA;

        // Check given restrictions
        if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: MUL/MLA PC register may not be involved in calculations!" << std::endl;
        }

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
        cpuInfo.cycleCount = (a ? 1 : 0);

        if (((rsVal >> 8) & 0x00FFFFFF) == 0 || ((rsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF) {
            cpuInfo.cycleCount += 1;
        } else if (((rsVal >> 16) & 0x0000FFFF) == 0 || ((rsVal >> 16) & 0x0000FFFF) == 0x0000FFFF) {
            cpuInfo.cycleCount += 2;
        } else if (((rsVal >> 24) & 0x000000FF) == 0 || ((rsVal >> 24) & 0x000000FF) == 0x000000FF) {
            cpuInfo.cycleCount += 3;
        } else {
            cpuInfo.cycleCount += 4;
        }
    }

    template <InstructionID id>
    void CPU::handleMultAccLong(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm)
    {
        constexpr bool signMul = id == SMLAL || id == SMULL;
        constexpr bool a = id == SMLAL || id == UMLAL;

        if (rd_lsw == rd_msw || rd_lsw == rm || rd_msw == rm) {
            std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL lo, high & rm registers may not be the same!" << std::endl;
        }
        if (rd_lsw == regs::PC_OFFSET || rd_msw == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL PC register may not be involved in calculations!" << std::endl;
        }

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
        cpuInfo.cycleCount = (a ? 2 : 1);

        if (((unExtRsVal >> 8) & 0x00FFFFFF) == 0 || (signMul && ((unExtRsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF)) {
            cpuInfo.cycleCount += 1;
        } else if (((unExtRsVal >> 16) & 0x0000FFFF) == 0 || (signMul && ((unExtRsVal >> 16) & 0x0000FFFF) == 0x0000FFFF)) {
            cpuInfo.cycleCount += 2;
        } else if (((unExtRsVal >> 24) & 0x000000FF) == 0 || (signMul && ((unExtRsVal >> 24) & 0x000000FF) == 0x000000FF)) {
            cpuInfo.cycleCount += 3;
        } else {
            cpuInfo.cycleCount += 4;
        }
    }

    template <InstructionID id>
    void CPU::handleDataSwp(uint8_t rn, uint8_t rd, uint8_t rm)
    {
        constexpr bool b = id == SWPB;
        //TODO maybe replace by LDR followed by STR?

        if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: SWP/SWPB PC register may not be involved in calculations!" << std::endl;
        }

        auto currentRegs = state.getCurrentRegs();
        uint32_t newMemVal = *currentRegs[rm];
        uint32_t memAddr = *currentRegs[rn];

        // Execution Time: 1S+2N+1I. That is, 2N data cycles (added through Memory class), 1S code cycle, plus 1I(initial value)

        cpuInfo.cycleCount = 1;

        if (b) {
            uint8_t memVal = state.memory.read8(memAddr, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS);
            state.memory.write8(memAddr, static_cast<uint8_t>(newMemVal & 0x0FF), &cpuInfo);
            *currentRegs[rd] = static_cast<uint32_t>(memVal);
        } else {
            // LDR part
            uint32_t alignedWord = state.memory.read32(memAddr, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS);
            alignedWord = shifts::shift(alignedWord, shifts::ShiftType::ROR, (memAddr & 0x03) * 8, false, false) & 0xFFFFFFFF;
            *currentRegs[rd] = alignedWord;

            // STR part
            state.memory.write32(memAddr, newMemVal, &cpuInfo);
        }
    }

    // Executes instructions belonging to the branch subsection
    void CPU::handleBranch(bool link, int32_t offset)
    {
        uint32_t pc = state.getCurrentPC();

        // If link is set, R14 will receive the address of the next instruction to be executed. So if we are
        // jumping but want to remember where to return to after the subroutine finished that might be usefull.
        if (link) {
            // Next instruction should be at: PC + 4
            state.accessReg(regs::LR_OFFSET) = (pc + 4);
        }

        // Offset is given in units of 4. Thus we need to shift it first by two
        offset *= 4;

        state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(pc) + 8 + offset);

        // Execution Time: 2S + 1N

        cpuInfo.additionalProgCyclesN = 1;
        cpuInfo.additionalProgCyclesS = 1;

        // This is a branch instruction so we need to consider self branches!
        cpuInfo.forceBranch = true;
    }

    // Executes instructions belonging to the branch and execute subsection
    void CPU::handleBranchAndExchange(uint8_t rn)
    {
        auto currentRegs = state.getCurrentRegs();

        // Load the content of register given by rm
        uint32_t rnValue = *currentRegs[rn];
        // If the first bit of rn is set
        bool changeToThumb = rnValue & 0x00000001;

        if (changeToThumb) {
            state.setFlag<cpsr_flags::THUMB_STATE>(true);
        }

        // Change the PC to the address given by rm. Note that we have to mask out the thumb switch bit.
        state.accessReg(regs::PC_OFFSET) = rnValue & 0xFFFFFFFE;

        // Execution Time: 2S + 1N

        cpuInfo.additionalProgCyclesN = 1;
        cpuInfo.additionalProgCyclesS = 1;

        // This is a branch instruction so we need to consider self branches!
        cpuInfo.forceBranch = true;
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
                            CMP, CMN, TST, TEQ)
    CREATE_CONSTEXPR_GETTER(InvertCarry,
                            CMP, SUB, SBC, RSB, RSC, NEG, SUB_SHORT_IMM)
    CREATE_CONSTEXPR_GETTER(MovSPSR,
                            SUB, MVN, ADC, ADD, AND,
                            BIC, EOR, MOV, ORR, RSB,
                            RSC, SBC, ADD_SHORT_IMM, SUB_SHORT_IMM)

    /* ALU functions */
    template <InstructionID id, bool thumb>
    void CPU::execDataProc(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2)
    {

        bool carry = state.getFlag<cpsr_flags::C_FLAG>();

        /* calculate shifter operand */
        shifts::ShiftType shiftType;
        uint8_t shiftAmount;
        uint8_t rm;
        uint8_t rs;
        uint8_t imm;
        uint64_t shifterOperand;
        bool shiftByReg = extractOperand2(shiftType, shiftAmount, rm, rs, imm, operand2, i);

        const auto currentRegs = state.getCurrentRegs();

        if (i) {
            shifterOperand = shifts::shift(imm, shifts::ShiftType::ROR, shiftAmount, carry, false);
        } else {
            if (shiftByReg)
                shiftAmount = *currentRegs[rs];

            uint32_t rmValue = *currentRegs[rm];

            if (rm == regs::PC_OFFSET) {
                // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
                // PC+12 if I=0,R=1 (shift by register),
                // otherwise PC+8 (shift by immediate).
                if (!i && shiftByReg) {
                    rmValue += thumb ? 6 : 12;
                } else {
                    rmValue += thumb ? 4 : 8;
                }
            }

            shifterOperand = shifts::shift(rmValue, shiftType, shiftAmount, carry, !shiftByReg);
        }

        bool shifterOperandCarry = shifterOperand & (static_cast<uint64_t>(1) << 32);
        shifterOperand &= 0xFFFFFFFF;

        uint64_t rnValue = *currentRegs[rn];
        if (rn == regs::PC_OFFSET) {
            // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
            // PC+12 if I=0,R=1 (shift by register),
            // otherwise PC+8 (shift by immediate).
            if (!i && shiftByReg) {
                rnValue += thumb ? 6 : 12;
            } else {
                rnValue += thumb ? 4 : 8;
            }
        }

        uint64_t resultValue;

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
                    resultValue = *currentRegs[regs::CPSR_OFFSET];
                break;
            }
            case MSR_SPSR:
            case MSR_CPSR: {
                // true iff write to flag field is allowed 31-24
                bool f = rn & 0x08;
                // true iff write to status field is allowed 23-16
                bool s = rn & 0x04;
                // true iff write to extension field is allowed 15-8
                bool x = rn & 0x02;
                // true iff write to control field is allowed 7-0
                bool c = rn & 0x01;

                uint32_t bitMask = (f ? 0xFF000000 : 0) | (s ? 0x00FF0000 : 0) | (x ? 0x0000FF00 : 0) | (c ? 0x000000FF : 0);

                constexpr bool r = id == MSR_SPSR;

                // Shady trick to fix destination register because extracted rd value is not used
                if (r) {
                    rd = regs::SPSR_OFFSET;
                    // clear fields that should be written to
                    resultValue = *currentRegs[regs::SPSR_OFFSET] & ~bitMask;
                } else {
                    rd = regs::CPSR_OFFSET;
                    // clear fields that should be written to
                    resultValue = *currentRegs[regs::CPSR_OFFSET] & ~bitMask;
                }

                // ensure that only fields that should be written to are changed!
                resultValue |= (shifterOperand & bitMask);

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
                std::cout << "ERROR: execDataProc can not handle instruction: " << instructionIDToString(id) << std::endl;
                break;
        }

        bool destPC = rd == regs::PC_OFFSET;

        // Special case 9000
        if (movSPSR && s && destPC) {
            state.accessReg(regs::CPSR_OFFSET) = *currentRegs[regs::SPSR_OFFSET];
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
                (shiftType != shifts::ShiftType::LSL || shiftAmount != 0)) {
                state.setFlag<cpsr_flags::C_FLAG>(shifterOperandCarry);
            }
        }

        if (!dontUpdateRD)
            state.accessReg(rd) = static_cast<uint32_t>(resultValue);

        if (destPC) {
            cpuInfo.additionalProgCyclesN = 1;
            cpuInfo.additionalProgCyclesS = 1;
        }
        if (shiftByReg) {
            cpuInfo.cycleCount += 1;
        }
    }

    template <InstructionID id, bool thumb>
    void CPU::execDataBlockTransfer(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList)
    {
        auto currentRegs = state.getCurrentRegs();

        //TODO NOTE: this instruction is reused for handleThumbMultLoadStore & handleThumbPushPopRegister

        constexpr bool load = id == LDM || id == LDMIA;

        if (forceUserRegisters && (!load || (rList & (1 << regs::PC_OFFSET)) == 0)) {
            currentRegs = state.getModeRegs(CPUState::UserMode);
        }

        uint32_t address = *currentRegs[rn];

        // Execution Time:
        // For normal LDM, nS+1N+1I. For LDM PC, (n+1)S+2N+1I.
        // For STM (n-1)S+2N. Where n is the number of words transferred.

        if (load) {
            // handle +1I
            cpuInfo.cycleCount = 1;
        } else {
            // same edge case as for STR
            cpuInfo.noDefaultSCycle = true;
            cpuInfo.additionalProgCyclesN = 1;
        }

        // The first read / write is non sequential but afterwards all accesses are sequential
        // because the memory class always adds non sequential accesses we need to handle this case explicitly
        bool nonSeqAccDone = false;

        //TODO is it fine to just walk in different direction?
        /*
        Transfer Order
        The lowest Register in Rlist (R0 if its in the list) will be loaded/stored to/from the lowest memory address.
        Internally, the rlist register are always processed with INCREASING addresses 
        (ie. for DECREASING addressing modes, the CPU does first calculate the lowest address,
        and does then process rlist with increasing addresses; this detail can be important when accessing memory mapped I/O ports).
        */
        uint32_t addrInc = up ? 4 : -static_cast<uint32_t>(4);

        bool edgeCaseEmptyRlist = false;
        uint32_t edgeCaseEmptyRlistAddrInc = 0;
        // Handle edge case: Empty Rlist: R15 loaded/stored (ARMv4 only)
        // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5).
        /*
        empty rlist edge cases:
        STMIA: str -> r0
            r0 = r0 + 0x40
        STMIB: str->r0 +4
            r0 = r0 + 0x40
        STMDB: str ->r0 - 0x40
            r0 = r0 - 0x40
        STMDA: str -> r0 -0x3C
            r0 = r0 - 0x40
        */
        if (rList == 0) {
            edgeCaseEmptyRlist = true;
            rList = (1 << regs::PC_OFFSET);
            writeback = true;
            if (up) {
                edgeCaseEmptyRlistAddrInc = static_cast<uint32_t>(0x3C);
            } else {
                addrInc = -(pre ? static_cast<uint32_t>(0x40) : static_cast<uint32_t>(0x3C));
                edgeCaseEmptyRlistAddrInc = pre ? 0 : -static_cast<uint32_t>(4);
                pre = true;
            }
        }

        uint32_t patchMemAddr;

        for (uint32_t i = 0; i < 16; ++i) {
            uint8_t currentIdx = (up ? i : 15 - i);
            if (rList & (1 << currentIdx)) {

                if (pre) {
                    address += addrInc;
                }

                if (load) {
                    if (currentIdx == regs::PC_OFFSET) {
                        *currentRegs[regs::PC_OFFSET] = state.memory.read32(address, &cpuInfo, nonSeqAccDone, this->executionMemReg == Memory::MemoryRegion::BIOS);
                        // Special case for pipeline refill
                        cpuInfo.additionalProgCyclesN = 1;
                        cpuInfo.additionalProgCyclesS = 1;

                        // More special cases
                        /*
                            When S Bit is set (S=1)
                            If instruction is LDM and R15 is in the list: (Mode Changes)
                              While R15 loaded, additionally: CPSR=SPSR_<current mode>
                            */
                        if (forceUserRegisters) {
                            *currentRegs[regs::CPSR_OFFSET] = *state.getCurrentRegs()[regs::SPSR_OFFSET];
                        }
                    } else {
                        *currentRegs[currentIdx] = state.memory.read32(address, &cpuInfo, nonSeqAccDone, this->executionMemReg == Memory::MemoryRegion::BIOS);
                    }
                } else {
                    // Shady hack to make edge case treatment easier
                    if (rn == currentIdx) {
                        patchMemAddr = address;
                    }

                    // Edge case of storing PC -> PC + 12 will be stored
                    state.memory.write32(address, *currentRegs[currentIdx] + (currentIdx == regs::PC_OFFSET ? (thumb ? 6 : 12) : 0), &cpuInfo, nonSeqAccDone);
                }

                nonSeqAccDone = true;

                if (!pre) {
                    address += addrInc;
                }
            }
        }

        // Final edge case address patch
        address += edgeCaseEmptyRlistAddrInc;

        if (!edgeCaseEmptyRlist && (rList & (1 << rn)) && writeback) {
            // Edge case: writeback enabled & rn is inside rlist
            // If load then it was overwritten anyway & we should not do another writeback as writeback comes before read!
            // else on STM it depends if this register was the first written to memory
            // if so we need to write the unchanged memory address into memory (which is what we currently do by default)
            // else we need to patch the written memory & store the address that would normally written back
            if (!load) {
                // Check if there are any registers that were written first to memory
                if (rList & ((1 << rn) - 1)) {
                    // We need to patch mem
                    state.memory.write32(patchMemAddr, address, nullptr);
                }

                // Also do the write back to the register!
                *currentRegs[rn] = address;
            }
        } else if (writeback) {
            *currentRegs[rn] = address;
        }
    }

    template <InstructionID id, bool thumb>
    void CPU::execLoadStoreRegUByte(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode)
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

        constexpr bool load = id == LDR || id == LDRB;
        constexpr bool byte = id == LDRB || id == STRB;

        bool immediate = !i;

        auto currentRegs = state.getCurrentRegs();

        if (!pre) {
            bool forceNonPrivAccess = writeback;
            // I assume that we access user regs... because user mode is the only non-priviledged mode
            if (forceNonPrivAccess) {
                currentRegs = state.getModeRegs(CPUState::UserMode);
                std::cout << "WARNING: force non priviledeg access!" << std::endl;
            }
        }

        /* these are computed in the next step */
        uint32_t memoryAddress;
        uint32_t offset;

        // Execution Time: For normal LDR: 1S+1N+1I. For LDR PC: 2S+2N+1I. For STR: 2N.

        if (load) {
            // 1 I for beeing complex
            cpuInfo.cycleCount = 1;

            // additional delays needed if PC gets loaded
            if (rd == regs::PC_OFFSET) {
                cpuInfo.additionalProgCyclesN = 1;
                cpuInfo.additionalProgCyclesS = 1;
            }
        } else {
            cpuInfo.additionalProgCyclesN = 1;
            cpuInfo.noDefaultSCycle = true;
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

        if (rn == regs::PC_OFFSET) {
            rnValue += thumb ? 4 : 8;

            if (thumb) {
                // special case for thumb PC_LD
                //TODO check that this wont affect other thumb instructions... probably not because normally they can only address r0-r7
                rnValue &= ~2;
            }
        }

        if (rd == regs::PC_OFFSET) {
            rdValue += thumb ? 6 : 12;
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
                *currentRegs[rd] = state.memory.read8(memoryAddress, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS);
            } else {
                // More edge case:
                /*
                When reading a word from a halfword-aligned address (which is located in the middle between two word-aligned addresses),
                the lower 16bit of Rd will contain [address] ie. the addressed halfword, 
                and the upper 16bit of Rd will contain [address-2] ie. more or less unwanted garbage. 
                However, by isolating lower bits this may be used to read a halfword from memory. 
                (Above applies to little endian mode, as used in GBA.)
                */
                uint32_t alignedWord = state.memory.read32(memoryAddress, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS);
                alignedWord = shifts::shift(alignedWord, shifts::ShiftType::ROR, (memoryAddress & 0x03) * 8, false, false) & 0xFFFFFFFF;
                *currentRegs[rd] = alignedWord;
            }
        } else {
            if (byte) {
                state.memory.write8(memoryAddress, rdValue, &cpuInfo);
            } else {
                // Mask out unaligned bits!
                state.memory.write32(memoryAddress, rdValue, &cpuInfo);
            }
        }

        if (!pre)
            memoryAddress += offset;

        if ((!pre || writeback) && (!load || rn != rd))
            *currentRegs[rn] = memoryAddress;
    }

    template <InstructionID id, bool thumb>
    void CPU::execHalfwordDataTransferImmRegSignedTransfer(bool pre, bool up, bool writeback,
                                                           uint8_t rn, uint8_t rd, uint32_t offset)
    {
        /*
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

        constexpr uint8_t transferSize_ = (id == LDRH || id == STRH || id == LDRSH) ? 16 : 8;
        constexpr bool load = id == LDRH || id == LDRSB || id == LDRSH;
        constexpr bool sign = id == LDRSB || id == LDRSH;

        uint8_t transferSize = transferSize_;

        //Execution Time: For Normal LDR, 1S+1N+1I. For LDR PC, 2S+2N+1I. For STRH 2N

        // both instructions have a + 1I for being complex
        if (load) {
            // 1N is handled by Memory class & 1S is handled globally
            cpuInfo.cycleCount = 1;
            // will PC be updated? if so we need an additional Prog N & S cycle
            if (load && rd == regs::PC_OFFSET) {
                cpuInfo.additionalProgCyclesN = 1;
                cpuInfo.additionalProgCyclesS = 1;
            }
        } else {
            // same edge case as for STR
            cpuInfo.noDefaultSCycle = true;
            cpuInfo.additionalProgCyclesN = 1;
        }

        uint32_t rnValue = state.accessReg(rn);
        uint32_t rdValue = state.accessReg(rd);

        if (rn == regs::PC_OFFSET) {
            rnValue += thumb ? 4 : 8;
        }

        if (rd == regs::PC_OFFSET) {
            rdValue += thumb ? 6 : 12;
        }

        offset = up ? offset : -offset;

        uint32_t memoryAddress;
        if (pre) {
            memoryAddress = rnValue + offset;
        } else
            memoryAddress = rnValue;

        if (load) {
            uint32_t readData;
            if (transferSize == 16) {
                // Handle misaligned address
                if (sign && memoryAddress & 1) {
                    // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]         ;sign-expand BYTE value
                    readData = state.memory.read8(memoryAddress, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS);
                    transferSize = 8;
                } else {
                    // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
                    // LDRH with ROR (see LDR with non word aligned)
                    readData = static_cast<uint32_t>(state.memory.read16(memoryAddress, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS));
                    readData = shifts::shift(readData, shifts::ShiftType::ROR, (memoryAddress & 0x01) * 8, false, false) & 0xFFFFFFFF;
                }
            } else {
                readData = static_cast<uint32_t>(state.memory.read8(memoryAddress, &cpuInfo, false, this->executionMemReg == Memory::MemoryRegion::BIOS));
            }

            if (sign) {
                // Do the sign extension by moving MSB to bit 31 and then reinterpret as signed and divide by power of 2 for a sign extended shift back
                readData = signExt<int32_t, uint32_t>(readData, transferSize);
            }

            state.accessReg(rd) = readData;
        } else {
            if (transferSize == 16) {
                state.memory.write16(memoryAddress, rdValue, &cpuInfo);
            } else {
                state.memory.write8(memoryAddress, rdValue, &cpuInfo);
            }
        }

        if ((!pre || writeback) && (!load || rn != rd)) {
            if (!pre) {
                memoryAddress += offset;
            }

            state.accessReg(rn) = memoryAddress;
        }
    }

    template void CPU::handleMultAcc<MLA>(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm);
    template void CPU::handleMultAcc<MUL>(bool s, uint8_t rd, uint8_t rn, uint8_t rs, uint8_t rm);

    template void CPU::handleDataSwp<SWP>(uint8_t rn, uint8_t rd, uint8_t rm);
    template void CPU::handleDataSwp<SWPB>(uint8_t rn, uint8_t rd, uint8_t rm);

    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRH, false>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<STRH, false>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRSB, false>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRSH, false>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);

    template void CPU::execDataProc<AND, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<EOR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<SUB, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<RSB, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ADD, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ADC, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<SBC, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<RSC, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<TST, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MRS_CPSR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MRS_SPSR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<TEQ, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MSR_CPSR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MSR_SPSR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<CMP, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<CMN, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ORR, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MOV, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<BIC, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MVN, false>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);

    template void CPU::execLoadStoreRegUByte<LDR, false>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<LDRB, false>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<STR, false>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<STRB, false>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);

    template void CPU::execDataBlockTransfer<LDM>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList);
    template void CPU::execDataBlockTransfer<STM>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList);

    template void CPU::handleMultAccLong<SMULL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);
    template void CPU::handleMultAccLong<SMLAL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);
    template void CPU::handleMultAccLong<UMLAL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);
    template void CPU::handleMultAccLong<UMULL>(bool s, uint8_t rd_msw, uint8_t rd_lsw, uint8_t rs, uint8_t rm);

    template void CPU::execDataProc<SUB, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ADD_SHORT_IMM, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<SUB_SHORT_IMM, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execLoadStoreRegUByte<STR, true>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<STRB, true>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<LDR, true>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execLoadStoreRegUByte<LDRB, true>(bool pre, bool up, bool i, bool writeback, uint8_t rn, uint8_t rd, uint16_t addrMode);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<STRH, true>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRSB, true>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRH, true>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);
    template void CPU::execHalfwordDataTransferImmRegSignedTransfer<LDRSH, true>(bool pre, bool up, bool writeback, uint8_t rn, uint8_t rd, uint32_t offset);

    template void CPU::execDataBlockTransfer<LDM, true>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList);
    template void CPU::execDataBlockTransfer<STM, true>(bool pre, bool up, bool writeback, bool forceUserRegisters, uint8_t rn, uint16_t rList);

    template void CPU::execDataProc<ADD, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<AND, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<EOR, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MOV, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ADC, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<SBC, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<TST, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<NEG, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<CMP, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<CMN, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<ORR, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MUL, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<BIC, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);
    template void CPU::execDataProc<MVN, true>(bool i, bool s, uint8_t rn, uint8_t rd, uint16_t operand2);

} // namespace gbaemu
