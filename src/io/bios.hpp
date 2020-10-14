#ifndef BIOS_HPP
#define BIOS_HPP

#include <cstddef>
#include <cstdint>
namespace gbaemu
{
    class Bios
    {
      public:
        // static const constexpr uint8_t BIOS_READ_AFTER_STARTUP[] = {0x00, 0xF0, 0x29, 0xE1};
        static const constexpr uint32_t BIOS_AFTER_STARTUP = 0xE129F000;
        // static const constexpr uint8_t BIOS_READ_AFTER_SWI[] = {0x04, 0x20, 0xA0, 0xE3};
        static const constexpr uint32_t BIOS_AFTER_SWI = 0xE3A02004;
        // static const constexpr uint8_t BIOS_READ_DURING_IRQ[] = {0x04, 0xF0, 0x5E, 0xE2};
        static const constexpr uint32_t BIOS_DURING_IRQ = 0xE25EF004;
        // static const constexpr uint8_t BIOS_READ_AFTER_IRQ[] = {0x02, 0xC0, 0x5E, 0xE5};
        static const constexpr uint32_t BIOS_AFTER_IRQ = 0xE55EC002;

      private:
        static const uint8_t fallbackBios[];

        const uint8_t *bios;
        size_t biosSize;
        bool externalBios;

        bool execInBios;

        uint32_t biosState;

      public:
        Bios();

        bool usesExternalBios() const
        {
            return externalBios;
        }
        size_t getBiosSize() const
        {
            return biosSize;
        }
        uint32_t getBiosState() const
        {
            return biosState;
        }
        void forceBiosState(uint32_t state)
        {
            this->biosState = state;
        }

        void setExecInsideBios(bool execBios)
        {
            this->execInBios = execBios;
        }

        void setExternalBios(const uint8_t *bios, size_t biosSize);

        uint8_t read8(uint32_t addr) const;
        uint16_t read16(uint32_t addr) const;
        uint16_t read16Inst(uint32_t addr);
        uint32_t read32(uint32_t addr) const;
        uint32_t read32Inst(uint32_t addr);
    };
} // namespace gbaemu
#endif