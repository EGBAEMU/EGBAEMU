#include "timer.hpp"
#include "cpu/cpu.hpp"
#include "interrupts.hpp"
#include "logging.hpp"
#include "util.hpp"

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
        nextActive = false;
    }

    uint8_t TimerGroup::Timer::read8FromReg(uint32_t offset)
    {
        if (offset >= offsetof(TimerRegs, control))
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        else {
            return ((counter >> preShift) >> (offset ? 8 : 0)) & 0x0FF;
        }
    }

    void TimerGroup::Timer::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        if (offset == offsetof(TimerRegs, control)) {
            nextActive = isBitSet<uint8_t, TIMER_START_OFFSET>(value);
            // if the active bit is set again this won't have a effect on the active flag
            // else if deactivated active will be set to false as well -> on reenable still false
            active = active && nextActive;
        }
    }

    void TimerGroup::Timer::receiveOverflowOfPrevTimer(uint32_t overflowTimes)
    {
        // check if active & configured
        if (nextActive && active && countUpTiming) {
            counter += overflowTimes;
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
        if (counter >= overflowVal) {

            if (irq)
                irqHandler.setInterrupt(static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::TIMER_0_OVERFLOW + id));

            uint32_t reloadValue = (static_cast<uint32_t>(le(regs.reload)) << preShift);
            // We know that we have at least 1 overflow, but there can be more if we have a high reload value
            // therefore we have to calculate the amount that overflowed the counter aka: counter - overflowVal
            // this gives us the needed rest to check how many more overflows where triggered
            // but this needs the difference of overflowVal and reloadValue which represents the count up times needed to trigger an interrupt from the given reload value
            uint32_t restCounter = counter - overflowVal;
            uint32_t neededOverflowVal = overflowVal - reloadValue;

            // Also inform next timer about overflow
            if (nextTimer != nullptr) {
                nextTimer->receiveOverflowOfPrevTimer(restCounter / neededOverflowVal + 1);
            }

            // update the counter
            counter = (restCounter % neededOverflowVal) + reloadValue;
        }
    }

    void TimerGroup::Timer::step(uint32_t cycles)
    {
        if (cycles == 0 || !nextActive)
            return;

        if (!active) {
            // Was previously disabled -> reconfigure timer
            uint16_t controlReg = le(regs.control);

            countUpTiming = id != 0 && (controlReg & TIMER_TIMING_MASK);
            if (countUpTiming) {
                preShift = 0;
            } else {
                preShift = preShifts[(controlReg & TIMER_PRESCALE_MASK)];
            }

            counter = static_cast<uint32_t>(le(regs.reload)) << preShift;
            overflowVal = (static_cast<uint32_t>(1) << (preShift + 16));

            irq = controlReg & TIMER_IRQ_EN_MASK;

            LOG_TIM(
                std::cout << "INFO: Enabled TIMER" << std::dec << static_cast<uint32_t>(id) << std::endl;
                std::cout << "      Prescale: /" << std::dec << (static_cast<uint32_t>(1) << preShift) << std::endl;
                std::cout << "      Count only up on prev Timer overflow: " << countUpTiming << std::endl;
                std::cout << "      IRQ enable: " << irq << std::endl;
                std::cout << "      Counter Value: 0x" << std::hex << static_cast<uint32_t>(counter >> preShift) << std::endl;);

            //TODO 1 cycle for setup?
            --cycles;
        }

        // if countUpTiming is true we only increment on overflow of the previous timer!
        if (!countUpTiming) {

            // Increment the timer counter and check for overflows
            counter += cycles;

            checkForOverflow();
        }

        // Update active flag
        active = true;
    }

    TimerGroup::TimerGroup(CPU *cpu) : tim0(cpu->state.memory, cpu->irqHandler, 0, &tim1), tim1(cpu->state.memory, cpu->irqHandler, 1, &tim2), tim2(cpu->state.memory, cpu->irqHandler, 2, &tim3), tim3(cpu->state.memory, cpu->irqHandler, 3, nullptr)
    {
    }

} // namespace gbaemu