#ifndef INST_ARM_HPP
#define INST_ARM_HPP

#include "cpu_state.hpp"
#include "inst.hpp"
#include "regs.hpp"
#include <cstdint>
#include <functional>

namespace gbaemu
{
    /*
        |..3 ..................2 ..................1 ..................0|
        |1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
        |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| DataProc
        |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Rs___|0|Typ|1|__Rm___| DataProc
        |_Cond__|0_0_1|___Op__|S|__Rn___|__Rd___|_Shift_|___Immediate___| DataProc
        |_Cond__|0_0_1_1_0_0_1_0_0_0_0_0_1_1_1_1_0_0_0_0|_____Hint______| ARM11:Hint
        |_Cond__|0_0_1_1_0|P|1|0|_Field_|__Rd___|_Shift_|___Immediate___| PSR Imm
        |_Cond__|0_0_0_1_0|P|L|0|_Field_|__Rd___|0_0_0_0|0_0_0_0|__Rm___| PSR Reg
        |_Cond__|0_0_0_1_0_0_1_0_1_1_1_1_1_1_1_1_1_1_1_1|0_0|L|1|__Rn___| BX,BLX
        |1_1_1_0|0_0_0_1_0_0_1_0|_____immediate_________|0_1_1_1|_immed_| ARM9:BKPT
        |_Cond__|0_0_0_1_0_1_1_0_1_1_1_1|__Rd___|1_1_1_1|0_0_0_1|__Rm___| ARM9:CLZ
        |_Cond__|0_0_0_1_0|Op_|0|__Rn___|__Rd___|0_0_0_0|0_1_0_1|__Rm___| ARM9:QALU
        |_Cond__|0_0_0_0_0_0|A|S|__Rd___|__Rn___|__Rs___|1_0_0_1|__Rm___| Multiply
        |_Cond__|0_0_0_0_0_1_0_0|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| ARM11:UMAAL
        |_Cond__|0_0_0_0_1|U|A|S|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| MulLong
        |_Cond__|0_0_0_1_0|Op_|0|Rd/RdHi|Rn/RdLo|__Rs___|1|y|x|0|__Rm___| MulHalfARM9
        |_Cond__|0_0_0_1_0|B|0_0|__Rn___|__Rd___|0_0_0_0|1_0_0_1|__Rm___| TransSwp12
        |_Cond__|0_0_0_1_1|_Op__|__Rn___|__Rd___|1_1_1_1|1_0_0_1|__Rm___| ARM11:LDREX
        |_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
        |_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
        |_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
        |_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
        |_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
        |_Cond__|0_1_1|Op_|x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x_x|1|x_x_x_x| ARM11:Media
        |1_1_1_1_0_1_0_1_0_1_1_1_1_1_1_1_1_1_1_1_0_0_0_0_0_0_0_1_1_1_1_1| ARM11:CLREX
        |_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
        |_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX
        |_Cond__|1_1_0|P|U|N|W|L|__Rn___|__CRd__|__CP#__|____Offset_____| CoDataTrans
        |_Cond__|1_1_0_0_0_1_0|L|__Rn___|__Rd___|__CP#__|_CPopc_|__CRm__| CoRR ARM9
        |_Cond__|1_1_1_0|_CPopc_|__CRn__|__CRd__|__CP#__|_CP__|0|__CRm__| CoDataOp
        |_Cond__|1_1_1_0|CPopc|L|__CRn__|__Rd___|__CP#__|_CP__|1|__CRm__| CoRegTrans
        |_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI
     */

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
        B,
        /* includes BL */ BIC,
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

    const char *instructionIDToString(ARMInstructionID id);

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

    class ARMInstruction : public Instruction
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

            } software_interrupt;
        } params;

        virtual void execute(CPUState *state);
        virtual std::string toString() const;

      private:
        static bool conditionSatisfied(ConditionOPCode executeCondition, uint32_t CPSR);
    };

    class ARMInstructionDecoder : public InstructionDecoder
    {
      public:
        virtual Instruction *decode(uint32_t inst) const;
    };

} // namespace gbaemu

#endif /* INST_ARM_HPP */