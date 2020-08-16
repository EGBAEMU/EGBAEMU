#ifndef INST_HPP
#define INST_HPP

#include <cstdint>
#include <functional>

namespace gbaemu
{

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

    struct CPUState;

    
    enum ARMInstructionCategory {
        MUL_ACC,
        MUL_ACC_LONG,
        DATA_SWP,
        HW_TRANSF_REG_OFF,
        HW_TRANSF_IMM_OFF,
        SIGN_TRANSF,
        DATA_PROC_PSR_TRANSF,
        LS_REG_UBYTE,
        BLOCK_DATA_TRANSF,
        BRANCH,
        SOFTWARE_INTERRUPT
    };

    enum ARMInstructionID : uint8_t {
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
        LDRD,
        MLA,
        MOV,
        MRS,
        MSR,
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
        STRD,
        SUB,
        SWI,
        SWP,
        SWPB,
        TEQ,
        TST,
        UMLAL,
        UMULL,
        INVALID
    };

    class ARMInstruction
    {
      public:
        ARMInstructionID id;
        ARMInstructionCategory cat;
        ConditionOPCode condition;

        union {
            struct {
                bool a, s;
                uint32_t rd, rn, rs, rm;
            } mul_acc;

            struct {
                bool u, a, s;
                uint32_t rd_msw, rd_lsw, rn, rm;
            } mul_acc_long;

            struct {
                bool b;
                uint32_t rn, rd, rm;
            } data_swp;

            struct {
                bool p, u, w, l;
                uint32_t rm;
            } hw_transf_reg_off;

            struct {
                bool p, u, w, l;
                uint32_t rn, rd, offset;
            } hw_transf_imm_off;

            struct {
                bool p, u, b, w, l, h;
                uint32_t rn, rd;
            } sign_transf;

            struct {
                bool i, s;
                uint32_t opCode, rn, rd, operand2;
            } data_proc_psr_transf;

            struct {
                bool p, u, b, w, l;
                uint32_t rn, rd, addrMode;
            } ls_reg_ubyte;

            struct {
                bool p, u, w, l;
                uint32_t rn, rList;
            } block_data_transf;

            struct {
                bool l;
                uint32_t offset;
            } branch;

            struct {
                uint32_t comment;
            } software_interrupt;
        } params;

        /* implemented in inst_arm.cpp */
        std::string toString() const;
        bool conditionSatisfied(uint32_t CPSR) const;
    };

    class ThumbInstruction
    {
    public:
        std::string toString() const;
    };


    /* an object that can represent an ARM and a THUMB instruction */
    class Instruction {
      public:
        ARMInstruction arm;
        ThumbInstruction thumb;
        bool isArm = true;
      public:
        void setArmInstruction(ARMInstruction &armInstruction);
        void setThumbInstruction(ThumbInstruction& thumbInstruction);
        bool isArmInstruction() const;
        static Instruction fromARM(ARMInstruction& armInst);
        static Instruction fromThumb(ThumbInstruction& thumbInst);
    };    

    class InstructionDecoder {
      public:
        virtual Instruction decode(uint32_t inst) const = 0;
    };

} // namespace gbaemu

#endif
