#include "cpu.hpp"
#include "logging.hpp"
#include "swi.hpp"
#include "util.hpp"

#include <iostream>
#include <set>

namespace gbaemu
{

    void CPU::handleThumbLongBranchWithLink(bool h, uint16_t offset)
    {
        auto currentRegs = state.getCurrentRegs();

        uint32_t extendedAddr = static_cast<uint32_t>(offset);
        if (h) {
            // Second instruction
            extendedAddr <<= 1;
            uint32_t pcVal = *currentRegs[regs::PC_OFFSET];
            *currentRegs[regs::PC_OFFSET] = *currentRegs[regs::LR_OFFSET] + extendedAddr;
            *currentRegs[regs::LR_OFFSET] = (pcVal + 2) | 1;

            // pipeline flush -> additional cycles needed
            // This is a branch instruction so we need to consider self branches!
            state.cpuInfo.forceBranch = true;
        } else {
            // First instruction
            extendedAddr <<= 12;
            // The destination address range is (PC+4)-400000h..+3FFFFEh -> sign extension is needed
            // Apply sign extension!
            extendedAddr = signExt<int32_t, uint32_t, 23>(extendedAddr);
            *currentRegs[regs::LR_OFFSET] = *currentRegs[regs::PC_OFFSET] + 4 + extendedAddr;
        }
    }

    void CPU::handleThumbUnconditionalBranch(int16_t offset)
    {
        state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(state.getCurrentPC() + 4 + static_cast<int32_t>(offset) * 2);

        // Unconditional branches take 2S + 1N
        // This is a branch instruction so we need to consider self branches!
        state.cpuInfo.forceBranch = true;
    }

    void CPU::handleThumbConditionalBranch(uint8_t cond, int8_t offset)
    {

        // Branch will be executed if condition is met
        if (conditionSatisfied(static_cast<ConditionOPCode>(cond), state)) {

            state.accessReg(regs::PC_OFFSET) = static_cast<uint32_t>(static_cast<int32_t>(state.getCurrentPC()) + 4 + (static_cast<int32_t>(offset) * 2));

            // If branch executed: 2S+1N
            // This is a branch instruction so we need to consider self branches!
            state.cpuInfo.forceBranch = true;
        }
    }

