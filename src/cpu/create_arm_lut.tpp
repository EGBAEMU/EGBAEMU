#ifndef CREATE_ARM_LUT_TPP
#define CREATE_ARM_LUT_TPP

namespace gbaemu
{

    // shout-outs to https://smolka.dev/eggvance/progress-3/ & https://smolka.dev/eggvance/progress-5/
    // for his insane optimization ideas
    template <uint32_t expandedHash>
    static constexpr InstructionID getALUOpInstruction()
    {
        constexpr uint8_t opCode = (expandedHash >> 21) & 0x0F;
        constexpr bool s = expandedHash & (1 << 20);
        switch (opCode) {
            case 0b0000:
                return AND;
            case 0b0001:
                return EOR;
            case 0b0010:
                return SUB;
            case 0b0011:
                return RSB;
            case 0b0100:
                return ADD;
            case 0b0101:
                return ADC;
            case 0b0110:
                return SBC;
            case 0b0111:
                return RSC;
            case 0b1000:
                if (s) {
                    return TST;
                } else {
                    return MRS_CPSR;
                }
            case 0b1001:
                if (s) {
                    return TEQ;
                } else {
                    return MSR_CPSR;
                }
            case 0b1010:
                if (s) {
                    return CMP;
                } else {
                    return MRS_SPSR;
                }
            case 0b1011:
                if (s) {
                    return CMN;
                } else {
                    return MSR_SPSR;
                }
            case 0b1100:
                return ORR;
            case 0b1101:
                return MOV;
            case 0b1110:
                return BIC;
            case 0b1111:
                return MVN;
        }
        return INVALID;
    }

    template <uint16_t hash>
    static constexpr arm::ARMInstructionCategory extractArmCategoryFromHash()
    {
        if ((hash & constexprHashArm(arm::MASK_MUL_ACC)) == constexprHashArm(arm::VAL_MUL_ACC)) {
            return arm::MUL_ACC;
        }
        if ((hash & constexprHashArm(arm::MASK_MUL_ACC_LONG)) == constexprHashArm(arm::VAL_MUL_ACC_LONG)) {
            return arm::MUL_ACC_LONG;
        }
        if ((hash & constexprHashArm(arm::MASK_BRANCH_XCHG)) == constexprHashArm(arm::VAL_BRANCH_XCHG)) {
            return arm::BRANCH_XCHG;
        }
        if ((hash & constexprHashArm(arm::MASK_DATA_SWP)) == constexprHashArm(arm::VAL_DATA_SWP)) {
            return arm::DATA_SWP;
        }
        if ((hash & constexprHashArm(arm::MASK_HW_TRANSF_REG_OFF)) == constexprHashArm(arm::VAL_HW_TRANSF_REG_OFF)) {
            return arm::HW_TRANSF_REG_OFF;
        }
        if ((hash & constexprHashArm(arm::MASK_HW_TRANSF_IMM_OFF)) == constexprHashArm(arm::VAL_HW_TRANSF_IMM_OFF)) {
            return arm::HW_TRANSF_IMM_OFF;
        }
        if ((hash & constexprHashArm(arm::MASK_SIGN_TRANSF)) == constexprHashArm(arm::VAL_SIGN_TRANSF)) {
            constexpr bool l = (dehashArm<hash>() >> 20) & 1;
            constexpr bool h = (dehashArm<hash>() >> 5) & 1;

            if ((!l && h) || (!l && !h))
                return arm::INVALID_CAT;
            return arm::SIGN_TRANSF;
        }
        if ((hash & constexprHashArm(arm::MASK_DATA_PROC_PSR_TRANSF)) == constexprHashArm(arm::VAL_DATA_PROC_PSR_TRANSF)) {
            return arm::DATA_PROC_PSR_TRANSF;
        }
        if ((hash & constexprHashArm(arm::MASK_LS_REG_UBYTE)) == constexprHashArm(arm::VAL_LS_REG_UBYTE)) {
            return arm::LS_REG_UBYTE;
        }
        if ((hash & constexprHashArm(arm::MASK_BLOCK_DATA_TRANSF)) == constexprHashArm(arm::VAL_BLOCK_DATA_TRANSF)) {
            return arm::BLOCK_DATA_TRANSF;
        }
        if ((hash & constexprHashArm(arm::MASK_BRANCH)) == constexprHashArm(arm::VAL_BRANCH)) {
            return arm::BRANCH;
        }
        if ((hash & constexprHashArm(arm::MASK_SOFTWARE_INTERRUPT)) == constexprHashArm(arm::VAL_SOFTWARE_INTERRUPT)) {
            return arm::SOFTWARE_INTERRUPT;
        }
        return arm::INVALID_CAT;
    }

