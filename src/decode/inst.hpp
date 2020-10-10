#ifndef INST_HPP
#define INST_HPP

#include "io/memory_defs.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace gbaemu
{
    struct CPUState;

    /*
    instruction set
    http://www.ecs.csun.edu/~smirzaei/docs/ece425/arm7tdmi_instruction_set_reference.pdf
 */

    struct InstructionExecutionInfo {
        /*
            1. open https://static.docs.arm.com/ddi0029/g/DDI0029.pdf
            2. go to "Instruction Cycle Timings"
            3. vomit        
         */
        /*
        http://problemkaputt.de/gbatek.htm#arminstructionsummary
        http://problemkaputt.de/gbatek.htm#armcpuinstructioncycletimes
        https://mgba.io/2015/06/27/cycle-counting-prefetch/
        The ARM7TDMI also has four different types of cycles, 
        on which the CPU clock may stall for a different amount of time: 
            S cycles, the most common type, refer to sequential memory access. 
              Basically, if you access memory at one location and then the location afterwards, 
              this second access is sequential, so the memory bus can fetch it more quickly. 
            Next are N cycles, which refer to non-sequential memory accesses. 
              N cycles occur when a memory address is fetched that has nothing to do with the previous instruction.
            Third are I cycles, which are internal cycles. 
              These occur when the CPU does a complicated operation, 
              such as multiplication, that it can’t complete in a single cycle. 
            Finally are C cycles, or coprocessor cycles, 
              which occur when communicating with coprocessors in the system.
              However, the GBA has no ARM-specified coprocessors, 
              and all instructions that try to interact with coprocessors trigger an error state.
        Thus, the only important cycles to the GBA are S, N and I.

        How long each stall is depends on which region of memory is being accessed.
        The GBA refers to these stalls as “wait states”.
        */
        uint32_t cycleCount;

        // Convert instruction fetch S cycle into N cycle (only relevant for STR AFAIK)
        bool noDefaultSCycle;

        // Needed for infinite loops caused by branching to current PC value -> no PC change would otherwise be interpreted as normal instruction and execution continues at PC + 4
        bool forceBranch;
        // Invalid operation executed -> abort
        bool hasCausedException;

        // CPU halting
        bool haltCPU;
        uint32_t haltCondition;

        // resolved Memory Region
        memory::MemoryRegion memReg;
    };

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

    const char *conditionCodeToString(ConditionOPCode condition);

    bool conditionSatisfied(ConditionOPCode condition, const CPUState &state);

    enum InstructionID : uint8_t {
        ADC,
        ADD,
        AND,
        B, /* includes BL */
        BIC,
        BX,
        CMN,
        CMP,
        EOR,
        LDM,
        LDR,
        LDRB,
        LDRH,
        LDRSB,
        LDRSH,
        //LDRD, /* supported arm5 and up */
        MLA,
        MOV,
        MRS_SPSR,
        MRS_CPSR,
        MSR_SPSR,
        MSR_CPSR,
        MUL,
        MVN,
        ORR,
        RSB,
        RSC,
        SBC,
        SMLAL,
        SMULL,
        STM,
        STR,
        STRB,
        STRH,
        //STRD, /* supported arm5 and up */
        SUB,
        SWI,
        SWP,
        SWPB,
        TEQ,
        TST,
        UMLAL,
        UMULL,

        // THUMB specials
        LSL,
        LSR,
        ASR,
        ROR,
        NOP,
        ADD_SHORT_IMM,
        SUB_SHORT_IMM,
        NEG,
        // This one is ARM9
        //BLX,
        POP,
        PUSH,
        STMIA,
        LDMIA,

        INVALID
    };

    const char *instructionIDToString(InstructionID id);

    namespace shifts
    {
        enum ShiftType : uint8_t {
            /* logical shift left */
            LSL = 0,
            /* logical shift right */
            LSR,
            /* arithmetic shift right */
            ASR,
            /* circular shift right (wrap around) */
            ROR
        };

        uint64_t shift(uint32_t value, ShiftType type, uint8_t amount, bool oldCarry, bool shiftByImm);
    } // namespace shifts

    namespace arm
    {
        enum ARMInstructionCategory {
            MUL_ACC = 0,
            MUL_ACC_LONG,
            BRANCH_XCHG,
            DATA_SWP,
            HW_TRANSF_REG_OFF,
            HW_TRANSF_IMM_OFF,
            SIGN_TRANSF,
            DATA_PROC_PSR_TRANSF,
            LS_REG_UBYTE,
            BLOCK_DATA_TRANSF,
            BRANCH,
            SOFTWARE_INTERRUPT,
            INVALID_CAT
        };

    } // namespace arm

    namespace thumb
    {
        /*
        enum ThumbInstructionID : uint8_t {
            MVN,
            AND,
            TST,
            BIC,
            ORR,
            EOR,
            LSL,
            LSR,
            ASR,
            ROR,
            NOP,
            ADC,
            ADD,
            ADD_SHORT_IMM,
            SUB,
            SUB_SHORT_IMM,
            MOV,
            CMP,
            SBC,
            NEG,
            CMN,
            MUL,
            BX,
            // This one is ARM9
            //BLX,
            POP,
            LDR,
            LDRB,
            LDRH,
            LDSB,
            LDSH,
            STR,
            STRB,
            STRH,
            PUSH,
            STMIA,
            LDMIA,
            SWI,
            B,
            INVALID
        };
        */

        enum ThumbInstructionCategory {
            MOV_SHIFT = 0,
            ADD_SUB,
            MOV_CMP_ADD_SUB_IMM,
            ALU_OP,
            BR_XCHG,
            PC_LD,
            LD_ST_REL_OFF,
            LD_ST_SIGN_EXT,
            LD_ST_IMM_OFF,
            LD_ST_HW,
            LD_ST_REL_SP,
            LOAD_ADDR,
            ADD_OFFSET_TO_STACK_PTR,
            PUSH_POP_REG,
            MULT_LOAD_STORE,
            COND_BRANCH,
            SOFTWARE_INTERRUPT,
            UNCONDITIONAL_BRANCH,
            LONG_BRANCH_WITH_LINK,
            INVALID_CAT
        };

    } // namespace thumb

    /* an object that can represent an ARM and a THUMB instruction */
    class Instruction
    {
      public:
        uint32_t inst;
        bool isArm = true;

      public:
        std::string toString() const;
    };

    class NopExecutor
    {
      public:
        template <typename T, typename... Args>
        void operator()(T, Args...)
        {
        }
    };

    typedef std::function<void(uint32_t)> InstructionDecodeAndExecutor;

    bool extractOperand2(shifts::ShiftType &shiftType, uint8_t &shiftAmount, uint8_t &rm, uint8_t &rs, uint8_t &imm, uint16_t operand2, bool i);

} // namespace gbaemu

#endif
