#include "io_regs.hpp"
#include "logging.hpp"
#include "memory.hpp"

#include <iostream>

// Includes for io mapped devices...
#include "cpu/cpu.hpp"
#include "lcd/lcd-controller.hpp"

// Include the makro mess, pls don't look at it:D
#include "io_regs_makros.h"

namespace gbaemu
{

    uint8_t IO_Handler::externalRead8(uint32_t addr) const
    {
        static_assert(sizeof(lcd::LCDIORegs) == 86);
        static_assert(sizeof(cpu->irqHandler.regs) == 10);
        static_assert(sizeof(cpu->dmaGroup.dma0.regs) == 12);
        static_assert(sizeof(cpu->dmaGroup.dma1.regs) == 12);
        static_assert(sizeof(cpu->dmaGroup.dma2.regs) == 12);
        static_assert(sizeof(cpu->dmaGroup.dma3.regs) == 12);
        static_assert(sizeof(cpu->keypad.regs) == 4);
        static_assert(sizeof(cpu->timerGroup.tim0.regs) == 4);
        static_assert(sizeof(cpu->timerGroup.tim1.regs) == 4);
        static_assert(sizeof(cpu->timerGroup.tim2.regs) == 4);
        static_assert(sizeof(cpu->timerGroup.tim3.regs) == 4);

        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;

        switch (globalOffset) {
            REP_CASE(86, 0x0, lcdController->read8FromReg(offset));
            REP_CASE(10, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->irqHandler.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma0.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma1.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma2.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma3.read8FromReg(offset));
            REP_CASE(4, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->keypad.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim0.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim1.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim2.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim3.read8FromReg(offset));
            default:
                //TODO how to handle not found?
                LOG_IO(std::cout << "WARNING: externalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
                return 0x0000;
        }
        return 0x0000;
    }

    void IO_Handler::externalWrite8(uint32_t addr, uint8_t value)
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;

        switch (globalOffset) {
            REP_CASE(86, 0x0, lcdController->write8ToReg(offset, value));
            REP_CASE(10, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->irqHandler.externalWrite8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma0.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma1.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma2.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma3.write8ToReg(offset, value));
            REP_CASE(4, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->keypad.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim0.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim1.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim2.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim3.write8ToReg(offset, value));
            default:
                //TODO how to handle not found? probably just ignore...
                LOG_IO(std::cout << "WARNING: externalWrite: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        }
    }

    uint8_t IO_Handler::internalRead8(uint32_t addr) const
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;
        switch (globalOffset) {
            REP_CASE(86, 0x0, lcdController->read8FromReg(offset));
            REP_CASE(10, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->irqHandler.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma0.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma1.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma2.read8FromReg(offset));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma3.read8FromReg(offset));
            REP_CASE(4, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->keypad.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim0.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim1.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim2.read8FromReg(offset));
            REP_CASE(4, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim3.read8FromReg(offset));
            default:
                //TODO how to handle not found?
                LOG_IO(std::cout << "WARNING: internalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
                return 0x0000;
        }
        return 0x0000;
    }

    void IO_Handler::internalWrite8(uint32_t addr, uint8_t value)
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;
        switch (globalOffset) {
            REP_CASE(86, 0x0, lcdController->write8ToReg(offset, value));
            REP_CASE(10, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->irqHandler.internalWrite8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma0.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma1.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma2.write8ToReg(offset, value));
            REP_CASE(12, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma3.write8ToReg(offset, value));
            REP_CASE(4, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->keypad.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim0.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim1.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim2.write8ToReg(offset, value));
            REP_CASE(4, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim3.write8ToReg(offset, value));
            default:
                //TODO how to handle not found? probably just ignore...
                LOG_IO(std::cout << "WARNING: internalWrite: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        }
    }

    uint16_t IO_Handler::externalRead16(uint32_t addr) const
    {
        return externalRead8(addr) | (static_cast<uint16_t>(externalRead8(addr + 1)) << 8);
    }
    uint32_t IO_Handler::externalRead32(uint32_t addr) const
    {
        return externalRead8(addr) | (static_cast<uint32_t>(externalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(externalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(externalRead8(addr + 3)) << 24);
    }
    void IO_Handler::externalWrite16(uint32_t addr, uint16_t value)
    {
        externalWrite8(addr, value & 0xFF);
        externalWrite8(addr + 1, (value >> 8) & 0xFF);
    }
    void IO_Handler::externalWrite32(uint32_t addr, uint32_t value)
    {
        externalWrite8(addr, value & 0xFF);
        externalWrite8(addr + 1, (value >> 8) & 0xFF);
        externalWrite8(addr + 2, (value >> 16) & 0xFF);
        externalWrite8(addr + 3, (value >> 24) & 0xFF);
    }
    uint16_t IO_Handler::internalRead16(uint32_t addr) const
    {
        return internalRead8(addr) | (static_cast<uint16_t>(internalRead8(addr + 1)) << 8);
    }
    uint32_t IO_Handler::internalRead32(uint32_t addr) const
    {
        return internalRead8(addr) | (static_cast<uint32_t>(internalRead8(addr + 1)) << 8) | (static_cast<uint32_t>(internalRead8(addr + 2)) << 16) | (static_cast<uint32_t>(internalRead8(addr + 3)) << 24);
    }
    void IO_Handler::internalWrite16(uint32_t addr, uint16_t value)
    {
        internalWrite8(addr, value & 0xFF);
        internalWrite8(addr + 1, (value >> 8) & 0xFF);
    }
    void IO_Handler::internalWrite32(uint32_t addr, uint32_t value)
    {
        internalWrite8(addr, value & 0xFF);
        internalWrite8(addr + 1, (value >> 8) & 0xFF);
        internalWrite8(addr + 2, (value >> 16) & 0xFF);
        internalWrite8(addr + 3, (value >> 24) & 0xFF);
    }
} // namespace gbaemu