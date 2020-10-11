#include "timer.hpp"
#include "cpu/cpu.hpp"
#include "decode/inst.hpp"
#include "interrupts.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <algorithm>
#include <functional>
#include <iostream>

namespace gbaemu
{

    template <uint8_t id>
    void TimerGroup::Timer<id>::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        counter = 0;
        active = false;
    }

    template <uint8_t id>
    uint8_t TimerGroup::Timer<id>::read8FromReg(uint32_t offset)
    {
        if (offset >= offsetof(TimerRegs, control))
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        else {
            return ((counter >> preShift) >> (offset ? 8 : 0)) & 0x0FF;
        }
    }

    template <uint8_t id>
    void TimerGroup::Timer<id>::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        if (offset == offsetof(TimerRegs, control)) {
            bool nextActive = isBitSet<uint8_t, TIMER_START_OFFSET>(value);
            // if the active bit is set again this won't have a effect on the active flag
            // else if deactivated active will be set to false as well -> on reenable still false
            active = active && nextActive;

            timEnableBitset = bitSet<uint8_t, 1, id>(timEnableBitset, bmap<uint8_t>(nextActive));
        }
    }

    template <uint8_t id>
    void TimerGroup::Timer<id>::receiveOverflowOfPrevTimer(uint32_t overflowTimes)
    {
        // check if active & configured
        if (active && countUpTiming) {
            counter += overflowTimes;
            checkForOverflow();
        }
    }

    template <uint8_t id>
    TimerGroup::Timer<id>::Timer(InterruptHandler &irqHandler, Timer<(id < 3) ? id + 1 : id> *nextTimer, uint8_t &timEnableBitset) : irqHandler(irqHandler), nextTimer(nextTimer), timEnableBitset(timEnableBitset)
    {
    }

    template <uint8_t id>
    void TimerGroup::Timer<id>::checkForOverflow()
    {
        if (counter >= overflowVal) {

            if (irq)
                irqHandler.setInterrupt<static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::TIMER_0_OVERFLOW + id)>();

            uint32_t reloadValue = (static_cast<uint32_t>(le(regs.reload)) << preShift);
            // We know that we have at least 1 overflow, but there can be more if we have a high reload value
            // therefore we have to calculate the amount that overflowed the counter aka: counter - overflowVal
            // this gives us the needed rest to check how many more overflows where triggered
            // but this needs the difference of overflowVal and reloadValue which represents the count up times needed to trigger an interrupt from the given reload value
            uint32_t restCounter = counter - overflowVal;
            uint32_t neededOverflowVal = overflowVal - reloadValue;

            // Also inform next timer about overflow
            if (id < 3) {
                nextTimer->receiveOverflowOfPrevTimer(restCounter / neededOverflowVal + 1);
            }

            // update the counter
            counter = (restCounter % neededOverflowVal) + reloadValue;
        }
    }

    template <uint8_t id>
    void TimerGroup::Timer<id>::step(uint32_t cycles)
    {
        // If this gets called be can safely assume that it is enabled for the first time or is not in countUpTiming mode

        if (!active) {
            //TODO we might extract this code entirely from the step function for less overhead!

            // Update active flag
            active = true;

            // Was previously disabled -> reconfigure timer
            uint16_t controlReg = le(regs.control);

            countUpTiming = id != 0 && (controlReg & TIMER_TIMING_MASK);
            if (countUpTiming) {
                preShift = 0;
                // If we are in count up timing then we can safely remove ourself from the enable bitset as we only count up through other timer overflows
                timEnableBitset = bitSet<uint8_t, 1, id>(timEnableBitset, bmap<uint8_t, false>());
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

            if (countUpTiming) {
                return;
            }
        }

        // Increment the timer counter and check for overflows
        counter += cycles;

        checkForOverflow();
    }

    TimerGroup::TimerGroup(CPU *cpu) : tim0(cpu->irqHandler, &tim1, timEnableBitset), tim1(cpu->irqHandler, &tim2, timEnableBitset), tim2(cpu->irqHandler, &tim3, timEnableBitset), tim3(cpu->irqHandler, nullptr, timEnableBitset)
    {
        reset();
    }

    template class TimerGroup::Timer<0>;
    template class TimerGroup::Timer<1>;
    template class TimerGroup::Timer<2>;
    template class TimerGroup::Timer<3>;

} // namespace gbaemu