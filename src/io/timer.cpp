#include "timer.hpp"
#include "cpu/cpu.hpp"
#include "interrupts.hpp"
#include "logging.hpp"

#include <algorithm>
#include <functional>
#include <iostream>

namespace gbaemu
{
    void TimerGroup::Timer::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        counter = 0;
        active = false;
    }

    uint8_t TimerGroup::Timer::read8FromReg(uint32_t offset)
    {
        if (offset >= offsetof(TimerRegs, control))
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        else {
            return ((counter / prescale) >> (offset ? 8 : 0)) & 0x0FF;
        }
    }

    void TimerGroup::Timer::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
    }

    void TimerGroup::Timer::receiveOverflowOfPrevTimer()
    {
        // check if still active
        bool nextActive = le(regs.control) & TIMER_START_MASK;

        if (nextActive && active && countUpTiming) {
            ++counter;
            checkForOverflow();
        }
    }

    TimerGroup::Timer::Timer(Memory &memory, InterruptHandler &irqHandler, uint8_t id, Timer *nextTimer) : memory(memory), irqHandler(irqHandler), nextTimer(nextTimer), id(id)
    {
        reset();
        memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                TIMER_REGS_BASE_OFFSET + sizeof(regs) * id,
                TIMER_REGS_BASE_OFFSET + sizeof(regs) * id + sizeof(regs) - 1,
                std::bind(&Timer::read8FromReg, this, std::placeholders::_1),
                std::bind(&Timer::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&Timer::read8FromReg, this, std::placeholders::_1),
                std::bind(&Timer::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
    }

    void TimerGroup::Timer::checkForOverflow()
    {
        if (counter == (static_cast<uint32_t>(prescale) << 16)) {
            // reload value!
            counter = static_cast<uint32_t>(le(regs.reload)) * prescale;

            if (irq)
                irqHandler.setInterrupt(static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::TIMER_0_OVERFLOW + id));

            // Also inform next timer about overflow
            if (nextTimer != nullptr) {
                nextTimer->receiveOverflowOfPrevTimer();
            }
        }
    }

    void TimerGroup::Timer::step()
    {
        uint16_t controlReg = le(regs.control);

        // Update active flag
        bool nextActive = controlReg & TIMER_START_MASK;

        if (nextActive) {
            if (active) {
                // was previously active
                // if countUpTiming is true we only increment on overflow of the previous timer!
                if (!countUpTiming) {

                    // Increment the timer counter and check for overflows
                    ++counter;

                    checkForOverflow();
                }
            } else {
                // Was previously disabled

                countUpTiming = id != 0 && (controlReg & TIMER_TIMING_MASK);
                if (countUpTiming) {
                    prescale = 1;
                } else {
                    prescale = prescales[(controlReg & TIMER_PRESCALE_MASK)];
                }

                counter = static_cast<uint32_t>(le(regs.reload)) * prescale;

                irq = controlReg & TIMER_IRQ_EN_MASK;

                LOG_TIM(
                    std::cout << "INFO: Enabled TIMER" << std::dec << static_cast<uint32_t>(id) << std::endl;
                    std::cout << "      Prescale: /" << std::dec << static_cast<uint32_t>(prescale) << std::endl;
                    std::cout << "      Count only up on prev Timer overflow: " << countUpTiming << std::endl;
                    std::cout << "      IRQ enable: " << irq << std::endl;
                    std::cout << "      Counter Value: 0x" << std::hex << static_cast<uint32_t>(counter) << std::endl;);
            }
        }

        active = nextActive;
    }

    TimerGroup::TimerGroup(CPU *cpu) : tim0(cpu->state.memory, cpu->irqHandler, 0, &tim1), tim1(cpu->state.memory, cpu->irqHandler, 1, &tim2), tim2(cpu->state.memory, cpu->irqHandler, 2, &tim3), tim3(cpu->state.memory, cpu->irqHandler, 3, nullptr)
    {
    }

} // namespace gbaemu