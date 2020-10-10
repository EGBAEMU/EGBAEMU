#ifndef VRAM_HPP
#define VRAM_HPP

#include <cstdint>

namespace gbaemu
{
    class VRAM
    {
      private:
        uint8_t *vram;

      public:
        VRAM();
        ~VRAM();

        void reset();

        uint8_t* rawAccess() {
            return vram;
        }

        uint8_t read8(uint32_t addr) const;
        uint16_t read16(uint32_t addr) const;
        uint32_t read32(uint32_t addr) const;

        void write8(uint32_t addr, uint8_t value);
        void write16(uint32_t addr, uint16_t value);
        void write32(uint32_t addr, uint32_t value);

        private:
          static uint32_t handleMirroring(uint32_t addr);
    };
} // namespace gbaemu


#endif