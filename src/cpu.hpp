#ifndef CPU_HPP
#define CPU_HPP

#include "cpu_state.hpp"
#include "inst_arm.hpp"
#include "inst_thumb.hpp"
#include "regs.hpp"
#include "swi.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <string>

namespace gbaemu
{

    /*
    instruction set
    http://www.ecs.csun.edu/~smirzaei/docs/ece425/arm7tdmi_instruction_set_reference.pdf
 */
    struct InstructionExecutionInfo {
        /*
            1. open https://static.docs.arm.com/ddi0029/g/DDI0029.pdf
            2. go to "Instruction Cycle Timings"
            3. vomit        
         */
        uint32_t cycleCount;
    };

    class CPU
    {
      public:
        CPUState state;

        void fetch()
        {

            // TODO: flush if branch happened?, else continue normally

            // propagate pipeline
            state.pipeline.fetch.lastInstruction = state.pipeline.fetch.instruction;
            state.pipeline.fetch.lastReadData = state.pipeline.fetch.readData;

            /* assume 26 ARM state here */
            /* pc is at [25:2] */
            uint32_t pc = (state.accessReg(regs::PC_OFFSET) >> 2) & 0x03FFFFFF;
            state.pipeline.fetch.instruction = state.memory.read32(pc * 4);

            //TODO where do we want to update pc? (+4)
        }

        void decode()
        {
            state.pipeline.decode.lastInstruction = state.pipeline.decode.instruction;
            state.pipeline.decode.instruction = state.decoder->decode(state.pipeline.fetch.lastInstruction);
        }

