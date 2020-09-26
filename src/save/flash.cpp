#include "flash.hpp"
#include "logging.hpp"

#include <iostream>

namespace gbaemu::save
{

    uint8_t FLASH::read(uint32_t address)
    {
        uint16_t lowerAddr = static_cast<uint16_t>(address & 0xFFFF);

        uint8_t result = 0;

        switch (state) {

            case READ_ID_1:
            case READ_ID_2:
                state = static_cast<FLASH_State>(state + 1);
                result = flashID[lowerAddr & 1];

                LOG_SAVE(std::cout << "FLASH: read ID" << std::endl;);
                break;

            case IDLE:
            default:
                saveFile.read(static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10), reinterpret_cast<char *>(&result), sizeof(result));

                LOG_SAVE(std::cout << "FLASH: read from: 0x" << std::hex << static_cast<uint32_t>(static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10)) << std::endl;);
                break;
        }

        return result;
    }

    void FLASH::write(uint32_t address, uint8_t data)
    {
        uint16_t lowerAddr = static_cast<uint16_t>(address & 0xFFFF);

        switch (state) {
            case IDLE:
                if (address == FLASH_CMD_SEQ1_ADDR) {
                    if (data == FLASH_CMD_START) {
                        state = RECV_INIT;
                    } else if (data == FLASH_CMD_TERMINATE) {
                        state = IDLE;

                        LOG_SAVE(std::cout << "FLASH: WARNING received terminate command" << std::endl;);
                    } else {
                        LOG_SAVE(std::cout << "FLASH: protocol error #1" << std::endl;);
                    }
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #2" << std::endl;);
                }
                break;

            case RECV_INIT:
                if (address == FLASH_CMD_SEQ2_ADDR && data == FLASH_CMD_INIT) {
                    state = RECV_CMD;
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #3" << std::endl;);
                }
                break;

            case RECV_CMD:
                if (address == FLASH_CMD_SEQ1_ADDR) {
                    switch (data) {
                        case FLASH_CMD_BANK_SW:
                            state = BANK_SW;
                            break;

                        case FLASH_CMD_ID:
                            state = READ_ID_1;
                            break;

                        case FLASH_CMD_WRITE:
                            state = WRITE;
                            break;

                        case FLASH_CMD_ERASE:
                            state = ERASE_1;
                            break;

                        default:
                            LOG_SAVE(std::cout << "FLASH: protocol error #4" << std::endl;);
                            break;
                    }
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #5" << std::endl;);
                }
                break;

            case ERASE_1:
            case READ_ID_ACK_1:
                if (address == FLASH_CMD_SEQ1_ADDR) {
                    if (data == FLASH_CMD_START) {
                        state = static_cast<FLASH_State>(state + 1);
                    } else {
                        LOG_SAVE(std::cout << "FLASH: protocol error #6" << std::endl;);
                    }
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #7" << std::endl;);
                }
                break;

            case ERASE_2:
            case READ_ID_ACK_2:
                if (address == FLASH_CMD_SEQ2_ADDR) {
                    if (data == FLASH_CMD_INIT) {
                        state = static_cast<FLASH_State>(state + 1);
                    } else {
                        LOG_SAVE(std::cout << "FLASH: protocol error #8" << std::endl;);
                    }
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #9" << std::endl;);
                }
                break;

            case ERASE_3:
                if (data == FLASH_CMD_ERASE_CHIP) {
                    saveFile.eraseAll();
                    state = IDLE;

                    LOG_SAVE(std::cout << "FLASH: erase chip" << std::endl;);
                } else if (data == FLASH_CMD_ERASE_4K) {
                    saveFile.erase(static_cast<uint32_t>(lowerAddr & 0x0F000) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10), 1 << 12);
                    state = IDLE;

                    LOG_SAVE(std::cout << "FLASH: erase 4K block: 0x" << std::hex << static_cast<uint32_t>(lowerAddr >> 12) << std::endl;);
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #10" << std::endl;);
                }
                break;

            case READ_ID_ACK_3:
                if (address == FLASH_CMD_SEQ1_ADDR && data == FLASH_CMD_TERMINATE) {
                    state = IDLE;
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #11" << std::endl;);
                }
                break;

            case BANK_SW:
                if (lowerAddr == static_cast<uint16_t>(0)) {
                    offsets64K = data;
                    state = IDLE;

                    LOG_SAVE(std::cout << "FLASH: switch bank: 0x" << std::hex << static_cast<uint32_t>(data) << std::endl;);
                } else {
                    LOG_SAVE(std::cout << "FLASH: protocol error #12" << std::endl;);
                }
                break;

            case WRITE:
                saveFile.write(static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10), reinterpret_cast<const char *>(&data), sizeof(data));
                state = IS_WRITE_ATMEL;

                LOG_SAVE(std::cout << "FLASH: write to: 0x" << std::hex << (static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10)) << std::endl;);
                break;

            case IS_WRITE_ATMEL:
                if (address == FLASH_CMD_SEQ1_ADDR) {
                    if (data == FLASH_CMD_START) {
                        state = RECV_INIT;
                    } else if (data == FLASH_CMD_TERMINATE) {
                        state = IDLE;
                    } else {
                        LOG_SAVE(std::cout << "FLASH: protocol error #13" << std::endl;);
                    }
                } else {
                    saveFile.write(static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10), reinterpret_cast<const char *>(&data), sizeof(data));
                    state = WRITE_ATMEL;
                    count = 126;

                    LOG_SAVE(std::cout << "FLASH: write ATMEL to: 0x" << std::hex << (static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10)) << std::endl;);
                }
                break;

            case WRITE_ATMEL:
                saveFile.write(static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10), reinterpret_cast<const char *>(&data), sizeof(data));
                if (--count == 0) {
                    state = IDLE;
                }

                LOG_SAVE(std::cout << "FLASH: write ATMEL to: 0x" << std::hex << (static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10)) << std::endl;);
                break;

            default:
                LOG_SAVE(std::cout << "FLASH: error unexpected write to: 0x" << std::hex << (static_cast<uint32_t>(lowerAddr) + static_cast<uint32_t>(offsets64K) * static_cast<uint32_t>(64 << 10)) << " in state: " << state << std::endl;);
                break;
        }
    }
} // namespace gbaemu::save
