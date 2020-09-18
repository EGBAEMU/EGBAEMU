#ifndef FLASH_HPP
#define FLASH_HPP

#include "save_file.hpp"

#include <cstdint>

namespace gbaemu::save
{

    class FLASH
    {
      private:
        static const constexpr uint32_t FLASH_CMD_SEQ1_ADDR = 0xE005555;
        static const constexpr uint32_t FLASH_CMD_SEQ2_ADDR = 0xE002AAA;

        static const constexpr uint8_t FLASH_CMD_START = 0xAA;
        static const constexpr uint8_t FLASH_CMD_INIT = 0x55;

        static const constexpr uint8_t FLASH_CMD_TERMINATE = 0xF0;

        static const constexpr uint8_t FLASH_CMD_ERASE = 0x80;
        static const constexpr uint8_t FLASH_CMD_WRITE = 0xA0;
        static const constexpr uint8_t FLASH_CMD_BANK_SW = 0xB0;
        static const constexpr uint8_t FLASH_CMD_ID = 0x90;

        static const constexpr uint8_t FLASH_CMD_ERASE_4K = 0x30;
        static const constexpr uint8_t FLASH_CMD_ERASE_CHIP = 0x10;

        //TODO little or big endian? (currently in little endian)
        static const constexpr uint8_t FLASH_ID_SST[2] = {0xBF, 0xD4};
        static const constexpr uint8_t FLASH_ID_MACRONIX_64K[2] = {0xC2, 0x1C};
        static const constexpr uint8_t FLASH_ID_PANASONIC[2] = {0x32, 0x1B};
        static const constexpr uint8_t FLASH_ID_ATMEL[2] = {0x1F, 0x3D};
        static const constexpr uint8_t FLASH_ID_SANYO[2] = {0x62, 0x12};
        static const constexpr uint8_t FLASH_ID_MACRONIX_128K[2] = {0xC2, 0x09};

        enum FLASH_State {
            IDLE = 0,
            RECV_INIT,
            RECV_CMD,

            //order is important
            ERASE_1,
            ERASE_2,
            ERASE_3,

            BANK_SW,

            WRITE,

            //order is important
            READ_ID_1,
            READ_ID_2,
            READ_ID_ACK_1,
            READ_ID_ACK_2,
            READ_ID_ACK_3,
        };
        FLASH_State state;

        SaveFile saveFile;
        uint8_t offsets64K;
        uint8_t count;

      public:
        FLASH(const char *path, bool &success, uint32_t size) : saveFile(path, success, size)
        {
            reset();
        }

        void reset()
        {
            state = IDLE;
            offsets64K = 0;
        }

        uint8_t read(uint32_t address);
        void write(uint32_t address, uint8_t data);
    };

} // namespace gbaemu::save
#endif