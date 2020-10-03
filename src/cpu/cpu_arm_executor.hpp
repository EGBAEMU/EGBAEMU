#ifndef CPU_ARM_EXECUTOR_HPP
#define CPU_ARM_EXECUTOR_HPP

#include "decode/inst.hpp"

namespace gbaemu
{
    class CPU;

    namespace arm
    {
        class ArmExecutor
        {
          private:
            CPU *cpu;

          public:
            template <arm::ARMInstructionCategory, InstructionID, typename... Args>
            void operator()(Args... args);

            friend CPU;
        };
    } // namespace arm

} // namespace gbaemu

#include "cpu_arm_executor.tpp"

#endif