#include "timer.hpp"
#include "cpu/cpu.hpp"
#include "interrupts.hpp"
#include "logging.hpp"

#include <functional>
#include <iostream>

namespace gbaemu
{
    uint8_t TimerGroup::Timer::read8FromReg(uint32_t offset)
    {
        if (offset >= 2)
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        else {
            return (counter >> (offset ? 8 : 0)) & 0x0FF;
        }
    }

    void TimerGroup::Timer::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        // This works regardless of the endianess because the offset is specified for little endian!
        writeToControl |= offset == 2;
        writeToReload |= offset < 2;
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
        if (counter == 0 && irq) {
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
                countUpTiming = id != 0 && (controlReg & TIMER_TIMING_MASK);
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

        writeToControl = false;
        writeToReload = false;
    }

    TimerGroup::TimerGroup(CPU *cpu) : tim0(cpu->state.memory, cpu->irqHandler, 0, &tim1), tim1(cpu->state.memory, cpu->irqHandler, 1, &tim2), tim2(cpu->state.memory, cpu->irqHandler, 2, &tim3), tim3(cpu->state.memory, cpu->irqHandler, 3, nullptr)
    {
    }

} // namespace gbaemu