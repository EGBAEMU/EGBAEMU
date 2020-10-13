#include "cpu.hpp"

#include "decode/inst.hpp"
#include "decode/inst_arm.hpp"
#include "decode/inst_thumb.hpp"
#include "lcd/lcd-controller.hpp"
#include "logging.hpp"
#include "swi.hpp"

#include <algorithm>
#include <sstream>

namespace gbaemu
{

    CPU::CPU() : dmaGroup(this), timerGroup(this), irqHandler(this), keypad(this)
    {
        reset();
    }

    void CPU::setLCDController(lcd::LCDController *lcdController)
    {
        dmaGroup.setLCDController(lcdController);
        state.memory.ioHandler.lcdController = lcdController;
    }

    void CPU::handleInvalid(uint32_t inst)
    {
        (void)inst;
        state.executionInfo.message << "ERROR: trying to execute invalid instruction!" << std::endl;
        state.execState = CPUState::EXEC_ERROR;
    }

    template <bool thumb>
    void CPU::softwareInterrupt(uint32_t inst)
    {
        uint8_t index;

        if (thumb)
            index = inst & 0x0FF;
        else
            index = static_cast<uint8_t>((inst & 0x00FFFFFF) >> 16);

        if (state.memory.usesExternalBios()) {
            swi::callBiosCodeSWIHandler(this);
        } else {
            if (index < sizeof(swi::biosCallHandler) / sizeof(swi::biosCallHandler[0])) {
                if (index != 5 && index != 0x2B) {
                    LOG_SWI(std::cout << "Info: trying to call bios handler: " << swi::biosCallHandlerStr[index] << " at PC: 0x" << std::hex << (state.getCurrentPC() - 4) << std::endl;);
                }
                swi::biosCallHandler[index](this);
            } else {
                state.executionInfo.message << "ERROR: trying to call invalid bios call handler: " << std::hex << index << " at PC: 0x" << std::hex << (state.getCurrentPC() - 4) << std::endl;
                state.execState = CPUState::EXEC_ERROR;
            }
        }
    }

// Trust me, you dont want to look at it :D
#include "rep_case_constexpr_makros.h"

    CPUExecutionInfoType CPU::step(uint32_t cycles)
    {
        cyclesLeft += cycles;

        uint32_t prevPC;

        while (cyclesLeft > 0) {

            switch (state.execState) {
                // We have 5 state bits that may interleave -> 2^5 cases = 32
                REP_CASE_CONSTEXPR(32, uint8_t, 0, execStep<offset>(prevPC));
                default:
                    state.executionInfo.message << "ERROR unhandled CPU state: 0x" << std::hex << static_cast<uint32_t>(state.execState) << std::endl;
                    // Fall through
                case CPUState::EXEC_ERROR:
                    state.executionInfo.message << "ERROR: Instruction at: 0x" << std::hex << prevPC << " has caused an exception\n";
                    state.executionInfo.infoType = CPUExecutionInfoType::EXCEPTION;
                    return CPUExecutionInfoType::EXCEPTION;
                    break;
            }
        }

        return CPUExecutionInfoType::NORMAL;
    }

