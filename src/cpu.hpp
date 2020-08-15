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

    // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
    /*
        Current Program Status Register (CPSR)
          Bit   Expl.
          31    N - Sign Flag       (0=Not Signed, 1=Signed)               ;\
          30    Z - Zero Flag       (0=Not Zero, 1=Zero)                   ; Condition
          29    C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow) ; Code Flags
          28    V - Overflow Flag   (0=No Overflow, 1=Overflow)            ;/
          27    Q - Sticky Overflow (1=Sticky Overflow, ARMv5TE and up only)
          26-8  Reserved            (For future use) - Do not change manually!
          7     I - IRQ disable     (0=Enable, 1=Disable)                     ;\
          6     F - FIQ disable     (0=Enable, 1=Disable)                     ; Control
          5     T - State Bit       (0=ARM, 1=THUMB) - Do not change manually!; Bits
          4-0   M4-M0 - Mode Bits   (See below)
          
          Bit 31-28: Condition Code Flags (N,Z,C,V)
          
            These bits reflect results of logical or arithmetic instructions. In ARM mode, it is often optionally whether an instruction should modify flags or not, for example, it is possible to execute a SUB instruction that does NOT modify the condition flags.
            In ARM state, all instructions can be executed conditionally depending on the settings of the flags, such like MOVEQ (Move if Z=1). While In THUMB state, only Branch instructions (jumps) can be made conditionally.

            Bit 27: Sticky Overflow Flag (Q) - ARMv5TE and ARMv5TExP and up only
                Used by QADD, QSUB, QDADD, QDSUB, SMLAxy, and SMLAWy only. These opcodes set the Q-flag in case of overflows, but leave it unchanged otherwise. The Q-flag can be tested/reset by MSR/MRS opcodes only.

            Bit 27-8: Reserved Bits (except Bit 27 on ARMv5TE and up, see above)
                These bits are reserved for possible future implementations. For best forwards compatibility, the user should never change the state of these bits, and should not expect these bits to be set to a specific value.

            Bit 7-0: Control Bits (I,F,T,M4-M0)
                These bits may change when an exception occurs. In privileged modes (non-user modes) they may be also changed manually.
                The interrupt bits I and F are used to disable IRQ and FIQ interrupts respectively (a setting of "1" means disabled).
                The T Bit signalizes the current state of the CPU (0=ARM, 1=THUMB), this bit should never be changed manually - instead, changing between ARM and THUMB state must be done by BX instructions.
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
    static const size_t FLAG_N_OFFSET = 31;
    static const size_t FLAG_Z_OFFSET = 30;
    static const size_t FLAG_C_OFFSET = 29;
    static const size_t FLAG_V_OFFSET = 28;
    static const size_t FLAG_Q_OFFSET = 27;
    static const size_t IRQ_DISABLE_OFFSET = 7;
    static const size_t IRQ_DISABLE_OFFSET = 6;
    static const size_t IRQ_DISABLE_OFFSET = 5;
    static const size_t MODE_BIT_MASK = 0x01F;

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
            ThumbInstruction instruction2;
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

    void setFlag(size_t flag)
    {
        state.regs.rx[CPUState::CPSR_OFFSET] |= (1 << flag);
    }

    void clearFlags()
    {
    }

    void execAdd(bool s, uint32_t rn, uint32_t rd, uint32_t shiftOperand)
    {
        uint8_t currentMode = 2;

        auto currentRegs = reinterpret_cast<int32_t *const *const>(state.regsHacks[currentMode]);

        // Get the value of the rn register
        int32_t rnValueSigned = *currentRegs[rn];
        // The value of the shift operand as signed int
        int32_t shiftOperandSigned = static_cast<int32_t>(shiftOperand);

        // Construt the sum. Give it some more bits so we can catch an overflow.
        int64_t resultSigned = rnValueSigned + shiftOperandSigned;
        uint64_t result = static_cast<uint32_t>(resultSigned);

        // Write the value back to rd
        *currentRegs[rd] = static_cast<uint32_t>(result & 0x00000000FFFFFFFF);

        // TODO Maybe clear flags?

        // If s is set, we have to update the N, Z, V and C Flag
        if (s) {
            if (result == 0) {
                setFlag(CPUState::FLAG_Z_OFFSET);
            }
            if (result < 0) {
                setFlag(CPUState::FLAG_N_OFFSET)
            }
            // If there is a bit set in the upper half there must have been an overflow (i guess)
            if (result & 0xFFFFFFFF00000000) {
                setFlag(CPUState::FLAG_C_OFFSET)
            }
        }
    }
};

} // namespace gbaemu

#endif /* CPU_HPP */