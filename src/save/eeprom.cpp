#include "eeprom.hpp"

#include <iostream>

namespace gbaemu::save
{

    void EEPROM::write(uint8_t data)
    {
        switch (state) {
            case IDLE:
                state = RECEIVE_REQUEST;
                buffer = (data & 0x1) << 1;
                break;

            case RECEIVE_REQUEST:
                counter = 0;
                buffer |= data & 0x1;
                if (buffer == 0b11) {
                    state = READ_RECV_ADDR;
                    std::cout << "EEPROM: read request detected!" << std::endl;
                } else if (buffer == 0b10) {
                    state = WRITE_RECV_ADDR;
                    std::cout << "EEPROM: write request detected!" << std::endl;
                } else {
                    // Invalid request!
                    std::cout << "ERROR: invalid EEPROM request!" << std::endl;
                    state = IDLE;
                }
                buffer = 0;
                break;

            case READ_RECV_ADDR:
            case WRITE_RECV_ADDR:
                buffer <<= 1;
                buffer |= data & 0x1;
                ++counter;
                if (counter == busWidth) {
                    addr = buffer;
                    buffer = 0;
                    counter = 0;
                    state = static_cast<EEPROM_State>(state + 1);
                    std::cout << "EEPROM: received address!" << std::endl;
                }
                break;

            case READ_RECV_ADDR_ACK:
                state = READ_WASTE;
                // Seek read pointer to wanted position
                saveFile.seekg(saveFile.beg + addr * 64);
                //TODO endianess or make save states device dependent?
                saveFile >> buffer;
                break;

            case WRITE:
                buffer <<= 1;
                buffer |= data & 0x1;
                ++counter;
                state = counter == 64 ? WRITE_ACK : WRITE;
                break;

            case WRITE_ACK:
                state = IDLE;
                // Seek write pointer to wanted position
                saveFile.seekp(saveFile.beg + addr * 64);
                //TODO endianess or make save states device dependent?
                saveFile << buffer;
                std::cout << "EEPROM: write done!" << std::endl;
                break;

            default:
                // Invalid actions: ignore
                //TODO log protocol errors!
                break;
        }
    }

    uint8_t EEPROM::read()
    {
        uint8_t data;
        switch (state) {
            case READ:
                data = buffer & 0x1;
                buffer >>= 1;
                ++counter;
                if (counter  == 64) {
                    state = IDLE;
                    std::cout << "EEPROM: read done!" << std::endl;
                }
                break;

            case READ_WASTE:
                ++counter;
                if (counter == 4) {
                    state = READ;
                    counter = 0;
                }
                break;

            default:
                break;
        }

        return data;
    }

} // namespace gbaemu::save