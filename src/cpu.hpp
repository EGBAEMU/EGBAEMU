#ifndef CPU_HPP
#define CPU_HPP

#include "inst_arm.hpp"
#include "inst_thumb.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace gbaemu
{
struct CPUState {
    /*
        https://problemkaputt.de/gbatek.htm#armcpuregisterset
        https://static.docs.arm.com/dvi0027/b/DVI_0027A_ARM7TDMI_PO.pdf
     */
    // Register R0 is shared in all modes

    /* constants */
    static const size_t SP_OFFSET = 13;
    static const size_t LR_OFFSET = 14;
    static const size_t PC_OFFSET = 15;
    static const size_t CPSR_OFFSET = 16;

    enum OperationState : uint8_t {
        ARMState,
        ThumbState
    } operationState;

    enum CPUMode : uint8_t {
        UserMode,
        FIQ,
        IRQ,
        SupervisorMode,
        AbortMode,
        UndefinedMode,
        SystemMode
    } mode;

    struct Regs {
        /*
            PC is r15 and has following bit layout:
            Bit   Name     Expl.
            31-28 N,Z,C,V  Flags (Sign, Zero, Carry, Overflow)
            27-26 I,F      Interrupt Disable bits (IRQ, FIQ) (1=Disable)
            25-2  PC       Program Counter, 24bit, Step 4 (64M range)
            1-0   M1,M0    Mode (0=User, 1=FIQ, 2=IRQ, 3=Supervisor)
         */
        uint32_t rx[16];
        uint32_t r8_14_fig[7];
        uint32_t r13_14_svc[2];
        uint32_t r13_14_abt[2];
        uint32_t r13_14_irq[2];
        uint32_t r13_14_und[2];
        uint32_t CPSR;
        uint32_t SPSR_fiq;
        uint32_t SPSR_svc;
        uint32_t SPSR_abt;
        uint32_t SPSR_irq;
        uint32_t SPSR_und;
    } __attribute__((packed)) regs;

    // Complain to: tammo.muermann@stud.tu-darmstadt.de
    uint32_t *const regsHacks[7][18] = {
        // System / User mode regs
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.rx + 13, regs.rx + 14, regs.rx + 15, &regs.CPSR, &regs.CPSR},
        // FIQ mode
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.r8_14_fig, regs.r8_14_fig + 1, regs.r8_14_fig + 2, regs.r8_14_fig + 3, regs.r8_14_fig + 4, regs.r8_14_fig + 5, regs.r8_14_fig + 6, regs.rx + 15, &regs.CPSR, &regs.SPSR_fiq},
        // IRQ mode
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_irq, regs.r13_14_irq + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_irq},
        // Supervisor Mode
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_svc, regs.r13_14_svc + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_svc},
        // Abort mode
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_abt, regs.r13_14_abt + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_abt},
        // Undefined Mode
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.r13_14_und, regs.r13_14_und + 1, regs.rx + 15, &regs.CPSR, &regs.SPSR_abt},
        // System / User mode regs
        {regs.rx, regs.rx + 1, regs.rx + 2, regs.rx + 3, regs.rx + 4, regs.rx + 5, regs.rx + 6, regs.rx + 7, regs.rx + 8, regs.rx + 9, regs.rx + 10, regs.rx + 11, regs.rx + 12, regs.rx + 13, regs.rx + 14, regs.rx + 15, &regs.CPSR, &regs.CPSR}};

    /* pipeline */
    struct Pipeline {
        struct {
            uint32_t lastInstruction;
            uint32_t lastReadData;

            uint32_t instruction;
            uint32_t readData;
        } fetch;

        struct {
            // TODO: create parent to unify that shit
            ARMInstruction instruction;
            ThumbInstruction instruction;
        } decode;
        /*
        struct {
            uint32_t result;
        } execute;
        */
    } pipeline;

    /* memory */
    struct Memory {
        /* keep it C-like */
        void *mem;
        size_t memSize;
    } memory;

    std::string toString() const
    {
        return "";
    }
};

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
        uint32_t pc = (state.regs.rx[CPUState::PC_OFFSET] >> 2) & 0x03FFFFFF;
        state.pipeline.fetch.instruction = static_cast<uint32_t *>(state.memory.mem)[pc];
    }

    void decode(uint32_t lastInst)
    {
    }

    void execute()
    {
        // TODO: Handle thumb / arm instructions
        // ARM: Bit 20:27 + 4:7 encode the instruction
        /*
        if (executeMe(state.pipeline.fetch.instruction, *(state.regsHacks[CPUState::CPSR_OFFSET]))) {
            //TODO execute
            Instruction instruction;

//TODO how to handle flags?... those need to be set
            if (multiplyAcc) {
                state.regsHacks[instruction.params.multiply.rd] = state.regsHacks[instruction.params.multiply.rn] +
                                                                  state.regsHacks[instruction.params.multiply.rs] * state.regsHacks[instruction.params.multiply.rm];
            }
        } else {
            //TODO execute nop
        }
         */
    }

    void step()
    {
        // TODO: Check for interrupt here
        //fetch();
        //decode();
        //execute();
    }
};

} // namespace gbaemu

#endif /* CPU_HPP */