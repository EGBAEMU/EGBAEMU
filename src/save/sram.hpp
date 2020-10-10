#ifndef SRAM_HPP
#define SRAM_HPP

#include "save_file.hpp"

namespace gbaemu::save
{
    class SRAM
    {
      private:
        SaveFile saveFile;

      public:
        SRAM(const char *path, bool &success, uint32_t fallBackSize);

        uint8_t read8(uint32_t addr);
        void write8(uint32_t addr, uint8_t value);
    };

} // namespace gbaemu::save
#endif