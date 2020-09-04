#ifndef TIMER_HPP
#define TIMER_HPP

#include "io_regs.hpp"
#include "memory.hpp"
#include "util.hpp"

namespace gbaemu
{

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

            struct TimerRegs {
                uint16_t reload;
                uint16_t control;
            } __attribute__((packed)) regs = {0};

            Memory &memory;

            Timer *const nextTimer;
            const bool hasPrev;

            //TODO the reload register is only loaded if active & reload was set at the same time!
            bool writeToControl = false;
            bool writeToReload = false;

            uint16_t counter = 0;
            uint16_t preCounter;
            uint16_t prescale;
            bool active = false;
            bool countUpTiming;
            bool irq;

          public:
            void step()
            {
                uint16_t controlReg = le(regs.control);

                // Update active flag
                bool nextActive = controlReg & TIMER_START_MASK;

                if (nextActive) {
                    if (active) {
                        // was previously active
                        // if countUpTiming is true we only increment on overflow of the previous timer!
                        if (!countUpTiming) {
                            --preCounter;

                            // Counter to apply the selected prescale
                            if (preCounter == 0) {
                                //TODO are prescale changes allowed during operation???
                                // reset pre counter
                                preCounter = prescale;

                                // Increment the real timer counter and check for overflows
                                ++counter;

                                checkForOverflow();
                            }
                        }
                    } else {
                        // Was previously disabled
                        if (writeToControl && writeToReload)
                            counter = le(regs.reload);

                        preCounter = prescale = prescales[(controlReg & TIMER_PRESCALE_MASK)];
                        countUpTiming = hasPrev && (controlReg & TIMER_TIMING_MASK);
                        irq = controlReg & TIMER_IRQ_EN_MASK;
                    }
                }

                active = nextActive;

                writeToControl = false;
                writeToReload = false;
            }

            Timer(Memory &memory, uint8_t id, Timer *nextTimer) : memory(memory), nextTimer(nextTimer), hasPrev(id != 0)
            {
                memory.ioHandler.registerIOMappedDevice(
                    IO_Mapped(
                        TIMER_REGS_BASE_OFFSET + sizeof(regs) * id,
                        TIMER_REGS_BASE_OFFSET + sizeof(regs) * id + sizeof(regs),
                        std::bind(&Timer::read8FromReg, this, std::placeholders::_1),
                        std::bind(&Timer::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                        std::bind(&Timer::read8FromReg, this, std::placeholders::_1),
                        std::bind(&Timer::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
            }

          private:
            uint8_t read8FromReg(uint32_t offset)
            {
                if (offset >= 2)
                    return *(offset + reinterpret_cast<uint8_t *>(&regs));
                else {
                    return (counter >> (offset ? 8 : 0)) & 0x0FF;
                }
            }
            void write8ToReg(uint32_t offset, uint8_t value)
            {
                *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

                // This works regardless of the endianess because the offset is specified for little endian!
                writeToControl |= offset == 2;
                writeToReload |= offset < 2;
            }

            void receiveOverflowOfPrevTimer()
            {
                // check if still active
                bool nextActive = le(regs.control) & TIMER_START_MASK;

                if (nextActive && active && countUpTiming) {
                    ++counter;
                    checkForOverflow();
                }
            }

            void checkForOverflow()
            {
                if (counter == 0 && irq) {
                    //TODO trigger overflow interrupt

                    // Also inform next timer about overflow
                    if (nextTimer != nullptr) {
                        nextTimer->receiveOverflowOfPrevTimer();
                    }
                }
            }
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

        TimerGroup(Memory &memory) : tim0(memory, 0, &tim1), tim1(memory, 1, &tim2), tim2(memory, 2, &tim3), tim3(memory, 3, nullptr)
        {
        }
    };

} // namespace gbaemu

#endif