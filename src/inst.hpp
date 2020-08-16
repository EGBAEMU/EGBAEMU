#ifndef INST_HPP
#define INST_HPP

#include <cstdint>
#include <functional>

namespace gbaemu
{

    enum ConditionOPCode : uint8_t {
        // Equal Z==1
        EQ,
        // Not equal Z==0
        NE,
        // Carry set / unsigned higher or same C==1
        CS_HS,
        // Carry clear / unsigned lower C==0
        CC_LO,
        // Minus / negative N==1
        MI,
        // Plus / positive or zero N==0
        PL,
        // Overflow V==1
        VS,
        // No overflow V==0
        VC,
        // Unsigned higher (C==1) AND (Z==0)
        HI,
        // Unsigned lower or same (C==0) OR (Z==1)
        LS,
        // Signed greater than or equal N == V
        GE,
        // Signed less than N != V
        LT,
        // Signed greater than (Z==0) AND (N==V)
        GT,
        // Signed less than or equal (Z==1) OR (N!=V)
        LE,
        // Always (unconditional) Not applicable
        AL,
        // Never Obsolete, unpredictable in ARM7TDMI
        NV
    };

    struct CPUState;

    class Instruction
    {
      public:
        virtual void execute(CPUState *state) = 0;
        virtual std::string toString() const = 0;
    };

    class InstructionDecoder
    {
      public:
        virtual Instruction *decode(uint32_t inst) const = 0;
    };

} // namespace gbaemu

#endif
