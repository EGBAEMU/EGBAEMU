#include "cpu.hpp"

#include "swi.hpp"
#include <cassert>
#include <limits>
#include <sstream>

namespace gbaemu
{
    CPU::CPU() : dma0(DMA::DMA0, this), dma1(DMA::DMA1, this), dma2(DMA::DMA2, this), dma3(DMA::DMA3, this), timerGroup(this), irqHandler(this)
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

    void CPU::initPipeline()
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

    void CPU::fetch()
    {
        // propagate pipeline
        state.pipeline.fetch.lastInstruction = state.pipeline.fetch.instruction;

        bool thumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        //TODO we only need to fetch 16 bit for thumb mode!
        //TODO we might need this info? (where nullptr is currently)
        if (thumbMode) {
            //TODO check this
            /* pc is at [27:1] */
            state.pipeline.fetch.instruction = state.memory.read16(state.accessReg(regs::PC_OFFSET) + 4, nullptr, false, true);
        } else {
            /* pc is at [27:2] */
            state.pipeline.fetch.instruction = state.memory.read32(state.accessReg(regs::PC_OFFSET) + 8, nullptr, false, true);
        }
    }

    void CPU::decode()
    {
        state.pipeline.decode.lastInstruction = state.pipeline.decode.instruction;
        state.pipeline.decode.instruction = state.decoder->decode(state.pipeline.fetch.lastInstruction);
    }

    CPUExecutionInfoType CPU::step()
    {
        static InstructionExecutionInfo info{0};
        static InstructionExecutionInfo dmaInfo{0};

        //TODO is it important if it gets executed first or last?
        timerGroup.step();

        if (dmaInfo.cycleCount == 0 && info.cycleCount == 0) {
            dmaInfo = dma0.step();
            if (!dmaInfo.dmaExecutes) {
                dmaInfo = dma1.step();
                if (!dmaInfo.dmaExecutes) {
                    dmaInfo = dma2.step();
                    if (!dmaInfo.dmaExecutes) {
                        dmaInfo = dma3.step();
                    }
                }
            }
        }
        if (dmaInfo.cycleCount) {
            --dmaInfo.cycleCount;
        }

        if (!dmaInfo.dmaExecutes) {
            if (info.haltCPU) {
                info.haltCPU = !irqHandler.checkForHaltCondition(info.haltCondition);
            } else {
                // Execute pipeline only after stall is over
                if (info.cycleCount == 0) {
                    irqHandler.checkForInterrupt();
                    fetch();
                    decode();
                    
                    uint32_t prevPC = state.getCurrentPC();
                    info = execute();

                    // Current cycle must be removed
                    --info.cycleCount;

                    if (info.hasCausedException) {
                        //TODO print cause
                        //TODO set cause in memory class

                        //TODO maybe return reason? as this might be needed to exit a game?
                        // Abort
                        std::stringstream ss;
                        ss << "ERROR: Instruction at: 0x" << std::hex << prevPC << " has caused an exception\n";

                        executionInfo = CPUExecutionInfo(EXCEPTION, ss.str());

                        return CPUExecutionInfoType::EXCEPTION;
                    }
                } else {
                    --info.cycleCount;
                }
            }
        }

        return CPUExecutionInfoType::NORMAL;
    }

    void CPU::setFlags(uint64_t resultValue, bool msbOp1, bool msbOp2, bool nFlag, bool zFlag, bool vFlag, bool cFlag, bool invertCarry)
    {
        /*
            The arithmetic operations (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN) treat each
            operand as a 32 bit integer (either unsigned or 2’s complement signed, the two are equivalent).
            the V flag in the CPSR will be set if
            an overflow occurs into bit 31 of the result; this may be ignored if the operands were
            considered unsigned, but warns of a possible error if the operands were 2’s
            complement signed. The C flag will be set to the carry out of bit 31 of the ALU, the Z
            flag will be set if and only if the result was zero, and the N flag will be set to the value
            of bit 31 of the result (indicating a negative result if the operands are considered to be
            2’s complement signed).
            */
        bool negative = (resultValue) & (static_cast<uint64_t>(1) << 31);
        bool zero = (resultValue & 0x0FFFFFFFF) == 0;
        bool overflow = msbOp1 == msbOp2 && (negative ^ msbOp1);
        bool carry = resultValue & (static_cast<uint64_t>(1) << 32);

        if (nFlag)
            state.setFlag(cpsr_flags::N_FLAG, negative);

        if (zFlag)
            state.setFlag(cpsr_flags::Z_FLAG, zero);

        if (vFlag)
            state.setFlag(cpsr_flags::V_FLAG, overflow);

        if (cFlag) {
            state.setFlag(cpsr_flags::C_FLAG, carry ^ invertCarry);
        }
    }

