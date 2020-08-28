#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <vector>
#include <memory>
#include "cpu_state.hpp"


namespace gbaemu::debugger {
    class Trap {
    public:
        virtual void trigger(uint32_t prevPC, uint32_t postPC, const Instruction& inst, const CPUState& state) = 0;
        virtual bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction& inst, const CPUState& state) = 0;
    };

    /* logs all branches, excluding loops */
    class JumpTrap : public Trap {
    public:
        struct JumpInfo {
            Instruction inst;
            uint32_t from, to;
        };
    private:
        std::vector<JumpInfo> jumps;

        bool isLoop(uint32_t from, uint32_t to) const;
    public:
        void trigger(uint32_t prevPC, uint32_t postPC, const Instruction& inst, const CPUState& state) override;
        bool satisfied(uint32_t prevPC, uint32_t postPC, const Instruction& inst, const CPUState& state) override;
        void print();
    };

    /*
        A Watchdog checks at every instruction/CPU-state if any of the traps can trigger
        and calls their trigger function.
     */
    class Watchdog {
    private:
        std::vector<std::shared_ptr<Trap>> traps;
    public:
        void registerTrap(Trap& t);
        void check(uint32_t prevPC, uint32_t postPC, const Instruction& inst, const CPUState& state);
    };
}

#endif /* DEBUGGER_HPP */
