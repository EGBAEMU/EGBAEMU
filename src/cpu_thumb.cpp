#include "cpu.hpp"
#include <set>

namespace gbaemu
{
    InstructionExecutionInfo CPU::handleThumbLongBranchWithLink(bool h, uint16_t offset)
    {
        auto currentRegs = state.getCurrentRegs();

        InstructionExecutionInfo info{0};

        uint32_t extendedAddr = static_cast<uint32_t>(offset);
        if (h) {
            // Second instruction
            extendedAddr <<= 1;
            uint32_t pcVal = *currentRegs[regs::PC_OFFSET];
            *currentRegs[regs::PC_OFFSET] = *currentRegs[regs::LR_OFFSET] + extendedAddr;
            *currentRegs[regs::LR_OFFSET] = (pcVal + 2) | 1;
        } else {
            // First instruction
            extendedAddr <<= 12;
            *currentRegs[regs::LR_OFFSET] = *currentRegs[regs::PC_OFFSET] + 4 + extendedAddr;

            // pipeline flush -> additional cycles needed
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbUnconditionalBranch(int16_t offset)
    {
        state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(state.getCurrentPC()) + 4 + (offset * 2));

        // Unconditional branches take 2S + 1N
        InstructionExecutionInfo info{0};
        info.additionalProgCyclesN = 1;
        info.additionalProgCyclesS = 1;

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbConditionalBranch(uint8_t cond, int8_t offset)
    {
        InstructionExecutionInfo info{0};

        // Branch will be executed if condition is met
        if (conditionSatisfied(static_cast<ConditionOPCode>(cond), state)) {

            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(state.getCurrentPC()) + 4 + (static_cast<int32_t>(offset) * 2));

            // If branch executed: 2S+1N
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbMultLoadStore(bool load, uint8_t rb, uint8_t rlist)
    {
        arm::ARMInstruction wrapper;
        wrapper.params.block_data_transf.l = load;
        wrapper.params.block_data_transf.rList = static_cast<uint16_t>(rlist);

        wrapper.params.block_data_transf.u = true;
        wrapper.params.block_data_transf.w = true;
        wrapper.params.block_data_transf.p = false;
        wrapper.params.block_data_transf.s = false;
        wrapper.params.block_data_transf.rn = rb;

        return execDataBlockTransfer(wrapper, true);
    }

    InstructionExecutionInfo CPU::handleThumbPushPopRegister(bool load, bool r, uint8_t rlist)
    {
        uint16_t extendedRList = static_cast<uint16_t>(rlist);

        // 8 PC/LR Bit (0-1)
        //    0: No
        //    1: PUSH LR (R14), or POP PC (R15)
        if (r) {
            if (!load) {
                extendedRList |= 1 << regs::LR_OFFSET;
            } else {
                extendedRList |= 1 << regs::PC_OFFSET;
            }
        }

        arm::ARMInstruction wrapper;
        //  // L - Load/Store bit (0=Store to memory, 1=Load from memory)
        wrapper.params.block_data_transf.l = load;
        //  Rlist - Register List
        wrapper.params.block_data_transf.rList = extendedRList;

        // U - Up/Down Bit (0=down; subtract offset from base, 1=up; add to base)
        //      0: PUSH {Rlist}{LR}   ;store in memory, decrements SP (R13)
        //      1: POP  {Rlist}{PC}   ;load from memory, increments SP (R13)
        wrapper.params.block_data_transf.u = load;
        //  W - Write-back bit (0=no write-back, 1=write address into base)
        wrapper.params.block_data_transf.w = true;
        // P - Pre/Post (0=post; add offset after transfer, 1=pre; before trans.)

        wrapper.params.block_data_transf.p = !load;

        wrapper.params.block_data_transf.s = false;

        wrapper.params.block_data_transf.rn = regs::SP_OFFSET;

        return execDataBlockTransfer(wrapper, true);
    }

    InstructionExecutionInfo CPU::handleThumbLoadStore(const thumb::ThumbInstruction &inst)
    {
        arm::ARMInstruction wrapper;

        wrapper.params.ls_reg_ubyte.addrMode = 0;
        wrapper.params.ls_reg_ubyte.l = false;
        wrapper.params.ls_reg_ubyte.b = false;
        wrapper.params.ls_reg_ubyte.i = false;
        // we want to apply the offset before reading/writing
        wrapper.params.ls_reg_ubyte.p = true;
        // all offsets here are added
        wrapper.params.ls_reg_ubyte.u = true;
        wrapper.params.ls_reg_ubyte.w = false;
        wrapper.params.ls_reg_ubyte.rn = 0;
        wrapper.params.ls_reg_ubyte.rd = 0;

        switch (inst.cat) {
            case thumb::ThumbInstructionCategory::LD_ST_REL_OFF: {
                // load/store
                wrapper.params.ls_reg_ubyte.l = inst.params.ld_st_rel_off.l;
                // byte/word
                wrapper.params.ls_reg_ubyte.b = inst.params.ld_st_rel_off.b;
                // base reg
                wrapper.params.ls_reg_ubyte.rn = inst.params.ld_st_rel_off.rb;
                // target reg
                wrapper.params.ls_reg_ubyte.rd = inst.params.ld_st_rel_off.rd;
                // register offset with LSL#0
                wrapper.params.ls_reg_ubyte.addrMode = (arm::ShiftType::LSL << 5) | static_cast<uint32_t>(inst.params.ld_st_rel_off.ro);
                break;
            }

            case thumb::ThumbInstructionCategory::LD_ST_IMM_OFF: {
                // load/store
                wrapper.params.ls_reg_ubyte.l = inst.params.ld_st_imm_off.l;
                // byte/word
                wrapper.params.ls_reg_ubyte.b = inst.params.ld_st_imm_off.b;
                // immediate
                wrapper.params.ls_reg_ubyte.i = true;
                // offset is in words(steps of 4) iff !b
                wrapper.params.ls_reg_ubyte.addrMode = static_cast<uint32_t>(inst.params.ld_st_imm_off.offset) << (inst.params.ld_st_imm_off.b ? 0 : 2);
                // base reg
                wrapper.params.ls_reg_ubyte.rn = inst.params.ld_st_imm_off.rb;
                // target reg
                wrapper.params.ls_reg_ubyte.rd = inst.params.ld_st_imm_off.rd;
                break;
            }

            case thumb::ThumbInstructionCategory::LD_ST_REL_SP: {
                // load/store
                wrapper.params.ls_reg_ubyte.l = inst.params.ld_st_rel_sp.l;
                // immediate
                wrapper.params.ls_reg_ubyte.i = true;
                // offset
                // 7-0    nn - Unsigned Offset              (0-1020, step 4)
                wrapper.params.ls_reg_ubyte.addrMode = (static_cast<uint32_t>(inst.params.ld_st_rel_sp.offset) << 2);
                // target reg
                wrapper.params.ls_reg_ubyte.rd = inst.params.ld_st_rel_sp.rd;
                // base reg
                wrapper.params.ls_reg_ubyte.rn = regs::SP_OFFSET;
                break;
            }
            case thumb::ThumbInstructionCategory::PC_LD: {
                // load
                wrapper.params.ls_reg_ubyte.l = true;
                // immediate
                wrapper.params.ls_reg_ubyte.i = true;
                // offset
                wrapper.params.ls_reg_ubyte.addrMode = (static_cast<uint32_t>(inst.params.pc_ld.offset) << 2);
                // target reg
                wrapper.params.ls_reg_ubyte.rd = inst.params.pc_ld.rd;
                // base reg
                wrapper.params.ls_reg_ubyte.rn = regs::PC_OFFSET;
                break;
            }

            default:
                break;
        }

        // i is inverted immediate bool
        wrapper.params.ls_reg_ubyte.i = !wrapper.params.ls_reg_ubyte.i;

        return execLoadStoreRegUByte(wrapper, true);
    }

    InstructionExecutionInfo CPU::handleThumbLoadStoreSignHalfword(const thumb::ThumbInstruction &inst)
    {
        bool pre = true;
        bool up = true;
        bool load = false;
        bool writeback = false;
        bool sign = false;
        uint8_t rn = 0;
        uint8_t rd = 0;
        uint32_t offset = 0;
        uint8_t transferSize = 16;

        switch (inst.cat) {
            case thumb::ThumbInstructionCategory::LD_ST_SIGN_EXT: {
                if (!inst.params.ld_st_sign_ext.h && !inst.params.ld_st_sign_ext.s) {
                    transferSize = 16;
                    load = false;
                } else {
                    transferSize = inst.params.ld_st_sign_ext.h ? 16 : 8;
                    sign = inst.params.ld_st_sign_ext.s;
                    load = true;
                }
                rn = inst.params.ld_st_sign_ext.rb;
                rd = inst.params.ld_st_sign_ext.rd;
                offset = state.accessReg(inst.params.ld_st_sign_ext.ro);
                break;
            }

            case thumb::ThumbInstructionCategory::LD_ST_HW: {
                load = inst.params.ld_st_hw.l;
                // 10-6   nn - Unsigned Offset              (0-62, step 2)
                offset = static_cast<uint32_t>(inst.params.ld_st_hw.offset) << 1;
                rn = inst.params.ld_st_hw.rb;
                rd = inst.params.ld_st_hw.rd;
                break;
            }

            default:
                break;
        }

        return execHalfwordDataTransferImmRegSignedTransfer(pre,
                                                            up,
                                                            load,
                                                            writeback,
                                                            sign,
                                                            rn,
                                                            rd,
                                                            offset,
                                                            transferSize,
                                                            true);
    }

    InstructionExecutionInfo CPU::handleThumbAddOffsetToStackPtr(bool s, uint8_t offset)
    {
        // nn - Unsigned Offset    (0-508, step 4)
        uint32_t extOffset = static_cast<uint32_t>(offset) << 2;

        if (s) {
            // 1: ADD  SP,#-nn      ;SP = SP - nn
            state.accessReg(regs::SP_OFFSET) -= extOffset;
        } else {
            // 0: ADD  SP,#nn       ;SP = SP + nn
            state.accessReg(regs::SP_OFFSET) += extOffset;
        }

        // Execution Time: 1S
        InstructionExecutionInfo info{0};

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd)
    {
        auto currentRegs = state.getCurrentRegs();

        // bool sp
        //          0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
        //          1: ADD  Rd,SP,#nn    ;Rd = SP + nn
        // nn step 4
        *currentRegs[rd] = (sp ? *currentRegs[regs::SP_OFFSET] : ((*currentRegs[regs::PC_OFFSET] + 4) & ~2)) + (static_cast<uint32_t>(offset) << 2);

        // Execution Time: 1S
        InstructionExecutionInfo info{0};
        return info;
    }

    InstructionExecutionInfo CPU::handleThumbAddSubtract(thumb::ThumbInstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        auto currentRegs = state.getCurrentRegs();

        uint32_t rsVal = *currentRegs[rs];
        uint32_t rnVal = *currentRegs[rn_offset];

        uint64_t result;

        switch (insID) {
            case thumb::ADD:
                result = rsVal + rnVal;
                break;
            case thumb::SUB:
                result = static_cast<int64_t>(rsVal) - static_cast<int64_t>(rnVal);
                rnVal = (-rnVal) & 0x0FFFFFFFF;
                break;
            case thumb::ADD_SHORT_IMM:
                result = rsVal + rn_offset;
                break;
            case thumb::SUB_SHORT_IMM:
                result = static_cast<int64_t>(rsVal) - static_cast<int64_t>(rn_offset);
                rn_offset = (-rn_offset) & 0x0FFFFFFFF;
                break;
            default:
                break;
        }

        *currentRegs[rd] = result;

        bool isAdd = (insID == thumb::ADD) || (insID == thumb::ADD_SHORT_IMM);

        setFlags(
            result,
            (rsVal >> 31) & 1,
            ((insID == thumb::SUB || insID == thumb::ADD ? rnVal : rn_offset) >> 31) & 1,
            true,
            true,
            true,
            true,
            !isAdd);

        InstructionExecutionInfo info{0};
        return info;
    }

    InstructionExecutionInfo CPU::handleThumbMovCmpAddSubImm(thumb::ThumbInstructionID ins, uint8_t rd, uint8_t offset)
    {
        // ARM equivalents for MOV/CMP/ADD/SUB are MOVS/CMP/ADDS/SUBS same format.

        arm::ARMInstruction armIns;
        armIns.params.data_proc_psr_transf.i = true;
        armIns.params.data_proc_psr_transf.s = true;
        armIns.params.data_proc_psr_transf.rd = rd;
        armIns.params.data_proc_psr_transf.rn = rd;
        armIns.params.data_proc_psr_transf.operand2 = offset;

        switch (ins) {
            case thumb::ADD:
                armIns.id = arm::ADD;
                break;
            case thumb::SUB:
                armIns.id = arm::SUB;
                break;
            case thumb::CMP:
                armIns.id = arm::CMP;
                break;
            case thumb::MOV:
                armIns.id = arm::MOV;
                break;

            default:
                break;
        }

        return execDataProc(armIns);
    }

    InstructionExecutionInfo CPU::handleThumbMoveShiftedReg(thumb::ThumbInstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset)
    {
        uint64_t rsValue = static_cast<uint64_t>(state.accessReg(rs));
        uint64_t rdValue = 0;

        arm::ShiftType shiftType = arm::ShiftType::LSL;
        switch (ins) {
            case thumb::LSL:
                shiftType = arm::ShiftType::LSL;
                break;
            case thumb::LSR:
                shiftType = arm::ShiftType::LSR;
                break;
            case thumb::ASR:
                shiftType = arm::ShiftType::ASR;
                break;
            default:
                break;
        }
        rdValue = arm::shift(rsValue, shiftType, offset, state.getFlag(cpsr_flags::C_FLAG), true);

        state.accessReg(rd) = static_cast<uint32_t>(rdValue & 0x0FFFFFFFF);

        // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
        setFlags(
            rdValue,
            false,
            false,
            true,                             // n Flag
            true,                             // z Flag
            false,                            // v Flag
            ins != thumb::LSL || offset != 0, // c flag
            false);

        // Execution Time: 1S
        InstructionExecutionInfo info{0};
        return info;
    }

    InstructionExecutionInfo CPU::handleThumbBranchXCHG(thumb::ThumbInstructionID id, uint8_t rd, uint8_t rs)
    {
        InstructionExecutionInfo info{0};

        auto currentRegs = state.getCurrentRegs();

        uint32_t rsValue = *currentRegs[rs] + (rs == 15 ? 4 : 0);
        uint32_t rdValue = *currentRegs[rd] + (rd == 15 ? 4 : 0);

        if (rd == 15 && (id == thumb::ADD || id == thumb::MOV)) {
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;
        }

        switch (id) {
            case thumb::ADD:
                *currentRegs[rd] = rdValue + rsValue;
                break;

            case thumb::CMP: {

                uint64_t result = static_cast<uint64_t>(rdValue) - static_cast<uint64_t>(rsValue);

                setFlags(result,
                         (rdValue >> 31) & 1,
                         (rsValue >> 31) & 1,
                         true,
                         true,
                         true,
                         true,
                         false);
                break;
            }

            case thumb::MOV:
                *currentRegs[rd] = rsValue;
                break;

            case thumb::BX: {
                // If the first bit of rs is set
                bool stayInThumbMode = rsValue & 0x00000001;

                if (!stayInThumbMode) {
                    state.setFlag(cpsr_flags::THUMB_STATE, false);
                }

                // Except for BX R15: CPU switches to ARM state, and PC is auto-aligned as (($+4) AND NOT 2).
                if (rs == 15) {
                    rsValue &= ~2;
                }

                // Change the PC to the address given by rs. Note that we have to mask out the thumb switch bit.
                state.accessReg(regs::PC_OFFSET) = rsValue & ~1;
                info.additionalProgCyclesN = 1;
                info.additionalProgCyclesS = 1;

                break;
            }

            case thumb::NOP:
            default:
                break;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbALUops(thumb::ThumbInstructionID instID, uint8_t rs, uint8_t rd)
    {

        static const std::set<thumb::ThumbInstructionID> updateNegative{
            thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

            thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR,

            thumb::MUL, thumb::AND, thumb::EOR, thumb::TST, thumb::ORR, thumb::BIC, thumb::MVN};

        static const std::set<thumb::ThumbInstructionID> updateZero{
            thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

            thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR,

            thumb::MUL, thumb::AND, thumb::EOR, thumb::TST, thumb::ORR, thumb::BIC, thumb::MVN};

        static const std::set<thumb::ThumbInstructionID> updateCarry{
            thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN,

            thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR};

        static const std::set<thumb::ThumbInstructionID> updateOverflow{
            thumb::ADC, thumb::SBC, thumb::NEG, thumb::CMP, thumb::CMN};

        static const std::set<thumb::ThumbInstructionID> dontUpdateRD{
            thumb::TST, thumb::CMP, thumb::CMN};

        static const std::set<thumb::ThumbInstructionID> shiftOps{
            thumb::LSL, thumb::LSR, thumb::ASR, thumb::ROR};

        static const std::set<thumb::ThumbInstructionID> invertCarry{
            thumb::SBC, thumb::CMP, thumb::NEG, thumb::SUB};

        InstructionExecutionInfo info{0};

        if (shiftOps.find(instID) != shiftOps.end()) {
            info.cycleCount = 1;
        }

        auto currentRegs = state.getCurrentRegs();

        uint64_t rsValue = *currentRegs[rs];
        uint64_t rdValue = *currentRegs[rd];
        int64_t resultValue;

        uint8_t shiftAmount = rsValue & 0xFF;

        bool carry = state.getFlag(cpsr_flags::C_FLAG);

        switch (instID) {

            case thumb::ADC:
                resultValue = rdValue + rsValue + (carry ? 1 : 0);
                break;
            case thumb::SBC:
                resultValue = static_cast<int64_t>(rdValue) - static_cast<int64_t>(rsValue) - (carry ? 0 : 1);
                rsValue = (-rsValue) & 0x0FFFFFFFF;
                break;
            case thumb::NEG:
                resultValue = 0 - static_cast<int64_t>(rsValue);
                rdValue = 0;
                rsValue = (-rsValue) & 0x0FFFFFFFF;
                break;
            case thumb::CMP:
                resultValue = static_cast<int64_t>(rdValue) - static_cast<int64_t>(rsValue);
                rsValue = (-rsValue) & 0x0FFFFFFFF;
                break;
            case thumb::CMN:
                resultValue = rdValue + rsValue;
                break;

            case thumb::LSL:
                resultValue = arm::shift(rdValue, arm::ShiftType::LSL, shiftAmount, carry, false);
                break;
            case thumb::LSR:
                resultValue = arm::shift(rdValue, arm::ShiftType::LSR, shiftAmount, carry, false);
                break;
            case thumb::ASR:
                resultValue = arm::shift(rdValue, arm::ShiftType::ASR, shiftAmount, carry, false);
                break;
            case thumb::ROR:
                resultValue = arm::shift(rdValue, arm::ShiftType::ROR, shiftAmount, carry, false);
                break;

            case thumb::MUL: {
                resultValue = rdValue * rsValue;

                if (((rsValue >> 8) & 0x00FFFFFF) == 0 || ((rsValue >> 8) & 0x00FFFFFF) == 0x00FFFFFF) {
                    info.cycleCount += 1;
                } else if (((rsValue >> 16) & 0x0000FFFF) == 0 || ((rsValue >> 16) & 0x0000FFFF) == 0x0000FFFF) {
                    info.cycleCount += 2;
                } else if (((rsValue >> 24) & 0x000000FF) == 0 || ((rsValue >> 24) & 0x000000FF) == 0x000000FF) {
                    info.cycleCount += 3;
                } else {
                    info.cycleCount += 4;
                }
                break;
            }
            case thumb::TST:
            case thumb::AND:
                resultValue = rdValue & rsValue;
                break;
            case thumb::EOR:
                resultValue = rdValue ^ rsValue;
                break;
            case thumb::ORR:
                resultValue = rdValue | rsValue;
                break;
            case thumb::BIC:
                resultValue = rdValue & ~rsValue;
                break;
            case thumb::MVN:
                resultValue = ~rsValue;
                break;

            default:
                break;
        }

        setFlags(
            resultValue,
            //TODO this does probably not work for all instructions!!!
            (rdValue >> 31) & 1,
            (rsValue >> 31) & 1,
            updateNegative.find(instID) != updateNegative.end(),
            updateZero.find(instID) != updateZero.end(),
            updateOverflow.find(instID) != updateOverflow.end(),
            updateCarry.find(instID) != updateCarry.end() && (shiftOps.find(instID) == shiftOps.end() || shiftAmount != 0),
            invertCarry.find(instID) != invertCarry.end());

        if (dontUpdateRD.find(instID) == dontUpdateRD.end())
            *currentRegs[rd] = resultValue;

        return info;
    }
} // namespace gbaemu
