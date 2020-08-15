#ifndef CPU_HPP
#define CPU_HPP

#include "inst_arm.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace gbaemu
{

// TODO verify bitmasks
static const uint32_t CPSR_N_FLAG_BITMASK = 0x80000000;
static const uint32_t CPSR_Z_FLAG_BITMASK = 0x40000000;
static const uint32_t CPSR_C_FLAG_BITMASK = 0x20000000;
static const uint32_t CPSR_V_FLAG_BITMASK = 0x10000000;
static const uint32_t CPSR_THUMB_FLAG_BITMASK = 0x00000010;

enum ConditionOPCode : uint8_t {
    // Equal Z==1
    EQ,
    // Not equal Z==0
    NE,
    // Carry set / unsigned higher or same C==1
    CS_HS,
    // Carry clear / unsigned lower C==0
    CC_LO,
    // Minus / negative N==1
    MI,
    // Plus / positive or zero N==0
    PL,
    // Overflow V==1
    VS,
    // No overflow V==0
    VC,
    // Unsigned higher (C==1) AND (Z==0)
    HI,
    // Unsigned lower or same (C==0) OR (Z==1)
    LS,
    // Signed greater than or equal N == V
    GE,
    // Signed less than N != V
    LT,
    // Signed greater than (Z==0) AND (N==V)
    GT,
    // Signed less than or equal (Z==1) OR (N!=V)
    LE,
    // Always (unconditional) Not applicable
    AL,
    // Never Obsolete, unpredictable in ARM7TDMI
    NV
};

// THUMB INSTRUCTION SET
/*
*/
// Move shifted register
static const uint16_t MASK_THUMB_MOV_SHIFT = 0b1110000000000000;
static const uint16_t VAL_THUMB_MOV_SHIFT = 0b0000000000000000;
// Add and subtract
static const uint16_t MASK_THUMB_ADD_SUB = 0b1111110000000000;
static const uint16_t VAL_THUMB_ADD_SUB = 0b0001110000000000;
// Move, compare, add, and subtract immediate
static const uint16_t MASK_THUMB_MOV_CMP_ADD_SUB_IMM = 0b1110000000000000;
static const uint16_t VAL_THUMB_MOV_CMP_ADD_SUB_IMM = 0b0010000000000000;
// ALU operation
static const uint16_t MASK_THUMB_ALU_OP = 0b1111110000000000;
static const uint16_t VAL_THUMB_ALU_OP = 0b0100000000000000;
// High register operations and branch exchange
static const uint16_t MASK_THUMB_BR_XCHG = 0b1111110000000000;
static const uint16_t VAL_THUMB_BR_XCHG = 0b0100010000000000;
// PC-relative load
static const uint16_t MASK_THUMB_PC_LD = 0b1111100000000000;
static const uint16_t VAL_THUMB_PC_LD = 0b0100100000000000;
// Load and store with relative offset
static const uint16_t MASK_THUMB_LD_ST_REL_OFF = 0b1111001000000000;
static const uint16_t VAL_THUMB_LD_ST_REL_OFF = 0b0101000000000000;
// Load and store sign-extended byte and halfword
static const uint16_t MASK_THUMB_LD_ST_SIGN_EXT = 0b1111001000000000;
static const uint16_t VAL_THUMB_LD_ST_SIGN_EXT = 0b0101001000000000;
// Load and store with immediate offset
static const uint16_t MASK_THUMB_LD_ST_IMM_OFF = 0b1110000000000000;
static const uint16_t VAL_THUMB_LD_ST_IMM_OFF = 0b0110000000000000;
// Load and store halfword
static const uint16_t MASK_THUMB_LD_ST_HW = 0b1111000000000000;
static const uint16_t VAL_THUMB_LD_ST_HW = 0b1000000000000000;
//SP-relative load and store
static const uint16_t MASK_THUMB_LD_ST_REL_SP = 0b1111000000000000;
static const uint16_t VAL_THUMB_LD_ST_REL_SP = 0b1001000000000000;
// Load address
static const uint16_t MASK_THUMB_LOAD_ADDR = 0b1111000000000000;
static const uint16_t VAL_THUMB_LOAD_ADDR = 0b1010000000000000;
// Add offset to stack pointer
static const uint16_t MASK_THUMB_ADD_OFFSET_TO_STACK_PTR = 0b1111111100000000;
static const uint16_t VAL_THUMB_ADD_OFFSET_TO_STACK_PTR = 0b1011000000000000;
// Push and pop registers
static const uint16_t MASK_THUMB_PUSH_POP_REG = 0b1111011000000000;
static const uint16_t VAL_THUMB_PUSH_POP_REG = 0b1011010000000000;
// Multiple load and store
static const uint16_t MASK_THUMB_MULT_LOAD_STORE = 0b1111000000000000;
static const uint16_t VAL_THUMB_MULT_LOAD_STORE = 0b1100000000000000;
// Conditional Branch
static const uint16_t MASK_THUMB_COND_BRANCH = 0b1111000000000000;
static const uint16_t VAL_THUMB_COND_BRANCH = 0b1101000000000000;
// Software interrupt
static const uint16_t MASK_THUMB_SOFTWARE_INTERRUPT = 0b1111111100000000;
static const uint16_t VAL_THUMB_SOFTWARE_INTERRUPT = 0b1101111100000000;
// Unconditional branch
static const uint16_t MASK_THUMB_UNCONDITIONAL_BRANCH = 0b1111100000000000;
static const uint16_t VAL_THUMB_UNCONDITIONAL_BRANCH = 0b1110000000000000;
// Long branch with link
static const uint16_t MASK_THUMB_LONG_BRANCH_WITH_LINK = 0b1111000000000000;
static const uint16_t VAL_THUMB_LONG_BRANCH_WITH_LINK = 0b1111000000000000;

// ARM INSTRUCTION SET
/*  NOTE: comparison order is important!

    Multiply (accumulate) cond 000000 A S Rd Rn Rs 1 0 0 1 Rm
    mask  0b0000_1111_1100_0000_0000_0000_1111_0000
    value 0b0000_0000_0000_0000_0000_0000_1001_0000
    
    Multiply (accumulate) long   cond 00001UAS Rd_MSW Rd_LSW Rn 1001Rm
    mask  0b0000_1111_1000_0000_0000_0000_1111_0000
    value 0b0000_0000_1000_0000_0000_0000_1001_0000
    
    Branch and exchangecond0001001011 1 1 111111110001Rn
    mask  0b0000_1111_1111_1111_1111_1111_1111_0000
    value 0b0000_0001_0010_1111_1111_1111_0001_0000

    Single data swap   cond 00010B00Rn Rd 00001001Rm
    mask  0b0000_1111_1011_0000_0000_1111_1111_0000
    value 0b0000_0001_0000_0000_0000_0000_1001_0000
    
    Halfword data transfer, register offset   cond 000PU0WLRn Rd 00001011Rm
    mask  0b0000_1110_0100_0000_0000_1111_1111_0000
    value 0b0000_0000_0000_0000_0000_0000_1011_0000
    
    Halfword data transfer, immediate offset   cond 000PU1WLRn Rd offset 1011    offset 
    mask  0b0000_1110_0100_0000_0000_0000_1111_0000
    value 0b0000_0000_0100_0000_0000_0000_1011_0000
    
    Signed data transfer (byte/halfword)   cond 000PUBWLRn Rd addr_mode11H1addr_mod
    mask  0b0000_1110_0000_0000_0000_0000_1101_0000
    value 0b0000_0000_0000_0000_0000_0000_1101_0000
    
    Data processing and PSR transfercond 0   0   Iopcode SRn Rd operand2 
    mask  0b0000_1100_0000_0000_0000_0000_0000_0000
    value 0b0000_0000_0000_0000_0000_0000_0000_0000
    
    Load/store register/unsigned bytecond    0 1 I P U B W L Rn Rd addr_mode
    mask  0b 0000 1100 0000 0000 0000 0000 0000 0000
    value 0b 0000 0100 0000 0000 0000 0000 0000 0000
    
    Undefined cond 0 1 1 1
    mask  0b 0000 1110 0000 0000 0000 0000 0001 0000
    value 0b 0000 0110 0000 0000 0000 0000 0001 0000
    
    Block data transfercond100PU0WLRnregisterli
    mask  0b 0000 1110 0100 0000 0000 0000 0000 0000
    value 0b 0000 1000 0000 0000 0000 0000 0000 0000

    Branch cond 1 0 1 L offset
    mask  0b 0000 1110 0000 0000 0000 0000 0000 0000
    value 0b 0000 1010 0000 0000 0000 0000 0000 0000
    
    Coprocessor data transfer cond 1 1 0 P U N W L Rn CRd CP# offset
    mask  0b 0000 1110 0000 0000 0000 0000 0000 0000
    value 0b 0000 1100 0000 0000 0000 0000 0000 0000
    
    Coprocessor data operationcond1110CP opcodeCRn CRd CP# CP 0CRm
    mask  0b 0000 1111 0000 0000 0000 0000 0001 0000
    value 0b 0000 1110 0000 0000 0000 0000 0000 0000
    
    Coprocessor register transfercond1110 CP opc   LCRn Rd CP# CP 1CRm
    mask  0b 0000 1111 0000 0000 0000 0000 0001 0000
    value 0b 0000 1110 0000 0000 0000 0000 0001 0000
    
    Software interrupt cond 1 1 1 1 ignored by processor
    mask  0b 0000 1111 0000 0000 0000 0000 0000 0000
    value 0b 0000 1111 0000 0000 0000 0000 0000 0000
 */
// Multiply (accumulate) cond 000000 A S Rd Rn Rs 1 0 0 1 Rm
static const uint32_t MASK_MUL_ACC = 0b00001111110000000000000011110000;
static const uint32_t VAL_MUL_ACC = 0b00000000000000000000000010010000;
// Multiply (accumulate) long   cond 00001UAS Rd_MSW Rd_LSW Rn 1001Rm
static const uint32_t MASK_MUL_ACC_LONG = 0b00001111100000000000000011110000;
static const uint32_t VAL_MUL_ACC_LONG = 0b00000000100000000000000010010000;
// Branch and exchangecond0001001011 1 1 111111110001Rn
static const uint32_t MASK_BRANCH_XCHG = 0b00001111111111111111111111110000;
static const uint32_t VAL_BRANCH_XCHG = 0b00000001001011111111111100010000;
// Single data swap   cond 00010B00Rn Rd 00001001Rm
static const uint32_t MASK_DATA_SWP = 0b00001111101100000000111111110000;
static const uint32_t VAL_DATA_SWP = 0b00000001000000000000000010010000;
// Halfword data transfer, register offset   cond 000PU0WLRn Rd 00001011Rm
static const uint32_t MASK_HW_TRANSF_REG_OFF = 0b00001110010000000000111111110000;
static const uint32_t VAL_HW_TRANSF_REG_OFF = 0b00000000000000000000000010110000;
// Halfword data transfer, immediate offset   cond 000PU1WLRn Rd offset 1011    offset
static const uint32_t MASK_HW_TRANSF_IMM_OFF = 0b00001110010000000000000011110000;
static const uint32_t VAL_HW_TRANSF_IMM_OFF = 0b00000000010000000000000010110000;
// Signed data transfer (byte/halfword)   cond 000PUBWLRn Rd addr_mode11H1addr_mod
static const uint32_t MASK_SIGN_TRANSF = 0b00001110000000000000000011010000;
static const uint32_t VAL_SIGN_TRANSF = 0b00000000000000000000000011010000;
// Data processing and PSR transfercond   0  Iopcode SRn Rd operand2
static const uint32_t MASK_DATA_PROC_PSR_TRANSF = 0b00001100000000000000000000000000;
static const uint32_t VAL_DATA_PROC_PSR_TRANSF = 0b00000000000000000000000000000000;
//  Load/store register/unsigned bytecond
static const uint32_t MASK_LS_REG_UBYTE = 0b00001100000000000000000000000000;
static const uint32_t VAL_LS_REG_UBYTE = 0b00000100000000000000000000000000;
// Undefined cond 01 1 1
static const uint32_t MASK_UNDEFINED = 0b00001110000000000000000000010000;
static const uint32_t VAL_UNDEFINED = 0b00000110000000000000000000010000;
// Block data transfercond100PU0WLRnregisterli
static const uint32_t MASK_BLOCK_DATA_TRANSF = 0b00001110010000000000000000000000;
static const uint32_t VAL_BLOCK_DATA_TRANSF = 0b00001000000000000000000000000000;
//Branch cond 1 01 L offset
static const uint32_t MASK_BRANCH = 0b00001110000000000000000000000000;
static const uint32_t VAL_BRANCH = 0b00001010000000000000000000000000;
// Coprocessor data transfer cond 1 1 0 P U N W L Rn CRd CP# offset
static const uint32_t MASK_COPROC_DATA_TRANSF = 0b00001110000000000000000000000000;
static const uint32_t VAL_COPROC_DATA_TRANSF = 0b00001100000000000000000000000000;
// Coprocessor data operationcond1110CP opcodeCRn CRd CP# CP 0CRm
static const uint32_t MASK_COPROC_OP = 0b00001111000000000000000000010000;
static const uint32_t VAL_COPROC_OP = 0b00001110000000000000000000000000;
// Coprocessor register transfercond1110 CP opc   LCRn Rd CP# CP 1CRm
static const uint32_t MASK_COPROC_REG_TRANSF = 0b00001111000000000000000000010000;
static const uint32_t VAL_COPROC_REG_TRANSF = 0b00001110000000000000000000010000;
// Software interrupt cond 1 1 1 1 ignored by processor
static const uint32_t MASK_SOFTWARE_INTERRUPT = 0b00001111000000000000000000000000;
static const uint32_t VAL_SOFTWARE_INTERRUPT = 0b00001111000000000000000000000000;

static bool executeMe(uint32_t instruction, uint32_t CPSR)
{

    ConditionOPCode executeCondition = static_cast<ConditionOPCode>(instruction >> 28);
    switch (executeCondition) {
        // Equal Z==1
        case EQ:
            return CPSR & CPSR_Z_FLAG_BITMASK;
            break;

        // Not equal Z==0
        case NE:
            return (CPSR & CPSR_Z_FLAG_BITMASK) == 0;
            break;

        // Carry set / unsigned higher or same C==1
        case CS_HS:
            return CPSR & CPSR_C_FLAG_BITMASK;
            break;

        // Carry clear / unsigned lower C==0
        case CC_LO:
            return (CPSR & CPSR_C_FLAG_BITMASK) == 0;
            break;

        // Minus / negative N==1
        case MI:
            return CPSR & CPSR_N_FLAG_BITMASK;
            break;

        // Plus / positive or zero N==0
        case PL:
            return (CPSR & CPSR_N_FLAG_BITMASK) == 0;
            break;

        // Overflow V==1
        case VS:
            return CPSR & CPSR_V_FLAG_BITMASK;
            break;

        // No overflow V==0
        case VC:
            return (CPSR & CPSR_V_FLAG_BITMASK) == 0;
            break;

        // Unsigned higher (C==1) AND (Z==0)
        case HI:
            return (CPSR & CPSR_C_FLAG_BITMASK) && (CPSR & CPSR_Z_FLAG_BITMASK);
            break;

        // Unsigned lower or same (C==0) OR (Z==1)
        case LS:
            return (CPSR & CPSR_C_FLAG_BITMASK) == 0 || (CPSR & CPSR_Z_FLAG_BITMASK);
            break;

        // Signed greater than or equal N == V
        case GE:
            return (bool)(CPSR & CPSR_N_FLAG_BITMASK) == (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed less than N != V
        case LT:
            return (bool)(CPSR & CPSR_N_FLAG_BITMASK) != (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed greater than (Z==0) AND (N==V)
        case GT:
            return (CPSR & CPSR_Z_FLAG_BITMASK) == 0 && (bool)(CPSR & CPSR_Z_FLAG_BITMASK) == (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Signed less than or equal (Z==1) OR (N!=V)
        case LE:
            return (CPSR & CPSR_Z_FLAG_BITMASK) || (bool)(CPSR & CPSR_Z_FLAG_BITMASK) != (bool)(CPSR & CPSR_V_FLAG_BITMASK);
            break;

        // Always (unconditional) Not applicable
        case AL:
            return true;
            break;

        // Never Obsolete, unpredictable in ARM7TDMI
        case NV:
        default:
            return false;
            break;
    }
}

enum OPCode : uint8_t {

};

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
    uint32_t *regsHacks[7][18] = {
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
            void *function;
            uint32_t arg1, arg2;
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

        //uint32_t lastInst = state.pipeline.fetch.lastInstruction;
        ConditionOPCode executeCondition = static_cast<ConditionOPCode>(lastInst >> 28);

        InstructionID id = InstructionID::INVALID;

        if (lastInst & MASK_MUL_ACC == VAL_MUL_ACC) {
            //TODO
            bool a = (lastInst >> 21) & 1;
            bool s = (lastInst >> 20) & 1;

            uint32_t rd = (lastInst >> 16) & 0xF;
            uint32_t rn = (lastInst >> 12) & 0xF;
            uint32_t rs = (lastInst >> 8) & 0xF;
            uint32_t rm = lastInst & 0xF;

            if (a) {
                id = InstructionID::MLA;
            } else {
                id = InstructionID::MUL;
            }
        } else if (lastInst & MASK_MUL_ACC_LONG == VAL_MUL_ACC_LONG) {
            //TODO
            bool u = (lastInst >> 22) & 1;
            bool a = (lastInst >> 21) & 1;
            bool s = (lastInst >> 20) & 1;

            uint32_t rd_msw = (lastInst >> 16) & 0xF;
            uint32_t rd_lsw = (lastInst >> 12) & 0xF;
            uint32_t rn = (lastInst >> 8) & 0xF;
            uint32_t rm = lastInst & 0xF;

            if (u && a) {
                id = InstructionID::SMLAL;
            } else if (u && !a) {
                id = InstructionID::SMULL;
            } else if (!u && a) {
                id = InstructionID::UMLAL;
            } else {
                id = InstructionID::UMULL;
            }
        } else if (lastInst & MASK_BRANCH_XCHG == VAL_BRANCH_XCHG) {
            id = InstructionID::BX;
        } else if (lastInst & MASK_DATA_SWP == VAL_DATA_SWP) {
            //TODO
            /* also called b */
            bool b = (lastInst >> 22) & 1;

            uint32_t rn = (lastInst >> 16) & 0xF;
            uint32_t rd = (lastInst >> 12) & 0xF;
            uint32_t rm = lastInst & 0xFF;

            if (!b) {
                id = InstructionID::SWP;
            } else {
                id = InstructionID::SWPB;
            }
        } else if (lastInst & MASK_HW_TRANSF_REG_OFF == VAL_HW_TRANSF_REG_OFF) {
            //TODO


        } else if (lastInst & MASK_HW_TRANSF_IMM_OFF == VAL_HW_TRANSF_IMM_OFF) {
            //TODO
            bool p = (lastInst >> 24) & 1;
            bool u = (lastInst >> 23) & 1;
            bool w = (lastInst >> 21) & 1;
            bool l = (lastInst >> 20) & 1;

            uint32_t rn = (lastInst >> 16) & 0xF;
            uint32_t rd = (lastInst >> 12) & 0xF;

            /* called addr_mode in detailed doc but is really offset because immediate flag I is 1 */
            uint32_t offset = ((lastInst >> 8) & 0xF) | (lastInst & 0xF);

            if (l) {
                id = InstructionID::LDRH;
            } else {
                id = InstructionID::STRH;
            }
        } else if (lastInst & MASK_SIGN_TRANSF == VAL_SIGN_TRANSF) {
            //TODO
            bool p = (lastInst >> 24) & 1;
            bool u = (lastInst >> 23) & 1;
            bool w = (lastInst >> 21) & 1;
            bool l = (lastInst >> 20) & 1;
            bool h = (lastInst >> 5) & 1;

            uint32_t rn = (lastInst >> 16) & 0xF;
            uint32_t rd = (lastInst >> 12) & 0xF;
            
            if (l && !h) {
                id = InstructionID::LDRSB;
            } else if (l && h) {
                id = InstructionID::LDRSH;
            }
        } else if (lastInst & MASK_DATA_PROC_PSR_TRANSF == VAL_DATA_PROC_PSR_TRANSF) {
            uint32_t opCode = (lastInst >> 21) & 0x0F;
            uint32_t rn = (lastInst >> 16) & 0x0F;
            uint32_t rd = (lastInst >> 12) & 0x0F;
            /* often shifter */
            uint32_t operand2 = lastInst & 0x0FFF;
            bool i = lastInst & (1 << 25);
            bool s = lastInst & (1 << 20);

//TODO take a second look
            switch (opCode) {
                case 0b0101:
                    id = InstructionID::ADC;
                    break;
                case 0b0100:
                    id = InstructionID::ADD;
                    break;
                case 0b0000:
                    id = InstructionID::AND;
                    break;
                case 0b1010:
                    if (s)
                        id = InstructionID::CMP;
                    break;
                case 0b1011:
                    id = InstructionID::CMN;
                    break;
                case 0b0001:
                    id = InstructionID::EOR;
                    break;
                case 0b1101:
                    id = InstructionID::MOV;
                    break;
                case 0b1110:
                    id = InstructionID::BIC;
                    break;
                case 0b1111:
                    id = InstructionID::MVN;
                    break;
                case 0b1100:
                    id = InstructionID::ORR;
                    break;
                case 0b0011:
                    id = InstructionID::RSB;
                    break;
                case 0b0111:
                    id = InstructionID::RSC;
                    break;
                case 0b0110:
                    id = InstructionID::SBC;
                    break;
                case 0b0010:
                    id = InstructionID::SUB;
                    break;
                case 0b1001:
                    if (s)
                        id = InstructionID::TEQ;
                    else
                        id = InstructionID::MSR;
                    break;
                case 0b1000:
                    if (s)
                        id = InstructionID::TST;
                    else
                        id = InstructionID::MSR;
                    break;
            }
        } else if (lastInst & MASK_LS_REG_UBYTE == VAL_LS_REG_UBYTE) {
            //TODO
            bool p = (lastInst >> 24) & 1;
            bool u = (lastInst >> 23) & 1;
            bool b = (lastInst >> 22) & 1;
            bool w = (lastInst >> 21) & 1;
            bool l = (lastInst >> 20) & 1;

            uint32_t rn = (lastInst >> 16) & 0xF;
            uint32_t rd = (lastInst >> 12) & 0xF;
            uint32_t addrMode = lastInst & 0xFF;

            if (!b && l) {
                id = InstructionID::LDR;
            } else if (b && l) {
                id = InstructionID::LDRB;
            } else if (!b && !l) {
                id = InstructionID::STR;
            } else {
                id = InstructionID::STRB;
            }
        } else if (lastInst & MASK_UNDEFINED == VAL_UNDEFINED) {
            //TODO
        } else if (lastInst & MASK_BLOCK_DATA_TRANSF == VAL_BLOCK_DATA_TRANSF) {
            //TODO
            bool p = (lastInst >> 24) & 1;
            bool u = (lastInst >> 23) & 1;
            bool w = (lastInst >> 21) & 1;
            bool l = (lastInst >> 20) & 1;

            uint32_t rn = (lastInst >> 16) & 0xF;
            uint32_t rList = lastInst & 0xFF;

            /* docs say there are two more distinct instructions in this category */
            if (l) {
                id = InstructionID::LDM;
            } else {
                id = InstructionID::STM;
            }
        } else if (lastInst & MASK_BRANCH == VAL_BRANCH) {
            uint32_t offset = lastInst & 0x00FFFFFF;
            bool l = (lastInst >> 24) & 1;

            id = InstructionID::B;
        } else if (lastInst & MASK_COPROC_DATA_TRANSF == VAL_COPROC_DATA_TRANSF) {
            //TODO
        } else if (lastInst & MASK_COPROC_OP == VAL_COPROC_OP) {
            //TODO
        } else if (lastInst & MASK_COPROC_REG_TRANSF == VAL_COPROC_REG_TRANSF) {
            //TODO
        } else if (lastInst & MASK_SOFTWARE_INTERRUPT == VAL_SOFTWARE_INTERRUPT) {
            //TODO
            id = InstructionID::SWI;
        } else {
            //TODO error no match!
        }

        if (id != InstructionID::SWI && id != InstructionID::INVALID)
            std::cout << instructionIDToString(id) << std::endl;
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
