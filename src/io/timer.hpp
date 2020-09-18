#ifndef TIMER_HPP
#define TIMER_HPP

#include "io_regs.hpp"
#include "memory.hpp"
#include "util.hpp"

namespace gbaemu
{

    class CPU;
    class InterruptHandler;

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
        class Timer
        {
          private:
            static const constexpr uint32_t TIMER_REGS_BASE_OFFSET = Memory::IO_REGS_OFFSET + 0x100;

            static const constexpr uint16_t TIMER_PRESCALE_MASK = 0x3;
            static const constexpr uint16_t TIMER_TIMING_MASK = static_cast<uint16_t>(1) << 2;
            static const constexpr uint16_t TIMER_IRQ_EN_MASK = static_cast<uint16_t>(1) << 6;
            static const constexpr uint16_t TIMER_START_MASK = static_cast<uint16_t>(1) << 7;

            static const constexpr uint16_t prescales[] = {1, 64, 256, 1024};

#include "packed.h"
            struct TimerRegs {
                uint16_t reload;
                uint16_t control;
            } PACKED regs = {0};
#include "endpacked.h"

            Memory &memory;
            InterruptHandler &irqHandler;
            Timer *const nextTimer;
            const uint8_t id;

            uint16_t counter = 0;
            uint16_t preCounter;
            uint16_t prescale;
            bool active = false;
            bool countUpTiming;
            bool irq;

          public:
            void step();

            Timer(Memory &memory, InterruptHandler &irqHandler, uint8_t id, Timer *nextTimer);

          private:
            uint8_t read8FromReg(uint32_t offset);
            void write8ToReg(uint32_t offset, uint8_t value);

            void receiveOverflowOfPrevTimer();

            void checkForOverflow();
        };

        Timer tim0;
        Timer tim1;
        Timer tim2;
        Timer tim3;

      public:
        void step()
        {
            tim0.step();
            tim1.step();
            tim2.step();
            tim3.step();
        }

        TimerGroup(CPU *cpu);
    };

} // namespace gbaemu

#endif