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

      private:
        std::array<lcd::OBJ, 128> objects;
        uint32_t dirtyFlags[4];

      public:
        OAM();
        ~OAM();

        void reset();

        void write16(uint32_t offset, uint16_t value);
        void write32(uint32_t offset, uint32_t value);

        std::array<lcd::OBJ, 128>::const_iterator getUpdateObjs(lcd::BGMode bgMode);
        std::array<lcd::OBJ, 128>::const_iterator getEndIt() const;

      private:
        void
        delegateDecode(uint32_t offset, uint16_t value);

        void setDirtyFlag(uint8_t objOffset);
    };

} // namespace gbaemu

#endif