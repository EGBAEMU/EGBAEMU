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

        void setKeyInputState(bool state, KeyInput key)
        {
            uint16_t currentValue = memory.read16(KEY_STATUS_REG, nullptr);
            currentValue = (currentValue & ~(1 << key)) | (state ? (1 << key) : 0);
            memory.write16(KEY_STATUS_REG, currentValue, nullptr);
        }
    };
} // namespace gbaemu

#endif