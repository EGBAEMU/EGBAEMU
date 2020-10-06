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

    void CPU::setLCDController(const lcd::LCDController *lcdController)
    {
        dmaGroup.setLCDController(lcdController);
    }

    void CPU::initPipeline()
    {
        // We need to fill the pipeline to the state where the instruction at PC is ready for execution -> fetched + decoded!
        uint32_t pc = state.accessReg(regs::PC_OFFSET);
        bool thumbMode = state.getFlag<cpsr_flags::THUMB_STATE>();
        state.propagatePipeline(pc - (thumbMode ? 4 : 8));
        state.propagatePipeline(pc - (thumbMode ? 2 : 4));
    }

    CPUExecutionInfoType CPU::step(uint32_t cycles)
    {
        cyclesLeft += cycles;

        if (cyclesLeft > 0) {
            dmaGroup.step(state.dmaInfo, cyclesLeft);

            cyclesLeft -= state.dmaInfo.cycleCount;
            timerGroup.step(state.dmaInfo.cycleCount);
            state.dmaInfo.cycleCount = 0;

            if (state.memory.usesExternalBios()) {
                while (cyclesLeft > 0) {
                    // check if we need to call the irq routine
                    irqHandler.checkForInterrupt();

                    // clear all fields in cpuInfo
                    const constexpr InstructionExecutionInfo zeroInitExecInfo{0};
                    state.cpuInfo = zeroInitExecInfo;

                    uint32_t prevPC = state.getCurrentPC();
                    execute(state.propagatePipeline(prevPC), prevPC);

                    if (state.cpuInfo.hasCausedException) {
                        //TODO print cause
                        //TODO set cause in memory class

                        //TODO maybe return reason? as this might be needed to exit a game?
                        // Abort
                        std::stringstream ss;
                        ss << "ERROR: Instruction at: 0x" << std::hex << prevPC << " has caused an exception\n";

                        executionInfo = CPUExecutionInfo(EXCEPTION, ss.str());

                        return CPUExecutionInfoType::EXCEPTION;
                    }

                    timerGroup.step(state.cpuInfo.cycleCount);

                    cyclesLeft -= state.cpuInfo.cycleCount;

                    if (cyclesLeft > 0) {
                        dmaGroup.step(state.dmaInfo, cyclesLeft);

                        cyclesLeft -= state.dmaInfo.cycleCount;
                        timerGroup.step(state.dmaInfo.cycleCount);
                        state.dmaInfo.cycleCount = 0;
                    }
                }
            } else {
                while (cyclesLeft > 0) {
                    if (state.cpuInfo.haltCPU) {
                        //TODO this can be removed if we remove swi.cpp
                        state.cpuInfo.haltCPU = !irqHandler.checkForHaltCondition(state.cpuInfo.haltCondition);
                        state.cpuInfo.cycleCount = 1;
                    } else {
                        // check if we need to call the irq routine
                        irqHandler.checkForInterrupt();

                        // clear all fields in cpuInfo
                        const constexpr InstructionExecutionInfo zeroInitExecInfo{0};
                        state.cpuInfo = zeroInitExecInfo;

                        uint32_t prevPC = state.getCurrentPC();
                        execute(state.propagatePipeline(prevPC), prevPC);

                        if (state.cpuInfo.hasCausedException) {
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

                    timerGroup.step(state.cpuInfo.cycleCount);

                    cyclesLeft -= state.cpuInfo.cycleCount;

                    if (cyclesLeft > 0) {
                        dmaGroup.step(state.dmaInfo, cyclesLeft);

                        cyclesLeft -= state.dmaInfo.cycleCount;
                        timerGroup.step(state.dmaInfo.cycleCount);
                        state.dmaInfo.cycleCount = 0;
                    }
                }
            }
        }

        return CPUExecutionInfoType::NORMAL;
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
            state.cpuInfo.forceBranch = true;
        }

        // We have a branch, return or something that changed our PC
        if (state.cpuInfo.forceBranch || prevPc != postPc) {
            // If an instruction jumps to a different memory area,
            // then all code cycles for that opcode are having waitstate characteristics
            // of the NEW memory area (except Thumb BL which still executes 1S in OLD area).
            // -> we have to undo the fetch added cycles and add it again after initPipeline
            state.cpuInfo.cycleCount -= state.memory.memCycles16(state.fetchInfo.memReg, true);
            postPc = state.normalizePC(postThumbMode);
            initPipeline();
            // Replace first S cycle of pipeline fetch by an N cycle as its random access!
            state.cpuInfo.cycleCount += state.memory.memCycles16(state.fetchInfo.memReg, false) - state.memory.memCycles16(state.fetchInfo.memReg, true);
            // Also apply additional 1S into new region
            state.cpuInfo.cycleCount += state.memory.memCycles16(state.fetchInfo.memReg, true);
        } else {
            // Increment the pc counter to the next instruction
            state.accessReg(regs::PC_OFFSET) = postPc + (postThumbMode ? 2 : 4);
        }

        // Add 1S cycle needed to fetch a instruction if not other requested
        // Handle wait cycles!
        state.cpuInfo.cycleCount += state.fetchInfo.cycleCount;
        state.fetchInfo.cycleCount = 0;

        if (state.cpuInfo.noDefaultSCycle) {
            // Fetch S cycle needs to be a N cycle -> add difference off N and S Cycle count to our cycle count
            state.cpuInfo.cycleCount += state.memory.memCycles16(state.fetchInfo.memReg, false) - state.memory.memCycles16(state.fetchInfo.memReg, true);
        }

        // sanity checks
        if (state.updateCPUMode()) {
            std::cout << "ERROR: invalid mode bits: 0x" << std::hex << static_cast<uint32_t>(state.accessReg(regs::CPSR_OFFSET) & cpsr_flags::MODE_BIT_MASK) << " prevPC: 0x" << std::hex << prevPc << std::endl;
            state.cpuInfo.hasCausedException = true;
            return;
        }
        if (state.fetchInfo.memReg == Memory::BIOS && postPc >= state.memory.getBiosSize()) {
            std::cout << "CRITIAL ERROR: PC points to bios address outside of our code! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            state.cpuInfo.hasCausedException = true;
            return;
        } else if (state.fetchInfo.memReg == Memory::OUT_OF_ROM) {
            std::cout << "CRITIAL ERROR: PC points out to address out of its ROM bounds! Aborting! PrevPC: 0x" << std::hex << prevPc << std::endl;
            state.cpuInfo.hasCausedException = true;
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

        cyclesLeft = 0;
    }
} // namespace gbaemu
