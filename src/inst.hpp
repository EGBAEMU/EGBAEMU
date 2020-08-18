#ifndef INST_HPP
#define INST_HPP

#include <cstdint>
#include <functional>

namespace gbaemu
{

    namespace arm
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
            BRANCH_XCHG,
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
                    uint32_t rd_msw, rd_lsw, rs, rm;
                } mul_acc_long;

                struct {
                    uint8_t rn;
                } branch_xchg;

                struct {
                    bool b;
                    uint32_t rn, rd, rm;
                } data_swp;

                struct {
                    bool p, u, w, l;
                    uint32_t rn, rd, rm;
                } hw_transf_reg_off;

                struct {
                    bool p, u, w, l;
                    uint32_t rn, rd, offset;
                } hw_transf_imm_off;

                struct {
                    /*
                        pre/post
                        up/down
                        byte/[word, halfword]
                        writeback/no writeback?
                        load/store
                        halfword/word                    
                     */
                    bool p, u, b, w, l, h;
                    uint32_t rn, rd;
                } sign_transf;

                struct {
                    bool i, s;
                    uint32_t opCode, rn, rd, operand2;

                    bool extractOperand2(ShiftType& shiftType, uint32_t& shiftAmount, uint32_t& rm, uint32_t& rs, uint32_t& imm) const {
                        bool shiftAmountFromReg = false;

                        if (i) {
                            /* ROR */
                            shiftType = ShiftType::ROR;
                            imm = operand2 & 0xFF;
                            shiftAmount = ((operand2 >> 8) & 0xF) * 2;
                        } else {
                            shiftType = static_cast<ShiftType>((operand2 >> 5) & 0b11);
                            rm = operand2 & 0xF;
                            shiftAmountFromReg = (operand2 >> 4) & 1;
                        }

                        if (shiftAmountFromReg)
                            rs = (operand2 >> 8) & 0xF;
                        else
                            shiftAmount = (operand2 >> 7) & 0b11111;

                        return shiftAmountFromReg;
                    }
                } data_proc_psr_transf;

                struct {
                    bool i, p, u, b, w, l;
                    uint32_t rn, rd, addrMode;
                } ls_reg_ubyte;

                struct {
                    bool p, u, w, l;
                    uint32_t rn, rList;
                } block_data_transf;

                struct {
                    bool l;
                    int32_t offset;
                } branch;

                struct {
                    uint32_t comment;
                } software_interrupt;
            } params;

            /* implemented in inst_arm.cpp */
            std::string toString() const;
            bool conditionSatisfied(uint32_t CPSR) const;
        };

    } // namespace arm

    namespace thumb
    {

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
            BLX,
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
            /*
            BEQ,
            BNE,
            BCS_BHS,
            BCC_BLO,
            BMI,
            BPL,
            BVS,
            BVC,
            BHI,
            BLS,
            BGE,
            BLT,
            BGT,
            BLE,
            */
            SWI,
            B,
            INVALID
        };

        enum ThumbInstructionCategory {
            MOV_SHIFT,
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
            LONG_BRANCH_WITH_LINK
        };

        class ThumbInstruction
        {
          public:
            ThumbInstructionID id;
            ThumbInstructionCategory cat;

            union {
                struct {
                    uint8_t rs;
                    uint8_t rd;
                    uint8_t offset;
                } mov_shift;

                struct {
                    // can be either rn or a offset
                    uint8_t rn_offset;
                    uint8_t rs;
                    uint8_t rd;
                } add_sub;

                struct {
                    uint8_t rd;
                    uint8_t offset;
                } mov_cmp_add_sub_imm;

                struct {
                    uint8_t rs;
                    uint8_t rd;
                } alu_op;

                struct {
                    uint8_t rs;
                    uint8_t rd;
                } br_xchg;

                struct {
                    uint8_t rd;
                    uint8_t offset;
                } pc_ld;

                struct {
                    bool l;
                    bool b;
                    uint8_t ro;
                    uint8_t rb;
                    uint8_t rd;
                } ld_st_rel_off;

                struct {
                    bool h;
                    bool s;
                    uint8_t ro;
                    uint8_t rb;
                    uint8_t rd;
                } ld_st_sign_ext;

                struct {
                    bool b;
                    bool l;
                    uint8_t offset;
                    uint8_t rb;
                    uint8_t rd;
                } ld_st_imm_off;

                struct {
                    bool l;
                    uint8_t offset;
                    uint8_t rb;
                    uint8_t rd;
                } ld_st_hw;

                struct {
                    bool l;
                    uint8_t rd;
                    uint8_t offset;
                } ld_st_rel_sp;

                struct {
                    bool sp;
                    uint8_t rd;
                    uint8_t offset;
                } load_addr;

                struct {
                    bool s;
                    uint8_t offset;
                } add_offset_to_stack_ptr;

                struct {
                    bool l;
                    bool r;
                    uint8_t rlist;
                } push_pop_reg;

                struct {
                    bool l;
                    uint8_t rb;
                    uint8_t rlist;
                } mult_load_store;

                struct {
                    uint8_t cond;
                    int8_t offset;
                } cond_branch;

                struct {
                    uint8_t comment;
                } software_interrupt;

                struct {
                    int16_t offset;
                } unconditional_branch;

                struct {
                    bool h;
                    uint16_t offset;
                } long_branch_with_link;
            } params;

            std::string
            toString() const;
        };
    } // namespace thumb

    /* an object that can represent an ARM and a THUMB instruction */
    class Instruction
    {
      public:
        arm::ARMInstruction arm;
        thumb::ThumbInstruction thumb;
        bool isArm = true;

      public:
        void setArmInstruction(arm::ARMInstruction &armInstruction);
        void setThumbInstruction(thumb::ThumbInstruction &thumbInstruction);
        bool isArmInstruction() const;
        static Instruction fromARM(arm::ARMInstruction &armInst);
        static Instruction fromThumb(thumb::ThumbInstruction &thumbInst);
    };

    class InstructionDecoder
    {
      public:
        virtual Instruction decode(uint32_t inst) const = 0;
    };

} // namespace gbaemu

#endif
