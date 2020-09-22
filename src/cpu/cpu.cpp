#include "cpu.hpp"

#include "cpu_arm_executor.hpp"
#include "cpu_thumb_executor.hpp"
#include "decode/inst_arm.hpp"
#include "decode/inst_thumb.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace gbaemu
{
    arm::ArmExecutor CPU::armExecutor;
    thumb::ThumbExecutor CPU::thumbExecutor;

    InstructionDecoder CPU::armDecoder = [](uint32_t inst) {
        if (conditionSatisfied(static_cast<ConditionOPCode>(inst >> 28), CPU::armExecutor.cpu->state)) {
            arm::ARMInstructionDecoder<arm::ArmExecutor>::decode<CPU::armExecutor>(inst);
        }
    };
    InstructionDecoder CPU::thumbDecoder = &thumb::ThumbInstructionDecoder<thumb::ThumbExecutor>::decode<CPU::thumbExecutor>;

    CPU::CPU() : dma0(DMA::DMA0, this), dma1(DMA::DMA1, this), dma2(DMA::DMA2, this), dma3(DMA::DMA3, this), timerGroup(this), irqHandler(this), keypad(this)
    {
        reset();
    }

    void CPU::initPipeline()
    {
        // We need to fill the pipeline to the state where the instruction at PC is ready for execution -> fetched + decoded!
        uint32_t pc = state.accessReg(regs::PC_OFFSET);
        bool thumbMode = state.getFlag(cpsr_flags::THUMB_STATE);
        propagatePipeline(pc - (thumbMode ? 4 : 8));
        propagatePipeline(pc - (thumbMode ? 2 : 4));
    }

    uint32_t CPU::propagatePipeline(uint32_t pc)
    {
        // propagate pipeline
        uint32_t currentInst = state.pipeline[1];
        state.pipeline[1] = state.pipeline[0];

        bool thumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        //TODO we might need this info? (where nullptr is currently)
        if (thumbMode) {
            pc += 4;
            state.pipeline[0] = state.memory.read16(pc, nullptr, false, true);
        } else {
            pc += 8;
            state.pipeline[0] = state.memory.read32(pc, nullptr, false, true);

            // auto update bios state if we are currently executing inside bios!
            if (pc < state.memory.getBiosSize()) {
                state.memory.setBiosState(state.pipeline[0]);
            }
        }

        return currentInst;
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

                        std::fill_n(reinterpret_cast<char *>(&cpuInfo), sizeof(cpuInfo), 0);

                        uint32_t prevPC = state.getCurrentPC();
                        execute(propagatePipeline(prevPC), prevPC);

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

    uint32_t CPU::normalizePC(bool thumbMode)
    {
        uint8_t bytes = thumbMode ? sizeof(uint16_t) : sizeof(uint32_t);
        uint32_t patchedPC = state.accessReg(regs::PC_OFFSET) = state.memory.normalizeAddress(state.accessReg(regs::PC_OFFSET) & (thumbMode ? 0xFFFFFFFE : 0xFFFFFFFC), executionMemReg);
        waitStatesSeq = state.memory.cyclesForVirtualAddrSeq(executionMemReg, bytes);
        waitStatesNonSeq = state.memory.cyclesForVirtualAddrNonSeq(executionMemReg, bytes);
        return patchedPC;
    }

    void CPU::execute(uint32_t inst, uint32_t prevPc)
    {
        const bool prevThumbMode = state.getFlag(cpsr_flags::THUMB_STATE);

        decoder(inst);

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
                decoder = thumbDecoder;
            } else {
                decoder = armDecoder;
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

        if (state.updateCPUMode()) {
            std::cout << "ERROR: invalid mode bits: 0x" << std::hex << static_cast<uint32_t>(state.accessReg(regs::CPSR_OFFSET) & cpsr_flags::MODE_BIT_MASK) << " prevPC: 0x" << std::hex << prevPc << std::endl;
            cpuInfo.hasCausedException = true;
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

        decoder = armDecoder;
        armExecutor.cpu = this;
        thumbExecutor.cpu = this;
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
