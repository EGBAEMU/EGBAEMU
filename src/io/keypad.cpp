#ifndef INPUT_HPP
#define INPUT_HPP

#include "keypad.hpp"
#include "cpu/cpu.hpp"
#include "interrupts.hpp"
#include "util.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>

namespace gbaemu
{
    void Keypad::reset()
    {
        std::fill_n(reinterpret_cast<char*>(&regs), sizeof(regs), 0);
        regs.keyStatus = 0xFFFF;
    }

    uint8_t Keypad::read8FromReg(uint32_t offset)
    {
        return *(offset + reinterpret_cast<uint8_t *>(&regs));
    }
    void Keypad::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
    }

    Keypad::Keypad(CPU *cpu) : irqHandler(cpu->irqHandler)
    {
        reset();
    }

    void Keypad::setKeyInputState(bool released, KeyInput key)
    {
        uint16_t currentValue = le(regs.keyStatus);
        currentValue = (currentValue & ~(static_cast<uint16_t>(1) << key)) | (released ? (static_cast<uint16_t>(1) << key) : 0);
        regs.keyStatus = le(currentValue);
        checkIRQConditions(currentValue);
    }

    void Keypad::checkIRQConditions(uint16_t keyinputReg)
    {
        uint16_t conditions = le(regs.keyIRQCnt);
        // Are interrupt enabled?
        if (conditions & (static_cast<uint16_t>(1) << KEY_IRQ_EN_OFFSET)) {
            bool andCond = conditions & (static_cast<uint16_t>(1) << KEY_IRQ_COND_OFFSET);

            // mask out not used bits
            keyinputReg &= 0x03FF;
            // mask out irrelevant select bits
            conditions &= 0x03FF;

            bool triggerIRQ;

            //  key input bits are 0 if pressed! therefore we invert its values to have a 1 if pressed
            keyinputReg = ~keyinputReg;
            if (andCond) {
                // true iff all selected inputs are pressed
                triggerIRQ = (conditions & keyinputReg) == conditions;
            } else {
                // true iff any selected input is pressed
                triggerIRQ = conditions & keyinputReg;
            }

            if (triggerIRQ) {
                irqHandler.setInterrupt<InterruptHandler::InterruptType::KEYPAD>();
            }
        }
    }

} // namespace gbaemu

#endif