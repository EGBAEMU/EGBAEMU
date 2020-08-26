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
      private:
        arm::ARMInstructionDecoder armDecoder;
        thumb::ThumbInstructionDecoder thumbDecoder;

      public:
        CPU()
        {
            state.decoder = &armDecoder;

            /*
            Default memory usage at 03007FXX (and mirrored to 03FFFFXX)
              Addr.    Size Expl.
              3007FFCh 4    Pointer to user IRQ handler (32bit ARM code)
              3007FF8h 2    Interrupt Check Flag (for IntrWait/VBlankIntrWait functions)
              3007FF4h 4    Allocated Area
              3007FF0h 4    Pointer to Sound Buffer
              3007FE0h 16   Allocated Area
              3007FA0h 64   Default area for SP_svc Supervisor Stack (4 words/time)
              3007F00h 160  Default area for SP_irq Interrupt Stack (6 words/time)
            Memory below 7F00h is free for User Stack and user data. The three stack pointers are initially initialized at the TOP of the respective areas:
              SP_svc=03007FE0h
              SP_irq=03007FA0h
              SP_usr=03007F00h
            The user may redefine these addresses and move stacks into other locations, however, the addresses for system data at 7FE0h-7FFFh are fixed.
            */
            // Set default SP values
            *state.getModeRegs(CPUState::CPUMode::UserMode)[regs::SP_OFFSET] = 0x03007F00;
            *state.getModeRegs(CPUState::CPUMode::IRQ)[regs::SP_OFFSET] = 0x3007FA0;
            *state.getModeRegs(CPUState::CPUMode::SupervisorMode)[regs::SP_OFFSET] = 0x03007FE0;
        }

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

            if (state.pipeline.decode.lastInstruction.isArmInstruction()) {
                if (state.pipeline.decode.lastInstruction.arm.id == arm::ARMInstructionID::INVALID) {
                    std::cout << "ERROR: trying to execute invalid ARM instruction!" << std::endl;
                } else {
                    arm::ARMInstruction &armInst = state.pipeline.decode.lastInstruction.arm;

                    // Do we even need an execution?
                    if (conditionSatisfied(armInst.condition, state)) {

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
                }
            } else {
                if (state.pipeline.decode.lastInstruction.thumb.id == thumb::ThumbInstructionID::INVALID) {
                    std::cout << "ERROR: trying to execute invalid THUMB instruction!" << std::endl;
                } else {
                    thumb::ThumbInstruction &thumbInst = state.pipeline.decode.lastInstruction.thumb;

                    // prefer using switch to get warned if a category is not handled
                    switch (thumbInst.cat) {
                        case thumb::ThumbInstructionCategory::MOV_SHIFT:
                            info = handleThumbMoveShiftedReg(thumbInst.id, thumbInst.params.mov_shift.rs, thumbInst.params.mov_shift.rd, thumbInst.params.mov_shift.offset);
                            break;
                        case thumb::ThumbInstructionCategory::ADD_SUB:
                            info = handleThumbAddSubtract(thumbInst.id, thumbInst.params.add_sub.rd, thumbInst.params.add_sub.rs, thumbInst.params.add_sub.rn_offset);
                            break;
                        case thumb::ThumbInstructionCategory::MOV_CMP_ADD_SUB_IMM:
                            info = handleThumbMovCmpAddSubImm(thumbInst.id, thumbInst.params.mov_cmp_add_sub_imm.rd, thumbInst.params.mov_cmp_add_sub_imm.offset);
                            break;
                        case thumb::ThumbInstructionCategory::ALU_OP:
                            info = handleThumbALUops(thumbInst.id, thumbInst.params.alu_op.rs, thumbInst.params.alu_op.rd);
                            break;
                        case thumb::ThumbInstructionCategory::BR_XCHG:
                            info = handleThumbBranchXCHG(thumbInst.id, thumbInst.params.br_xchg.rd, thumbInst.params.br_xchg.rs);
                            break;
                        case thumb::ThumbInstructionCategory::PC_LD:
                            info = handleThumbLoadPCRelative(thumbInst.params.pc_ld.rd, thumbInst.params.pc_ld.offset);
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_REL_OFF:
                            info = handleThumbLoadStoreRegisterOffset(thumbInst.params.ld_st_rel_off.l, thumbInst.params.ld_st_rel_off.b, thumbInst.params.ld_st_rel_off.ro, thumbInst.params.ld_st_rel_off.rb, thumbInst.params.ld_st_rel_off.rd);
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_SIGN_EXT:
                            info = handleThumbLoadStoreSignExt(thumbInst.params.ld_st_sign_ext.h, thumbInst.params.ld_st_sign_ext.s, thumbInst.params.ld_st_sign_ext.ro, thumbInst.params.ld_st_sign_ext.rb, thumbInst.params.ld_st_sign_ext.rd);
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_IMM_OFF:
                            info = handleThumbLoadStoreImmOff(thumbInst.params.ld_st_imm_off.l, thumbInst.params.ld_st_imm_off.b, thumbInst.params.ld_st_imm_off.offset, thumbInst.params.ld_st_imm_off.rb, thumbInst.params.ld_st_imm_off.rd);
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_HW:
                            info = handleThumbLoadStoreHalfword(thumbInst.params.ld_st_hw.l, thumbInst.params.ld_st_hw.offset, thumbInst.params.ld_st_hw.rb, thumbInst.params.ld_st_hw.rd);
                            break;
                        case thumb::ThumbInstructionCategory::LD_ST_REL_SP:
                            info = handleThumbLoadStoreSPRelative(thumbInst.params.ld_st_rel_sp.l, thumbInst.params.ld_st_rel_sp.rd, thumbInst.params.ld_st_rel_sp.offset);
                            break;
                        case thumb::ThumbInstructionCategory::LOAD_ADDR:
                            info = handleThumbRelAddr(thumbInst.params.load_addr.sp, thumbInst.params.load_addr.offset, thumbInst.params.load_addr.rd);
                            break;
                        case thumb::ThumbInstructionCategory::ADD_OFFSET_TO_STACK_PTR:
                            info = handleThumbAddOffsetToStackPtr(thumbInst.params.add_offset_to_stack_ptr.s, thumbInst.params.add_offset_to_stack_ptr.offset);
                            break;
                        case thumb::ThumbInstructionCategory::PUSH_POP_REG:
                            info = handleThumbPushPopRegister(thumbInst.params.push_pop_reg.l, thumbInst.params.push_pop_reg.r, thumbInst.params.push_pop_reg.rlist);
                            break;
                        case thumb::ThumbInstructionCategory::MULT_LOAD_STORE:
                            info = handleThumbMultLoadStore(thumbInst.params.mult_load_store.l, thumbInst.params.mult_load_store.rb, thumbInst.params.mult_load_store.rlist);
                            break;
                        case thumb::ThumbInstructionCategory::COND_BRANCH:
                            info = handleThumbConditionalBranch(thumbInst.params.cond_branch.cond, thumbInst.params.cond_branch.offset);
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
                            info = handleThumbUnconditionalBranch(thumbInst.params.unconditional_branch.offset);
                            break;
                        case thumb::ThumbInstructionCategory::LONG_BRANCH_WITH_LINK:
                            info = handleThumbLongBranchWithLink(thumbInst.params.long_branch_with_link.h, thumbInst.params.long_branch_with_link.offset);
                            break;

                        default:
                            std::cout << "INVALID" << std::endl;
                            break;
                    }
                }
            }

            const uint32_t postPc = state.getCurrentPC();
            const bool postThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

            // Add 1S cycle needed to fetch a instruction if not other requested
            if (!info.noDefaultSCycle) {
                info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(postPc, prevThumbMode ? sizeof(uint16_t) : sizeof(uint32_t));
            }
            if (info.additionalProgCyclesN) {
                info.cycleCount += state.memory.nonSeqWaitCyclesForVirtualAddr(postPc, postThumbMode ? sizeof(uint16_t) : sizeof(uint32_t)) * info.additionalProgCyclesN;
            }
            if (info.additionalProgCyclesS) {
                info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(postPc, postThumbMode ? sizeof(uint16_t) : sizeof(uint32_t)) * info.additionalProgCyclesS;
            }

            // Change from arm mode to thumb mode or vice versa
            if (prevThumbMode != postThumbMode) {
                //TODO change fetch and decode strategy to corresponding code

                std::cout << "INFO: MODE CHANGE" << std::endl;
                if (state.decoder == &armDecoder) {
                    state.decoder = &thumbDecoder;
                } else {
                    state.decoder = &armDecoder;
                }
                //TODO can we assume that on change the pc counter will always be modified as well?
            }
            // We have a branch, return or something that changed our PC
            if (prevPc != postPc) {
                std::cout << "INFO: PIPELINE FLUSH" << std::endl;
                initPipeline();
            } else {
                //TODO this is probably unwanted if we changed the mode?
                // Increment the pc counter to the next instruction
                state.accessReg(regs::PC_OFFSET) = postPc + (postThumbMode ? 2 : 4);
            }

            //TODO update current user mode(not thumb/arm)!
            /*
            The Mode Bits M4-M0 contain the current operating mode.
                    Binary Hex Dec  Expl.
                    0xx00b 00h 0  - Old User       ;\26bit Backward Compatibility modes
                    0xx01b 01h 1  - Old FIQ        ; (supported only on ARMv3, except ARMv3G,
                    0xx10b 02h 2  - Old IRQ        ; and on some non-T variants of ARMv4)
                    0xx11b 03h 3  - Old Supervisor ;/
                    10000b 10h 16 - User (non-privileged)
                    10001b 11h 17 - FIQ
                    10010b 12h 18 - IRQ
                    10011b 13h 19 - Supervisor (SWI)
                    10111b 17h 23 - Abort
                    11011b 1Bh 27 - Undefined
                    11111b 1Fh 31 - System (privileged 'User' mode) (ARMv4 and up)
            Writing any other values into the Mode bits is not allowed. 
            */
            uint8_t modeBits = state.accessReg(regs::CPSR_OFFSET) & cpsr_flags::MODE_BIT_MASK;
            switch (modeBits) {
                case 0b10000:
                    state.mode = CPUState::UserMode;
                    break;
                case 0b10001:
                    state.mode = CPUState::FIQ;
                    break;
                case 0b10010:
                    state.mode = CPUState::IRQ;
                    break;
                case 0b10011:
                    state.mode = CPUState::SupervisorMode;
                    break;
                case 0b10111:
                    state.mode = CPUState::AbortMode;
                    break;
                case 0b11011:
                    state.mode = CPUState::UndefinedMode;
                    break;
                case 0b11111:
                    state.mode = CPUState::SystemMode;
                    break;

                default:
                    /*
                    switch (modeBits & 0x13) {
                        case 0b0000:
                            state.mode = CPUState::UserMode;
                            break;
                        case 0b0001:
                            state.mode = CPUState::FIQ;
                            break;
                        case 0b0010:
                            state.mode = CPUState::IRQ;
                            break;
                        case 0b0011:
                            state.mode = CPUState::SupervisorMode;
                            break;
                    }
                    */
                    break;
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

        void setFlags(uint64_t resultValue, bool nFlag, bool zFlag, bool vFlag, bool cFlag)
        {
            bool negative = resultValue & (1 << 31);
            bool zero = resultValue == 0;
            bool overflow = (resultValue >> 32) & 0xFFFFFFFF;
            bool carry = resultValue & (static_cast<uint64_t>(1) << 32);

            if (nFlag)
                state.setFlag(cpsr_flags::N_FLAG, negative);

            if (zFlag)
                state.setFlag(cpsr_flags::Z_FLAG, zero);

            if (vFlag)
                state.setFlag(cpsr_flags::V_FLAG, overflow);

            if (cFlag)
                state.setFlag(cpsr_flags::C_FLAG, carry);
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
                setFlags(
                    mulRes,
                    true,
                    true,
                    false,
                    false);
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

            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(pc) + 8 + offset);

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

            bool negative = state.getFlag(cpsr_flags::N_FLAG),
                 zero = state.getFlag(cpsr_flags::Z_FLAG),
                 overflow = state.getFlag(cpsr_flags::V_FLAG),
                 carry = state.getFlag(cpsr_flags::C_FLAG);

            /* calculate shifter operand */
            arm::ShiftType shiftType;
            uint32_t shiftAmount;
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
                uint32_t shiftAmount = (inst.params.ls_reg_ubyte.addrMode >> 7) & 0x1F;
                auto shiftType = static_cast<arm::ShiftType>((inst.params.ls_reg_ubyte.addrMode >> 5) & 0b11);
                uint32_t rm = inst.params.ls_reg_ubyte.addrMode & 0xF;

                offset = arm::shift(state.accessReg(rm), shiftType, shiftAmount, state.getFlag(cpsr_flags::C_FLAG), true) & 0xFFFFFFFF;
            }

            uint32_t rnValue = state.accessReg(rn);
            uint32_t rdValue = state.accessReg(rd);

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
                    state.accessReg(rd) = state.memory.read8(memoryAddress, &info.cycleCount);
                } else {
                    // More edge case:
                    /*
                    When reading a word from a halfword-aligned address (which is located in the middle between two word-aligned addresses),
                    the lower 16bit of Rd will contain [address] ie. the addressed halfword, 
                    and the upper 16bit of Rd will contain [Rd-2] ie. more or less unwanted garbage. 
                    However, by isolating lower bits this may be used to read a halfword from memory. 
                    (Above applies to little endian mode, as used in GBA.)
                    */
                    if (memoryAddress & 0x02) {
                        // Not word aligned address
                        uint16_t lowerBits = state.memory.read16(memoryAddress, nullptr);
                        uint32_t upperBits = state.memory.read16(rdValue - 2, nullptr);
                        state.accessReg(rd) = lowerBits | (upperBits << 16);

                        // Super strange edge case simulate normal read for latency
                        state.memory.read32(memoryAddress, &info.cycleCount);
                    } else {
                        state.accessReg(rd) = state.memory.read32(memoryAddress, &info.cycleCount);
                    }
                }
            } else {
                if (byte) {
                    state.memory.write8(memoryAddress, rdValue, &info.cycleCount);
                } else {
                    state.memory.write32(memoryAddress, rdValue, &info.cycleCount);
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

        InstructionExecutionInfo execDataBlockTransfer(arm::ARMInstruction &inst)
        {
            auto currentRegs = state.getCurrentRegs();

            //TODO S bit seems relevant for register selection, NOTE: this instruction is reused for handleThumbMultLoadStore & handleThumbPushPopRegister
            bool forceUserRegisters = inst.params.block_data_transf.s;

            if (forceUserRegisters) {
                currentRegs = state.getModeRegs(CPUState::UserMode);
            }

            bool pre = inst.params.block_data_transf.p;
            bool up = inst.params.block_data_transf.u;
            bool writeback = inst.params.block_data_transf.w;
            bool load = inst.params.block_data_transf.l;
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
                //TODO not sure why STR instructions have 2N ...
                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            // The first read / write is non sequential but afterwards all accesses are sequential
            // because the memory class always adds non sequential accesses we need to handle this case explicitly
            bool nonSeqAccDone = false;

            // TODO there are even more edge cases:
            /* Different behaviour dependent on mode...
            ARM:
            Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist, otherwise store NEW base (STM/ARMv4), always store OLD base (STM/ARMv5), no writeback (LDM/ARMv4), writeback if Rb is "the ONLY register, or NOT the LAST register" in Rlist (LDM/ARMv5).

            THUMB:
            Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist, otherwise store NEW base (STM/ARMv4), always store OLD base (STM/ARMv5), no writeback (LDM/ARMv4/ARMv5; at this point, THUMB opcodes work different than ARM opcodes).

            */

            bool edgeCaseEmptyRlist = false;
            // Handle edge case: Empty Rlist: R15 loaded/stored (ARMv4 only)
            if (inst.params.block_data_transf.rList == 0) {
                inst.params.block_data_transf.rList = (1 << 15);
                edgeCaseEmptyRlist = true;
            }

            for (uint32_t i = 0; i < 16; ++i) {
                if (inst.params.block_data_transf.rList & (1 << i)) {
                    if (pre && up)
                        address += 4;
                    else if (pre && !up)
                        address -= 4;

                    if (load) {
                        if (i == 15) {
                            *currentRegs[regs::PC_OFFSET] = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount) & 0xFFFFFFFC;
                            // Special case for pipeline refill
                            info.additionalProgCyclesN = 1;
                            info.additionalProgCyclesS = 1;
                        } else {
                            *currentRegs[i] = state.memory.read32(address, nonSeqAccDone ? nullptr : &info.cycleCount);
                        }
                    } else {
                        state.memory.write32(address, *currentRegs[i], nonSeqAccDone ? nullptr : &info.cycleCount);
                    }
                    if (nonSeqAccDone) {
                        info.cycleCount += state.memory.seqWaitCyclesForVirtualAddr(address, sizeof(uint32_t));
                    }
                    nonSeqAccDone = true;

                    if (!pre && up)
                        address += 4;
                    else if (!pre && !up)
                        address -= 4;
                }
            }

            /* TODO: not sure if address (+/-) 4 */
            if (writeback)
                *currentRegs[rn] = address;

            // Handle edge case: Empty Rlist: Rb=Rb+40h (ARMv4-v5)
            if (edgeCaseEmptyRlist) {
                *currentRegs[rn] = *currentRegs[rn] + 0x40;
            }

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

            uint32_t rnValue = state.accessReg(rn);
            uint32_t rdValue = state.accessReg(rd);

            if (rn == regs::PC_OFFSET)
                rnValue += 8;

            if (rd == regs::PC_OFFSET)
                rdValue += 12;

            if (pre) {
                if (up)
                    memoryAddress = rnValue + offset;
                else
                    memoryAddress = rnValue - offset;
            } else
                memoryAddress = rnValue;

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
                    state.memory.write16(memoryAddress, rdValue, &info.cycleCount);
                } else {
                    state.memory.write8(memoryAddress, rdValue, &info.cycleCount);
                }
            }

            if (writeback || !pre) {
                if (!pre) {
                    if (up)
                        memoryAddress = rnValue + offset;
                    else
                        memoryAddress = rnValue - offset;
                }

                state.accessReg(rn) = memoryAddress;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLongBranchWithLink(bool h, uint16_t offset)
        {
            auto currentRegs = state.getCurrentRegs();

            InstructionExecutionInfo info{0};

            uint32_t extendedAddr = static_cast<uint32_t>(offset);
            if (h) {
                // Second instruction
                extendedAddr <<= 1;
                uint32_t pcVal = *currentRegs[regs::PC_OFFSET];
                *currentRegs[regs::PC_OFFSET] = *currentRegs[regs::LR_OFFSET] + extendedAddr;
                // TODO OR 1? not sure if we need this, this would cause invalid addresses if not catched by fetch (masking out)
                *currentRegs[regs::LR_OFFSET] = (pcVal + 2) | 1;
            } else {
                // First instruction
                extendedAddr <<= 12;
                *currentRegs[regs::LR_OFFSET] = *currentRegs[regs::PC_OFFSET] + 4 + extendedAddr;

                // pipeline flush -> additional cycles needed
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbUnconditionalBranch(int16_t offset)
        {
            // TODO: Offset may be shifted by 1 or not at all
            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(state.getCurrentPC()) + 4 + (offset * 2));

            // Unconditional branches take 2S + 1N
            InstructionExecutionInfo info{0};
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;

            return info;
        }

        InstructionExecutionInfo handleThumbConditionalBranch(uint8_t cond, int8_t offset)
        {
            InstructionExecutionInfo info{0};

            // Branch will be executed if condition is met
            if (conditionSatisfied(static_cast<ConditionOPCode>(cond), state)) {

                state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(state.getCurrentPC()) + 4 + (static_cast<int32_t>(offset) * 2));

                // If branch executed: 2S+1N
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbMultLoadStore(bool load, uint8_t rb, uint8_t rlist)
        {
            arm::ARMInstruction wrapper;
            wrapper.params.block_data_transf.l = load;
            wrapper.params.block_data_transf.rList = static_cast<uint16_t>(rlist);

            wrapper.params.block_data_transf.u = true;
            wrapper.params.block_data_transf.w = true;
            wrapper.params.block_data_transf.p = false;
            wrapper.params.block_data_transf.rn = rb;

            return execDataBlockTransfer(wrapper);
        }

        InstructionExecutionInfo handleThumbPushPopRegister(bool load, bool r, uint8_t rlist)
        {
            uint16_t extendedRList = static_cast<uint16_t>(rlist);

            // 8 PC/LR Bit (0-1)
            //    0: No
            //    1: PUSH LR (R14), or POP PC (R15)
            if (r) {
                if (!load) {
                    extendedRList |= 1 << regs::LR_OFFSET;
                } else {
                    extendedRList |= 1 << regs::PC_OFFSET;
                }
            }

            arm::ARMInstruction wrapper;
            //  // L - Load/Store bit (0=Store to memory, 1=Load from memory)
            wrapper.params.block_data_transf.l = load;
            //  Rlist - Register List
            wrapper.params.block_data_transf.rList = extendedRList;

            // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
            //      0: PUSH {Rlist}{LR}   ;store in memory, decrements SP (R13)
            //      1: POP  {Rlist}{PC}   ;load from memory, increments SP (R13)
            wrapper.params.block_data_transf.u = load;
            //  W - Write-back bit (0=no write-back, 1=write address into base)
            wrapper.params.block_data_transf.w = true;
            // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

            //TODO you have to consider conventions regarding on which address SP is pointing to: first free vs last used
            wrapper.params.block_data_transf.p = !load;

            wrapper.params.block_data_transf.rn = regs::SP_OFFSET;

            return execDataBlockTransfer(wrapper);
        }

        InstructionExecutionInfo handleThumbAddOffsetToStackPtr(bool s, uint8_t offset)
        {
            // nn - Unsigned Offset    (0-508, step 4)
            offset <<= 2;

            if (s) {
                // 1: ADD  SP,#-nn      ;SP = SP - nn
                state.accessReg(regs::PC_OFFSET) = state.getCurrentPC() - offset;
            } else {
                // 0: ADD  SP,#nn       ;SP = SP + nn
                state.accessReg(regs::PC_OFFSET) = state.getCurrentPC() + offset;
            }

            // Execution Time: 1S
            InstructionExecutionInfo info{0};

            return info;
        }

        InstructionExecutionInfo handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd)
        {
            auto currentRegs = state.getCurrentRegs();

            // bool sp
            //          0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
            //          1: ADD  Rd,SP,#nn    ;Rd = SP + nn
            // nn step 4
            *currentRegs[rd] = (sp ? *currentRegs[regs::SP_OFFSET] : ((*currentRegs[regs::PC_OFFSET] + 4) & ~2)) + (static_cast<uint32_t>(offset) << 2);

            // Execution Time: 1S
            InstructionExecutionInfo info{0};
            return info;
        }

        InstructionExecutionInfo handleThumbLoadStoreSPRelative(bool l, uint8_t rd, uint8_t offset)
        {
            InstructionExecutionInfo info{0};

            uint32_t memoryAddress = state.accessReg(regs::SP_OFFSET) + (offset << 4);

            if (l) {
                // 1: LDR  Rd,[SP,#nn]  ;load  32bit data   Rd = WORD[SP+nn]
                state.accessReg(rd) = state.memory.read32(memoryAddress, &info.cycleCount);
                info.cycleCount += 1;
            } else {
                // 0: STR  Rd,[SP,#nn]  ;store 32bit data   WORD[SP+nn] = Rd
                state.memory.write32(memoryAddress, state.accessReg(rd), &info.cycleCount);

                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLoadStoreHalfword(bool l, uint8_t offset, uint8_t rb, uint8_t rd)
        {

            InstructionExecutionInfo info{0};

            // 10-6   nn - Unsigned Offset              (0-62, step 2)
            uint32_t memoryAddress = state.accessReg(rb) + (offset << 1);

            if (l) {

                // 1: LDRH Rd,[Rb,#nn]  ;load  16bit data   Rd = HALFWORD[Rb+nn]
                state.accessReg(rd) = state.memory.read16(memoryAddress, &info.cycleCount);

                info.cycleCount += 1;
            } else {

                //  0: STRH Rd,[Rb,#nn]  ;store 16bit data   HALFWORD[Rb+nn] = Rd
                state.memory.write16(memoryAddress, state.accessReg(rd) & 0x0FFFF, &info.cycleCount);

                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLoadStoreImmOff(bool l, bool b, uint8_t offset, uint8_t rb, uint8_t rd)
        {

            auto currentRegs = state.getCurrentRegs();

            uint32_t address = *currentRegs[rb];

            InstructionExecutionInfo info{0};

            if (l) {
                info.cycleCount = 1;
            } else {
                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            if (b) {
                address += offset;
                if (l) {
                    *currentRegs[rd] = state.memory.read8(address, &info.cycleCount);
                } else {
                    state.memory.write8(address, *currentRegs[rd] & 0x0FF, &info.cycleCount);
                }
            } else {
                // offset is in words
                address += (offset << 2);
                if (l) {
                    *currentRegs[rd] = state.memory.read32(address, &info.cycleCount);
                } else {
                    state.memory.write16(address, *currentRegs[rd], &info.cycleCount);
                }
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLoadStoreSignExt(bool h, bool s, uint8_t ro, uint8_t rb, uint8_t rd)
        {
            auto currentRegs = state.getCurrentRegs();

            InstructionExecutionInfo info{0};

            uint32_t memoryAddress = *currentRegs[rb] + *currentRegs[ro];

            if (!h && !s) {
                // STRH
                state.memory.write16(memoryAddress, *currentRegs[rd], &info.cycleCount);
                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            } else {
                // LDR / LDS
                info.cycleCount = 1;

                if (s) {
                    uint16_t data;
                    uint8_t shiftAmount;

                    if (h) {
                        data = state.memory.read16(memoryAddress, &info.cycleCount);
                        shiftAmount = 16;
                    } else {
                        data = state.memory.read8(memoryAddress, &info.cycleCount);
                        shiftAmount = 8;
                    }

                    // Magic sign extension
                    *currentRegs[rd] = static_cast<int32_t>(static_cast<uint32_t>(data) << shiftAmount) / (1 << shiftAmount);
                } else {
                    // LDRH zero extended
                    *currentRegs[rd] = state.memory.read16(memoryAddress, &info.cycleCount);
                }
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLoadStoreRegisterOffset(bool l, bool b, uint8_t ro, uint8_t rb, uint8_t rd)
        {

            InstructionExecutionInfo info{0};
            uint32_t memoryAddress = state.accessReg(rb) + state.accessReg(ro);

            if (l) {
                if (b) {
                    // 3: LDRB Rd,[Rb,Ro]   ;load   8bit data  Rd = BYTE[Rb+Ro]
                    state.accessReg(rd) = state.memory.read8(memoryAddress, &info.cycleCount);
                } else {
                    // 2: LDR  Rd,[Rb,Ro]   ;load  32bit data  Rd = WORD[Rb+Ro]
                    state.accessReg(rd) = state.memory.read32(memoryAddress, &info.cycleCount);
                }

                info.cycleCount += 1;
            } else {
                if (b) {
                    // 1: STRB Rd,[Rb,Ro]   ;store  8bit data  BYTE[Rb+Ro] = Rd
                    state.memory.write8(memoryAddress, state.accessReg(rd) & 0x0F, &info.cycleCount);
                } else {
                    // 0: STR  Rd,[Rb,Ro]   ;store 32bit data  WORD[Rb+Ro] = Rd
                    state.memory.write32(memoryAddress, state.accessReg(rd), &info.cycleCount);
                }

                info.noDefaultSCycle = true;
                info.additionalProgCyclesN = 1;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbLoadPCRelative(uint8_t rd, uint8_t offset)
        {
            InstructionExecutionInfo info{0};

            // 7-0    nn - Unsigned offset        (0-1020 in steps of 4)
            uint32_t memoryAddress = ((state.accessReg(regs::PC_OFFSET) + 4) & ~2) + (offset << 2);
            //  LDR Rd,[PC,#nn]      ;load 32bit    Rd = WORD[PC+nn]
            state.accessReg(rd) = state.memory.read32(memoryAddress, &info.cycleCount);

            // Execution Time: 1S+1N+1I
            info.cycleCount += 1;

            return info;
        }

        InstructionExecutionInfo handleThumbAddSubtract(thumb::ThumbInstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset)
        {
            auto currentRegs = state.getCurrentRegs();

            uint32_t rsVal = *currentRegs[rs];
            uint32_t rnVal = *currentRegs[rn_offset];

            uint64_t result;

            switch (insID) {
                case thumb::ADD:
                    result = rsVal + rnVal;
                    break;
                case thumb::SUB:
                    result = static_cast<uint32_t>(rsVal) - rnVal;
                    break;
                case thumb::ADD_SHORT_IMM:
                    result = rsVal + rn_offset;
                    break;
                case thumb::SUB_SHORT_IMM:
                    result = static_cast<uint32_t>(rsVal) - rn_offset;
                    break;
                default:
                    break;
            }

            *currentRegs[rd] = result;

            setFlags(
                result,
                true,
                true,
                true,
                true);

            InstructionExecutionInfo info{0};
            return info;
        }

        InstructionExecutionInfo handleThumbMovCmpAddSubImm(thumb::ThumbInstructionID ins, uint8_t rd, uint8_t offset)
        {
            // ARM equivalents for MOV/CMP/ADD/SUB are MOVS/CMP/ADDS/SUBS same format.

            arm::ARMInstruction armIns;
            armIns.params.data_proc_psr_transf.i = true;
            armIns.params.data_proc_psr_transf.s = true;
            armIns.params.data_proc_psr_transf.rd = rd;
            armIns.params.data_proc_psr_transf.rn = rd;
            armIns.params.data_proc_psr_transf.operand2 = offset;

            switch (ins) {
                case thumb::ADD:
                    armIns.id = arm::ADD;
                    break;
                case thumb::SUB:
                    armIns.id = arm::SUB;
                    break;
                case thumb::CMP:
                    armIns.id = arm::CMP;
                    break;
                case thumb::MOV:
                    armIns.id = arm::MOV;
                    break;

                default:
                    break;
            }

            return execDataProc(armIns);
        }

        InstructionExecutionInfo handleThumbMoveShiftedReg(thumb::ThumbInstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset)
        {

            // Zero shift amount is having special meaning (same as for ARM shifts),
            // LSL#0 performs no shift (the carry flag remains unchanged), LSR/ASR#0
            // are interpreted as LSR/ASR#32. Attempts to specify LSR/ASR#0 in source
            // code are automatically redirected as LSL#0, and source LSR/ASR#32 is
            // redirected as opcode LSR/ASR#0.
            if (ins != thumb::LSL && offset == 0) {
                offset = 32;
            }

            uint64_t rsValue = static_cast<uint64_t>(state.accessReg(rs));
            uint64_t rdValue = 0;

            arm::ShiftType shiftType = arm::ShiftType::LSL;
            switch (ins) {
                case thumb::LSL:
                    shiftType = arm::ShiftType::LSL;
                    break;
                case thumb::LSR:
                    shiftType = arm::ShiftType::LSR;
                    break;
                case thumb::ASR:
                    shiftType = arm::ShiftType::ASR;
                    break;
                default:
                    break;
            }
            rdValue = arm::shift(rsValue, shiftType, offset, state.getFlag(cpsr_flags::C_FLAG), true);

            state.accessReg(rd) = static_cast<uint32_t>(rdValue & 0x0FFFFFFFF);

            // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
            setFlags(
                rdValue,
                true,                              // n Flag
                true,                              // z Flag
                false,                             // v Flag
                ins != thumb::LSL || offset != 0); // c flag

            // Execution Time: 1S
            InstructionExecutionInfo info{0};
            return info;
        }

        InstructionExecutionInfo handleThumbBranchXCHG(thumb::ThumbInstructionID id, uint8_t rd, uint8_t rs)
        {
            InstructionExecutionInfo info{0};

            auto currentRegs = state.getCurrentRegs();

            uint32_t rsValue = *currentRegs[rs] + (rs == 15 ? 4 : 0);
            uint32_t rdValue = *currentRegs[rd] + (rd == 15 ? 4 : 0);

            if (rd == 15 && (id == thumb::ADD || id == thumb::MOV)) {
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;
            }

            switch (id) {
                case thumb::ADD:
                    *currentRegs[rd] = rdValue + rsValue;
                    break;

                case thumb::CMP: {

                    uint64_t result = static_cast<uint64_t>(rdValue) - static_cast<uint64_t>(rsValue);

                    setFlags(result,
                             true,
                             true,
                             true,
                             true);
                    break;
                }

                case thumb::MOV:
                    *currentRegs[rd] = rsValue;
                    break;

                case thumb::BX: {
                    // If the first bit of rs is set
                    bool stayInThumbMode = rsValue & 0x00000001;

                    if (!stayInThumbMode) {
                        state.setFlag(cpsr_flags::THUMB_STATE, false);
                    }

                    // Except for BX R15: CPU switches to ARM state, and PC is auto-aligned as (($+4) AND NOT 2).
                    if (rs == 15) {
                        rsValue &= ~2;
                    }

                    // Change the PC to the address given by rs. Note that we have to mask out the thumb switch bit.
                    state.accessReg(regs::PC_OFFSET) = rsValue & ~1;
                    info.additionalProgCyclesN = 1;
                    info.additionalProgCyclesS = 1;

                    break;
                }

                case thumb::NOP:
                default:
                    break;
            }

            return info;
        }

        InstructionExecutionInfo handleThumbALUops(thumb::ThumbInstructionID instID, uint8_t rs, uint8_t rd)
        {

            static const std::set<thumb::ThumbInstructionID> updateNegative{
                thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

                thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR,

                thumb::MUL, thumb::AND, thumb::EOR, thumb::TST, thumb::ORR, thumb::BIC, thumb::MVN};

            static const std::set<thumb::ThumbInstructionID> updateZero{
                thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

                thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR,

                thumb::MUL, thumb::AND, thumb::EOR, thumb::TST, thumb::ORR, thumb::BIC, thumb::MVN};

            static const std::set<thumb::ThumbInstructionID> updateCarry{
                thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

                thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR};

            static const std::set<thumb::ThumbInstructionID> updateOverflow{
                thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN};

            static const std::set<thumb::ThumbInstructionID> dontUpdateRD{
                thumb::TST, thumb::CMP, thumb::CMN};

            static const std::set<thumb::ThumbInstructionID> shiftOps{
                thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR};

            InstructionExecutionInfo info{0};

            if (shiftOps.find(instID) != shiftOps.end()) {
                info.cycleCount = 1;
            }

            auto currentRegs = state.getCurrentRegs();

            uint64_t rsValue = *currentRegs[rs];
            uint64_t rdValue = *currentRegs[rd];
            uint64_t resultValue;

            uint8_t shiftAmount = rsValue & 0xFF;

            bool carry = state.getFlag(cpsr_flags::C_FLAG);

            switch (instID) {

                case thumb::ADC:
                    resultValue = rdValue + rsValue + (carry ? 1 : 0);
                    break;
                case thumb::SBC:
                    resultValue = rdValue - rsValue - (carry ? 0 : 1);
                    break;
                case thumb::NEG:
                    resultValue = 0 - rsValue;
                    break;
                case thumb::CMP:
                    resultValue = rdValue - rsValue;
                    break;
                case thumb::CMN:
                    resultValue = rdValue + rsValue;
                    break;

                    //TODO disabled edge cases because they were not mentioned in the corresponding problemkaputt.de/gbatek.htm section
                case thumb::LSL:
                    resultValue = arm::shift(rdValue, arm::ShiftType::LSL, shiftAmount, carry, false);
                    break;
                case thumb::LSR:
                    resultValue = arm::shift(rdValue, arm::ShiftType::LSR, shiftAmount, carry, false);
                    break;
                case thumb::ASR:
                    resultValue = arm::shift(rdValue, arm::ShiftType::ASR, shiftAmount, carry, false);
                    break;
                case thumb::ROR:
                    resultValue = arm::shift(rdValue, arm::ShiftType::ROR, shiftAmount, carry, false);
                    break;

                case thumb::MUL: {
                    resultValue = rdValue * rsValue;

                    if (((rsValue >> 8) & 0x00FFFFFF) == 0 || ((rsValue >> 8) & 0x00FFFFFF) == 0x00FFFFFF) {
                        info.cycleCount += 1;
                    } else if (((rsValue >> 16) & 0x0000FFFF) == 0 || ((rsValue >> 16) & 0x0000FFFF) == 0x0000FFFF) {
                        info.cycleCount += 2;
                    } else if (((rsValue >> 24) & 0x000000FF) == 0 || ((rsValue >> 24) & 0x000000FF) == 0x000000FF) {
                        info.cycleCount += 3;
                    } else {
                        info.cycleCount += 4;
                    }
                    break;
                }
                case thumb::TST:
                case thumb::AND:
                    resultValue = rdValue & rsValue;
                    break;
                case thumb::EOR:
                    resultValue = rdValue ^ rsValue;
                    break;
                case thumb::ORR:
                    resultValue = rdValue | rsValue;
                    break;
                case thumb::BIC:
                    resultValue = rdValue & ~rsValue;
                    break;
                case thumb::MVN:
                    resultValue = ~rsValue;
                    break;

                default:
                    break;
            }

            setFlags(
                resultValue,
                updateNegative.find(instID) != updateNegative.end(),
                updateZero.find(instID) != updateZero.end(),
                updateOverflow.find(instID) != updateOverflow.end(),
                updateCarry.find(instID) != updateCarry.end() && (shiftOps.find(instID) != shiftOps.end() || shiftAmount != 0));

            if (dontUpdateRD.find(instID) == dontUpdateRD.end())
                *currentRegs[rd] = resultValue;

            return info;
        }
    }; // namespace gbaemu

} // namespace gbaemu

#endif /* CPU_HPP */