        void execute()
        {
            if (state.pipeline.decode.lastInstruction.arm.id != arm::ARMInstructionID::INVALID || state.pipeline.decode.lastInstruction.thumb.id != thumb::ThumbInstructionID::INVALID) {

                if (state.pipeline.decode.lastInstruction.isArmInstruction()) {
                    arm::ARMInstruction &armInst = state.pipeline.decode.lastInstruction.arm;

                    // Do we even need an execution?
                    if (armInst.conditionSatisfied(state)) {

                        // prefer using switch to get warned if a category is not handled
                        switch (armInst.cat) {
                            case arm::ARMInstructionCategory::MUL_ACC:
                                handleMultAcc(armInst.params.mul_acc.a,
                                              armInst.params.mul_acc.s,
                                              armInst.params.mul_acc.rd,
                                              armInst.params.mul_acc.rn,
                                              armInst.params.mul_acc.rs,
                                              armInst.params.mul_acc.rm);
                                break;
                            case arm::ARMInstructionCategory::MUL_ACC_LONG:
                                handleMultAccLong(armInst.params.mul_acc_long.u,
                                                  armInst.params.mul_acc_long.a,
                                                  armInst.params.mul_acc_long.s,
                                                  armInst.params.mul_acc_long.rd_msw,
                                                  armInst.params.mul_acc_long.rd_lsw,
                                                  armInst.params.mul_acc_long.rs,
                                                  armInst.params.mul_acc_long.rm);
                                break;
                            case arm::ARMInstructionCategory::BRANCH_XCHG:
                                handleBranchAndExchange(armInst.params.branch_xchg.rn);
                                break;
                            case arm::ARMInstructionCategory::DATA_SWP:
                                handleDataSwp(armInst.params.data_swp.b,
                                              armInst.params.data_swp.rn,
                                              armInst.params.data_swp.rd,
                                              armInst.params.data_swp.rm);
                                break;
                            case arm::ARMInstructionCategory::HW_TRANSF_REG_OFF:
                                break;
                            case arm::ARMInstructionCategory::HW_TRANSF_IMM_OFF:
                                break;
                            case arm::ARMInstructionCategory::SIGN_TRANSF:
                                break;
                            case arm::ARMInstructionCategory::DATA_PROC_PSR_TRANSF:
                                break;
                            case arm::ARMInstructionCategory::LS_REG_UBYTE:
                                execLoadStoreRegUByte(armInst);
                                break;
                            case arm::ARMInstructionCategory::BLOCK_DATA_TRANSF:
                                execDataBlockTransfer(armInst);
                                break;
                            case arm::ARMInstructionCategory::BRANCH:
                                handleBranch(armInst.params.branch.l, armInst.params.branch.offset);
                                break;

                            case arm::ARMInstructionCategory::SOFTWARE_INTERRUPT: {

                                /*
                                SWIs can be called from both within THUMB and ARM mode. In ARM mode, only the upper 8bit of the 24bit comment field are interpreted.
                                //TODO is switching needed?
                                Each time when calling a BIOS function 4 words (SPSR, R11, R12, R14) are saved on Supervisor stack (_svc). Once it has saved that data, the SWI handler switches into System mode, so that all further stack operations are using user stack.
                                In some cases the BIOS may allow interrupts to be executed from inside of the SWI procedure. If so, and if the interrupt handler calls further SWIs, then care should be taken that the Supervisor Stack does not overflow.
                                */
                                uint8_t index = armInst.params.software_interrupt.comment >> 16;
                                if (index < sizeof(swi::biosCallHandler) / sizeof(swi::biosCallHandler[0])) {
                                    swi::biosCallHandler[index](&state);
                                } else {
                                    std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << std::endl;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    thumb::ThumbInstruction &thumbInst = state.pipeline.decode.lastInstruction.thumb;
                    //TODO thumb instruction execution

                    // prefer using switch to get warned if a category is not handled
                    switch (thumbInst.cat) {
                        case thumb::ThumbInstructionCategory::MOV_SHIFT:
                            break;
                        case thumb::ThumbInstructionCategory::ADD_SUB:
                            break;
                        case thumb::ThumbInstructionCategory::MOV_CMP_ADD_SUB_IMM:
                            break;
                        case thumb::ThumbInstructionCategory::ALU_OP:
                            break;
                        case thumb::ThumbInstructionCategory::BR_XCHG:
                            break;
                        case thumb::ThumbInstructionCategory::PC_LD:
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_REL_OFF:
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_SIGN_EXT:
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_IMM_OFF:
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_HW:
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_REL_SP:
                            break;
                        case thumb::ThumbInstructionCategory::LOAD_ADDR:
                            break;
                        case thumb::ThumbInstructionCategory::ADD_OFFSET_TO_STACK_PTR:
                            break;
                        case thumb::ThumbInstructionCategory::PUSH_POP_REG:
                            break;
                        case thumb::ThumbInstructionCategory::MULT_LOAD_STORE:
                            break;
                        case thumb::ThumbInstructionCategory::COND_BRANCH:
                            break;
                        case thumb::ThumbInstructionCategory::SOFTWARE_INTERRUPT: {

                            /*
                                SWIs can be called from both within THUMB and ARM mode. In ARM mode, only the upper 8bit of the 24bit comment field are interpreted.
                                //TODO is switching needed?
                                Each time when calling a BIOS function 4 words (SPSR, R11, R12, R14) are saved on Supervisor stack (_svc). Once it has saved that data, the SWI handler switches into System mode, so that all further stack operations are using user stack.
                                In some cases the BIOS may allow interrupts to be executed from inside of the SWI procedure. If so, and if the interrupt handler calls further SWIs, then care should be taken that the Supervisor Stack does not overflow.
                                */
                            uint8_t index = thumbInst.params.software_interrupt.comment;
                            if (index < sizeof(swi::biosCallHandler) / sizeof(swi::biosCallHandler[0])) {
                                swi::biosCallHandler[index](&state);
                            } else {
                                std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << std::endl;
                            }
                            break;
                        }
                        case thumb::ThumbInstructionCategory::UNCONDITIONAL_BRANCH:
                            break;
                        case thumb::ThumbInstructionCategory::LONG_BRANCH_WITH_LINK:
                            break;
                    }
                }
            }
        }

        void step()
        {
            // TODO: Check for interrupt here
            // TODO: stall for certain instructions like wait for interrupt...
            // TODO: Fetch can be executed always. Decode and Execute stages might have been flushed after branch
            fetch();
            decode();
            execute();
        }

        void handleMultAcc(bool a, bool s, uint32_t rd, uint32_t rn, uint32_t rs, uint32_t rm)
        {
            // Check given restrictions
            if (rd == rm) {
                std::cout << "ERROR: MUL/MLA destination register may not be the same as the first operand!" << std::endl;
            }
            if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
                std::cout << "ERROR: MUL/MLA PC register may not be involved in calculations!" << std::endl;
            }

            auto currentRegs = state.getCurrentRegs();
            uint32_t rmVal = *currentRegs[rm];
            uint32_t rsVal = *currentRegs[rs];
            uint32_t rnVal = *currentRegs[rn];

            uint32_t mulRes = rmVal * rsVal;

            if (a) { // MLA: add RN
                mulRes += rnVal;
            }

            *currentRegs[rd] = static_cast<uint32_t>(mulRes & 0x0FFFFFFFF);

            if (s) {
                // update zero flag & signed flags
                // the rest is unaffected
                state.setFlag(cpsr_flags::Z_FLAG, mulRes == 0);
                state.setFlag(cpsr_flags::N_FLAG, mulRes >> 31);
            }
        }

        void handleMultAccLong(bool signMul, bool a, bool s, uint32_t rd_msw, uint32_t rd_lsw, uint32_t rs, uint32_t rm)
        {
            if (rd_lsw == rd_msw || rd_lsw == rm || rd_msw == rm) {
                std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL lo, high & rm registers may not be the same!" << std::endl;
            }
            if (rd_lsw == regs::PC_OFFSET || rd_msw == regs::PC_OFFSET || rs == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
                std::cout << "ERROR: SMULL/SMLAL/UMULL/UMLAL PC register may not be involved in calculations!" << std::endl;
            }

            auto currentRegs = state.getCurrentRegs();

            uint64_t rdVal = (static_cast<uint64_t>(*currentRegs[rd_msw]) << 32) | *currentRegs[rd_lsw];

            uint64_t mulRes;

            if (!signMul) {
                uint64_t rmVal = static_cast<uint64_t>(*currentRegs[rm]);
                uint64_t rsVal = static_cast<uint64_t>(*currentRegs[rs]);

                mulRes = rmVal * rsVal;

                if (a) { // UMLAL: add rdVal
                    mulRes += rdVal;
                }
            } else {
                // Enforce sign extension
                int64_t rmVal = static_cast<int64_t>(static_cast<int32_t>(*currentRegs[rm]));
                int64_t rsVal = static_cast<int64_t>(static_cast<int32_t>(*currentRegs[rs]));

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
                state.setFlag(cpsr_flags::Z_FLAG, mulRes == 0);
                state.setFlag(cpsr_flags::N_FLAG, mulRes >> 31);
            }
        }

        void handleDataSwp(bool b, uint32_t rn, uint32_t rd, uint32_t rm)
        {
            if (rd == regs::PC_OFFSET || rn == regs::PC_OFFSET || rm == regs::PC_OFFSET) {
                std::cout << "ERROR: SWP/SWPB PC register may not be involved in calculations!" << std::endl;
            }

            auto currentRegs = state.getCurrentRegs();
            uint32_t newMemVal = *currentRegs[rm];
            uint32_t memAddr = *currentRegs[rn];

            if (b) {
                uint8_t memVal = state.memory.read8(memAddr);
                state.memory.write8(memAddr, static_cast<uint8_t>(newMemVal & 0x0FF));
                //TODO overwrite upper 24 bits?
                *currentRegs[rd] = static_cast<uint32_t>(memVal);
            } else {
                uint32_t memVal = state.memory.read32(memAddr);
                state.memory.write32(memAddr, newMemVal);
                *currentRegs[rd] = memVal;
            }
        }

        // Executes instructions belonging to the branch subsection
        void handleBranch(bool link, uint32_t offset)
        {

            uint32_t pc = state.getCurrentPC();

            // If link is set, R14 will receive the address of the next instruction to be executed. So if we are
            // jumping but want to remember where to return to after the subroutine finished that might be usefull.
            if (link) {
                // Next instruction should be at: PC - 4
                state.accessReg(regs::LR_OFFSET) = (pc - 4);
            }

            // Offset is given in units of 4. Thus we need to shift it first by two
            offset = offset << 2;

            // TODO: Is there a nice way to add a signed to a unsigned. Plus might want to check for overflows.
            // Although there probably is nothing we can do in those cases....

            // We need to handle the addition of pc and offset as signed as we jump back
            int32_t offsetSigned = static_cast<int32_t>(offset);
            int32_t pcSigned = static_cast<int32_t>(pc);

            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(pcSigned + offsetSigned);

            state.branchOccurred = true;
        }

        // Executes instructions belonging to the branch and execute subsection
        void handleBranchAndExchange(uint32_t rn)
        {
            auto currentRegs = state.getCurrentRegs();

            // Load the content of register given by rm
            uint32_t rnValue = *currentRegs[rn];
            // If the first bit of rn is set
            bool changeToThumb = rnValue & 0x00000001;

            if (changeToThumb) {
                // TODO: Flag change to thumb mode
            }

            // Change the PC to the address given by rm. Note that we have to mask out the thumb switch bit.
            state.accessReg(regs::PC_OFFSET) = rnValue & 0xFFFFFFFE;
            state.branchOccurred = true;
        }

        /* ALU functions */
        InstructionExecutionInfo execDataProc(arm::ARMInstruction &inst)
        {
            /* calculate shifter operand */
            arm::ShiftType shiftType;
            uint32_t shiftAmount, rm, rs, imm, shifterOperand;
            bool shiftByReg = inst.params.data_proc_psr_transf.extractOperand2(shiftType, shiftAmount, rm, rs, imm);

            if (inst.params.data_proc_psr_transf.i) {
                shifterOperand = (imm >> shiftAmount) | (imm << (32 - shiftAmount));
            } else {
                if (shiftByReg)
                    shiftAmount = *state.getCurrentRegs()[rs];

                uint32_t rmValue = *state.getCurrentRegs()[rm];

                switch (shiftType) {
                    case arm::ShiftType::LSL:
                        shifterOperand = rmValue << shiftAmount;
                        break;
                    case arm::ShiftType::LSR:
                        shifterOperand = rmValue >> shiftAmount;
                        break;
                    case arm::ShiftType::ASR:
                        shifterOperand = static_cast<uint32_t>(static_cast<int32_t>(rmValue) >> shiftAmount);
                        break;
                    case arm::ShiftType::ROR:
                        /* shift with wrap around */
                        shifterOperand = (rmValue >> shiftAmount) | (rmValue << (32 - shiftAmount));
                        break;
                }
            }

            bool negative = state.getFlag(cpsr_flags::N_FLAG),
                 zero = state.getFlag(cpsr_flags::Z_FLAG),
                 overflow = state.getFlag(cpsr_flags::V_FLAG),
                 carry = state.getFlag(cpsr_flags::C_FLAG);

            uint64_t rnValue = state.accessReg(inst.params.block_data_transf.rn);
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
                arm::ADC, arm::ADD, arm::AND, arm::CMN, arm::CMP,
                arm::EOR, arm::MVN, arm::ORR, arm::RSB, arm::RSC,
                arm::SBC, arm::SUB, arm::TEQ, arm::TST};

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
                case arm::MSR:
                    if (inst.params.data_proc_psr_transf.r)
                        resultValue = state.accessReg(regs::SPSR_OFFSET);
                    else
                        resultValue = state.accessReg(regs::CPSR_OFFSET);
                    break;
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
                negative = resultValue & (1 << 31);
                zero = resultValue == 0;
                overflow = (resultValue >> 32) & 0xFFFFFFFF;
                carry = resultValue & (static_cast<uint64_t>(1) << 32);

                if (updateNegative.find(inst.id) != updateNegative.end())
                    state.setFlag(cpsr_flags::N_FLAG, negative);

                if (updateZero.find(inst.id) != updateZero.end())
                    state.setFlag(cpsr_flags::Z_FLAG, zero);

                if (updateOverflow.find(inst.id) != updateOverflow.end())
                    state.setFlag(cpsr_flags::V_FLAG, overflow);

                if (updateCarry.find(inst.id) != updateCarry.end())
                    state.setFlag(cpsr_flags::C_FLAG, carry);
            }

            if (dontUpdateRD.find(inst.id) == dontUpdateRD.end())
                state.accessReg(inst.params.data_proc_psr_transf.rd) = resultValue;

            /* TODO: cycle timings */
            InstructionExecutionInfo info{ 0 };
            bool destPC = inst.params.data_proc_psr_transf.rd == regs::PC_OFFSET;

            if (!destPC && !shiftByReg)
                info.cycleCount = 1;
            else if (destPC && !shiftByReg)
                info.cycleCount = 3;
            else if (!destPC && shiftByReg)
                info.cycleCount = 2;
            else
                info.cycleCount = 4;
            
            return info;
        }

        void execLoadStoreRegUByte(const arm::ARMInstruction& inst) {
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

            uint32_t rn = inst.params.ls_reg_ubyte.rn;
            uint32_t rd = inst.params.ls_reg_ubyte.rd;
            /* these are computed in the next step */
            uint32_t memoryAddress;
            uint32_t offset;

            /* offset is calculated differently, depending on the I-bit */
            if (immediate) {
                offset = inst.params.ls_reg_ubyte.addrMode;
            } else {
                /* TODO: 0 has special meaning */
                uint32_t shiftAmount = (inst.params.ls_reg_ubyte.addrMode >> 7) & 0x1F;
                auto shiftType = static_cast<arm::ShiftType>((inst.params.ls_reg_ubyte.addrMode >> 5) & 0b11);
                uint32_t rm = inst.params.ls_reg_ubyte.addrMode & 0xF;

                offset = arm::shift(state.accessReg(rm), shiftType, shiftAmount);
            }

            /* if the offset is added depends on the indexing mode */
            if (pre)
                memoryAddress = up ? rn + offset : rn - offset;
            else
                memoryAddress = rn;

            /* transfer */
            if (load) {
                if (byte) {
                    state.accessReg(rd) = state.memory.read8(memoryAddress);
                } else {
                    state.accessReg(rd) = state.memory.read32(memoryAddress);
                }
            } else {
                if (byte) {
                    state.memory.write8(memoryAddress, state.accessReg(rd));
                } else {
                    state.memory.write32(memoryAddress, state.accessReg(rd));
                }
            }

            if (!pre || writeback)
                state.accessReg(rn) = memoryAddress;

            if (!pre) {
                /* TODO: What's this? */
                bool forcePrivAccess = writeback;
            }
        }

        void execDataBlockTransfer(const arm::ARMInstruction& inst) {
            bool pre = inst.params.block_data_transf.p;
            bool up = inst.params.block_data_transf.u;
            bool writeback = inst.params.block_data_transf.w;
            bool load = inst.params.block_data_transf.l;
            uint32_t rn = inst.params.block_data_transf.rn;
            uint32_t address = rn;

            for (uint32_t i = 0; i < 16; ++i) {
                if (pre && up)
                    address += 4;
                else if (pre && !up)
                    address -= 4;

                if (inst.params.block_data_transf.rList & (1 << i)) {
                    if (load)
                        if (i == 15)
                            state.accessReg(regs::PC_OFFSET) = state.memory.read32(address) & 0xFFFFFFFC;
                        else
                            state.accessReg(i) = state.memory.read32(address);
                    else
                        state.memory.write32(address, state.accessReg(i));
                }

                if (!pre && up)
                    address += 4;
                else if (!pre && !up)
                    address -= 4;
            }

            /* TODO: not sure if address - 4 */
            if (writeback)
                state.accessReg(rn) = address;
        }
    };

} // namespace gbaemu

#endif /* CPU_HPP */