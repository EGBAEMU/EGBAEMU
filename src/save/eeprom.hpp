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
        const uint8_t busWidth;

        void reset()
        {
            state = IDLE;
        }

        EEPROM(const char *path, bool &success, uint8_t busWidth = 6) : saveFile(path, success, (1 << busWidth) * 64), busWidth(busWidth)
        {
            reset();
        }

        void write(uint8_t data);
        uint8_t read();
    };

} // namespace gbaemu::save
#endif