#ifndef OAM_HPP
#define OAM_HPP

#include "lcd/obj.hpp"
#include <array>
#include <cstdint>

namespace gbaemu
{

    class OAM
    {
      public:
        uint8_t *mem;

        std::array<lcd::OBJ, 128> objects;

      public:
        OAM();
        ~OAM();

        void reset();

        void write16(uint32_t offset, uint16_t value);
        void write32(uint32_t offset, uint32_t value);
    };

} // namespace gbaemu

#endif