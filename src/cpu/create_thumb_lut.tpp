#ifndef CREATE_THUMB_LUT_TPP
#define CREATE_THUMB_LUT_TPP

namespace gbaemu
{

    // shout-outs to https://smolka.dev/eggvance/progress-3/ & https://smolka.dev/eggvance/progress-5/
    // for his insane optimization ideas
    template <uint16_t hash>
    static constexpr thumb::ThumbInstructionCategory extractThumbCategoryFromHash()
    {
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_ADD_SUB)) == constexprHashThumb(thumb::VAL_THUMB_ADD_SUB)) {
            return thumb::ADD_SUB;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_MOV_SHIFT)) == constexprHashThumb(thumb::VAL_THUMB_MOV_SHIFT)) {
            return thumb::MOV_SHIFT;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_MOV_CMP_ADD_SUB_IMM)) == constexprHashThumb(thumb::VAL_THUMB_MOV_CMP_ADD_SUB_IMM)) {
            return thumb::MOV_CMP_ADD_SUB_IMM;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_ALU_OP)) == constexprHashThumb(thumb::VAL_THUMB_ALU_OP)) {
            return thumb::ALU_OP;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_BR_XCHG)) == constexprHashThumb(thumb::VAL_THUMB_BR_XCHG)) {
            return thumb::BR_XCHG;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_PC_LD)) == constexprHashThumb(thumb::VAL_THUMB_PC_LD)) {
            return thumb::PC_LD;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LD_ST_REL_OFF)) == constexprHashThumb(thumb::VAL_THUMB_LD_ST_REL_OFF)) {
            return thumb::LD_ST_REL_OFF;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LD_ST_SIGN_EXT)) == constexprHashThumb(thumb::VAL_THUMB_LD_ST_SIGN_EXT)) {
            return thumb::LD_ST_SIGN_EXT;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LD_ST_IMM_OFF)) == constexprHashThumb(thumb::VAL_THUMB_LD_ST_IMM_OFF)) {
            return thumb::LD_ST_IMM_OFF;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LD_ST_HW)) == constexprHashThumb(thumb::VAL_THUMB_LD_ST_HW)) {
            return thumb::LD_ST_HW;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LD_ST_REL_SP)) == constexprHashThumb(thumb::VAL_THUMB_LD_ST_REL_SP)) {
            return thumb::LD_ST_REL_SP;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LOAD_ADDR)) == constexprHashThumb(thumb::VAL_THUMB_LOAD_ADDR)) {
            return thumb::LOAD_ADDR;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_ADD_OFFSET_TO_STACK_PTR)) == constexprHashThumb(thumb::VAL_THUMB_ADD_OFFSET_TO_STACK_PTR)) {
            return thumb::ADD_OFFSET_TO_STACK_PTR;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_PUSH_POP_REG)) == constexprHashThumb(thumb::VAL_THUMB_PUSH_POP_REG)) {
            return thumb::PUSH_POP_REG;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_MULT_LOAD_STORE)) == constexprHashThumb(thumb::VAL_THUMB_MULT_LOAD_STORE)) {
            return thumb::MULT_LOAD_STORE;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_SOFTWARE_INTERRUPT)) == constexprHashThumb(thumb::VAL_THUMB_SOFTWARE_INTERRUPT)) {
            return thumb::SOFTWARE_INTERRUPT;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_COND_BRANCH)) == constexprHashThumb(thumb::VAL_THUMB_COND_BRANCH)) {
            return thumb::COND_BRANCH;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_UNCONDITIONAL_BRANCH)) == constexprHashThumb(thumb::VAL_THUMB_UNCONDITIONAL_BRANCH)) {
            return thumb::UNCONDITIONAL_BRANCH;
        }
        if ((hash & constexprHashThumb(thumb::MASK_THUMB_LONG_BRANCH_WITH_LINK)) == constexprHashThumb(thumb::VAL_THUMB_LONG_BRANCH_WITH_LINK)) {
            return thumb::LONG_BRANCH_WITH_LINK;
        }

        return thumb::INVALID_CAT;
    }

    template <uint16_t hash>
    constexpr CPU::InstExecutor CPU::resolveThumbHashHandler()
    {
        constexpr uint16_t expandedHash = dehashThumb<hash>();

        switch (extractThumbCategoryFromHash<hash>()) {
            case thumb::ADD_SUB: {
                constexpr uint8_t opCode = (expandedHash >> 9) & 0x3;

                switch (opCode) {
                    case 0b00:
                        return &CPU::execDataProc<ADD, false, true, true, thumb::ADD_SUB>;
                    case 0b01:
                        return &CPU::execDataProc<SUB, false, true, true, thumb::ADD_SUB>;
                    case 0b10:
                        return &CPU::execDataProc<ADD_SHORT_IMM, true, true, true, thumb::ADD_SUB>;
                    case 0b11:
                        return &CPU::execDataProc<SUB_SHORT_IMM, true, true, true, thumb::ADD_SUB>;
                }
                break;
            }
            case thumb::MOV_SHIFT: {
                constexpr uint8_t opCode = (expandedHash >> 11) & 0x3;

                switch (opCode) {
                    case 0b00:
                        return &CPU::handleThumbMoveShiftedReg<LSL>;
                    case 0b01:
                        return &CPU::handleThumbMoveShiftedReg<LSR>;
                    case 0b10:
                        return &CPU::handleThumbMoveShiftedReg<ASR>;

                    // This case belongs to ADD_SUB
                    case 0b11:
                        break;
                }
                break;
            }
            case thumb::MOV_CMP_ADD_SUB_IMM: {
                constexpr uint8_t opCode = (expandedHash >> 11) & 0x3;

                switch (opCode) {
                    case 0b00:
                        return &CPU::execDataProc<MOV, true, true, true, thumb::MOV_CMP_ADD_SUB_IMM>;
                    case 0b01:
                        return &CPU::execDataProc<CMP, true, true, true, thumb::MOV_CMP_ADD_SUB_IMM>;
                    case 0b10:
                        return &CPU::execDataProc<ADD, true, true, true, thumb::MOV_CMP_ADD_SUB_IMM>;
                    case 0b11:
                        return &CPU::execDataProc<SUB, true, true, true, thumb::MOV_CMP_ADD_SUB_IMM>;
                }

                break;
            }
            case thumb::ALU_OP: {
                constexpr uint8_t opCode = (expandedHash >> 6) & 0x0F;

                switch (opCode) {
                    case 0b0000:
                        return &CPU::execDataProc<AND, false, true, true, thumb::ALU_OP, AND>;
                    case 0b0001:
                        return &CPU::execDataProc<EOR, false, true, true, thumb::ALU_OP, EOR>;
                    case 0b0010:;
                        return &CPU::execDataProc<MOV, false, true, true, thumb::ALU_OP, LSL>;
                    case 0b0011:
                        return &CPU::execDataProc<MOV, false, true, true, thumb::ALU_OP, LSR>;
                    case 0b0100:
                        return &CPU::execDataProc<MOV, false, true, true, thumb::ALU_OP, ASR>;
                    case 0b0101:
                        return &CPU::execDataProc<ADC, false, true, true, thumb::ALU_OP, ADC>;
                    case 0b0110:
                        return &CPU::execDataProc<SBC, false, true, true, thumb::ALU_OP, SBC>;
                    case 0b0111:
                        return &CPU::execDataProc<MOV, false, true, true, thumb::ALU_OP, ROR>;
                    case 0b1000:
                        return &CPU::execDataProc<TST, false, true, true, thumb::ALU_OP, TST>;
                    case 0b1001:
                        return &CPU::execDataProc<NEG, false, true, true, thumb::ALU_OP, NEG>;
                    case 0b1010:
                        return &CPU::execDataProc<CMP, false, true, true, thumb::ALU_OP, CMP>;
                    case 0b1011:
                        return &CPU::execDataProc<CMN, false, true, true, thumb::ALU_OP, CMN>;
                    case 0b1100:
                        return &CPU::execDataProc<ORR, false, true, true, thumb::ALU_OP, ORR>;
                    case 0b1101:
                        return &CPU::handleMultAcc<false, true, true>;
                    case 0b1110:
                        return &CPU::execDataProc<BIC, false, true, true, thumb::ALU_OP, BIC>;
                    case 0b1111:
                        return &CPU::execDataProc<MVN, false, true, true, thumb::ALU_OP, MVN>;
                }
                break;
            }
            case thumb::BR_XCHG: {

                constexpr uint8_t opCode = (expandedHash >> 8) & 0x3;
                constexpr bool msbDst = (expandedHash >> 7) & 1;

                switch (opCode) {
                    case 0b00:
                        // BR_XCHG, ADD
                        return &CPU::handleThumbBranchXCHG<ADD>;
                    case 0b01:
                        // BR_XCHG, CMP
                        return &CPU::handleThumbBranchXCHG<CMP>;
                    case 0b10:
                        //Assemblers/Disassemblers should use MOV R8,R8 as NOP (in THUMB mode)
                        return &CPU::handleThumbBranchXCHG<MOV>;
                    case 0b11:
                        if (msbDst) {
                            //BLX
                            return &CPU::handleInvalid;
                        } else {
                            return &CPU::handleThumbBranchXCHG<BX>;
                        }
                }
            }
            case thumb::PC_LD:
                // LDR - <id, thumb, pre, up, i, writeback>
                return &CPU::execLoadStoreRegUByte<LDR, true, true, true, false, false, thumb::PC_LD>;
            case thumb::LD_ST_REL_OFF: {

                constexpr uint8_t opCode = (expandedHash >> 10) & 3;

                switch (opCode) {
                    case 0b00:
                        // LD_ST_REL_OFF, STR
                        return &CPU::execLoadStoreRegUByte<STR, true, true, true, true, false, thumb::LD_ST_REL_OFF>;
                    case 0b01:
                        // LD_ST_REL_OFF, STRB
                        return &CPU::execLoadStoreRegUByte<STRB, true, true, true, true, false, thumb::LD_ST_REL_OFF>;
                    case 0b10:
                        // LD_ST_REL_OFF, LDR
                        return &CPU::execLoadStoreRegUByte<LDR, true, true, true, true, false, thumb::LD_ST_REL_OFF>;
                    case 0b11:
                        // LD_ST_REL_OFF, LDRB
                        return &CPU::execLoadStoreRegUByte<LDRB, true, true, true, true, false, thumb::LD_ST_REL_OFF>;
                }

                break;
            }
            case thumb::LD_ST_SIGN_EXT: {
                constexpr uint8_t opCode = (expandedHash >> 10) & 3;

                switch (opCode) {
                    case 0b00:
                        //LD_ST_SIGN_EXT, STRH;
                        return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, STRH, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_SIGN_EXT>;
                    case 0b01:
                        // LD_ST_SIGN_EXT, LDRSB
                        return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRSB, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_SIGN_EXT>;
                    case 0b10:
                        // LD_ST_SIGN_EXT, LDRH
                        return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRH, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_SIGN_EXT>;
                    case 0b11:
                        // LD_ST_SIGN_EXT, LDRSH
                        return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRSH, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_SIGN_EXT>;
                }
            }
            case thumb::LD_ST_IMM_OFF: {
                constexpr uint8_t opCode = (expandedHash >> 11) & 3;

                /* InstructionID id, bool thumb, bool pre, bool up, bool i, bool writeback, thumb::ThumbInstructionCategory thumbCat */
                switch (opCode) {
                    case 0b00:
                        return &CPU::execLoadStoreRegUByte<STR, true,                /* id, thumb */
                                                           true, true, false, false, /* pre, up, imm, wb */
                                                           thumb::LD_ST_IMM_OFF>;
                    case 0b01:
                        return &CPU::execLoadStoreRegUByte<LDR, true,                /* id, thumb */
                                                           true, true, false, false, /* pre, up, imm, wb */
                                                           thumb::LD_ST_IMM_OFF>;
                    case 0b10:
                        return &CPU::execLoadStoreRegUByte<STRB, true,               /* id, thumb */
                                                           true, true, false, false, /* pre, up, imm, wb */
                                                           thumb::LD_ST_IMM_OFF>;
                    case 0b11:
                        return &CPU::execLoadStoreRegUByte<LDRB, true,               /* id, thumb */
                                                           true, true, false, false, /* pre, up, imm, wb */
                                                           thumb::LD_ST_IMM_OFF>;
                }

                break;
            }
            case thumb::LD_ST_HW: {

                constexpr bool l = (expandedHash >> 11) & 0x1;

                if (l) {
                    // LD_ST_HW, LDRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRH, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_HW>;
                } else {
                    // LD_ST_HW, STRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, STRH, true, true, true, false, arm::INVALID_CAT, thumb::LD_ST_HW>;
                }
            }
            case thumb::LD_ST_REL_SP: {

                constexpr bool l = (expandedHash >> 11) & 0x1;

                if (l) {
                    // LD_ST_REL_SP, LDR - <id, thumb, pre, up, i, writeback>
                    return &CPU::execLoadStoreRegUByte<LDR, true, true, true, false, false, thumb::LD_ST_REL_SP>;
                } else {
                    // LD_ST_REL_SP, STR
                    return &CPU::execLoadStoreRegUByte<STR, true, true, true, false, false, thumb::LD_ST_REL_SP>;
                }
            }
            case thumb::LOAD_ADDR: {
                // LOAD_ADDR, ADD
                constexpr bool sp = (expandedHash >> 11) & 0x1;
                return &CPU::handleThumbRelAddr<sp>;
            }
            case thumb::ADD_OFFSET_TO_STACK_PTR: {
                // ADD_OFFSET_TO_STACK_PTR, ADD
                constexpr bool s = (expandedHash >> 7) & 0x1;
                return &CPU::handleThumbAddOffsetToStackPtr<s>;
            }
            case thumb::PUSH_POP_REG: {
                constexpr bool l = expandedHash & (1 << 11);
                constexpr bool patchRList = expandedHash & (1 << 8);
                if (l) {
                    //POP
                    return &CPU::execDataBlockTransfer<true, false, true, true, false, l, patchRList, true>;
                } else {
                    //PUSH
                    return &CPU::execDataBlockTransfer<true, true, false, true, false, l, patchRList, true>;
                }
            }
            case thumb::MULT_LOAD_STORE: {
                constexpr bool l = expandedHash & (1 << 11);
                if (l) {
                    // LDMIA
                    return &CPU::execDataBlockTransfer<true, false, true, true, false, l>;
                } else {
                    // STMIA
                    return &CPU::execDataBlockTransfer<true, false, true, true, false, l>;
                }
            }
            case thumb::SOFTWARE_INTERRUPT:
                return &CPU::softwareInterrupt<true>;
            case thumb::COND_BRANCH: {
                // COND_BRANCH, B
                return &CPU::handleThumbConditionalBranch;
            }
            case thumb::UNCONDITIONAL_BRANCH: {
                // UNCONDITIONAL_BRANCH, B
                return &CPU::handleThumbUnconditionalBranch;
            }
            case thumb::LONG_BRANCH_WITH_LINK: {
                // LONG_BRANCH_WITH_LINK, B
                constexpr bool h = expandedHash & (1 << 11);
                return &CPU::handleThumbLongBranchWithLink<h>;
            }
            case thumb::INVALID_CAT:
                return &CPU::handleInvalid;
        }

        return &CPU::handleInvalid;
    }

