
#include "io/dma.hpp"
#include "io/interrupts.hpp"
#include "io/timer.hpp"

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

            // This is a branch instruction so we need to consider self branches!
            info.forceBranch = true;
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

        // This is a branch instruction so we need to consider self branches!
        info.forceBranch = true;

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

            // This is a branch instruction so we need to consider self branches!
            info.forceBranch = true;
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
                wrapper.params.ls_reg_ubyte.addrMode = (shifts::ShiftType::LSL << 5) | static_cast<uint32_t>(inst.params.ld_st_rel_off.ro);
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

    InstructionExecutionInfo CPU::handleThumbAddSubtract(InstructionID insID, uint8_t rd, uint8_t rs, uint8_t rn_offset)
    {
        arm::ARMInstruction wrapper;
        wrapper.id = insID;
        // Imm?
        wrapper.params.data_proc_psr_transf.i = insID == ADD_SHORT_IMM || insID == SUB_SHORT_IMM;
        // encode rs register or imm (both at lower bits and rest 0 -> ROR#0 / LSL#0)
        wrapper.params.data_proc_psr_transf.operand2 = rn_offset;
        // Only for MSR & MRS relevant
        wrapper.params.data_proc_psr_transf.r = false;
        // First operand
        wrapper.params.data_proc_psr_transf.rn = rs;
        // Dest reg
        wrapper.params.data_proc_psr_transf.rd = rd;
        // We want to update flags!
        wrapper.params.data_proc_psr_transf.s = true;

        return execDataProc(wrapper, true);
    }

    InstructionExecutionInfo CPU::handleThumbMovCmpAddSubImm(InstructionID ins, uint8_t rd, uint8_t offset)
    {
        // ARM equivalents for MOV/CMP/ADD/SUB are MOVS/CMP/ADDS/SUBS same format.

        arm::ARMInstruction armIns;
        armIns.params.data_proc_psr_transf.i = true;
        armIns.params.data_proc_psr_transf.s = true;
        armIns.params.data_proc_psr_transf.rd = rd;
        armIns.params.data_proc_psr_transf.rn = rd;
        armIns.params.data_proc_psr_transf.operand2 = offset;
        armIns.id = ins;

        return execDataProc(armIns);
    }

    InstructionExecutionInfo CPU::handleThumbMoveShiftedReg(InstructionID ins, uint8_t rs, uint8_t rd, uint8_t offset)
    {
        uint64_t rsValue = static_cast<uint64_t>(state.accessReg(rs));
        uint64_t rdValue = 0;

        shifts::ShiftType shiftType = shifts::ShiftType::LSL;
        switch (ins) {
            case LSL:
                shiftType = shifts::ShiftType::LSL;
                break;
            case LSR:
                shiftType = shifts::ShiftType::LSR;
                break;
            case ASR:
                shiftType = shifts::ShiftType::ASR;
                break;
            default:
                break;
        }
        rdValue = shifts::shift(rsValue, shiftType, offset, state.getFlag(cpsr_flags::C_FLAG), true);

        state.accessReg(rd) = static_cast<uint32_t>(rdValue & 0x0FFFFFFFF);

        // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
        setFlags(
            rdValue,
            false,
            false,
            true,  // n Flag
            true,  // z Flag
            false, // v Flag
            true,  // c flag
            false);

        // Execution Time: 1S
        InstructionExecutionInfo info{0};
        return info;
    }

    InstructionExecutionInfo CPU::handleThumbBranchXCHG(InstructionID id, uint8_t rd, uint8_t rs)
    {
        InstructionExecutionInfo info{0};

        auto currentRegs = state.getCurrentRegs();

        uint32_t rsValue = *currentRegs[rs] + (rs == 15 ? 4 : 0);
        uint32_t rdValue = *currentRegs[rd] + (rd == 15 ? 4 : 0);

        if (rd == 15 && (id == ADD || id == MOV)) {
            info.additionalProgCyclesN = 1;
            info.additionalProgCyclesS = 1;
        }

        switch (id) {
            case ADD:
                *currentRegs[rd] = rdValue + rsValue;
                break;

            case CMP: {

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

            case MOV:
                *currentRegs[rd] = rsValue;
                break;

            case BX: {
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

                // This is a branch instruction so we need to consider self branches!
                info.forceBranch = true;

                break;
            }

            case NOP:
            default:
                break;
        }

        return info;
    }

    InstructionExecutionInfo CPU::handleThumbALUops(InstructionID instID, uint8_t rs, uint8_t rd)
    {
        arm::ARMInstruction wrapper;
        wrapper.id = instID;

        // We have no immediate
        wrapper.params.data_proc_psr_transf.i = false;
        // Only relevant for MSR/MRS
        wrapper.params.data_proc_psr_transf.r = false;
        // We want to update the flags!
        wrapper.params.data_proc_psr_transf.s = true;
        // Destination reg
        wrapper.params.data_proc_psr_transf.rd = rd;
        // First operand
        wrapper.params.data_proc_psr_transf.rn = rd;

        shifts::ShiftType shiftType = shifts::ShiftType::LSL;

        switch (instID) {
            case LSL:
                shiftType = shifts::ShiftType::LSL;
                break;
            case LSR:
                shiftType = shifts::ShiftType::LSR;
                break;
            case ASR:
                shiftType = shifts::ShiftType::ASR;
                break;
            case ROR:
                shiftType = shifts::ShiftType::ROR;
                break;
            default:
                break;
        }

        switch (instID) {
            case LSL:
            case LSR:
            case ASR:
            case ROR:
                // Patch id to MOV as shifts is done during operand2 evaluation
                wrapper.id = MOV;
                //TODO previously we did: uint8_t shiftAmount = rsValue & 0xFF; we might need to enforce this in execDataProc as well!
                // Set bit 4 for shiftAmountFromReg flag & move regs at their positions & include the shifttype to use
                wrapper.params.data_proc_psr_transf.operand2 = (static_cast<uint16_t>(1) << 4) | rd | (static_cast<uint16_t>(rs) << 8) | (static_cast<uint16_t>(shiftType) << 5);
                break;

            case MUL: {
                return handleMultAcc(false, true, rd, 0, rs, rd);
            }

            default:
                // We only want the value of rs & nothing else
                wrapper.params.data_proc_psr_transf.operand2 = rs;
                break;
        }

        return execDataProc(wrapper, true);
    }
} // namespace gbaemu
