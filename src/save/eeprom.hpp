#ifndef EEPROM_HPP
#define EEPROM_HPP

#include <cstdint>
#include <fstream>
#include <string>

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
        EEPROM_State state = IDLE;
        uint32_t counter;
        uint64_t buffer;
        uint16_t addr;
        
        std::fstream saveFile;

      public:
        const uint8_t busWidth;

        EEPROM(const char* path, bool &success, uint8_t busWidth = 6) : busWidth(busWidth) {
            bool isNewFile = !std::ifstream(path).good();

            saveFile = std::fstream(path, std::ios::binary | std::ios::in | std::ios::out);
            success = saveFile.is_open();

            if (success && isNewFile) {
                // Fill with needed size!
                const auto prevWidth = saveFile.width();
                const auto prevFill = saveFile.fill();
                // Set fill character
                saveFile.fill(0x00);
                // Set target file size
                saveFile.width(1 << busWidth);
                // Write single fill char -> rest will be filled automagically
                saveFile << 0x00;

                // Restore default settings
                saveFile.fill(prevFill);
                saveFile.width(prevWidth);
            }
        }

        ~EEPROM() {
            if (saveFile.is_open())
                saveFile.close();
        }

        void write(uint8_t data);
        uint8_t read();
    };

} // namespace gbaemu::save
#endif