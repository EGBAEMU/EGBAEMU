#include "cpu/cpu.hpp"
#include "inst_thumb.hpp"
#include "util.hpp"

namespace gbaemu
{

    namespace thumb
    {
        template <typename Executor>
        template <Executor &exec>
        void ThumbInstructionDecoder<Executor>::decode(uint32_t lastInst)
        {

            switch (lastInst >> 12) {
                case 0b0000:
                case 0b0001: {
                    // Move shifted register
                    // 0b1110000000000000;
                    // 0b0000000000000000;

                    // Add and subtract
                    // 0b1111100000000000;
                    // 0b0001100000000000;

                    uint8_t opCode = (lastInst >> 11) & 0x3;

                    uint8_t rs = (lastInst >> 3) & 0x7;
                    uint8_t rd = lastInst & 0x7;
                    uint8_t offset = (lastInst >> 6) & 0x1F;

                    switch (opCode) {
                        case 0b00:
                            exec.Executor::template operator()<MOV_SHIFT, LSL>(rs, rd, offset);
                            break;
                        case 0b01:
                            exec.Executor::template operator()<MOV_SHIFT, LSR>(rs, rd, offset);
                            break;
                        case 0b10:
                            exec.Executor::template operator()<MOV_SHIFT, ASR>(rs, rd, offset);
                            break;

                        // This case belongs to ADD_SUB
                        case 0b11: {
                            uint8_t opCode = (lastInst >> 9) & 0x3;

                            offset &= 0x7;

                            switch (opCode) {
                                case 0b00:
                                    exec.Executor::template operator()<ADD_SUB, ADD>(rd, rs, offset);
                                    break;
                                case 0b01:
                                    exec.Executor::template operator()<ADD_SUB, SUB>(rd, rs, offset);
                                    break;
                                case 0b10:
                                    exec.Executor::template operator()<ADD_SUB, ADD_SHORT_IMM>(rd, rs, offset);
                                    break;
                                case 0b11:
                                    exec.Executor::template operator()<ADD_SUB, SUB_SHORT_IMM>(rd, rs, offset);
                                    break;
                            }
                            break;
                        }
                    }

                    break;
                }
                case 0b0010:
                case 0b0011: {
                    // Move, compare, add, and subtract immediate
                    // 0b1110000000000000;
                    // 0b0010000000000000;
                    uint8_t opCode = (lastInst >> 11) & 0x3;

                    uint8_t rd = (lastInst >> 8) & 0x7;
                    uint8_t offset = lastInst & 0x0FF;

                    switch (opCode) {
                        case 0b00:
                            exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, MOV>(rd, offset);
                            break;
                        case 0b01:
                            exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, CMP>(rd, offset);
                            break;
                        case 0b10:
                            exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, ADD>(rd, offset);
                            break;
                        case 0b11:
                            exec.Executor::template operator()<MOV_CMP_ADD_SUB_IMM, SUB>(rd, offset);
                            break;
                    }

                    break;
                }
                case 0b0100: {

                    if (isBitSet<uint16_t, 11>(lastInst)) {
                        // PC-relative load
                        // 0b1111100000000000;
                        // 0b0100100000000000;

                        uint8_t offset = lastInst & 0x0FF;
                        uint8_t rd = (lastInst >> 8) & 0x7;
                        exec.Executor::template operator()<PC_LD, LDR>(rd, offset);
                    } else {
                        if (isBitSet<uint16_t, 10>(lastInst)) {
                            // High register operations and branch exchange
                            // 0b1111110000000000;
                            // 0b0100010000000000;

                            uint8_t opCode = (lastInst >> 8) & 0x3;
                            // Destination Register most significant bit (or BL/BLX flag)
                            uint8_t msbDst = (lastInst >> 7) & 1;
                            // Source Register most significant bit
                            uint8_t msbSrc = (lastInst >> 6) & 1;
                            uint8_t rd = (lastInst & 0x7) | (msbDst << 3);
                            uint8_t rs = ((lastInst >> 3) & 0x7) | (msbSrc << 3);

                            switch (opCode) {
                                case 0b00:
                                    exec.Executor::template operator()<BR_XCHG, ADD>(rd, rs);
                                    break;
                                case 0b01:
                                    exec.Executor::template operator()<BR_XCHG, CMP>(rd, rs);
                                    break;
                                case 0b10:
                                    //Assemblers/Disassemblers should use MOV R8,R8 as NOP (in THUMB mode)
                                    if (rd == rs && rd == regs::R8_OFFSET) {
                                        exec.Executor::template operator()<BR_XCHG, NOP>(rd, rs);
                                    } else {
                                        exec.Executor::template operator()<BR_XCHG, MOV>(rd, rs);
                                    }
                                    break;
                                case 0b11:
                                    if (msbDst) {
                                        // exec.Executor::template operator()<BR_XCHG, BLX>(rd, rs);
                                    } else {
                                        exec.Executor::template operator()<BR_XCHG, BX>(rd, rs);
                                    }
                                    break;
                            }
                        } else {
                            // ALU operation
                            // 0b1111110000000000;
                            // 0b0100000000000000;

                            uint8_t opCode = (lastInst >> 6) & 0x0F;
                            uint8_t rd = lastInst & 0x7;
                            uint8_t rs = (lastInst >> 3) & 0x7;

                            switch (opCode) {
                                case 0b0000:
                                    exec.Executor::template operator()<ALU_OP, AND>(rs, rd);
                                    break;
                                case 0b0001:
                                    exec.Executor::template operator()<ALU_OP, EOR>(rs, rd);
                                    break;
                                case 0b0010:;
                                    exec.Executor::template operator()<ALU_OP, LSL>(rs, rd);
                                    break;
                                case 0b0011:
                                    exec.Executor::template operator()<ALU_OP, LSR>(rs, rd);
                                    break;
                                case 0b0100:
                                    exec.Executor::template operator()<ALU_OP, ASR>(rs, rd);
                                    break;
                                case 0b0101:
                                    exec.Executor::template operator()<ALU_OP, ADC>(rs, rd);
                                    break;
                                case 0b0110:
                                    exec.Executor::template operator()<ALU_OP, SBC>(rs, rd);
                                    break;
                                case 0b0111:
                                    exec.Executor::template operator()<ALU_OP, ROR>(rs, rd);
                                    break;
                                case 0b1000:
                                    exec.Executor::template operator()<ALU_OP, TST>(rs, rd);
                                    break;
                                case 0b1001:
                                    exec.Executor::template operator()<ALU_OP, NEG>(rs, rd);
                                    break;
                                case 0b1010:
                                    exec.Executor::template operator()<ALU_OP, CMP>(rs, rd);
                                    break;
                                case 0b1011:
                                    exec.Executor::template operator()<ALU_OP, CMN>(rs, rd);
                                    break;
                                case 0b1100:
                                    exec.Executor::template operator()<ALU_OP, ORR>(rs, rd);
                                    break;
                                case 0b1101:
                                    exec.Executor::template operator()<ALU_OP, MUL>(rs, rd);
                                    break;
                                case 0b1110:
                                    exec.Executor::template operator()<ALU_OP, BIC>(rs, rd);
                                    break;
                                case 0b1111:
                                    exec.Executor::template operator()<ALU_OP, MVN>(rs, rd);
                                    break;
                            }
                        }
                    }
                    break;
                }
                case 0b0101: {
                    // Load and store with relative offset
                    // 0b1111001000000000;
                    // 0b0101000000000000;

                    // Load and store sign-extended byte and halfword
                    // 0b1111001000000000;
                    // 0b0101001000000000;

                    uint8_t opCode = (lastInst >> 9) & 0x7;
                    uint8_t ro = (lastInst >> 6) & 0x7;
                    uint8_t rb = (lastInst >> 3) & 0x7;
                    uint8_t rd = lastInst & 0x7;

                    switch (opCode) {
                        case 0b001:
                            exec.Executor::template operator()<LD_ST_SIGN_EXT, STRH>(ro, rb, rd);
                            break;
                        case 0b011:
                            exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRSB>(ro, rb, rd);
                            break;
                        case 0b101:
                            exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRH>(ro, rb, rd);
                            break;
                        case 0b111:
                            exec.Executor::template operator()<LD_ST_SIGN_EXT, LDRSH>(ro, rb, rd);
                            break;
                        case 0b000:
                            exec.Executor::template operator()<LD_ST_REL_OFF, STR>(ro, rb, rd);
                            break;
                        case 0b010:
                            exec.Executor::template operator()<LD_ST_REL_OFF, STRB>(ro, rb, rd);
                            break;
                        case 0b100:
                            exec.Executor::template operator()<LD_ST_REL_OFF, LDR>(ro, rb, rd);
                            break;
                        case 0b110:
                            exec.Executor::template operator()<LD_ST_REL_OFF, LDRB>(ro, rb, rd);
                            break;
                    }
                    break;
                }
                case 0b0110:
                case 0b0111: {
                    // Load and store with immediate offset
                    // 0b1110000000000000;
                    // 0b0110000000000000;

                    uint8_t opCode = (lastInst >> 11) & 3;

                    uint8_t offset = (lastInst >> 6) & 0x1F;
                    uint8_t rb = (lastInst >> 3) & 0x7;
                    uint8_t rd = lastInst & 0x7;

                    switch (opCode) {
                        case 0b00:
                            exec.Executor::template operator()<LD_ST_IMM_OFF, STR>(rb, rd, offset);
                            break;
                        case 0b01:
                            exec.Executor::template operator()<LD_ST_IMM_OFF, LDR>(rb, rd, offset);
                            break;
                        case 0b10:
                            exec.Executor::template operator()<LD_ST_IMM_OFF, STRB>(rb, rd, offset);
                            break;
                        case 0b11:
                            exec.Executor::template operator()<LD_ST_IMM_OFF, LDRB>(rb, rd, offset);
                            break;
                    }
                    break;
                }
                case 0b1000: {
                    // Load and store halfword
                    // 0b1111000000000000;
                    // 0b1000000000000000;

                    bool l = (lastInst >> 11) & 0x1;
                    uint8_t offset = (lastInst >> 6) & 0x1F;
                    uint8_t rb = (lastInst >> 3) & 0x7;
                    uint8_t rd = lastInst & 0x7;

                    if (l) {
                        exec.Executor::template operator()<LD_ST_HW, LDRH>(rb, rd, offset);
                    } else {
                        exec.Executor::template operator()<LD_ST_HW, STRH>(rb, rd, offset);
                    }

                    break;
                }
                case 0b1001: {
                    //SP-relative load and store
                    // 0b1111000000000000;
                    // 0b1001000000000000;

                    bool l = (lastInst >> 11) & 0x1;

                    uint8_t offset = lastInst & 0x0FF;
                    uint8_t rd = (lastInst >> 8) & 0x7;

                    if (l) {
                        exec.Executor::template operator()<LD_ST_REL_SP, LDR>(rd, offset);
                    } else {
                        exec.Executor::template operator()<LD_ST_REL_SP, STR>(rd, offset);
                    }
                    break;
                }
                case 0b1010: {
                    // Load address
                    // 0b1111000000000000;
                    // 0b1010000000000000;

                    bool sp = (lastInst >> 11) & 0x1;
                    uint8_t offset = lastInst & 0x0FF;
                    uint8_t rd = (lastInst >> 8) & 0x7;

                    exec.Executor::template operator()<LOAD_ADDR, ADD>(sp, rd, offset);
                    break;
                }
                case 0b1011: {
                    // Push and pop registers
                    // 0b1111011000000000;
                    // 0b1011010000000000;

                    // Add offset to stack pointer
                    // 0b1111111100000000;
                    // 0b1011000000000000;

                    if ((lastInst & MASK_THUMB_PUSH_POP_REG) == VAL_THUMB_PUSH_POP_REG) {

                        bool l = lastInst & (1 << 11);
                        bool r = lastInst & (1 << 8);
                        uint8_t rlist = lastInst & 0x0FF;

                        if (l) {
                            exec.Executor::template operator()<PUSH_POP_REG, POP>(r, rlist);
                        } else {
                            exec.Executor::template operator()<PUSH_POP_REG, PUSH>(r, rlist);
                        }

                    } else if ((lastInst & MASK_THUMB_ADD_OFFSET_TO_STACK_PTR) == VAL_THUMB_ADD_OFFSET_TO_STACK_PTR) {

                        bool s = (lastInst >> 7) & 0x1;
                        uint8_t offset = lastInst & 0x7F;

                        exec.Executor::template operator()<ADD_OFFSET_TO_STACK_PTR, ADD>(s, offset);
                    } else {
                        exec.Executor::template operator()<INVALID_CAT, INVALID>();
                    }

                    break;
                }
                case 0b1100: {
                    // Multiple load and store
                    // 0b1111000000000000;
                    // 0b1100000000000000;

                    bool l = lastInst & (1 << 11);
                    uint8_t rb = (lastInst >> 8) & 0x7;
                    uint8_t rlist = lastInst & 0x0FF;

                    if (l) {
                        exec.Executor::template operator()<MULT_LOAD_STORE, LDMIA>(rb, rlist);
                    } else {
                        exec.Executor::template operator()<MULT_LOAD_STORE, STMIA>(rb, rlist);
                    }
                    break;
                }
                case 0b1101: {
                    uint8_t cond = (lastInst >> 8) & 0x0F;

                    if (cond == 0x0F) {
                        // Software interrupt
                        // 0b1111111100000000;
                        // 0b1101111100000000;
                        uint8_t comment = lastInst & 0x0FF;
                        exec.Executor::template operator()<SOFTWARE_INTERRUPT, SWI>(comment);
                    } else {
                        // Conditional Branch
                        // 0b1111000000000000;
                        // 0b1101000000000000;
                        int8_t offset = static_cast<int8_t>(lastInst & 0x0FF);
                        exec.Executor::template operator()<COND_BRANCH, B>(cond, offset);
                    }
                    break;
                }
                case 0b1110: {
                    // Unconditional branch
                    // 0b1111100000000000;
                    // 0b1110000000000000;
                    if (isBitSet<uint16_t, 11>(lastInst)) {
                        exec.Executor::template operator()<INVALID_CAT, INVALID>();
                    } else {
                        // first we extract the 11 bit offset, then we place it at the MSB so we can automatically sign extend it after casting to a signed type
                        // finally shift back where it came from but this time with correct sign
                        int16_t offset = signExt<int16_t, uint16_t, 11>(static_cast<uint16_t>(lastInst & 0x07FF));

                        exec.Executor::template operator()<UNCONDITIONAL_BRANCH, B>(offset);
                    }
                    break;
                }
                case 0b1111: {
                    // Long branch with link
                    // 0b1111000000000000;
                    // 0b1111000000000000;

                    bool h = lastInst & (1 << 11);
                    uint16_t offset = lastInst & 0x07FF;

                    exec.Executor::template operator()<LONG_BRANCH_WITH_LINK, B>(h, offset);
                    break;
                }
            }
        }

    } // namespace thumb
} // namespace gbaemu
