#include "cpu.hpp"
#include <set>

namespace gbaemu
{
    InstructionExecutionInfo CPU::handleMultAcc(bool a, bool s, uint32_t rd, uint32_t rn, uint32_t rs, uint32_t rm)
    {
        // Check given restrictions
        if (rd == rm) {
            std::cout << "ERROR: MUL/MLA destination register may not be the same as the first operand!" << std::endl;
        }
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
            setFlags(
                mulRes,
                true,
                true,
                false,
                false);
        }

        /*
            Execution Time: 1S+mI for MUL, and 1S+(m+1)I for MLA.
            Whereas 'm' depends on whether/how many most significant bits of Rs are all zero or all one.
            That is m=1 for Bit 31-8, m=2 for Bit 31-16, m=3 for Bit 31-24, and m=4 otherwise.
            */
        InstructionExecutionInfo info{0};
        // bool a decides if it is a MLAL instruction or MULL
        info.cycleCount = (a ? 1 : 0);

        if (((rsVal >> 8) & 0x00FFFFFF) == 0 || ((rsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF) {
            info.cycleCount += 1;
        } else if (((rsVal >> 16) & 0x0000FFFF) == 0 || ((rsVal >> 16) & 0x0000FFFF) == 0x0000FFFF) {
            info.cycleCount += 2;
        } else if (((rsVal >> 24) & 0x000000FF) == 0 || ((rsVal >> 24) & 0x000000FF) == 0x000000FF) {
            info.cycleCount += 3;
        } else {
            info.cycleCount += 4;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleMultAccLong(bool signMul, bool a, bool s, uint32_t rd_msw, uint32_t rd_lsw, uint32_t rs, uint32_t rm)
    {
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

            state.setFlag(cpsr_flags::N_FLAG, negative);

            state.setFlag(cpsr_flags::Z_FLAG, zero);
        }

        /*
            Execution Time: 1S+(m+1)I for MULL, and 1S+(m+2)I for MLAL.
            Whereas 'm' depends on whether/how many most significant bits of Rs are "all zero" (UMULL/UMLAL)
            or "all zero or all one" (SMULL,SMLAL).
            That is m=1 for Bit31-8, m=2 for Bit31-16, m=3 for Bit31-24, and m=4 otherwise.
            */
        InstructionExecutionInfo info{0};
        // bool a decides if it is a MLAL instruction or MULL
        info.cycleCount = (a ? 2 : 1);

        if (((unExtRsVal >> 8) & 0x00FFFFFF) == 0 || (signMul && ((unExtRsVal >> 8) & 0x00FFFFFF) == 0x00FFFFFF)) {
            info.cycleCount += 1;
        } else if (((unExtRsVal >> 16) & 0x0000FFFF) == 0 || (signMul && ((unExtRsVal >> 16) & 0x0000FFFF) == 0x0000FFFF)) {
            info.cycleCount += 2;
        } else if (((unExtRsVal >> 24) & 0x000000FF) == 0 || (signMul && ((unExtRsVal >> 24) & 0x000000FF) == 0x000000FF)) {
            info.cycleCount += 3;
        } else {
            info.cycleCount += 4;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleDataSwp(bool b, uint32_t rn, uint32_t rd, uint32_t rm)
    {
        //TODO maybe replace by LDR followed by STR?

        if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
            std::cout << "ERROR: SWP/SWPB PC register may not be involved in calculations!" << std::endl;
        }

        auto currentRegs = state.getCurrentRegs();
        uint32_t newMemVal = *currentRegs[rm];
        uint32_t memAddr = *currentRegs[rn];

        // Execution Time: 1S+2N+1I. That is, 2N data cycles (added through Memory class), 1S code cycle, plus 1I(initial value)
        InstructionExecutionInfo info{0};
        info.cycleCount = 1;

        if (b) {
            uint8_t memVal = state.memory.read8(memAddr, &info.cycleCount);
            state.memory.write8(memAddr, static_cast<uint8_t>(newMemVal & 0x0FF), &info.cycleCount);
            //TODO overwrite upper 24 bits?
            *currentRegs[rd] = static_cast<uint32_t>(memVal);
        } else {
            uint32_t memVal = state.memory.read32(memAddr, &info.cycleCount);
            state.memory.write32(memAddr, newMemVal, &info.cycleCount);
            *currentRegs[rd] = memVal;
        }

        return info;
    }

    // Executes instructions belonging to the branch subsection
    InstructionExecutionInfo CPU::handleBranch(bool link, int32_t offset)
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
        InstructionExecutionInfo info{0};
        info.additionalProgCyclesN = 1;
        info.additionalProgCyclesS = 1;

        return info;
    }

    // Executes instructions belonging to the branch and execute subsection
    InstructionExecutionInfo CPU::handleBranchAndExchange(uint32_t rn)
    {
        auto currentRegs = state.getCurrentRegs();

        // Load the content of register given by rm
        uint32_t rnValue = *currentRegs[rn];
        // If the first bit of rn is set
        bool changeToThumb = rnValue & 0x00000001;

        if (changeToThumb) {
            state.setFlag(cpsr_flags::THUMB_STATE, true);
        }

        // Change the PC to the address given by rm. Note that we have to mask out the thumb switch bit.
        state.accessReg(regs::PC_OFFSET) = rnValue & 0xFFFFFFFE;

        // Execution Time: 2S + 1N
        InstructionExecutionInfo info{0};
        info.additionalProgCyclesN = 1;
        info.additionalProgCyclesS = 1;

        return info;
    }

    /* ALU functions */
    InstructionExecutionInfo CPU::execDataProc(arm::ARMInstruction &inst)
    {

        bool carry = state.getFlag(cpsr_flags::C_FLAG);

        /* calculate shifter operand */
        arm::ShiftType shiftType;
        uint8_t shiftAmount;
        uint32_t rm;
        uint32_t rs;
        uint32_t imm;
        uint64_t shifterOperand;
        bool shiftByReg = inst.params.data_proc_psr_transf.extractOperand2(shiftType, shiftAmount, rm, rs, imm);

        if (inst.params.data_proc_psr_transf.i) {
            shifterOperand = arm::shift(imm, arm::ShiftType::ROR, shiftAmount, carry, false);
        } else {
            if (shiftByReg)
                shiftAmount = *state.getCurrentRegs()[rs];

            uint32_t rmValue = *state.getCurrentRegs()[rm];

            if (rm == regs::PC_OFFSET) {
                // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
                // PC+12 if I=0,R=1 (shift by register),
                // otherwise PC+8 (shift by immediate).
                std::cout << "INFO: Edge case triggered, by using PC as RM operand on ALU operation! Pls verify that this is correct behaviour for this instruction!" << std::endl;
                if (!inst.params.data_proc_psr_transf.i && shiftByReg) {
                    rmValue += 12;
                } else {
                    rmValue += 8;
                }
            }

            shifterOperand = arm::shift(rmValue, shiftType, shiftAmount, carry, !shiftByReg);
        }

        bool shifterOperandCarry = shifterOperand & (static_cast<uint64_t>(1) << 32);
        shifterOperand &= 0xFFFFFFFF;

        uint64_t rnValue = state.accessReg(inst.params.data_proc_psr_transf.rn);
        if (inst.params.data_proc_psr_transf.rn == regs::PC_OFFSET) {
            // When using R15 as operand (Rm or Rn), the returned value depends on the instruction:
            // PC+12 if I=0,R=1 (shift by register),
            // otherwise PC+8 (shift by immediate).
            std::cout << "INFO: Edge case triggered, by using PC as RN operand on ALU operation! Pls verify that this is correct behaviour for this instruction!" << std::endl;
            if (!inst.params.data_proc_psr_transf.i && shiftByReg) {
                rnValue += 12;
            } else {
                rnValue += 8;
            }
        }

        uint64_t resultValue;

        /* Different instructions cause different flags to be changed. */
        /* TODO: This can be extended for all instructions. */
        static const std::set<arm::ARMInstructionID> updateNegative{
            arm::ADC, arm::ADD, arm::AND, arm::BIC, arm::CMN,
            arm::CMP, arm::EOR, arm::MOV, arm::MVN, arm::ORR,
            arm::RSB, arm::RSC, arm::SBC, arm::SUB, arm::TEQ,
            arm::TST};

        static const std::set<arm::ARMInstructionID> updateZero{
            arm::ADC, arm::ADD, arm::AND, arm::BIC, arm::CMN,
            arm::CMP, arm::EOR, arm::MOV, arm::MVN, arm::ORR,
            arm::RSB, arm::RSC, arm::SBC, arm::SUB, arm::TEQ,
            arm::TST};

        static const std::set<arm::ARMInstructionID> updateOverflow{
            arm::ADC, arm::ADD, arm::CMN, arm::CMP, arm::MOV,
            arm::RSB, arm::RSC, arm::SBC, arm::SUB};

        static const std::set<arm::ARMInstructionID> updateCarry{
            arm::ADC, arm::ADD, arm::CMN, arm::CMP, arm::RSB,
            arm::RSC, arm::SBC, arm::SUB};

        static const std::set<arm::ARMInstructionID> updateCarryFromShiftOp{
            arm::AND, arm::EOR, arm::MOV, arm::MVN, arm::ORR,
            arm::BIC, arm::TEQ, arm::TST};

        static const std::set<arm::ARMInstructionID> dontUpdateRD{
            arm::CMP, arm::CMN, arm::TST, arm::TEQ};

        /* execute functions */
        switch (inst.id) {
            case arm::ADC:
                resultValue = rnValue + shifterOperand + (carry ? 1 : 0);
                break;
            case arm::ADD:
                resultValue = rnValue + shifterOperand;
                break;
            case arm::AND:
                resultValue = rnValue & shifterOperand;
                break;
            case arm::BIC:
                resultValue = rnValue & (~shifterOperand);
                break;
            case arm::CMN:
                resultValue = rnValue + shifterOperand;
                break;
            case arm::CMP:
                resultValue = rnValue - shifterOperand;
                break;
            case arm::EOR:
                resultValue = rnValue ^ shifterOperand;
                break;
            case arm::MOV:
                resultValue = shifterOperand;
                if (inst.params.data_proc_psr_transf.s && inst.params.data_proc_psr_transf.rd == 15)
                    state.accessReg(regs::CPSR_OFFSET) = state.accessReg(regs::SPSR_OFFSET);
                break;
            case arm::MRS:
                if (inst.params.data_proc_psr_transf.r)
                    resultValue = state.accessReg(regs::SPSR_OFFSET);
                else
                    resultValue = state.accessReg(regs::CPSR_OFFSET);
                break;
            case arm::MSR: {
                // true iff write to flag field is allowed 31-24
                bool f = inst.params.data_proc_psr_transf.rn & 0x08;
                // true iff write to status field is allowed 23-16
                bool s = inst.params.data_proc_psr_transf.rn & 0x04;
                // true iff write to extension field is allowed 15-8
                bool x = inst.params.data_proc_psr_transf.rn & 0x02;
                // true iff write to control field is allowed 7-0
                bool c = inst.params.data_proc_psr_transf.rn & 0x01;

                uint32_t bitMask = (f ? 0xFF000000 : 0) | (s ? 0x00FF0000 : 0) | (x ? 0x0000FF00 : 0) | (c ? 0x000000FF : 0);

                // Shady trick to fix destination register because extracted rd value is not used
                if (inst.params.data_proc_psr_transf.r) {
                    inst.params.data_proc_psr_transf.rd = regs::SPSR_OFFSET;
                    // clear fields that should be written to
                    resultValue = state.accessReg(regs::SPSR_OFFSET) & ~bitMask;
                } else {
                    inst.params.data_proc_psr_transf.rd = regs::CPSR_OFFSET;
                    // clear fields that should be written to
                    resultValue = state.accessReg(regs::CPSR_OFFSET) & ~bitMask;
                }

                // ensure that only fields that should be written to are changed!
                resultValue |= (shifterOperand & bitMask);

                break;
            }
            case arm::MVN:
                resultValue = ~shifterOperand;
                if (inst.params.data_proc_psr_transf.s && inst.params.data_proc_psr_transf.rd == 15)
                    state.accessReg(regs::CPSR_OFFSET) = state.accessReg(regs::SPSR_OFFSET);
                break;
            case arm::ORR:
                resultValue = rnValue | shifterOperand;
                break;
                /* TODO: subtraction is oh no */
            case arm::RSB:
                resultValue = shifterOperand - static_cast<uint32_t>(rnValue);
                break;
            case arm::RSC:
                resultValue = shifterOperand - static_cast<uint32_t>(rnValue) - (carry ? 0 : 1);
                break;
            case arm::SBC:
                resultValue = static_cast<uint32_t>(rnValue) - shifterOperand - (carry ? 0 : 1);
                break;
            case arm::SUB:
                resultValue = static_cast<uint32_t>(rnValue) - shifterOperand;
                break;
            case arm::TEQ:
                resultValue = rnValue ^ shifterOperand;
                break;
            case arm::TST:
                resultValue = rnValue & shifterOperand;
                break;
            default:
                break;
        }

        /* set flags */
        if (inst.params.data_proc_psr_transf.s) {
            setFlags(
                resultValue,
                updateNegative.find(inst.id) != updateNegative.end(),
                updateZero.find(inst.id) != updateZero.end(),
                updateOverflow.find(inst.id) != updateOverflow.end(),
                updateCarry.find(inst.id) != updateCarry.end());

            if (updateCarryFromShiftOp.find(inst.id) != updateCarryFromShiftOp.end() &&
                (shiftType != arm::ShiftType::LSL || shiftAmount != 0)) {
                state.setFlag(cpsr_flags::C_FLAG, shifterOperandCarry);
            }
        }

        if (dontUpdateRD.find(inst.id) == dontUpdateRD.end())
            state.accessReg(inst.params.data_proc_psr_transf.rd) = resultValue;

        /* TODO: cycle timings */
        InstructionExecutionInfo info{0};
        bool destPC = inst.params.data_proc_psr_transf.rd == regs::PC_OFFSET;

        if (destPC) {
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;
        }
        if (shiftByReg) {
            info.cycleCount += 1;
        }

        return info;
    }

    InstructionExecutionInfo CPU::execLoadStoreRegUByte(const arm::ARMInstruction &inst)
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

        /* use variable names with semantics */
        bool pre = inst.params.ls_reg_ubyte.p;
        /* add or substract offset? */
        bool up = inst.params.ls_reg_ubyte.u;
        bool load = inst.params.ls_reg_ubyte.l;
        bool immediate = !inst.params.ls_reg_ubyte.i;
        bool byte = inst.params.ls_reg_ubyte.b;
        bool writeback = inst.params.ls_reg_ubyte.w;

        auto currentRegs = state.getCurrentRegs();

        if (!pre) {
            /* TODO: What's this? */
            bool forceNonPrivAccess = writeback;
            // I assume that we access user regs... because user mode is the only non-priviledged mode
            if (forceNonPrivAccess) {
                currentRegs = state.getModeRegs(CPUState::UserMode);
                std::cout << "WARNING: force non priviledeg access!" << std::endl;
            }
        }

        uint32_t rn = inst.params.ls_reg_ubyte.rn;
        uint32_t rd = inst.params.ls_reg_ubyte.rd;
        /* these are computed in the next step */
        uint32_t memoryAddress;
        uint32_t offset;

        // Execution Time: For normal LDR: 1S+1N+1I. For LDR PC: 2S+2N+1I. For STR: 2N.
        InstructionExecutionInfo info{0};
        if (load) {
            // 1 I for beeing complex
            info.cycleCount = 1;

            // additional delays needed if PC gets loaded
            if (rd == regs::PC_OFFSET) {
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;
            }
        } else {
            //TODO not sure why STR instructions have 2N ...
            info.additionalProgCyclesN = 1;
            info.noDefaultSCycle = true;
        }

        /* offset is calculated differently, depending on the I-bit */
        if (immediate) {
            offset = inst.params.ls_reg_ubyte.addrMode;
        } else {
            uint32_t shiftAmount = (inst.params.ls_reg_ubyte.addrMode >> 7) & 0x1F;
            auto shiftType = static_cast<arm::ShiftType>((inst.params.ls_reg_ubyte.addrMode >> 5) & 0b11);
            uint32_t rm = inst.params.ls_reg_ubyte.addrMode & 0xF;

            offset = arm::shift(*currentRegs[rm], shiftType, shiftAmount, state.getFlag(cpsr_flags::C_FLAG), true) & 0xFFFFFFFF;
        }

        uint32_t rnValue = *currentRegs[rn];
        uint32_t rdValue = *currentRegs[rd];

        if (rn == regs::PC_OFFSET)
            rnValue += 8;

        if (rd == regs::PC_OFFSET)
            rdValue += 12;

        /* if the offset is added depends on the indexing mode */
        if (pre)
            memoryAddress = up ? rnValue + offset : rnValue - offset;
        else
            memoryAddress = rnValue;

        /* transfer */
        if (load) {
            if (byte) {
                *currentRegs[rd] = state.memory.read8(memoryAddress, &info.cycleCount);
            } else {
                // More edge case:
                /*
                    When reading a word from a halfword-aligned address (which is located in the middle between two word-aligned addresses),
                    the lower 16bit of Rd will contain [address] ie. the addressed halfword, 
                    and the upper 16bit of Rd will contain [address-2] ie. more or less unwanted garbage. 
                    However, by isolating lower bits this may be used to read a halfword from memory. 
                    (Above applies to little endian mode, as used in GBA.)
                    */
                if (memoryAddress & 0x02) {
                    std::cout << "LDR WARNING: word read on non word aligned address!" << std::endl;

                    // Not word aligned address
                    uint16_t lowerBits = state.memory.read16(memoryAddress, nullptr);
                    uint32_t upperBits = state.memory.read16(memoryAddress - 2, nullptr);
                    *currentRegs[rd] = lowerBits | (upperBits << 16);

                    // simulate normal read for latency as if read word aligned address
                    state.memory.read32(memoryAddress - 2, &info.cycleCount);
                } else {
                    *currentRegs[rd] = state.memory.read32(memoryAddress, &info.cycleCount);
                }
            }
        } else {
            if (byte) {
                state.memory.write8(memoryAddress, rdValue, &info.cycleCount);
            } else {
                state.memory.write32(memoryAddress, rdValue, &info.cycleCount);
            }
        }

        if (!pre)
            memoryAddress += up ? offset : -offset;

        if (!pre || writeback)
            *currentRegs[rn] = memoryAddress;

        return info;
    }

    InstructionExecutionInfo CPU::execDataBlockTransfer(arm::ARMInstruction &inst)
    {
        auto currentRegs = state.getCurrentRegs();

        //TODO S bit seems relevant for register selection, NOTE: this instruction is reused for handleThumbMultLoadStore & handleThumbPushPopRegister
        bool forceUserRegisters = inst.params.block_data_transf.s;

        bool pre = inst.params.block_data_transf.p;
        bool up = inst.params.block_data_transf.u;
        bool writeback = inst.params.block_data_transf.w;
        bool load = inst.params.block_data_transf.l;

        if (forceUserRegisters && (!load || (inst.params.block_data_transf.rList & (1 << regs::PC_OFFSET)) == 0)) {
            currentRegs = state.getModeRegs(CPUState::UserMode);
            std::cout << "WARNING: force user registers!" << std::endl;
        }

        uint32_t rn = inst.params.block_data_transf.rn;
        uint32_t address = *currentRegs[rn];

        // Execution Time:
        // For normal LDM, nS+1N+1I. For LDM PC, (n+1)S+2N+1I.
        // For STM (n-1)S+2N. Where n is the number of words transferred.
        InstructionExecutionInfo info{0};
        if (load) {
            // handle +1I
            info.cycleCount = 1;
        } else {
            // same edge case as for STR
            info.noDefaultSCycle = true;
            info.additionalProgCyclesN = 1;
        }

        // The first read / write is non sequential but afterwards all accesses are sequential
        // because the memory class always adds non sequential accesses we need to handle this case explicitly
        bool nonSeqAccDone = false;

        bool edgeCaseEmptyRlist = false;
        // Handle edge case: Empty Rlist: R15 loaded/stored (ARMv4 only)
        if (inst.params.block_data_transf.rList == 0) {
            inst.params.block_data_transf.rList = (1 << 15);
            edgeCaseEmptyRlist = true;
        }

        //TODO is it fine to just walk in different direction?
        /*
            Transfer Order
            The lowest Register in Rlist (R0 if its in the list) will be loaded/stored to/from the lowest memory address.
            Internally, the rlist register are always processed with INCREASING addresses 
            (ie. for DECREASING addressing modes, the CPU does first calculate the lowest address,
            and does then process rlist with increasing addresses; this detail can be important when accessing memory mapped I/O ports).
            */
        uint32_t addrInc = up ? 4 : static_cast<uint32_t>(static_cast<int32_t>(-4));

        uint32_t patchMemAddr;

        for (uint32_t i = 0; i < 16; ++i) {
            uint8_t currentIdx = (up ? i : 15 - i);
            if (inst.params.block_data_transf.rList & (1 << currentIdx)) {

                if (pre)
                    address += addrInc;

                if (load) {
                    if (currentIdx == regs::PC_OFFSET) {
                        *currentRegs[regs::PC_OFFSET] = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount);
                        // Special case for pipeline refill
                        info.additionalProgCyclesN = 1;
                        info.additionalProgCyclesS = 1;

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
                        *currentRegs[currentIdx] = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount);
                    }
                } else {
                    // Shady hack to make edge case treatment easier
                    if (rn == currentIdx) {
                        patchMemAddr = address;
                    }

                    // Edge case of storing PC -> PC + 12 will be stored
                    state.memory.write32(address, *currentRegs[currentIdx] + (currentIdx == regs::PC_OFFSET ? 12 : 0), nonSeqAccDone ? nullptr : &info.cycleCount);
                }

                if (nonSeqAccDone) {
                    info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(address, sizeof(uint32_t));
                }
                nonSeqAccDone = true;

                if (!pre)
                    address += addrInc;
            }
        }

