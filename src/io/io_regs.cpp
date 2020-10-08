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
        static_assert(sizeof(lcdController->regs) == 86);
        static_assert(sizeof(cpu->sound.regs) == 56);
        static_assert(sizeof(cpu->sound.channel1.regs) == 8);
        static_assert(sizeof(cpu->sound.channel2.regs) == 8);
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

        static_assert(offsetof(lcd::LCDIORegs, WININ) == 0x48);
        static_assert(offsetof(lcd::LCDIORegs, BLDCNT) == 0x50);

        static_assert(offsetof(sound::SoundOrchestrator::SoundControlRegs, fifoAL) == 48);

        static_assert(offsetof(DMAGroup::DMA<DMAGroup::DMA0>::DMARegs, cntReg) == 0xA);
        static_assert(offsetof(DMAGroup::DMA<DMAGroup::DMA1>::DMARegs, cntReg) == 0xA);
        static_assert(offsetof(DMAGroup::DMA<DMAGroup::DMA2>::DMARegs, cntReg) == 0xA);
        static_assert(offsetof(DMAGroup::DMA<DMAGroup::DMA3>::DMARegs, cntReg) == 0xA);

        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;

        switch (globalOffset) {
            // Most lcd regs are write only
            REP_CASE(16, globalOffset, 0x0, return lcdController->read8FromReg(offset));
            REP_CASE(4, globalOffset, offsetof(lcd::LCDIORegs, WININ), return lcdController->read8FromReg(offset + offsetof(lcd::LCDIORegs, WININ)));
            REP_CASE(4, globalOffset, offsetof(lcd::LCDIORegs, BLDCNT), return lcdController->read8FromReg(offset + offsetof(lcd::LCDIORegs, BLDCNT)));
            // Sound register only fifo regs are write only
            //TODO split for unused fields!
            REP_CASE(48, globalOffset, sound::SoundOrchestrator::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->sound.read8FromReg(offset));
            //TODO split for unused fields!
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->sound.channel1.read8FromReg(offset));
            //TODO split for unused fields!
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET + sizeof(cpu->sound.channel2.regs), return cpu->sound.channel2.read8FromReg(offset));
            // DMA: only the higher control register is readable!
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA0>::DMARegs, cntReg), return cpu->dmaGroup.dma0.read8FromReg(offset + offsetof(DMAGroup::DMA<DMAGroup::DMA0>::DMARegs, cntReg)));
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA1>::DMARegs, cntReg), return cpu->dmaGroup.dma1.read8FromReg(offset + offsetof(DMAGroup::DMA<DMAGroup::DMA1>::DMARegs, cntReg)));
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA2>::DMARegs, cntReg), return cpu->dmaGroup.dma2.read8FromReg(offset + offsetof(DMAGroup::DMA<DMAGroup::DMA2>::DMARegs, cntReg)));
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA3>::DMARegs, cntReg), return cpu->dmaGroup.dma3.read8FromReg(offset + offsetof(DMAGroup::DMA<DMAGroup::DMA3>::DMARegs, cntReg)));
            // DMA count registers read returns always 0
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA0>::DMARegs, count), (void)offset; return 0);
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA1>::DMARegs, count), (void)offset; return 0);
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA2>::DMARegs, count), (void)offset; return 0);
            REP_CASE(2, globalOffset, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET + offsetof(DMAGroup::DMA<DMAGroup::DMA3>::DMARegs, count), (void)offset; return 0);
            // Timer: all regs are readable
            REP_CASE(4, globalOffset, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim0.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim1.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim2.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim3.read8FromReg(offset));
            // Keypad regs
            REP_CASE(4, globalOffset, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->keypad.read8FromReg(offset));
            // IRQ regs
            REP_CASE(10, globalOffset, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->irqHandler.read8FromReg(offset));
        }
        LOG_IO(std::cout << "WARNING: externalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        return cpu->state.memory.readUnusedHandle() >> ((addr & 3) << 3);
    }

    void IO_Handler::externalWrite8(uint32_t addr, uint8_t value)
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;

        static_assert(offsetof(lcd::LCDIORegs, BGCNT) == 0x08);

        switch (globalOffset) {
            // LCD: only VCOUNT reg is not writable
            REP_CASE(6, globalOffset, 0x0, lcdController->write8ToReg(offset, value));
            REP_CASE(78, globalOffset, offsetof(lcd::LCDIORegs, BGCNT), lcdController->write8ToReg(offset + offsetof(lcd::LCDIORegs, BGCNT), value));
            // Sound register
            REP_CASE(56, globalOffset, sound::SoundOrchestrator::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->sound.externalWrite8ToReg(offset, value));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->sound.channel1.write8ToReg(offset, value));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET + sizeof(cpu->sound.channel2.regs), cpu->sound.channel2.write8ToReg(offset, value));
            // DMA: all writable
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma0.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma1.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma2.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma3.write8ToReg(offset, value));
            // Timer: all writable
            REP_CASE(4, globalOffset, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim0.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim1.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim2.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim3.write8ToReg(offset, value));
            // IRQ: all writable
            REP_CASE(10, globalOffset, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->irqHandler.externalWrite8ToReg(offset, value));
            // Keypad only KEYCNT writable
            REP_CASE(2, globalOffset, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET + sizeof(uint16_t), cpu->keypad.write8ToReg(offset + sizeof(uint16_t), value));
            default:
                //TODO how to handle not found? probably just ignore...
                LOG_IO(std::cout << "WARNING: externalWrite: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        }
    }

    uint8_t IO_Handler::internalRead8(uint32_t addr) const
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;
        switch (globalOffset) {
            // Allow all reads!
            // LCD
            REP_CASE(86, globalOffset, 0x0, return lcdController->read8FromReg(offset));
            // Sound register
            REP_CASE(56, globalOffset, sound::SoundOrchestrator::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->sound.read8FromReg(offset));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->sound.channel1.read8FromReg(offset));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET + sizeof(cpu->sound.channel2.regs), return cpu->sound.channel2.read8FromReg(offset));
            // DMA
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma0.read8FromReg(offset));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma1.read8FromReg(offset));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma2.read8FromReg(offset));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->dmaGroup.dma3.read8FromReg(offset));
            // Timer
            REP_CASE(4, globalOffset, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim0.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim1.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim2.read8FromReg(offset));
            REP_CASE(4, globalOffset, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, return cpu->timerGroup.tim3.read8FromReg(offset));
            // IRQ
            REP_CASE(10, globalOffset, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, return cpu->irqHandler.read8FromReg(offset));
            // Keypad
            REP_CASE(4, globalOffset, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, return cpu->keypad.read8FromReg(offset));
        }
        LOG_IO(std::cout << "WARNING: internalRead: no io handler registered for address: 0x" << std::hex << addr << std::endl;);
        return cpu->state.memory.readUnusedHandle() >> ((addr & 3) << 3);
    }

    void IO_Handler::internalWrite8(uint32_t addr, uint8_t value)
    {
        uint32_t globalOffset = addr - Memory::IO_REGS_OFFSET;
        switch (globalOffset) {
            // Allow all writes!
            REP_CASE(86, globalOffset, 0x0, lcdController->write8ToReg(offset, value));
            // Sound register
            REP_CASE(56, globalOffset, sound::SoundOrchestrator::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->sound.internalWrite8ToReg(offset, value));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->sound.channel1.write8ToReg(offset, value));
            REP_CASE(8, globalOffset, sound::SquareWaveChannel::SOUND_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET + sizeof(cpu->sound.channel2.regs), cpu->sound.channel2.write8ToReg(offset, value));
            // DMA
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA0>::DMA0_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma0.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA1>::DMA1_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma1.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA2>::DMA2_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma2.write8ToReg(offset, value));
            REP_CASE(12, globalOffset, DMAGroup::DMA<DMAGroup::DMA3>::DMA3_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->dmaGroup.dma3.write8ToReg(offset, value));
            // Timer
            REP_CASE(4, globalOffset, TimerGroup::Timer<0>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim0.regs) * 0 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim0.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<1>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim1.regs) * 1 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim1.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<2>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim2.regs) * 2 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim2.write8ToReg(offset, value));
            REP_CASE(4, globalOffset, TimerGroup::Timer<3>::TIMER_REGS_BASE_OFFSET + sizeof(cpu->timerGroup.tim3.regs) * 3 - Memory::IO_REGS_OFFSET, cpu->timerGroup.tim3.write8ToReg(offset, value));
            // IRQ
            REP_CASE(10, globalOffset, InterruptHandler::INTERRUPT_CONTROL_REG_ADDR - Memory::IO_REGS_OFFSET, cpu->irqHandler.internalWrite8ToReg(offset, value));
            // Keypad
            REP_CASE(4, globalOffset, Keypad::KEYPAD_REG_BASE_ADDR - Memory::IO_REGS_OFFSET, cpu->keypad.write8ToReg(offset, value));
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