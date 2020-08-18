#ifndef CPU_HPP
#define CPU_HPP

#include "cpu_state.hpp"
#include "inst_arm.hpp"
#include "inst_thumb.hpp"
#include "regs.hpp"
#include "swi.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace gbaemu
{

    /*
    instruction set
    http://www.ecs.csun.edu/~smirzaei/docs/ece425/arm7tdmi_instruction_set_reference.pdf
 */
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
                    if (armInst.conditionSatisfied(state.accessReg(regs::CPSR_OFFSET))) {

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
                                break;
                            case arm::ARMInstructionCategory::BLOCK_DATA_TRANSF:
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
                auto &cpsr = *currentRegs[regs::CPSR_OFFSET];
                cpsr = (cpsr & ~(cpsr_flags::Z_FLAG_BITMASK | cpsr_flags::N_FLAG_BITMASK)) | ((mulRes >> 31) << cpsr_flags::FLAG_N_OFFSET) | (mulRes == 0 ? (1 << cpsr_flags::FLAG_Z_OFFSET) : 0);
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

            uint64_t rdVal = (static_cast<uint64_t>(*currentRegs[rd_msw]) << 32) | *currentRegs[rd_lsw]);

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
                auto &cpsr = *currentRegs[regs::CPSR_OFFSET];
                cpsr = (cpsr & ~(cpsr_flags::Z_FLAG_BITMASK | cpsr_flags::N_FLAG_BITMASK)) | ((mulRes >> 31) << cpsr_flags::FLAG_N_OFFSET) | (mulRes == 0 ? (1 << cpsr_flags::FLAG_Z_OFFSET) : 0);
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

        void handleBitClear(uint8_t rn, uint8_t rd, uint8_t shifterOperand)
        {

            auto currentRegs = state.getCurrentRegs();
            uint32_t rnValue = *currentRegs[rn];

            //TODO this seems undone???
            uint32_t result = rnValue & shifterOperand;
        }

        void exec_add(bool s, uint32_t rn, uint32_t rd, uint32_t shiftOperand)
        {
            auto currentRegs = state.getCurrentRegs();

            // Get the value of the rn register
            int32_t rnValueSigned = static_cast<int32_t>(*currentRegs[rn]);
            // The value of the shift operand as signed int
            int32_t shiftOperandSigned = static_cast<int32_t>(shiftOperand);

            // Construt the sum. Give it some more bits so we can catch an overflow.
            int64_t resultSigned = rnValueSigned + shiftOperandSigned;
            uint64_t result = static_cast<uint64_t>(resultSigned);

            // Write the value back to rd
            *currentRegs[rd] = static_cast<uint32_t>(result & 0x00000000FFFFFFFF);

            // If s is set, we have to update the N, Z, V and C Flag
            if (s) {

                state.clearFlags();

                if (result == 0) {
                    state.setFlag(cpsr_flags::FLAG_Z_OFFSET);
                }
                if (result < 0) {
                    state.setFlag(cpsr_flags::FLAG_N_OFFSET);
                }

                //TODO does this work with negative values as well?
                // If there is a bit set in the upper half there must have been an overflow (i guess)
                if (result & 0xFFFFFFFF00000000) {
                    state.setFlag(cpsr_flags::FLAG_C_OFFSET);
                }
            }
        }

        void execMOV(arm::ARMInstruction &inst)
        {
            auto currentRegs = state.getCurrentRegs();
            arm::ShiftType shiftType;
            uint32_t shiftAmount, rm, rs, imm;

            bool shiftByReg = inst.params.data_proc_psr_transf.extractOperand2(shiftType, shiftAmount, rm, rs, imm);

            if (inst.params.data_proc_psr_transf.i) {
                *currentRegs[inst.params.data_proc_psr_transf.rd] = (imm >> shiftAmount) | (imm << (32 - shiftAmount));
            } else {
                if (shiftByReg)
                    shiftAmount = *state.getCurrentRegs()[rs];

                uint32_t rmValue = *state.getCurrentRegs()[rm];

                /* (0=LSL, 1=LSR, 2=ASR, 3=ROR) */
                uint32_t newValue;

                switch (shiftType) {
                    case 0:
                        newValue = rmValue << shiftAmount;
                        break;
                    case 1:
                        newValue = rmValue >> shiftAmount;
                        break;
                    case 2:
                        newValue = static_cast<uint32_t>(static_cast<int32_t>(rmValue) >> shiftAmount);
                        break;
                    case 3:
                        /* shift with wrap around */
                        newValue = (rmValue >> shiftAmount) | (rmValue << (32 - shiftAmount));
                        break;
                }

                *currentRegs[inst.params.data_proc_psr_transf.rd] = newValue;
            }

            /* rd == R15 */
            if (inst.params.data_proc_psr_transf.s && inst.params.data_proc_psr_transf.rd == regs::PC_OFFSET)
                *currentRegs[regs::CPSR_OFFSET] = *currentRegs[regs::SPSR_OFFSET];
        }
    };

} // namespace gbaemu

#endif /* CPU_HPP */