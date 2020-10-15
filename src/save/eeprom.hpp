#ifndef EEPROM_HPP
#define EEPROM_HPP

#include "save_file.hpp"

#include <cstdint>

namespace gbaemu::save
{
    class EEPROM
    {
      private:
        enum EEPROM_State {
            IDLE = 0,
            RECEIVE_REQUEST,
            // The order here is important because of state + 1 hacks in eeprom.cpp
            READ_RECV_ADDR,
            READ_RECV_ADDR_ACK,
            READ_WASTE,
            READ,
            // The order here is important because of state + 1 hacks in eeprom.cpp
            WRITE_RECV_ADDR,
            WRITE,
            WRITE_ACK
        };
        EEPROM_State state;
        uint32_t counter;
        uint64_t buffer;
        uint16_t addr;

        SaveFile saveFile;

      public:
        uint8_t busWidth;

        void reset()
        {
            state = IDLE;
        }

        EEPROM(const char *path, bool &success, uint8_t busWidth = 6) : saveFile(path, success, (static_cast<uint32_t>(1) << busWidth) * sizeof(buffer))
        {
            this->busWidth = saveFile.getSize() == 0x2000 ? 14 : 6;
            reset();
        }

        void expand(uint8_t busWidth);

        void write(uint8_t data);
        uint8_t read();

        bool knowsBitWidth() const
        {
            return !saveFile.isNewSaveFile();
        }
    };

} // namespace gbaemu::save
#endif