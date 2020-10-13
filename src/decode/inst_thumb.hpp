#ifndef INST_THUMB_HPP
#define INST_THUMB_HPP

#include "cpu/regs.hpp"
#include "inst.hpp"
#include <cstdint>

namespace gbaemu
{
    class CPU;

    namespace thumb
    {

        // THUMB INSTRUCTION SET

        // Move shifted register
        static const constexpr uint16_t MASK_THUMB_MOV_SHIFT = 0b1110000000000000;
        static const constexpr uint16_t VAL_THUMB_MOV_SHIFT = 0b0000000000000000;
        // Add and subtract
        static const  constexpr uint16_t MASK_THUMB_ADD_SUB = 0b1111100000000000;
        static const  constexpr uint16_t VAL_THUMB_ADD_SUB = 0b0001100000000000;
        // Move, compare, add, and subtract immediate
        static const  constexpr uint16_t MASK_THUMB_MOV_CMP_ADD_SUB_IMM = 0b1110000000000000;
        static const  constexpr uint16_t VAL_THUMB_MOV_CMP_ADD_SUB_IMM = 0b0010000000000000;
        // ALU operation
        static const  constexpr uint16_t MASK_THUMB_ALU_OP = 0b1111110000000000;
        static const  constexpr uint16_t VAL_THUMB_ALU_OP = 0b0100000000000000;
        // High register operations and branch exchange
        static const  constexpr uint16_t MASK_THUMB_BR_XCHG = 0b1111110000000000;
        static const  constexpr uint16_t VAL_THUMB_BR_XCHG = 0b0100010000000000;
        // PC-relative load
        static const  constexpr uint16_t MASK_THUMB_PC_LD = 0b1111100000000000;
        static const  constexpr uint16_t VAL_THUMB_PC_LD = 0b0100100000000000;
        // Load and store with relative offset
        static const  constexpr uint16_t MASK_THUMB_LD_ST_REL_OFF = 0b1111001000000000;
        static const  constexpr uint16_t VAL_THUMB_LD_ST_REL_OFF = 0b0101000000000000;
        // Load and store sign-extended byte and halfword
        static const  constexpr uint16_t MASK_THUMB_LD_ST_SIGN_EXT = 0b1111001000000000;
        static const  constexpr uint16_t VAL_THUMB_LD_ST_SIGN_EXT = 0b0101001000000000;
        // Load and store with immediate offset
        static const  constexpr uint16_t MASK_THUMB_LD_ST_IMM_OFF = 0b1110000000000000;
        static const  constexpr uint16_t VAL_THUMB_LD_ST_IMM_OFF = 0b0110000000000000;
        // Load and store halfword
        static const  constexpr uint16_t MASK_THUMB_LD_ST_HW = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_LD_ST_HW = 0b1000000000000000;
        //SP-relative load and store
        static const  constexpr uint16_t MASK_THUMB_LD_ST_REL_SP = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_LD_ST_REL_SP = 0b1001000000000000;
        // Load address
        static const  constexpr uint16_t MASK_THUMB_LOAD_ADDR = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_LOAD_ADDR = 0b1010000000000000;
        // Add offset to stack pointer
        static const  constexpr uint16_t MASK_THUMB_ADD_OFFSET_TO_STACK_PTR = 0b1111111100000000;
        static const  constexpr uint16_t VAL_THUMB_ADD_OFFSET_TO_STACK_PTR = 0b1011000000000000;
        // Push and pop registers
        static const  constexpr uint16_t MASK_THUMB_PUSH_POP_REG = 0b1111011000000000;
        static const  constexpr uint16_t VAL_THUMB_PUSH_POP_REG = 0b1011010000000000;
        // Multiple load and store
        static const  constexpr uint16_t MASK_THUMB_MULT_LOAD_STORE = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_MULT_LOAD_STORE = 0b1100000000000000;
        // Conditional Branch
        static const  constexpr uint16_t MASK_THUMB_COND_BRANCH = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_COND_BRANCH = 0b1101000000000000;
        // Software interrupt
        static const  constexpr uint16_t MASK_THUMB_SOFTWARE_INTERRUPT = 0b1111111100000000;
        static const  constexpr uint16_t VAL_THUMB_SOFTWARE_INTERRUPT = 0b1101111100000000;
        // Unconditional branch
        static const  constexpr uint16_t MASK_THUMB_UNCONDITIONAL_BRANCH = 0b1111100000000000;
        static const  constexpr uint16_t VAL_THUMB_UNCONDITIONAL_BRANCH = 0b1110000000000000;
        // Long branch with link
        static const  constexpr uint16_t MASK_THUMB_LONG_BRANCH_WITH_LINK = 0b1111000000000000;
        static const  constexpr uint16_t VAL_THUMB_LONG_BRANCH_WITH_LINK = 0b1111000000000000;

        template <class Executor>
        class ThumbInstructionDecoder
        {
          private:
            ThumbInstructionDecoder();

          public:
            template <Executor &exec>
            static void decode(uint32_t inst);
        };

    } // namespace thumb
} // namespace gbaemu

#include "inst_thumb.tpp"

#endif /* INST_THUMB_HPP */