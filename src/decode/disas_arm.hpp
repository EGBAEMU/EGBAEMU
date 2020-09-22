#ifndef DISAS_ARM_HPP
#define DISAS_ARM_HPP

#include "inst.hpp"

namespace gbaemu::arm
{

    class ArmDisas
    {
      public:
        ARMInstruction inst;

        template <ARMInstructionCategory, InstructionID, typename T, typename... Args>
        void operator()(T t, Args... args);
    };

    template <ARMInstructionCategory, InstructionID, typename T, typename... Args>
    void ArmDisas::operator()(T thumbInst, Args... a)
    {
        //TODO rework
        /*
        inst = thumbInst;
        */
    }

} // namespace gbaemu::arm

#endif