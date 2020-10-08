#ifndef KEYPAD_HPP
#define KEYPAD_HPP

#include "io_regs.hpp"
#include "memory.hpp"
#include "packed.h"
#include "util.hpp"
#include <cstdint>

namespace gbaemu
{
    class CPU;
    class InterruptHandler;

    class Keypad
    {
      private:
        static const constexpr uint32_t KEYPAD_REG_BASE_ADDR = Memory::IO_REGS_OFFSET + 0x130;

        InterruptHandler &irqHandler;

        /*
        Keypad Input
          4000130h  2    R    KEYINPUT  Key Status
          4000132h  2    R/W  KEYCNT    Key Interrupt Control
        */
        PACK_STRUCT(KeypadRegister, regs,
                    uint16_t keyStatus;
                    uint16_t keyIRQCnt;);

        uint8_t read8FromReg(uint32_t offset);
        void write8ToReg(uint32_t offset, uint8_t value);

      public:
        Keypad(CPU *cpu);

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

        static const constexpr uint8_t KEY_IRQ_EN_OFFSET = 14;
        static const constexpr uint8_t KEY_IRQ_COND_OFFSET = 15;

        void setKeyInputState(bool released, KeyInput key);

        void reset();

      private:
        void checkIRQConditions(uint16_t keyinputReg);

        friend class IO_Handler;
    };
} // namespace gbaemu

#endif