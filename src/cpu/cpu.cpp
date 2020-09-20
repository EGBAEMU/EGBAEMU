#include "cpu.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace gbaemu
{
    CPU::CPU() : dma0(DMA::DMA0, this), dma1(DMA::DMA1, this), dma2(DMA::DMA2, this), dma3(DMA::DMA3, this), timerGroup(this), irqHandler(this), keypad(this)
    {
        reset();
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

        uint32_t pc = state.accessReg(regs::PC_OFFSET);

        //TODO we might need this info? (where nullptr is currently)
        if (thumbMode) {
            pc += 4;
            state.pipeline.fetch.instruction = state.memory.read16(pc, nullptr, false, true);
        } else {
            pc += 8;
            state.pipeline.fetch.instruction = state.memory.read32(pc, nullptr, false, true);

            // auto update bios state if we are currently executing inside bios!
            if (pc < state.memory.getBiosSize()) {
                state.memory.setBiosState(state.pipeline.fetch.instruction);
            }
        }
    }

    void CPU::decode()
    {
        state.pipeline.decode.lastInstruction = state.pipeline.decode.instruction;
        state.pipeline.decode.instruction = state.pipeline.fetch.lastInstruction;
    }

    CPUExecutionInfoType CPU::step()
    {
        //TODO is it important if it gets executed first or last?
        timerGroup.step();

        if (dmaInfo.cycleCount == 0) {
            if (cpuInfo.cycleCount != 0 || (dma0.step(dmaInfo) && dma1.step(dmaInfo) && dma2.step(dmaInfo) && dma3.step(dmaInfo))) {
                if (cpuInfo.haltCPU) {
                    //TODO this can be removed if we remove swi.cpp
                    cpuInfo.haltCPU = !irqHandler.checkForHaltCondition(cpuInfo.haltCondition);
                } else {
                    // Execute pipeline only after stall is over
                    if (cpuInfo.cycleCount == 0) {
                        irqHandler.checkForInterrupt();
                        fetch();
                        decode();

                        std::fill_n(reinterpret_cast<char *>(&cpuInfo), sizeof(cpuInfo), 0);

                        uint32_t prevPC = state.getCurrentPC();
                        execute();

                        // Current cycle must be removed
                        --cpuInfo.cycleCount;

                        if (cpuInfo.hasCausedException) {
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
                        --cpuInfo.cycleCount;
                    }
                }
            }
        }
        if (dmaInfo.cycleCount) {
            --dmaInfo.cycleCount;
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
        bool overflow = msbOp1 == msbOp2 && (negative != msbOp1);
        bool carry = resultValue & (static_cast<uint64_t>(1) << 32);

        if (nFlag)
            state.setFlag(cpsr_flags::N_FLAG, negative);

        if (zFlag)
            state.setFlag(cpsr_flags::Z_FLAG, zero);

        if (vFlag)
            state.setFlag(cpsr_flags::V_FLAG, overflow);

        if (cFlag) {
            state.setFlag(cpsr_flags::C_FLAG, carry != invertCarry);
        }
    }

    uint32_t CPU::normalizePC(bool thumbMode)
    {
        uint8_t bytes = thumbMode ? sizeof(uint16_t) : sizeof(uint32_t);
        uint32_t patchedPC = state.accessReg(regs::PC_OFFSET) = state.memory.normalizeAddress(state.accessReg(regs::PC_OFFSET) & (thumbMode ? 0xFFFFFFFE : 0xFFFFFFFC), executionMemReg);
        waitStatesSeq = state.memory.cyclesForVirtualAddrSeq(executionMemReg, bytes);
        waitStatesNonSeq = state.memory.cyclesForVirtualAddrNonSeq(executionMemReg, bytes);
        return patchedPC;
    }

    void CPU::execute()
    {
        static Instruction inst;

        const uint32_t prevPc = state.getCurrentPC();
        const bool prevThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        state.decoder->decode(state.pipeline.decode.lastInstruction, inst);
        if (!inst.isValid()) {
            std::cout << "ERROR: Decoded instruction is invalid: [" << std::hex << state.pipeline.decode.lastInstruction << "] @ " << state.accessReg(regs::PC_OFFSET);
            cpuInfo.hasCausedException = true;
            return;
        }

        if (inst.isArmInstruction()) {
            arm::ARMInstruction &armInst = inst.inst.arm;
            if (conditionSatisfied(armInst.condition, state))
                armExecuteHandler[inst.inst.arm.cat](armInst, this);
        } else {
            thumb::ThumbInstruction &thumbInst = inst.inst.thumb;
            thumbExecuteHandler[inst.inst.thumb.cat](thumbInst, this);
        }

        const bool postThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);
        uint32_t postPc = state.accessReg(regs::PC_OFFSET);

        // Add 1S cycle needed to fetch a instruction if not other requested
        // Handle wait cycles!
        if (!cpuInfo.noDefaultSCycle) {
            cpuInfo.cycleCount += waitStatesSeq;
        }

        // Change from arm mode to thumb mode or vice versa
        if (prevThumbMode != postThumbMode) {
            if (postThumbMode) {
                state.decoder = &thumbDecoder;
            } else {
                state.decoder = &armDecoder;
            }
            cpuInfo.forceBranch = true;
        }

        // We have a branch, return or something that changed our PC
        if (cpuInfo.forceBranch || prevPc != postPc) {
            postPc = normalizePC(postThumbMode);
            initPipeline();
        } else {
            // Increment the pc counter to the next instruction
            state.accessReg(regs::PC_OFFSET) = postPc + (postThumbMode ? 2 : 4);
        }

        cpuInfo.cycleCount += waitStatesNonSeq * cpuInfo.additionalProgCyclesN;
        cpuInfo.cycleCount += waitStatesSeq * cpuInfo.additionalProgCyclesS;

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
        uint8_t modeBits = state.accessReg(regs::CPSR_OFFSET) & cpsr_flags::MODE_BIT_MASK & 0xF;
        switch (modeBits) {
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
            case 0b0111:
                state.mode = CPUState::AbortMode;
                break;
            case 0b1011:
                state.mode = CPUState::UndefinedMode;
                break;
            case 0b1111:
                state.mode = CPUState::SystemMode;
                break;

            default:
                std::cout << "ERROR: invalid mode bits: 0x" << std::hex << static_cast<uint32_t>(state.accessReg(regs::CPSR_OFFSET) & cpsr_flags::MODE_BIT_MASK) << " prevPC: 0x" << std::hex << prevPc << std::endl;
                cpuInfo.hasCausedException = true;
                break;
        }

        if (executionMemReg == Memory::BIOS && postPc >= state.memory.getBiosSize()) {
            std::cout << "CRITIAL ERROR: PC points to bios address outside of our code! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            cpuInfo.hasCausedException = true;
            return;
        } else if (executionMemReg == Memory::OUT_OF_ROM) {
            std::cout << "CRITIAL ERROR: PC points out to address out of its ROM bounds! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            cpuInfo.hasCausedException = true;
            return;
        }
    }

    void CPU::reset()
    {
        state.reset();
        dma0.reset();
        dma1.reset();
        dma2.reset();
        dma3.reset();
        timerGroup.reset();
        irqHandler.reset();
        keypad.reset();

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

        std::fill_n(reinterpret_cast<char *>(&cpuInfo), sizeof(cpuInfo), 0);
        std::fill_n(reinterpret_cast<char *>(&dmaInfo), sizeof(dmaInfo), 0);

        state.accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
        normalizePC(false);
    }
} // namespace gbaemu