    template <uint16_t hash>
    constexpr CPU::InstExecutor CPU::resolveArmHashHandler()
    {
        constexpr uint32_t expandedHash = dehashArm<hash>();

        switch (extractArmCategoryFromHash<hash>()) {
            case arm::MUL_ACC: {
                constexpr bool a = (expandedHash >> 21) & 1;
                constexpr bool s = (expandedHash >> 20) & 1;
                return &CPU::handleMultAcc<a, s, false>;
            }
            case arm::MUL_ACC_LONG: {
                constexpr bool signMul = (expandedHash >> 22) & 1;
                constexpr bool a = (expandedHash >> 21) & 1;
                constexpr bool s = (expandedHash >> 20) & 1;
                return &CPU::handleMultAccLong<a, s, signMul>;
            }
            case arm::BRANCH_XCHG:
                return &CPU::handleBranchAndExchange;
            case arm::DATA_SWP: {
                constexpr bool b = (expandedHash >> 22) & 1;
                return &CPU::handleDataSwp<b>;
            }
            case arm::HW_TRANSF_REG_OFF: {

                constexpr bool p = (expandedHash >> 24) & 1;
                constexpr bool u = (expandedHash >> 23) & 1;
                constexpr bool w = (expandedHash >> 21) & 1;
                constexpr bool l = (expandedHash >> 20) & 1;

                // register offset variants
                if (l) {
                    // HW_TRANSF_REG_OFF, LDRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRH, false, p, u, w, arm::HW_TRANSF_REG_OFF, thumb::INVALID_CAT>;
                } else {
                    // HW_TRANSF_REG_OFF, STRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, STRH, false, p, u, w, arm::HW_TRANSF_REG_OFF, thumb::INVALID_CAT>;
                }
            }
            case arm::HW_TRANSF_IMM_OFF: {

                constexpr bool p = (expandedHash >> 24) & 1;
                constexpr bool u = (expandedHash >> 23) & 1;
                constexpr bool w = (expandedHash >> 21) & 1;
                constexpr bool l = (expandedHash >> 20) & 1;

                if (l) {
                    // HW_TRANSF_IMM_OFF, LDRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, LDRH, false, p, u, w, arm::HW_TRANSF_IMM_OFF, thumb::INVALID_CAT>;
                } else {
                    // HW_TRANSF_IMM_OFF, STRH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<false, STRH, false, p, u, w, arm::HW_TRANSF_IMM_OFF, thumb::INVALID_CAT>;
                }
            }
            case arm::SIGN_TRANSF: {
                // extHash, id, thumb, pre, up, writeback, armCat, thumbCat>

                constexpr bool p = (expandedHash >> 24) & 1;
                constexpr bool u = (expandedHash >> 23) & 1;
                constexpr bool b = (expandedHash >> 22) & 1;
                constexpr bool w = (expandedHash >> 21) & 1;

                constexpr bool l = (expandedHash >> 20) & 1;
                constexpr bool h = (expandedHash >> 5) & 1;

                if (l && !h) {
                    // SIGN_TRANSF, LDRSB
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<b, LDRSB, false, p, u, w, arm::SIGN_TRANSF, thumb::INVALID_CAT>;
                } else if (l && h) {
                    // SIGN_TRANSF, LDRSH
                    return &CPU::execHalfwordDataTransferImmRegSignedTransfer<b, LDRSH, false, p, u, w, arm::SIGN_TRANSF, thumb::INVALID_CAT>;
                } else {
                    // INVALID_CAT, INVALID
                    return &CPU::handleInvalid;
                }
            }
            case arm::DATA_PROC_PSR_TRANSF: {
                constexpr InstructionID id = getALUOpInstruction<expandedHash>();
                constexpr bool i = (expandedHash >> 25) & 1;
                constexpr bool s = expandedHash & (1 << 20);

                return &CPU::execDataProc<id, i, s, false>;
            }
            case arm::LS_REG_UBYTE: {
                constexpr bool i = (expandedHash >> 25) & 1;
                constexpr bool p = (expandedHash >> 24) & 1;
                constexpr bool u = (expandedHash >> 23) & 1;
                constexpr bool b = (expandedHash >> 22) & 1;
                constexpr bool w = (expandedHash >> 21) & 1;
                constexpr bool l = (expandedHash >> 20) & 1;

                if (!b && l) {
                    return &CPU::execLoadStoreRegUByte<LDR, false, p, u, i, w, thumb::INVALID_CAT>;
                } else if (b && l) {
                    return &CPU::execLoadStoreRegUByte<LDRB, false, p, u, i, w, thumb::INVALID_CAT>;
                } else if (!b && !l) {
                    return &CPU::execLoadStoreRegUByte<STR, false, p, u, i, w, thumb::INVALID_CAT>;
                } else {
                    return &CPU::execLoadStoreRegUByte<STRB, false, p, u, i, w, thumb::INVALID_CAT>;
                }
            }
            case arm::BLOCK_DATA_TRANSF: {
                constexpr bool pre = (expandedHash >> 24) & 1;
                constexpr bool up = (expandedHash >> 23) & 1;
                constexpr bool writeback = (expandedHash >> 21) & 1;
                constexpr bool forceUserRegisters = (expandedHash >> 22) & 1;
                constexpr bool load = (expandedHash >> 20) & 1;
                return &CPU::execDataBlockTransfer<false, pre, up, writeback, forceUserRegisters, load>;
            }
            case arm::BRANCH: {
                constexpr bool link = (expandedHash >> 24) & 1;
                return &CPU::handleBranch<link>;
            }
            case arm::SOFTWARE_INTERRUPT:
                return &CPU::softwareInterrupt<false>;
            case arm::INVALID_CAT:
                return &CPU::handleInvalid;
        }

        return &CPU::handleInvalid;
    }

#define DECODE_LUT_ENTRY(hash) CPU::resolveArmHashHandler<hash>()
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
#define DECODE_LUT_ENTRY_4096(hash)             \
    DECODE_LUT_ENTRY_1024(hash + 0 * 1024),     \
        DECODE_LUT_ENTRY_1024(hash + 1 * 1024), \
        DECODE_LUT_ENTRY_1024(hash + 2 * 1024), \
        DECODE_LUT_ENTRY_1024(hash + 3 * 1024)

    const CPU::InstExecutor CPU::armExeLUT[4096] = {DECODE_LUT_ENTRY_4096(0)};

#undef DECODE_LUT_ENTRY
#undef DECODE_LUT_ENTRY_4
#undef DECODE_LUT_ENTRY_16
#undef DECODE_LUT_ENTRY_64
#undef DECODE_LUT_ENTRY_512
#undef DECODE_LUT_ENTRY_1024
#undef DECODE_LUT_ENTRY_4096

} // namespace gbaemu

#endif