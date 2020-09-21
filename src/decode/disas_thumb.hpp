#ifndef DISAS_THUMB_HPP
#define DISAS_THUMB_HPP

#include "inst.hpp"

namespace gbaemu::thumb
{

    class ThumbDisas
    {
      public:
        ThumbInstruction inst;

        template <ThumbInstructionCategory, InstructionID, typename T, typename... Args>
        void operator()(T t, Args... args);
    };

    template <ThumbInstructionCategory, InstructionID, typename T, typename... Args>
    void ThumbDisas::operator()(T thumbInst, Args... a)
    {
        inst = thumbInst;
    }

} // namespace gbaemu::thumb

#endif