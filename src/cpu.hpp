#ifndef CPU_HPP
#define CPU_HPP

#include "cpu_state.hpp"
#include "inst_arm.hpp"
#include "inst_thumb.hpp"
#include "regs.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace gbaemu
{

/*
    instruction set
    http://www.ecs.csun.edu/~smirzaei/docs/ece425/arm7tdmi_instruction_set_reference.pdf
 */
class CPU
{
  public:
    CPUState state;

    void fetch()
    {

        // TODO: flush if branch happened?, else continue normally

        // propagate pipeline
        state.pipeline.fetch.lastInstruction = state.pipeline.fetch.instruction;
        state.pipeline.fetch.lastReadData = state.pipeline.fetch.readData;

        /* assume 26 ARM state here */
        /* pc is at [25:2] */
        uint32_t pc = (state.accessReg(regs::PC_OFFSET) >> 2) & 0x03FFFFFF;
        state.pipeline.fetch.instruction = static_cast<uint32_t *>(state.memory.mem)[pc];

        //TODO where do we want to update pc? (+4)
    }

    void decode()
    {
        state.pipeline.decode.lastInstruction = state.pipeline.decode.instruction;
        state.pipeline.decode.instruction = state.decoder->decode(state.pipeline.fetch.lastInstruction);
    }

    void execute()
    {
        state.pipeline.decode.lastInstruction->execute(&state);
    }

    void step()
    {
        // TODO: Check for interrupt here
        fetch();
        decode();
        execute();
    }

    void execAdd(bool s, uint32_t rn, uint32_t rd, uint32_t shiftOperand)
    {
        auto currentRegs = state.getCurrentRegs();

        // Get the value of the rn register
        int32_t rnValueSigned = *currentRegs[rn];
        // The value of the shift operand as signed int
        int32_t shiftOperandSigned = static_cast<int32_t>(shiftOperand);

        // Construt the sum. Give it some more bits so we can catch an overflow.
        int64_t resultSigned = rnValueSigned + shiftOperandSigned;
        uint64_t result = static_cast<uint64_t>(resultSigned);

        // Write the value back to rd
        *currentRegs[rd] = static_cast<uint32_t>(result & 0x00000000FFFFFFFF);

        // TODO Maybe clear flags?

        // If s is set, we have to update the N, Z, V and C Flag
        if (s) {
            if (result == 0) {
                state.setFlag(cpsr_flags::FLAG_Z_OFFSET);
            }
            if (result < 0) {
                state.setFlag(cpsr_flags::FLAG_N_OFFSET);
            }

            //TODO does this work with negative values as well?
            // If there is a bit set in the upper half there must have been an overflow (i guess)
            if (result & 0xFFFFFFFF00000000) {
                state.setFlag(cpsr_flags::FLAG_C_OFFSET);
            }
        }
    }
};

} // namespace gbaemu

#endif /* CPU_HPP */