    // Use a template so that most ifs are constexpr -> better loop performance
    template <uint8_t execState>
    void CPU::execStep(uint32_t &prevPC)
    {
        uint32_t currentPC = state.getCurrentPC();

        do {
            prevPC = currentPC;
            state.cpuInfo.cycleCount = 0;

            // If dma executes cpu is stalled!
            if (execState & CPUState::EXEC_DMA) {
                dmaGroup.step(state.cpuInfo, cyclesLeft);
            } else {
                if (execState & CPUState::EXEC_HALT) {
                    irqHandler.checkForHaltCondition(state.haltCondition);
                    state.cpuInfo.cycleCount = 1;
                } else {
                    // We can only execute the interrupt if not disabled by CPSR register and if so we need a state change because we need to change into arm mode!
                    if (execState & CPUState::EXEC_IRQ && !state.getFlag<cpsr_flags::IRQ_DISABLE>()) {
                        //TODO how long does irq handler call take? additionally to pipeline flush?
                        irqHandler.callIRQHandler();
                        // we jump to bios, so there must be a currentPC update even if the state does not change!
                        currentPC = state.getCurrentPC();
                    } else {
                        // forward the pipeline
                        uint32_t inst = state.pipeline[1];
                        state.pipeline[1] = state.pipeline[0];

                        if (execState & CPUState::EXEC_THUMB) {
                            // fetch new instruction to fill the pipeline
                            // already increment PC
                            state.getPC() = currentPC + 2;
                            state.pipeline[0] = state.memory.readInst16(currentPC + 4, state.cpuInfo);
                            (this->*thumbExeLUT[hashThumb(inst)])(inst);
                        } else {
                            // fetch new instruction to fill the pipeline
                            // already increment PC
                            state.getPC() = currentPC + 4;
                            state.pipeline[0] = state.memory.readInst32(currentPC + 8, state.cpuInfo);
                            if (conditionSatisfied(static_cast<ConditionOPCode>(inst >> 28), state)) {
                                (this->*armExeLUT[hashArm(inst)])(inst);
                            }
                        }
                        currentPC = state.getCurrentPC();

#if false
                        // PC sanity checks
                        if (state.memory.extractMemoryRegion(currentPC) == memory::BIOS && currentPC >= state.memory.getBiosSize()) {
                            state.executionInfo.message << "CRITIAL ERROR: PC(0x" << std::hex << currentPC << ") points to bios address outside of our code! Aborting!" << std::endl;
                            state.execState = CPUState::EXEC_ERROR;
                        } else if (state.memory.extractMemoryRegion(currentPC) >= memory::EXT_ROM1 && currentPC >= memory::EXT_ROM_OFFSET + state.memory.getRomSize()) {
                            state.executionInfo.message << "CRITIAL ERROR: PC(0x" << std::hex << currentPC << ") points out to address out of its ROM bounds! Aborting!" << std::endl;
                            state.execState = CPUState::EXEC_ERROR;
                        }
#endif
                    }
                }
            }

            // always execute the timer with the given time that lapsed
            if (execState & CPUState::EXEC_TIMER) {
                timerGroup.step(state.cpuInfo.cycleCount);
            }

            cyclesLeft -= state.cpuInfo.cycleCount;
        } while (cyclesLeft > 0 && execState == state.execState);
    }

    void CPU::patchFetchToNCycle()
    {
        // Fetch S cycle needs to be a N cycle -> add difference off N and S Cycle count to our cycle count
        state.cpuInfo.cycleCount += state.nonSeqCycles - state.seqCycles;
    }

    void CPU::refillPipeline()
    {
        if (state.getFlag<cpsr_flags::THUMB_STATE>()) {
            refillPipelineAfterBranch<true>();
        } else {
            refillPipelineAfterBranch<false>();
        }
    }

    template <bool thumbMode>
    void CPU::refillPipelineAfterBranch()
    {
        // If an instruction jumps to a different memory area,
        // then all code cycles for that opcode are having waitstate characteristics
        // of the NEW memory area (except Thumb BL which still executes 1S in OLD area).
        // -> we have to undo the fetch added cycles and add it again after initPipeline
        state.cpuInfo.cycleCount -= state.seqCycles;

        // We need to fill the pipeline to the state where the instruction at PC is ready for execution -> fetched + decoded!
        uint32_t pc = state.normalizePC<thumbMode>();
        state.memory.setExecInsideBios(false);
        if (thumbMode) {
            state.pipeline[1] = state.memory.readInst16(pc, state.cpuInfo);
            state.pipeline[0] = state.memory.readInst16(pc + 2, state.cpuInfo);
            state.seqCycles = state.memory.memCycles16(state.cpuInfo.memReg, true);
            state.nonSeqCycles = state.memory.memCycles16(state.cpuInfo.memReg, false);
        } else {
            state.pipeline[1] = state.memory.readInst32(pc, state.cpuInfo);
            state.pipeline[0] = state.memory.readInst32(pc + 4, state.cpuInfo);
            state.seqCycles = state.memory.memCycles32(state.cpuInfo.memReg, true);
            state.nonSeqCycles = state.memory.memCycles32(state.cpuInfo.memReg, false);
        }

        // Replace first S cycle of pipeline fetch by an N cycle as its random access!
        // Also apply additional 1S into new region
        state.cpuInfo.cycleCount += state.nonSeqCycles;
    }

    void CPU::reset()
    {
        state.reset();
        dmaGroup.reset();
        timerGroup.reset();
        irqHandler.reset();
        keypad.reset();

        state.memory.ioHandler.cpu = this;

        cyclesLeft = 0;
    }

    template void CPU::refillPipelineAfterBranch<true>();
    template void CPU::refillPipelineAfterBranch<false>();

    template void CPU::softwareInterrupt<true>(uint32_t);
    template void CPU::softwareInterrupt<false>(uint32_t);
} // namespace gbaemu