#define DECODE_LUT_ENTRY(hash) CPU::resolveThumbHashHandler<hash>()
#define DECODE_LUT_ENTRY_4(hash)        \
    DECODE_LUT_ENTRY(hash + 0 * 1),     \
        DECODE_LUT_ENTRY(hash + 1 * 1), \
        DECODE_LUT_ENTRY(hash + 2 * 1), \
        DECODE_LUT_ENTRY(hash + 3 * 1)
#define DECODE_LUT_ENTRY_16(hash)         \
    DECODE_LUT_ENTRY_4(hash + 0 * 4),     \
        DECODE_LUT_ENTRY_4(hash + 1 * 4), \
        DECODE_LUT_ENTRY_4(hash + 2 * 4), \
        DECODE_LUT_ENTRY_4(hash + 3 * 4)
#define DECODE_LUT_ENTRY_64(hash)           \
    DECODE_LUT_ENTRY_16(hash + 0 * 16),     \
        DECODE_LUT_ENTRY_16(hash + 1 * 16), \
        DECODE_LUT_ENTRY_16(hash + 2 * 16), \
        DECODE_LUT_ENTRY_16(hash + 3 * 16)
#define DECODE_LUT_ENTRY_256(hash)          \
    DECODE_LUT_ENTRY_64(hash + 0 * 64),     \
        DECODE_LUT_ENTRY_64(hash + 1 * 64), \
        DECODE_LUT_ENTRY_64(hash + 2 * 64), \
        DECODE_LUT_ENTRY_64(hash + 3 * 64)
#define DECODE_LUT_ENTRY_1024(hash)           \
    DECODE_LUT_ENTRY_256(hash + 0 * 256),     \
        DECODE_LUT_ENTRY_256(hash + 1 * 256), \
        DECODE_LUT_ENTRY_256(hash + 2 * 256), \
        DECODE_LUT_ENTRY_256(hash + 3 * 256)

    const CPU::InstExecutor CPU::thumbExeLUT[1024] = {DECODE_LUT_ENTRY_1024(0)};

#undef DECODE_LUT_ENTRY
#undef DECODE_LUT_ENTRY_4
#undef DECODE_LUT_ENTRY_16
#undef DECODE_LUT_ENTRY_64
#undef DECODE_LUT_ENTRY_512
#undef DECODE_LUT_ENTRY_1024
} // namespace gbaemu

#endif