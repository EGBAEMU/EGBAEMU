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
                                break;
                            case arm::ARMInstructionCategory::MUL_ACC_LONG:
                                break;
                            case arm::ARMInstructionCategory::BRANCH_XCHG:
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
                                break;

                            case arm::ARMInstructionCategory::SOFTWARE_INTERRUPT:
                                /*
                                SWIs can be called from both within THUMB and ARM mode. In ARM mode, only the upper 8bit of the 24bit comment field are interpreted.
                                //TODO is switching needed?
                                Each time when calling a BIOS function 4 words (SPSR, R11, R12, R14) are saved on Supervisor stack (_svc). Once it has saved that data, the SWI handler switches into System mode, so that all further stack operations are using user stack.
                                In some cases the BIOS may allow interrupts to be executed from inside of the SWI procedure. If so, and if the interrupt handler calls further SWIs, then care should be taken that the Supervisor Stack does not overflow.
                                */
                                swi::biosCallHandler[armInst.params.software_interrupt.comment >> 24](&state);
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
                        case thumb::ThumbInstructionCategory::SOFTWARE_INTERRUPT:
                            break;
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

        // Executes instructions belonging to the branch subsection
        void handleBranch(bool link, uint32_t offset)
        {

            uint32_t pc = state.getCurrentPC();

            // If link is set, R14 will receive the address of the next instruction to be executed. So if we are
            // jumping but want to remeber where to return to after the subroutine finished that might be usefull.
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
        void handleBranchAndExchange(uint32_t rm)
        {
            auto currentRegs = state.getCurrentRegs();

            // Load the content of register given by rm
            uint32_t rmValue = *currentRegs[rm];
            // If the first bit of rn is set
            bool changeToThumb = rmValue & 0x00000001;

            if (changeToThumb) {
                // TODO: Flag change to thumb mode
            }

            // Change the PC to the address given by rm. Note that we have to mask out the thumb switch bit.
            state.accessReg(regs::PC_OFFSET) = rmValue & 0xFFFFFFFE;
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

        void execMOV(bool i, bool s, uint32_t rd, uint32_t op2)
        {
            auto currentRegs = state.getCurrentRegs();

            if (i) {
                *currentRegs[rd] = op2;
            } else {
                bool r = (op2 >> 4) & 1;
                uint32_t shiftAmount;

                if (r) {
                    uint32_t rs = (op2 >> 8) & 0b1111;
                    shiftAmount = *currentRegs[rs] & 0xFF;
                } else {
                    shiftAmount = (op2 >> 7) & 0b11111;
                }

                /* (0=LSL, 1=LSR, 2=ASR, 3=ROR) */
                uint32_t shiftType = (op2 >> 5) & 0b11;
                uint32_t rm = op2 & 0b1111;
                uint32_t newValue = *currentRegs[rm];

                switch (shiftType) {
                    case 0:
                        newValue <<= shiftAmount;
                        break;
                    case 1:
                        newValue >>= shiftAmount;
                        break;
                    case 2:
                        newValue = static_cast<uint32_t>(static_cast<int32_t>(newValue) >> shiftAmount);
                        break;
                    case 3:
                        /* shift with wrap around */
                        newValue = (newValue >> shiftAmount) | (newValue << (32 - shiftAmount));
                        break;
                }

                *currentRegs[rd] = newValue;
            }

            /* rd == R15 */
            if (s && rd == regs::PC_OFFSET)
                *currentRegs[regs::CPSR_OFFSET] = *currentRegs[regs::SPSR_OFFSET];
        }
    };

} // namespace gbaemu

#endif /* CPU_HPP */