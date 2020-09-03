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

    class InterruptHandler 
    {

        public:

            bool isInterruptMasterSet() const;

            bool isCPSRInterruptSet() const;
            bool isCPSRFastInterruptSet() const;

            bool isInterruptEnabled(InterruptType type) const;
            bool wasInterruptAcknowledged(InterruptType type) const;
    
            void checkForInterrupt() const;

        private:

            CPU *cpu;
          
    };

}