    InstructionExecutionInfo CPU::execute()
    {
        InstructionExecutionInfo info{0};

        const uint32_t prevPc = state.getCurrentPC();
        const bool prevThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        if (state.pipeline.decode.lastInstruction.isArmInstruction()) {
            if (state.pipeline.decode.lastInstruction.arm.id == InstructionID::INVALID) {
                std::cout << "ERROR: trying to execute invalid ARM instruction! PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
                info.hasCausedException = true;
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
                            info = execHalfwordDataTransferImmRegSignedTransfer(
                                armInst.params.hw_transf_reg_off.p,
                                armInst.params.hw_transf_reg_off.u,
                                armInst.params.hw_transf_reg_off.l,
                                armInst.params.hw_transf_reg_off.w,
                                false,
                                armInst.params.hw_transf_reg_off.rn,
                                armInst.params.hw_transf_reg_off.rd,
                                state.accessReg(armInst.params.hw_transf_reg_off.rm),
                                16);
                            break;
                        case arm::ARMInstructionCategory::HW_TRANSF_IMM_OFF:
                            info = execHalfwordDataTransferImmRegSignedTransfer(
                                armInst.params.hw_transf_imm_off.p,
                                armInst.params.hw_transf_imm_off.u,
                                armInst.params.hw_transf_imm_off.l,
                                armInst.params.hw_transf_imm_off.w,
                                false,
                                armInst.params.hw_transf_imm_off.rn,
                                armInst.params.hw_transf_imm_off.rd,
                                armInst.params.hw_transf_imm_off.offset,
                                16);
                            break;
                        case arm::ARMInstructionCategory::SIGN_TRANSF:
                            info = execHalfwordDataTransferImmRegSignedTransfer(
                                armInst.params.sign_transf.p,
                                armInst.params.sign_transf.u,
                                armInst.params.sign_transf.l,
                                armInst.params.sign_transf.w,
                                true,
                                armInst.params.sign_transf.rn,
                                armInst.params.sign_transf.rd,
                                armInst.params.sign_transf.b ? armInst.params.sign_transf.addrMode : state.accessReg(armInst.params.sign_transf.addrMode & 0x0F),
                                armInst.params.sign_transf.h ? 16 : 8);
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
                                if (index != 5 && index != 0x2B)
                                    std::cout << "Info: trying to call bios handler: " << swi::biosCallHandlerStr[index] << " at PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
                                info = swi::biosCallHandler[index](this);
                            } else {
                                std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << " at PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
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
            if (state.pipeline.decode.lastInstruction.thumb.id == InstructionID::INVALID) {
                std::cout << "ERROR: trying to execute invalid THUMB instruction! PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
                info.hasCausedException = true;
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
                        // info = handleThumbLoadPCRelative(thumbInst.params.pc_ld.rd, thumbInst.params.pc_ld.offset);
                        // break;
                    case thumb::ThumbInstructionCategory::LD_ST_REL_OFF:
                        // info = handleThumbLoadStoreRegisterOffset(thumbInst.params.ld_st_rel_off.l, thumbInst.params.ld_st_rel_off.b, thumbInst.params.ld_st_rel_off.ro, thumbInst.params.ld_st_rel_off.rb, thumbInst.params.ld_st_rel_off.rd);
                        // break;
                    case thumb::ThumbInstructionCategory::LD_ST_IMM_OFF:
                        // info = handleThumbLoadStoreImmOff(thumbInst.params.ld_st_imm_off.l, thumbInst.params.ld_st_imm_off.b, thumbInst.params.ld_st_imm_off.offset, thumbInst.params.ld_st_imm_off.rb, thumbInst.params.ld_st_imm_off.rd);
                        // break;
                    case thumb::ThumbInstructionCategory::LD_ST_REL_SP:
                        // info = handleThumbLoadStoreSPRelative(thumbInst.params.ld_st_rel_sp.l, thumbInst.params.ld_st_rel_sp.rd, thumbInst.params.ld_st_rel_sp.offset);
                        info = handleThumbLoadStore(thumbInst);
                        break;
                    case thumb::ThumbInstructionCategory::LD_ST_SIGN_EXT:
                        // info = handleThumbLoadStoreSignExt(thumbInst.params.ld_st_sign_ext.h, thumbInst.params.ld_st_sign_ext.s, thumbInst.params.ld_st_sign_ext.ro, thumbInst.params.ld_st_sign_ext.rb, thumbInst.params.ld_st_sign_ext.rd);
                        // break;
                    case thumb::ThumbInstructionCategory::LD_ST_HW:
                        // info = handleThumbLoadStoreHalfword(thumbInst.params.ld_st_hw.l, thumbInst.params.ld_st_hw.offset, thumbInst.params.ld_st_hw.rb, thumbInst.params.ld_st_hw.rd);
                        info = handleThumbLoadStoreSignHalfword(thumbInst);
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
                            if (index != 5 && index != 0x2B)
                                std::cout << "Info: trying to call bios handler: " << swi::biosCallHandlerStr[index] << " at PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
                            info = swi::biosCallHandler[index](this);
                        } else {
                            std::cout << "ERROR: trying to call invalid bios call handler: " << std::hex << index << " at PC: 0x" << std::hex << state.getCurrentPC() << std::endl;
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

        const bool postThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        /*
        Mis-aligned PC/R15 (branch opcodes, or MOV/ALU/LDR with Rd=R15)
        For ARM code, the low bits of the target address should be usually zero, otherwise, R15 is forcibly aligned by clearing the lower two bits.
        For THUMB code, the low bit of the target address may/should/must be set, the bit is (or is not) interpreted as thumb-bit (depending on the opcode), and R15 is then forcibly aligned by clearing the lower bit.
        In short, R15 will be always forcibly aligned, so mis-aligned branches won't have effect on subsequent opcodes that use R15, or [R15+disp] as operand.
        */
        // Ensure that pc is word / halfword aligned & apply normalization to handle mirroring
        Memory::MemoryRegion memReg;
        const uint32_t postPc = state.accessReg(regs::PC_OFFSET) = state.memory.normalizeAddress(state.accessReg(regs::PC_OFFSET) & (postThumbMode ? 0xFFFFFFFE : 0xFFFFFFFC), memReg);

        if (memReg == Memory::BIOS && postPc >= state.memory.getBiosSize()) {
            std::cout << "CRITIAL ERROR: PC points to bios address outside of our code! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            info.hasCausedException = true;
            return info;
        } else if (memReg == Memory::OUT_OF_ROM) {
            std::cout << "CRITIAL ERROR: PC points out to address out of its ROM bounds! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            info.hasCausedException = true;
            return info;
        }

        // Add 1S cycle needed to fetch a instruction if not other requested
        // Handle wait cycles!
        if (!info.noDefaultSCycle) {
            info.cycleCount += state.memory.cyclesForVirtualAddrSeq(postPc, prevThumbMode ? sizeof(uint16_t) : sizeof(uint32_t));
        }
        info.cycleCount += state.memory.cyclesForVirtualAddrNonSeq(postPc, postThumbMode ? sizeof(uint16_t) : sizeof(uint32_t)) * info.additionalProgCyclesN;
        info.cycleCount += state.memory.cyclesForVirtualAddrSeq(postPc, postThumbMode ? sizeof(uint16_t) : sizeof(uint32_t)) * info.additionalProgCyclesS;

        // Change from arm mode to thumb mode or vice versa
        if (prevThumbMode != postThumbMode) {
            if (postThumbMode) {
                state.decoder = &thumbDecoder;
            } else {
                state.decoder = &armDecoder;
            }
            //TODO can we assume that on change the pc counter will always be modified as well?
        }
        // We have a branch, return or something that changed our PC
        if (info.forceBranch || prevPc != postPc) {
            initPipeline();
        } else {
            //TODO this is probably unwanted if we changed the mode?
            if (prevThumbMode != postThumbMode) {
                std::cout << "WARNING: mode change, but no PC change-> no flush occurred & wrong mode code will be executed! At PrevPC: 0x" << std::hex << prevPc << std::endl;
            }
            // Increment the pc counter to the next instruction
            state.accessReg(regs::PC_OFFSET) = postPc + (postThumbMode ? 2 : 4);
        }

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
                std::cout << "ERROR: invalid mode bits: 0x" << std::hex << static_cast<uint32_t>(modeBits) << " prevPC: 0x" << std::hex << prevPc << std::endl;
                break;
        }

        return info;
    }
} // namespace gbaemu
