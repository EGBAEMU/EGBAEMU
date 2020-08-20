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

    class CPU
    {
      public:
        CPUState state;

        void initPipeline()
        {
            // We need to fill the pipeline to the state where the instruction at PC is ready for execution -> fetched + decoded!
            uint32_t pc = state.accessReg(regs::PC_OFFSET);
            bool thumbMode = state.getFlag(cpsr_flags::THUMB_STATE);
            state.accessReg(regs::PC_OFFSET) = pc - (thumbMode ? 4 : 8);
            fetch();
            state.accessReg(regs::PC_OFFSET) = pc - (thumbMode ? 2 : 4);
            fetch();
            decode();
            state.accessReg(regs::PC_OFFSET) = pc;
        }

        void fetch()
        {
            // propagate pipeline
            state.pipeline.fetch.lastInstruction = state.pipeline.fetch.instruction;
            state.pipeline.fetch.lastReadData = state.pipeline.fetch.readData;

            bool thumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

            //TODO we only need to fetch 16 bit for thumb mode!
            //TODO we might need this info? (where nullptr is currently)
            if (thumbMode) {
                //TODO check this
                /* pc is at [27:1] */
                uint32_t pc = (state.accessReg(regs::PC_OFFSET) >> 1) & 0x07FFFFFF;
                state.pipeline.fetch.instruction = state.memory.read16((pc * 2) + 4, nullptr);
            } else {
                /* pc is at [27:2] */
                uint32_t pc = (state.accessReg(regs::PC_OFFSET) >> 2) & 0x03FFFFFF;
                state.pipeline.fetch.instruction = state.memory.read32((pc * 4) + 8, nullptr);
            }

            //TODO where do we want to update pc? (+4)
        }

        void decode()
        {
            state.pipeline.decode.lastInstruction = state.pipeline.decode.instruction;
            state.pipeline.decode.instruction = state.decoder->decode(state.pipeline.fetch.lastInstruction);
        }

        InstructionExecutionInfo execute()
        {
            InstructionExecutionInfo info{0};

            const uint32_t prevPc = state.getCurrentPC();
            const bool prevThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

            if (state.pipeline.decode.lastInstruction.arm.id != arm::ARMInstructionID::INVALID || state.pipeline.decode.lastInstruction.thumb.id != thumb::ThumbInstructionID::INVALID) {

                if (state.pipeline.decode.lastInstruction.isArmInstruction()) {
                    arm::ARMInstruction &armInst = state.pipeline.decode.lastInstruction.arm;

                    // Do we even need an execution?
                    if (armInst.conditionSatisfied(state)) {

                        // prefer using switch to get warned if a category is not handled
                        switch (armInst.cat) {
                            case arm::ARMInstructionCategory::MUL_ACC:
                                info = handleMultAcc(armInst.params.mul_acc.a,
                                                     armInst.params.mul_acc.s,
                                                     armInst.params.mul_acc.rd,
                                                     armInst.params.mul_acc.rn,
                                                     armInst.params.mul_acc.rs,
                                                     armInst.params.mul_acc.rm);
                                break;
                            case arm::ARMInstructionCategory::MUL_ACC_LONG:
                                info = handleMultAccLong(armInst.params.mul_acc_long.u,
                                                         armInst.params.mul_acc_long.a,
                                                         armInst.params.mul_acc_long.s,
                                                         armInst.params.mul_acc_long.rd_msw,
                                                         armInst.params.mul_acc_long.rd_lsw,
                                                         armInst.params.mul_acc_long.rs,
                                                         armInst.params.mul_acc_long.rm);
                                break;
                            case arm::ARMInstructionCategory::BRANCH_XCHG:
                                info = handleBranchAndExchange(armInst.params.branch_xchg.rn);
                                break;
                            case arm::ARMInstructionCategory::DATA_SWP:
                                info = handleDataSwp(armInst.params.data_swp.b,
                                                     armInst.params.data_swp.rn,
                                                     armInst.params.data_swp.rd,
                                                     armInst.params.data_swp.rm);
                                break;
                                /* those two are the same */
                            case arm::ARMInstructionCategory::HW_TRANSF_REG_OFF:
                            case arm::ARMInstructionCategory::HW_TRANSF_IMM_OFF:
                            case arm::ARMInstructionCategory::SIGN_TRANSF:
                                info = execHalfwordDataTransferImmRegSignedTransfer(armInst);
                                break;
                            case arm::ARMInstructionCategory::DATA_PROC_PSR_TRANSF:
                                info = execDataProc(armInst);
                                break;
                            case arm::ARMInstructionCategory::LS_REG_UBYTE:
                                info = execLoadStoreRegUByte(armInst);
                                break;
                            case arm::ARMInstructionCategory::BLOCK_DATA_TRANSF:
                                info = execDataBlockTransfer(armInst);
                                break;
                            case arm::ARMInstructionCategory::BRANCH:
                                info = handleBranch(armInst.params.branch.l, armInst.params.branch.offset);
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
                                    info = swi::biosCallHandler[index](&state);
                                } else {
                                    std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << std::endl;
                                }
                                break;
                            }

                            default:
                                std::cout << "INVALID" << std::endl;
                                break;
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
                                info = swi::biosCallHandler[index](&state);
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

            //TODO check if every instruction has this 1S cycle
            //TODO we need to consider here branch instructions -> which PC to use for this calculation + how often (pipeline flush?)
            // Add 1S cycle needed to fetch a instruction if not other requested
            if (!info.noDefaultSCycle) {
                info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(state.getCurrentPC(), sizeof(uint32_t));
            }
            if (info.additionalProgCyclesN) {
                info.cycleCount += state.memory.nonSeqWaitCyclesForVirtualAddr(state.getCurrentPC(), sizeof(uint32_t)) * info.additionalProgCyclesN;
            }
            if (info.additionalProgCyclesS) {
                info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(state.getCurrentPC(), sizeof(uint32_t)) * info.additionalProgCyclesS;
            }

            const uint32_t postPc = state.getCurrentPC();
            const bool postThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

            // Change from arm mode to thumb mode or vice versa
            if (prevThumbMode != postThumbMode) {
                //TODO change fetch and decode strategy to corresponding code

                std::cout << "INFO: MODE CHANGE" << std::endl;
            }
            // We have a branch, return or something that changed our PC
            if (prevPc != postPc) {
                initPipeline();

                std::cout << "INFO: PIPELINE FLUSH" << std::endl;
            } else {
                //TODO this is probably unwanted if we changed the mode?
                // Increment the pc counter to the next instruction
                state.accessReg(regs::PC_OFFSET) = postPc + (postThumbMode ? 2 : 4);
            }

            return info;
        }

        void step()
        {
            static InstructionExecutionInfo info{0};

            // Execute pipeline only after stall is over
            if (info.cycleCount == 0) {
                // TODO: Check for interrupt here
                // TODO: stall for certain instructions like wait for interrupt...
                // TODO: Fetch can be executed always. Decode and Execute stages might have been flushed after branch
                fetch();
                decode();
                info = execute();
                // Current cycle must be removed
                --info.cycleCount;
            } else {
                --info.cycleCount;
            }
        }

        InstructionExecutionInfo handleMultAcc(bool a, bool s, uint32_t rd, uint32_t rn, uint32_t rs, uint32_t rm)
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
                state.setFlag(cpsr_flags::Z_FLAG, mulRes == 0);
                state.setFlag(cpsr_flags::N_FLAG, mulRes >> 31);
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

        InstructionExecutionInfo handleMultAccLong(bool signMul, bool a, bool s, uint32_t rd_msw, uint32_t rd_lsw, uint32_t rs, uint32_t rm)
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
                state.setFlag(cpsr_flags::Z_FLAG, mulRes == 0);
                state.setFlag(cpsr_flags::N_FLAG, mulRes >> 31);
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

        InstructionExecutionInfo handleDataSwp(bool b, uint32_t rn, uint32_t rd, uint32_t rm)
        {
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
        InstructionExecutionInfo handleBranch(bool link, int32_t offset)
        {
            uint32_t pc = state.getCurrentPC();

            // If link is set, R14 will receive the address of the next instruction to be executed. So if we are
            // jumping but want to remember where to return to after the subroutine finished that might be usefull.
            if (link) {
                // Next instruction should be at: PC + 4
                state.accessReg(regs::LR_OFFSET) = (pc + 4);
            }

            // Offset is given in units of 4. Thus we need to shift it first by two
            offset = offset << 2;

            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(pc) + offset);

            // Execution Time: 2S + 1N
            InstructionExecutionInfo info{0};
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;

            return info;
        }

        // Executes instructions belonging to the branch and execute subsection
        InstructionExecutionInfo handleBranchAndExchange(uint32_t rn)
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

        InstructionExecutionInfo execLoadStoreRegUByte(const arm::ARMInstruction &inst)
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
                    state.accessReg(rd) = state.memory.read8(memoryAddress, &info.cycleCount);
                } else {
                    state.accessReg(rd) = state.memory.read32(memoryAddress, &info.cycleCount);
                }
            } else {
                if (byte) {
                    state.memory.write8(memoryAddress, state.accessReg(rd), &info.cycleCount);
                } else {
                    state.memory.write32(memoryAddress, state.accessReg(rd), &info.cycleCount);
                }
            }

            if (!pre || writeback)
                state.accessReg(rn) = memoryAddress;

            if (!pre) {
                /* TODO: What's this? */
                bool forcePrivAccess = writeback;
            }

            return info;
        }

        InstructionExecutionInfo execDataBlockTransfer(const arm::ARMInstruction &inst)
        {
            bool pre = inst.params.block_data_transf.p;
            bool up = inst.params.block_data_transf.u;
            bool writeback = inst.params.block_data_transf.w;
            bool load = inst.params.block_data_transf.l;
            uint32_t rn = inst.params.block_data_transf.rn;
            uint32_t address = rn;

            // Execution Time:
            // For normal LDM, nS+1N+1I. For LDM PC, (n+1)S+2N+1I.
            // For STM (n-1)S+2N. Where n is the number of words transferred.
            InstructionExecutionInfo info{0};
            if (load) {
                // handle +1I
                info.cycleCount = 1;
            } else {
                // same edge case as for STR
                //TODO not sure why STR instructions have 2N ...
                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            // The first read / write is non sequential but afterwards all accesses are sequential
            // because the memory class always adds non sequential accesses we need to handle this case explicitly
            bool nonSeqAccDone = false;

            for (uint32_t i = 0; i < 16; ++i) {
                if (pre && up)
                    address += 4;
                else if (pre && !up)
                    address -= 4;

                if (inst.params.block_data_transf.rList & (1 << i)) {
                    if (load) {
                        if (i == 15) {
                            state.accessReg(regs::PC_OFFSET) = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount) & 0xFFFFFFFC;
                            // Special case for pipeline refill
                            info.additionalProgCyclesN = 1;
                            info.additionalProgCyclesS = 1;
                        } else {
                            state.accessReg(i) = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount);
                        }
                    } else {
                        state.memory.write32(address, state.accessReg(i), nonSeqAccDone ? nullptr : &info.cycleCount);
                    }
                    if (nonSeqAccDone) {
                        info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(address, sizeof(uint32_t));
                    }
                    nonSeqAccDone = true;
                }

                if (!pre && up)
                    address += 4;
                else if (!pre && !up)
                    address -= 4;
            }

            /* TODO: not sure if address - 4 */
            if (writeback)
                state.accessReg(rn) = address;

            return info;
        }

        InstructionExecutionInfo execHalfwordDataTransferImmRegSignedTransfer(const arm::ARMInstruction &inst)
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
                    uint32_t rd = inst.params.sign_transf.addrMode & 0xF;
                    offset = state.accessReg(rd);
                } else
                    offset = inst.params.sign_transf.addrMode;

                if (inst.params.sign_transf.h)
                    transferSize = 16;
                else
                    transferSize = 8;

                sign = true;
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

            if (pre) {
                if (up)
                    memoryAddress = state.accessReg(rn) + offset;
                else
                    memoryAddress = state.accessReg(rn) - offset;
            } else
                memoryAddress = state.accessReg(rn);

            if (load) {
                if (sign) {
                    if (transferSize == 16) {
                        state.accessReg(rd) = static_cast<int32_t>(state.memory.read16(memoryAddress, &info.cycleCount));
                    } else {
                        state.accessReg(rd) = static_cast<int32_t>(state.memory.read8(memoryAddress, &info.cycleCount));
                    }
                } else {
                    if (transferSize == 16) {
                        state.accessReg(rd) = state.memory.read16(memoryAddress, &info.cycleCount);
                    } else {
                        state.accessReg(rd) = state.memory.read8(memoryAddress, &info.cycleCount);
                    }
                }
            } else {
                if (transferSize == 16) {
                    state.memory.write16(memoryAddress, state.accessReg(rd), &info.cycleCount);
                } else {
                    state.memory.write8(memoryAddress, state.accessReg(rd), &info.cycleCount);
                }
            }

            if (writeback || !pre) {
                if (!pre) {
                    if (up)
                        memoryAddress = state.accessReg(rn) + offset;
                    else
                        memoryAddress = state.accessReg(rn) - offset;
                }

                state.accessReg(rn) = memoryAddress;
            }

            return info;
        }
    };

} // namespace gbaemu

#endif /* CPU_HPP */