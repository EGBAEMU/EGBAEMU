#ifndef INPUT_HPP
#define INPUT_HPP

#include "memory.hpp"
#include <cstdint>

namespace gbaemu
{
    class Keypad
    {
      private:
        static const constexpr uint32_t KEYPAD_REG_BASE_ADDR = Memory::IO_REGS_OFFSET + 0x130;

        Memory &memory;

        /*
        Keypad Input
          4000130h  2    R    KEYINPUT  Key Status
          4000132h  2    R/W  KEYCNT    Key Interrupt Control
        */
        struct KeypadRegister {
            uint16_t keyStatus;
            uint16_t keyIRQCnt;
        } __attribute__((packed)) regs = {0};

        uint8_t read8FromReg(uint32_t offset)
        {
            //TODO endianess???
            return *(offset + reinterpret_cast<uint8_t *>(&regs));
        }
        void write8ToReg(uint32_t offset, uint8_t value)
        {
            //TODO endianess???
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }

      public:
        Keypad(Memory &memory) : memory(memory)
        {
            memory.ioHandler.registerIOMappedDevice(
                IO_Mapped(
                    KEYPAD_REG_BASE_ADDR,
                    KEYPAD_REG_BASE_ADDR + sizeof(regs),
                    std::bind(&Keypad::read8FromReg, this, std::placeholders::_1),
                    std::bind(&Keypad::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&Keypad::read8FromReg, this, std::placeholders::_1),
                    std::bind(&Keypad::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
        }

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

        static const uint8_t KEY_IRQ_EN_OFFSET = 14;
        static const uint8_t KEY_IRQ_COND_OFFSET = 15;

        void setKeyInputState(bool released, KeyInput key)
        {
            uint16_t currentValue = regs.keyStatus;
            currentValue = (currentValue & ~(static_cast<uint16_t>(1) << key)) | (released ? (static_cast<uint16_t>(1) << key) : 0);
            regs.keyStatus = currentValue;
            checkIRQConditions(currentValue);
        }

      private:
        void checkIRQConditions(uint16_t keyinputReg)
        {
            uint16_t conditions = regs.keyIRQCnt;
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