    void CPU::handleThumbAddOffsetToStackPtr(bool s, uint8_t offset)
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
    }

    void CPU::handleThumbRelAddr(bool sp, uint8_t offset, uint8_t rd)
    {
        auto currentRegs = state.getCurrentRegs();

        // bool sp
        //          0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
        //          1: ADD  Rd,SP,#nn    ;Rd = SP + nn
        // nn step 4
        *currentRegs[rd] = (sp ? *currentRegs[regs::SP_OFFSET] : ((*currentRegs[regs::PC_OFFSET] + 4) & ~2)) + (static_cast<uint32_t>(offset) << 2);

        // Execution Time: 1S
    }

    static constexpr shifts::ShiftType getShiftType(InstructionID id)
    {
        switch (id) {
            case LSL:
                return shifts::ShiftType::LSL;
            case LSR:
                return shifts::ShiftType::LSR;
            case ASR:
                return shifts::ShiftType::ASR;
            case ROR:
                return shifts::ShiftType::ROR;
            default:
                return shifts::ShiftType::LSL;
        }
    }

    template <InstructionID id>
    void CPU::handleThumbMoveShiftedReg(uint8_t rs, uint8_t rd, uint8_t offset)
    {
        constexpr shifts::ShiftType shiftType = getShiftType(id);

        uint32_t rsValue = state.accessReg(rs);
        uint64_t rdValue = shifts::shift(rsValue, shiftType, offset, state.getFlag<cpsr_flags::C_FLAG>(), true);

        state.accessReg(rd) = static_cast<uint32_t>(rdValue & 0x0FFFFFFFF);

        // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
        setFlags<true,  // n Flag
                 true,  // z Flag
                 false, // v Flag
                 true,  // c flag
                 false>(
            rdValue,
            false,
            false);

        // Execution Time: 1S
    }

    template <InstructionID id>
    void CPU::handleThumbBranchXCHG(uint8_t rd, uint8_t rs)
    {

        auto currentRegs = state.getCurrentRegs();

        uint32_t rsValue = *currentRegs[rs] + (rs == regs::PC_OFFSET ? 4 : 0);
        uint32_t rdValue = *currentRegs[rd] + (rd == regs::PC_OFFSET ? 4 : 0);

        if (rd == regs::PC_OFFSET && (id == ADD || id == MOV)) {
            state.cpuInfo.forceBranch = true;
        }

        switch (id) {
            case ADD:
                *currentRegs[rd] = rdValue + rsValue;
                break;

            case CMP: {

                uint64_t result = static_cast<int64_t>(rdValue) - static_cast<int64_t>(rsValue);

                setFlags<true,
                         true,
                         true,
                         true,
                         true>(result,
                               (rdValue >> 31) & 1,
                               (rsValue >> 31) & 1 ? false : true);
                break;
            }

            case MOV:
                *currentRegs[rd] = rsValue;
                break;

            case BX: {
                // If the first bit of rs is set
                bool stayInThumbMode = rsValue & 0x00000001;

                if (!stayInThumbMode) {
                    state.setFlag<cpsr_flags::THUMB_STATE>(false);
                }

                // Except for BX R15: CPU switches to ARM state, and PC is auto-aligned as (($+4) AND NOT 2).
                if (rs == 15) {
                    rsValue &= ~2;
                }

                // Change the PC to the address given by rs. Note that we have to mask out the thumb switch bit.
                *currentRegs[regs::PC_OFFSET] = rsValue & ~1;

                // This is a branch instruction so we need to consider self branches!
                state.cpuInfo.forceBranch = true;

                break;
            }

            case NOP:
            default:
                break;
        }
    }

    template <InstructionID id, InstructionID origID>
    void CPU::handleThumbALUops(uint8_t rs, uint8_t rd)
    {

        constexpr shifts::ShiftType shiftType = getShiftType(origID);

        uint16_t operand2;

        switch (id) {
            case MOV:
                //TODO previously we did: uint8_t shiftAmount = rsValue & 0xFF; we might need to enforce this in execDataProc as well!
                // Set bit 4 for shiftAmountFromReg flag & move regs at their positions & include the shifttype to use
                operand2 = (static_cast<uint16_t>(1) << 4) | rd | (static_cast<uint16_t>(rs) << 8) | (static_cast<uint16_t>(shiftType) << 5);
                break;

            case MUL: {
                handleMultAcc<MUL>(true, rd, 0, rs, rd);
                return;
            }

            default:
                // We only want the value of rs & nothing else
                operand2 = rs;
                break;
        }

        execDataProc<id, true>(false, true, rd, rd, operand2);
    }

    template void CPU::handleThumbMoveShiftedReg<LSL>(uint8_t rs, uint8_t rd, uint8_t offset);
    template void CPU::handleThumbMoveShiftedReg<LSR>(uint8_t rs, uint8_t rd, uint8_t offset);
    template void CPU::handleThumbMoveShiftedReg<ASR>(uint8_t rs, uint8_t rd, uint8_t offset);

    template void CPU::handleThumbBranchXCHG<ADD>(uint8_t rd, uint8_t rs);
    template void CPU::handleThumbBranchXCHG<CMP>(uint8_t rd, uint8_t rs);
    template void CPU::handleThumbBranchXCHG<MOV>(uint8_t rd, uint8_t rs);
    template void CPU::handleThumbBranchXCHG<BX>(uint8_t rd, uint8_t rs);

    template void CPU::handleThumbALUops<AND, AND>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<EOR, EOR>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MOV, LSL>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MOV, LSR>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MOV, ASR>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MOV, ROR>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<ADC, ADC>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<SBC, SBC>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<TST, TST>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<NEG, NEG>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<CMP, CMP>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<CMN, CMN>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<ORR, ORR>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MUL, MUL>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<BIC, BIC>(uint8_t rs, uint8_t rd);
    template void CPU::handleThumbALUops<MVN, MVN>(uint8_t rs, uint8_t rd);

} // namespace gbaemu