        // Edge case: writeback enabled & rn is inside rlist
        if ((inst.params.block_data_transf.rList & (1 << rn)) && writeback) {
            // If load then it was overwritten anyway & we should not do another writeback as writeback comes before read!
            // else on STM it depends if this register was the first written to memory
            // if so we need to write the unchanged memory address into memory (which is what we currently do by default)
            // else we need to patch the written memory & store the address that would normally written back
            if (!load) {
                // Check if there are any registers that were written first to memory
                if (inst.params.block_data_transf.rList & ((1 << rn) - 1)) {
                    // We need to patch mem
                    state.memory.write32(patchMemAddr, address, nullptr);
                }

                // Also do the write back to the register!
                *currentRegs[rn] = address;
            }
        } else if (writeback) {
            *currentRegs[rn] = address;
        }

        // Handle edge case: Empty Rlist: Rb=Rb+40h (ARMv4-v5)
        if (edgeCaseEmptyRlist) {
            *currentRegs[rn] = *currentRegs[rn] + 0x40;
        }

        return info;
    }

    InstructionExecutionInfo CPU::execHalfwordDataTransferImmRegSignedTransfer(const arm::ARMInstruction &inst)
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
        bool pre, up, load, writeback, sign;
        uint32_t rn, rd, offset, memoryAddress, transferSize;

        /* this is the only difference between imm and reg */
        if (inst.cat == arm::ARMInstructionCategory::HW_TRANSF_IMM_OFF) {
            pre = inst.params.hw_transf_imm_off.p;
            up = inst.params.hw_transf_imm_off.u;
            load = inst.params.hw_transf_imm_off.l;
            writeback = inst.params.hw_transf_imm_off.w;
            rn = inst.params.hw_transf_imm_off.rn;
            rd = inst.params.hw_transf_imm_off.rd;
            offset = inst.params.hw_transf_imm_off.offset;
            transferSize = 16;
            sign = false;
        } else if (inst.cat == arm::ARMInstructionCategory::HW_TRANSF_REG_OFF) {
            pre = inst.params.hw_transf_reg_off.p;
            up = inst.params.hw_transf_reg_off.u;
            load = inst.params.hw_transf_reg_off.l;
            writeback = inst.params.hw_transf_reg_off.w;
            rn = inst.params.hw_transf_reg_off.rn;
            rd = inst.params.hw_transf_reg_off.rd;
            offset = state.accessReg(inst.params.hw_transf_reg_off.rm);
            transferSize = 16;
            sign = false;
        } else if (inst.cat == arm::ARMInstructionCategory::SIGN_TRANSF) {
            pre = inst.params.sign_transf.p;
            up = inst.params.sign_transf.u;
            load = inst.params.sign_transf.l;
            writeback = inst.params.sign_transf.w;
            rn = inst.params.sign_transf.rn;
            rd = inst.params.sign_transf.rd;

            if (inst.params.sign_transf.b) {
                uint32_t rm = inst.params.sign_transf.addrMode & 0x0F;
                offset = state.accessReg(rm);
            } else
                offset = inst.params.sign_transf.addrMode;

            if (inst.params.sign_transf.h)
                transferSize = 16;
            else
                transferSize = 8;

            sign = true;
        } else {
            std::cout << "ERROR: Invalid arm instruction category given to execHalfwordDataTransferImmRegSignedTransfer: " << inst.cat << std::endl;
        }

        //Execution Time: For Normal LDR, 1S+1N+1I. For LDR PC, 2S+2N+1I. For STRH 2N
        InstructionExecutionInfo info{0};
        // both instructions have a + 1I for being complex
        if (load) {
            // 1N is handled by Memory class & 1S is handled globally
            info.cycleCount = 1;
            // will PC be updated? if so we need an additional Prog N & S cycle
            if (load && rd == regs::PC_OFFSET) {
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;
            }
        } else {
            // same edge case as for STR
            //TODO not sure why STR instructions have 2N ...
            info.noDefaultSCycle = true;
            info.additionalProgCyclesN = 1;
        }

        uint32_t rnValue = state.accessReg(rn);
        uint32_t rdValue = state.accessReg(rd);

        if (rn == regs::PC_OFFSET)
            rnValue += 8;

        if (rd == regs::PC_OFFSET)
            rdValue += 12;

        if (pre) {
            memoryAddress = rnValue + (up ? offset : -offset);
        } else
            memoryAddress = rnValue;

        if (load) {
            uint32_t readData;
            if (transferSize == 16) {
                readData = static_cast<uint32_t>(state.memory.read16(memoryAddress, &info.cycleCount));
            } else {
                readData = static_cast<uint32_t>(state.memory.read8(memoryAddress, &info.cycleCount));
            }

            if (sign) {
                // Do the sign extension by moving MSB to bit 31 and then reinterpret as signed and divide by power of 2 for a sign extended shift back
                state.accessReg(rd) = static_cast<uint32_t>(static_cast<int32_t>(readData << transferSize) / (1 << transferSize));
            } else {
                state.accessReg(rd) = readData;
            }
        } else {
            if (transferSize == 16) {
                state.memory.write16(memoryAddress, rdValue, &info.cycleCount);
            } else {
                state.memory.write8(memoryAddress, rdValue, &info.cycleCount);
            }
        }

        if (writeback || !pre) {
            if (!pre) {
                memoryAddress += (up ? offset : -offset);
            }

            state.accessReg(rn) = memoryAddress;
        }

        return info;
    }
} // namespace gbaemu
