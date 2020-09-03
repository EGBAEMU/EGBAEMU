#ifndef INPUT_HPP
#define INPUT_HPP

#include "memory.hpp"
#include <cstdint>

namespace gbaemu
{
    class Keypad
    {
      private:
        Memory &memory;

      public:
        Keypad(Memory &memory) : memory(memory) {}

        enum KeyInput : uint8_t {
            BUTTON_A = 0,
            BUTTON_B,
            SELECT,
            START,
            RIGHT,
            LEFT,
            UP,
            DOWN,
            BUTTON_L,
            BUTTON_R,
        };

        static const uint32_t KEY_STATUS_REG = 0x04000130;
        static const uint32_t KEY_INTERRUPT_CNT_REG = 0x04000132;

        static const uint8_t KEY_IRQ_EN_OFFSET = 14;
        static const uint8_t KEY_IRQ_COND_OFFSET = 15;

        void setKeyInputState(bool released, KeyInput key)
        {
            uint16_t currentValue = memory.read16(KEY_STATUS_REG, nullptr);
            currentValue = (currentValue & ~(static_cast<uint16_t>(1) << key)) | (released ? (static_cast<uint16_t>(1) << key) : 0);
            memory.write16(KEY_STATUS_REG, currentValue, nullptr);
            checkIRQConditions(currentValue);
        }

      private:
        void checkIRQConditions(uint16_t keyinputReg)
        {
            uint16_t conditions = memory.read16(KEY_STATUS_REG, nullptr);
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
                  //TODO trigger a IRQ!!!
                }
            }
        }
    };
} // namespace gbaemu

#endif