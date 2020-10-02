#include "cpu.hpp"

#include "cpu_arm_executor.hpp"
#include "cpu_thumb_executor.hpp"
#include "decode/inst_arm.hpp"
#include "decode/inst_thumb.hpp"
#include "lcd/lcd-controller.hpp"

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

    CPU::CPU() : dmaGroup(this), timerGroup(this), irqHandler(this), keypad(this)
    {
        reset();
    }

    void CPU::setLCDController(const lcd::LCDController *lcdController) {
        dmaGroup.setLCDController(lcdController);
    }

    void CPU::initPipeline()
    {
        // We need to fill the pipeline to the state where the instruction at PC is ready for execution -> fetched + decoded!
        uint32_t pc = state.accessReg(regs::PC_OFFSET);
        bool thumbMode = state.getFlag<cpsr_flags::THUMB_STATE>();
        propagatePipeline(pc - (thumbMode ? 4 : 8));
        propagatePipeline(pc - (thumbMode ? 2 : 4));
    }

    uint32_t CPU::propagatePipeline(uint32_t pc)
    {
        // propagate pipeline
        uint32_t currentInst = state.pipeline[1];
        state.pipeline[1] = state.pipeline[0];

        bool thumbMode = state.getFlag<cpsr_flags::THUMB_STATE>();

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

    CPUExecutionInfoType CPU::step(uint32_t cycles)
    {
        cyclesLeft += cycles;

        if (cyclesLeft > 0) {
            dmaGroup.step(dmaInfo, cyclesLeft);

            cyclesLeft -= dmaInfo.cycleCount;
            timerGroup.step(dmaInfo.cycleCount);
            dmaInfo.cycleCount = 0;

            while (cyclesLeft > 0) {
                if (cpuInfo.haltCPU) {
                    //TODO this can be removed if we remove swi.cpp
                    cpuInfo.haltCPU = !irqHandler.checkForHaltCondition(cpuInfo.haltCondition);
                    cpuInfo.cycleCount = 1;
                } else {
                    // check if we need to call the irq routine
                    irqHandler.checkForInterrupt();

                    // clear all fields in cpuInfo
                    const constexpr InstructionExecutionInfo zeroInitExecInfo{0};
                    cpuInfo = zeroInitExecInfo;

                    uint32_t prevPC = state.getCurrentPC();
                    execute(propagatePipeline(prevPC), prevPC);

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
                }

                timerGroup.step(cpuInfo.cycleCount);

                cyclesLeft -= cpuInfo.cycleCount;

                if (cyclesLeft > 0) {
                    dmaGroup.step(dmaInfo, cyclesLeft);

                    cyclesLeft -= dmaInfo.cycleCount;
                    timerGroup.step(dmaInfo.cycleCount);
                    dmaInfo.cycleCount = 0;
                }
            }
        }

        return CPUExecutionInfoType::NORMAL;
    }

    uint32_t CPU::normalizePC(bool thumbMode)
    {
        uint32_t patchedPC = state.accessReg(regs::PC_OFFSET) = state.memory.normalizeAddress(state.accessReg(regs::PC_OFFSET) & (thumbMode ? 0xFFFFFFFE : 0xFFFFFFFC), executionMemReg);
        if (thumbMode) {
            waitStatesSeq = state.memory.memCycles16(executionMemReg, true);
            waitStatesNonSeq = state.memory.memCycles16(executionMemReg, false);
        } else {
            waitStatesSeq = state.memory.memCycles32(executionMemReg, true);
            waitStatesNonSeq = state.memory.memCycles32(executionMemReg, false);
        }
        return patchedPC;
    }

    void CPU::execute(uint32_t inst, uint32_t prevPc)
    {
        const bool prevThumbMode = state.getFlag<cpsr_flags::THUMB_STATE>();

        decoder(inst);

        const bool postThumbMode = state.getFlag<cpsr_flags::THUMB_STATE>();
        uint32_t postPc = state.accessReg(regs::PC_OFFSET);

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

        // Add 1S cycle needed to fetch a instruction if not other requested
        // Handle wait cycles!
        if (!cpuInfo.noDefaultSCycle) {
            cpuInfo.cycleCount += waitStatesSeq;
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
        dmaGroup.reset();
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
        *state.getModeRegs(CPUState::CPUMode::FIQ)[regs::SP_OFFSET] = 0x03007F00;
        *state.getModeRegs(CPUState::CPUMode::AbortMode)[regs::SP_OFFSET] = 0x03007F00;
        *state.getModeRegs(CPUState::CPUMode::UndefinedMode)[regs::SP_OFFSET] = 0x03007F00;
        *state.getModeRegs(CPUState::CPUMode::SupervisorMode)[regs::SP_OFFSET] = 0x03007FE0;
        *state.getModeRegs(CPUState::CPUMode::IRQ)[regs::SP_OFFSET] = 0x3007FA0;

        std::fill_n(reinterpret_cast<char *>(&cpuInfo), sizeof(cpuInfo), 0);
        std::fill_n(reinterpret_cast<char *>(&dmaInfo), sizeof(dmaInfo), 0);

        state.accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
        normalizePC(false);

        cyclesLeft = 0;
    }
} // namespace gbaemu
