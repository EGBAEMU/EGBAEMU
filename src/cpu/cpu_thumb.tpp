#include "cpu.hpp"
#include "logging.hpp"
#include "swi.hpp"
#include "util.hpp"

#include "decode/inst.hpp"
#include "decode/inst_thumb.hpp"

#include <set>

namespace gbaemu
{
    template <bool h>
    void CPU::handleThumbLongBranchWithLink(uint32_t instruction)
    {
        uint16_t offset = instruction & 0x07FF;
        auto currentRegs = state.getCurrentRegs();

        uint32_t extendedAddr = static_cast<uint32_t>(offset);
        if (h) {
            // Second instruction
            extendedAddr <<= 1;
            uint32_t pcVal = *currentRegs[regs::PC_OFFSET];
            *currentRegs[regs::PC_OFFSET] = *currentRegs[regs::LR_OFFSET] + extendedAddr;
            // Note that pc is already incremented by 2
            *currentRegs[regs::LR_OFFSET] = pcVal | 1;

            // pipeline flush -> additional cycles needed
            // This is a branch instruction so we need to consider self branches!
            refillPipelineAfterBranch<true>();
        } else {
            // First instruction
            extendedAddr <<= 12;
            // Note that pc is already incremented by 2
            // The destination address range is (PC+4)-400000h..+3FFFFEh -> sign extension is needed
            // Apply sign extension!
            extendedAddr = signExt<int32_t, uint32_t, 23>(extendedAddr);
            *currentRegs[regs::LR_OFFSET] = *currentRegs[regs::PC_OFFSET] + 2 + extendedAddr;
        }
    }

    void CPU::handleThumbUnconditionalBranch(uint32_t instruction)
    {
        int16_t offset = signExt<int16_t, uint16_t, 11>(static_cast<uint16_t>(instruction & 0x07FF));

        // Note that pc is already incremented by 2
        state.getPC() += static_cast<uint32_t>(2 + static_cast<int32_t>(offset) * 2);

        // Unconditional branches take 2S + 1N
        // This is a branch instruction so we need to consider self branches!
        refillPipelineAfterBranch<true>();
    }

    void CPU::handleThumbConditionalBranch(uint32_t instruction)
    {
        int8_t offset = static_cast<int8_t>(instruction & 0x0FF);
        uint8_t cond = (instruction >> 8) & 0x0F;

        // Branch will be executed if condition is met
        if (conditionSatisfied(static_cast<ConditionOPCode>(cond), state)) {
            // Note that pc is already incremented by 2
            state.getPC() += 2 + (static_cast<int32_t>(offset) * 2);

            // If branch executed: 2S+1N
            // This is a branch instruction so we need to consider self branches!
            refillPipelineAfterBranch<true>();
        }
    }

    template <bool s>
    void CPU::handleThumbAddOffsetToStackPtr(uint32_t instruction)
    {
        uint8_t offset = instruction & 0x7F;
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

    template <bool sp>
    void CPU::handleThumbRelAddr(uint32_t instruction)
    {

        uint8_t offset = instruction & 0x0FF;
        uint8_t rd = (instruction >> 8) & 0x7;

        auto currentRegs = state.getCurrentRegs();

        // bool sp
        //          0: ADD  Rd,PC,#nn    ;Rd = (($+4) AND NOT 2) + nn
        //          1: ADD  Rd,SP,#nn    ;Rd = SP + nn
        // nn step 4
        // Note that pc is already incremented by 2
        *currentRegs[rd] = (sp ? *currentRegs[regs::SP_OFFSET] : ((*currentRegs[regs::PC_OFFSET] + 2) & ~2)) + (static_cast<uint32_t>(offset) << 2);

        // Execution Time: 1S
    }

    template <InstructionID id>
    void CPU::handleThumbMoveShiftedReg(uint32_t inst)
    {
        constexpr shifts::ShiftType shiftType = getShiftType(id);

        uint8_t rs = (inst >> 3) & 0x7;
        uint8_t rd = inst & 0x7;
        uint8_t offset = (inst >> 6) & 0x1F;

        uint32_t rsValue = state.accessReg(rs);
        uint64_t rdValue = shifts::shift<true>(rsValue, shiftType, offset, state.getFlag<cpsr_flags::C_FLAG>());

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
    void CPU::handleThumbBranchXCHG(uint32_t instruction)
    {
        // Destination Register most significant bit (or BL/BLX flag)
        uint8_t msbDst = (instruction >> 7) & 1;
        // Source Register most significant bit
        uint8_t msbSrc = (instruction >> 6) & 1;

        uint8_t rd = (instruction & 0x7) | (msbDst << 3);
        uint8_t rs = ((instruction >> 3) & 0x7) | (msbSrc << 3);

        auto currentRegs = state.getCurrentRegs();

        // Note that pc is already incremented by 2
        uint32_t rsValue = *currentRegs[rs] + (rs == regs::PC_OFFSET ? 2 : 0);
        uint32_t rdValue = *currentRegs[rd] + (rd == regs::PC_OFFSET ? 2 : 0);

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

                // If msbDst set this wont execute
                // if (msbDst)
                    // return;

                // If the first bit of rs is set
                bool stayInThumbMode = rsValue & 0x00000001;

                // Except for BX R15: CPU switches to ARM state, and PC is auto-aligned as (($+4) AND NOT 2).
                // Automatically handled by refill pipeline
                //if (rs == regs::PC_OFFSET) {
                //    rsValue &= ~2;
                //}

                // Change the PC to the address given by rs. Note that we have to mask out the thumb switch bit.
                *currentRegs[regs::PC_OFFSET] = rsValue & ~1;

                // This is a branch instruction so we need to refill the pipeline!
                if (!stayInThumbMode) {
                    state.setFlag<cpsr_flags::THUMB_STATE>(false);
                    refillPipelineAfterBranch<false>();
                } else {
                    refillPipelineAfterBranch<true>();
                }

                break;
            }

            case NOP:
            default:
                break;
        }

        if ((id == ADD || id == MOV) && rd == regs::PC_OFFSET) {
            refillPipelineAfterBranch<true>();
        }
    }

} // namespace gbaemu

#include "create_thumb_lut.tpp"
