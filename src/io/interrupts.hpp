#include "regs.hpp"
#include "cpu.hpp"

namespace gbaemu 
{

    /*
        Bit   Expl.
        0     LCD V-Blank                    (0=Disable)
        1     LCD H-Blank                    (etc.)
        2     LCD V-Counter Match            (etc.)
        3     Timer 0 Overflow               (etc.)
        4     Timer 1 Overflow               (etc.)
        5     Timer 2 Overflow               (etc.)
        6     Timer 3 Overflow               (etc.)
        7     Serial Communication           (etc.)
        8     DMA 0                          (etc.)
        9     DMA 1                          (etc.)
        10    DMA 2                          (etc.)
        11    DMA 3                          (etc.)
        12    Keypad                         (etc.)
        13    Game Pak (external IRQ source) (etc.)
        14-15 Not used
    */
    enum InterruptType: uint8_t {
        LCD_V_BLANK,
        LCD_H_BLANK,
        LCD_V_COUNTER_MATCH,
        TIMER_0_OVERFLOW,
        TIMER_1_OVERFLOW,
        TIMER_2_OVERFLOW,
        TIMER_3_OVERFLOW,
        SERIAL_COMM,
        DMA_0,
        DMA_1,
        DMA_2,
        DMA_3,
        KEYPAD,
        GAME_PAK
    };

    struct InterruptControlRegs {
        uint16_t irqEnable;
        uint16_t irqRequest;
        uint16_t waitStateCnt;
        uint16_t _;
        uint16_t irqMasterEnable;
    } __attribute__((packed));

    class InterruptHandler 
    {
        private:
          CPU *cpu;
          InterruptControlRegs regs = {0};

          static const constexpr uint32_t INTERRUPT_CONTROL_REG_ADDR = Memory::IO_REGS_OFFSET + 0x200;

          uint8_t read8FromReg(uint32_t offset)
          {
              //TODO endianess???
              return *(offset + reinterpret_cast<uint8_t *>(&regs));
          }
          void internalWrite8ToReg(uint32_t offset, uint8_t value)
          {
              //TODO endianess???
              *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
          }
          void externalWrite8ToReg(uint32_t offset, uint8_t value)
          {
              //TODO endianess???
              //TODO implement special behaviour, of clearing where it applies etc.!
          }

        public:
            InterruptHandler(CPU* cpu) : cpu(cpu) {
                cpu->state.memory.ioHandler.registerIOMappedDevice(
                    IO_Mapped(
                        INTERRUPT_CONTROL_REG_ADDR,
                        INTERRUPT_CONTROL_REG_ADDR + sizeof(regs),
                        std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                        std::bind(&InterruptHandler::externalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                        std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                        std::bind(&InterruptHandler::internalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
            }

            bool isInterruptMasterSet() const;

            bool isCPSRInterruptSet() const;
            bool isCPSRFastInterruptSet() const;

            bool isInterruptEnabled(InterruptType type) const;
            bool wasInterruptAcknowledged(InterruptType type) const;
    
            void checkForInterrupt() const;
    };

}