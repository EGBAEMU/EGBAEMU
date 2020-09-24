#ifndef DISAS_THUMB_HPP
#define DISAS_THUMB_HPP

#include "inst.hpp"

#include <iostream>
#include <sstream>

namespace gbaemu::thumb
{
    class ThumbDisas
    {
      public:
        std::stringstream ss;

        template <ThumbInstructionCategory, typename... Args>
        void disas(InstructionID id, Args... args);

        template <ThumbInstructionCategory cat, InstructionID id, typename... Args>
        void operator()(Args... args)
        {
            ss << instructionIDToString(id);
            disas<cat>(id, args...);
        }
    };

} // namespace gbaemu::thumb

#endif