#include "flash.hpp"

namespace gbaemu::save
{

    uint8_t FLASH::read(uint32_t address)
    {
        uint16_t lowerAddr = static_cast<uint16_t>(address & 0xFFFF);

        uint8_t result = 0;

        switch (state) {

            case READ_ID_1:
            case READ_ID_2:
                //TODO which manufacturer to use?
                state = static_cast<FLASH_State>(state + 1);
                result = FLASH_ID_SST[lowerAddr & 1];
                break;

            case IDLE:
            default:
                saveFile.read(lowerAddr + offsets64K * (64 << 10), reinterpret_cast<char *>(&result), sizeof(result));
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
                        //TODO what to do on terminate command?
                    } else {
                        //TODO error?
                    }
                }
                break;

            case RECV_INIT:
                if (address == FLASH_CMD_SEQ2_ADDR && data == FLASH_CMD_INIT) {
                    state = RECV_CMD;
                } else {
                    //TODO error?
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
                            //TODO we need to differentiate between atmel & non atmel
                            if (/*atmel*/ false) {
                                count = 128;
                            } else {
                                count = 1;
                            }
                            state = WRITE;
                            break;

                        case FLASH_CMD_ERASE:
                            state = ERASE_1;
                            break;

                        default:
                            //TODO error?
                            break;
                    }
                } else {
                    //TODO error?
                }
                break;

            case ERASE_1:
            case READ_ID_ACK_1:
                if (address == FLASH_CMD_SEQ1_ADDR) {
                    if (data == FLASH_CMD_START) {
                        state = static_cast<FLASH_State>(state + 1);
                    } else {
                        //TODO error?
                    }
                }
                break;

            case ERASE_2:
            case READ_ID_ACK_2:
                if (address == FLASH_CMD_SEQ2_ADDR) {
                    if (data == FLASH_CMD_INIT) {
                        state = static_cast<FLASH_State>(state + 1);
                    } else {
                        //TODO error?
                    }
                }
                break;

            case ERASE_3:
                if (data == FLASH_CMD_ERASE_CHIP) {
                    saveFile.eraseAll();
                    state = IDLE;
                } else if (data == FLASH_CMD_ERASE_4K) {
                    saveFile.erase(lowerAddr & 0x0F000 + offsets64K * (64 << 10), 1 << 12);
                    state = IDLE;
                } else {
                    //TODO error?
                }
                break;

            case READ_ID_ACK_3:
                if (address == FLASH_CMD_SEQ1_ADDR && data == FLASH_CMD_TERMINATE) {
                    state = IDLE;
                } else {
                    //TODO error?
                }
                break;

            case BANK_SW:
                if (lowerAddr == static_cast<uint16_t>(0)) {
                    offsets64K = data;
                    state = IDLE;
                } else {
                    //TODO error?
                }
                break;

            case WRITE:
                saveFile.write(lowerAddr + offsets64K * (64 << 10), reinterpret_cast<const char *>(&data), sizeof(data));
                if (--count == 0) {
                    state = IDLE;
                }
                break;
        }
    }
} // namespace gbaemu::save
