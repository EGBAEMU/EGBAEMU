#ifndef TIMER_HPP
#define TIMER_HPP

#include "io_regs.hpp"
#include "memory_defs.hpp"
#include "packed.h"
#include "util.hpp"

namespace gbaemu
{

    class CPU;
    class InterruptHandler;
    struct InstructionExecutionInfo;

    class TimerGroup
    {

      private:
        /*
        Timer Registers
          4000100h  2    R/W  TM0CNT_L  Timer 0 Counter/Reload
          4000102h  2    R/W  TM0CNT_H  Timer 0 Control
          4000104h  2    R/W  TM1CNT_L  Timer 1 Counter/Reload
          4000106h  2    R/W  TM1CNT_H  Timer 1 Control
          4000108h  2    R/W  TM2CNT_L  Timer 2 Counter/Reload
          400010Ah  2    R/W  TM2CNT_H  Timer 2 Control
          400010Ch  2    R/W  TM3CNT_L  Timer 3 Counter/Reload
          400010Eh  2    R/W  TM3CNT_H  Timer 3 Control
          4000110h       -    -         Not used
        */
        template <uint8_t id>
        class Timer
        {
          private:
            static const constexpr uint32_t TIMER_REGS_BASE_OFFSET = memory::IO_REGS_OFFSET + 0x100;
            static const constexpr uint8_t TIMER_START_OFFSET = 7;

            static const constexpr uint16_t TIMER_PRESCALE_MASK = 0x3;
            static const constexpr uint16_t TIMER_TIMING_MASK = static_cast<uint16_t>(1) << 2;
            static const constexpr uint16_t TIMER_IRQ_EN_MASK = static_cast<uint16_t>(1) << 6;
            static const constexpr uint16_t TIMER_START_MASK = static_cast<uint16_t>(1) << TIMER_START_OFFSET;

            // prescale = 1 << preShift -> prescale values are: 1, 64, 256, 1024
            static const constexpr uint8_t preShifts[] = {0, 6, 8, 10};

            PACK_STRUCT(TimerRegs, regs,
                        uint16_t reload;
                        uint16_t control;);

            InterruptHandler &irqHandler;
            Timer<(id < 3) ? id + 1 : id> *const nextTimer;

            TimerGroup &timerGroup;

            uint32_t counter;
            uint32_t overflowVal;
            uint8_t preShift;
            bool active;
            bool countUpTiming;
            bool irq;

          public:
            void step(uint32_t cycles);

            Timer(InterruptHandler &irqHandler, Timer<(id < 3) ? id + 1 : id> *nextTimer, TimerGroup &timerGroup);

            void reset();

          private:
            void initialize();

            uint8_t read8FromReg(uint32_t offset);
            void write8ToReg(uint32_t offset, uint8_t value);

            void receiveOverflowOfPrevTimer(uint32_t overflowTimes);

            void checkForOverflow();

            friend class Timer<(id > 0) ? id - 1 : id>;
            friend class IO_Handler;
        };

        CPU *cpu;

        Timer<0> tim0;
        Timer<1> tim1;
        Timer<2> tim2;
        Timer<3> tim3;

        uint8_t timEnableBitset;

      public:
        void step(uint32_t cycles)
        {
            if (cycles) {
                /* Same optimization as in DMAGroup::step() */
                if (timEnableBitset & 1)
                    tim0.step(cycles);

                if (timEnableBitset & 2)
                    tim1.step(cycles);

                if (timEnableBitset & 4)
                    tim2.step(cycles);

                if (timEnableBitset & 8)
                    tim3.step(cycles);
            }
        }

        void reset()
        {
            tim0.reset();
            tim1.reset();
            tim2.reset();
            tim3.reset();
            timEnableBitset = 0;
        }

        TimerGroup(CPU *cpu);

        void checkRunCondition() const;

        friend class IO_Handler;
    };

} // namespace gbaemu

#endif