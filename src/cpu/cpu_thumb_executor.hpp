#ifndef CPU_THUMB_EXECUTOR_HPP
#define CPU_THUMB_EXECUTOR_HPP

#include "decode/inst.hpp"

#include <iostream>

namespace gbaemu
{
    class CPU;

    namespace thumb
    {
        class ThumbExecutor
        {
          private:
            CPU *cpu;

          public:
            template <ThumbInstructionCategory, InstructionID id, typename T, typename... Args>
            void operator()(T t, Args... args);

            friend CPU;
        };

    } // namespace thumb
} // namespace gbaemu

#include "cpu_thumb_executor.tpp"

#endif