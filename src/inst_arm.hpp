#ifndef INST_ARM_HPP
#define INST_ARM_HPP

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

    enum InstructionID {
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

    const char *instructionIDToString(InstructionID id) {
        switch (id) {
            case ADC: return "ADC";
            case ADD: return "ADD";
            case AND: return "AND";
            case B: return "B";
            case BIC: return "BIC";
            case BX: return "BX";
            case CMN: return "CMN";
            case CMP: return "CMP";
            case EOR: return "EOR";
            case LDM: return "LDM";
            case LDR: return "LDR";
            case LDRB: return "LDRB";
            case LDRH: return "LDRH";
            case LDRSB: return "LDRSB";
            case LDRSH: return "LDRSH";
            case MLA: return "MLA";
            case MOV: return "MOV";
            case MRS: return "MRS";
            case MSR: return "MSR";
            case MUL: return "MUL";
            case MVN: return "MVN";
            case ORR: return "ORR";
            case RSB: return "RSB";
            case RSC: return "RSC";
            case SBC: return "SBC";
            case SMLAL: return "SMLAL";
            case SMULL: return "SMULL";
            case STM: return "STM";
            case STR: return "STR";
            case STRB: return "STRB";
            case STRH: return "STRH";
            case SUB: return "SUB";
            case SWI: return "SWI";
            case SWP: return "SWP";
            case SWPB: return "SWPB";
            case TEQ: return "TEQ";
            case TST: return "TST";
            case UMLAL: return "UMLAL";
            case UMULL: return "UMULL";
            case INVALID: return "INVALID";
        }

        return "NULL";
    }


    class InstructionFunctions {
    public:

    };

    struct Instruction
    {
        union {
            struct {
                uint32_t cond, rd, rn, rs, rm;
                bool a, s;
            } multiply;
        } params;
    } __attribute__((packed));
}

#endif /* INST_ARM_HPP */
