#ifndef REGS_HPP
#define REGS_HPP

#include <cstdint>

namespace gbaemu
{
    namespace regs
    {
        /* constants */
        static const uint32_t R0_OFFSET = 0;
        static const uint32_t R1_OFFSET = 1;
        static const uint32_t R2_OFFSET = 2;
        static const uint32_t R3_OFFSET = 3;
        static const uint32_t R4_OFFSET = 4;
        static const uint32_t R5_OFFSET = 5;
        static const uint32_t R6_OFFSET = 6;
        static const uint32_t R7_OFFSET = 7;
        static const uint32_t R8_OFFSET = 8;
        static const uint32_t R9_OFFSET = 9;
        static const uint32_t R10_OFFSET = 10;
        static const uint32_t R11_OFFSET = 11;
        static const uint32_t R12_OFFSET = 12;

        static const uint32_t SP_OFFSET = 13;
        static const uint32_t LR_OFFSET = 14;
        static const uint32_t PC_OFFSET = 15;
        // Avoid direct accesses!
        //static const uint32_t CPSR_OFFSET = 16;
        static const uint32_t SPSR_OFFSET = 17;
    } // namespace regs
    namespace cpsr_flags
    {

        // Flags: Z=zeroflag, N=sign, C=carry (except LSL#0: C=unchanged), V=unchanged.
        /*
        Current Program Status Register (CPSR)
          Bit   Expl.
          31    N - Sign Flag       (0=Not Signed, 1=Signed)               ;\
          30    Z - Zero Flag       (0=Not Zero, 1=Zero)                   ; Condition
          29    C - Carry Flag      (0=Borrow/No Carry, 1=Carry/No Borrow) ; Code Flags
          28    V - Overflow Flag   (0=No Overflow, 1=Overflow)            ;/
          27    Q - Sticky Overflow (1=Sticky Overflow, ARMv5TE and up only)
          26-8  Reserved            (For future use) - Do not change manually!
          7     I - IRQ disable     (0=Enable, 1=Disable)                     ;\
          6     F - FIQ disable     (0=Enable, 1=Disable)                     ; Control
          5     T - State Bit       (0=ARM, 1=THUMB) - Do not change manually!; Bits
          4-0   M4-M0 - Mode Bits   (See below)
          
          Bit 31-28: Condition Code Flags (N,Z,C,V)
          
            These bits reflect results of logical or arithmetic instructions. In ARM mode, it is often optionally whether an instruction should modify flags or not, for example, it is possible to execute a SUB instruction that does NOT modify the condition flags.
            In ARM state, all instructions can be executed conditionally depending on the settings of the flags, such like MOVEQ (Move if Z=1). While In THUMB state, only Branch instructions (jumps) can be made conditionally.

            Bit 27: Sticky Overflow Flag (Q) - ARMv5TE and ARMv5TExP and up only
                Used by QADD, QSUB, QDADD, QDSUB, SMLAxy, and SMLAWy only. These opcodes set the Q-flag in case of overflows, but leave it unchanged otherwise. The Q-flag can be tested/reset by MSR/MRS opcodes only.

            Bit 27-8: Reserved Bits (except Bit 27 on ARMv5TE and up, see above)
                These bits are reserved for possible future implementations. For best forwards compatibility, the user should never change the state of these bits, and should not expect these bits to be set to a specific value.

            Bit 7-0: Control Bits (I,F,T,M4-M0)
                These bits may change when an exception occurs. In privileged modes (non-user modes) they may be also changed manually.
                The interrupt bits I and F are used to disable IRQ and FIQ interrupts respectively (a setting of "1" means disabled).
                The T Bit signalizes the current state of the CPU (0=ARM, 1=THUMB), this bit should never be changed manually - instead, changing between ARM and THUMB state must be done by BX instructions.
                The Mode Bits M4-M0 contain the current operating mode.
                    Binary Hex Dec  Expl.
                    0xx00b 00h 0  - Old User       ;\26bit Backward Compatibility modes
                    0xx01b 01h 1  - Old FIQ        ; (supported only on ARMv3, except ARMv3G,
                    0xx10b 02h 2  - Old IRQ        ; and on some non-T variants of ARMv4)
                    0xx11b 03h 3  - Old Supervisor ;/
                    10000b 10h 16 - User (non-privileged)
                    10001b 11h 17 - FIQ
                    10010b 12h 18 - IRQ
                    10011b 13h 19 - Supervisor (SWI)
                    10111b 17h 23 - Abort
                    11011b 1Bh 27 - Undefined
                    11111b 1Fh 31 - System (privileged 'User' mode) (ARMv4 and up)
            Writing any other values into the Mode bits is not allowed. 
    */
        enum CPSR_FLAGS : uint8_t {
            N_FLAG = 31,
            Z_FLAG = 30,
            C_FLAG = 29,
            V_FLAG = 28,
            IRQ_DISABLE = 7,
            FIQ_DISABLE = 6,
            THUMB_STATE = 5,
        };
        static const constexpr uint32_t MODE_BIT_MASK = 0x01F;
        /*
        static const constexpr uint32_t N_FLAG_BITMASK = (1 << FLAG_N_OFFSET);
        static const constexpr uint32_t Z_FLAG_BITMASK = (1 << FLAG_Z_OFFSET);
        static const constexpr uint32_t C_FLAG_BITMASK = (1 << FLAG_C_OFFSET);
        static const constexpr uint32_t V_FLAG_BITMASK = (1 << FLAG_V_OFFSET);
        static const constexpr uint32_t THUMB_FLAG_BITMASK = (1 << THUMB_STATE_OFFSET);
        */
    } // namespace cpsr_flags

} // namespace gbaemu

#endif