#include "eeprom.hpp"
#include "logging.hpp"

#include <iostream>

namespace gbaemu::save
{
    void EEPROM::expand(uint8_t busWidth)
    {
        this->busWidth = busWidth;
        // For bus width of 14 only the lower 10 bits are actually used
        saveFile.expandSaveFileSize((1 << (busWidth == 14 ? 10 : 6)) * sizeof(buffer));
    }

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
                    LOG_SAVE(std::cout << "EEPROM: read request detected!" << std::endl;);
                } else if (buffer == 0b10) {
                    state = WRITE_RECV_ADDR;
                    LOG_SAVE(std::cout << "EEPROM: write request detected!" << std::endl;);
                } else {
                    // Invalid request!
                    std::cout << "ERROR: invalid EEPROM request! 0x" << std::hex << static_cast<uint32_t>(buffer) << std::endl;
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
                    // Ensure that at max. 10 bit address are used (needed for 14 bit buswidth)
                    addr = static_cast<uint16_t>(buffer) & 0x3FF;
                    buffer = 0;
                    counter = 0;
                    state = static_cast<EEPROM_State>(state + 1);
                    LOG_SAVE(std::cout << "EEPROM: received address!" << std::endl;);
                }
                break;

            case READ_RECV_ADDR_ACK:
                state = READ_WASTE;
                //TODO endianess or make save states device dependent?
                saveFile.read(addr * sizeof(buffer), reinterpret_cast<char *>(&buffer), sizeof(buffer));
                break;

            case WRITE:
                buffer <<= 1;
                buffer |= data & 0x1;
                ++counter;
                state = counter == 64 ? WRITE_ACK : WRITE;
                break;

            case WRITE_ACK:
                state = IDLE;
                //TODO endianess or make save states device dependent?
                saveFile.write(addr * sizeof(buffer), reinterpret_cast<char *>(&buffer), sizeof(buffer));
                LOG_SAVE(std::cout << "EEPROM: write done!" << std::endl;);
                break;

            default:
                // Invalid actions: ignore
                //TODO log protocol errors!
                break;
        }
    }

    uint8_t EEPROM::read()
    {
        uint8_t data = 0;
        switch (state) {
            case READ:
                data = (buffer >> 63) & 0x1;
                buffer <<= 1;
                ++counter;
                if (counter == 64) {
                    state = IDLE;
                    LOG_SAVE(std::cout << "EEPROM: read done!" << std::endl;);
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