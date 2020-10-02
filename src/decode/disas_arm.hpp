#ifndef DISAS_ARM_HPP
#define DISAS_ARM_HPP

#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::arm
{
    class ArmDisas
    {
      public:
        std::stringstream ss;

        template <ARMInstructionCategory, typename... Args>
        void disas(InstructionID id, Args... args);

        template <ARMInstructionCategory cat, InstructionID id, typename... Args>
        void operator()(Args... args)
        {
            ss << instructionIDToString(id);
            disas<cat>(id, args...);
        }
    };
} // namespace gbaemu::arm

#endif