/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <ch.hpp>
#include <cstdint>
#include <hal.h>

using namespace chibios_rt;

#define BUFFER_SIZE 1
//#define BAUD_RATE 9600
//#define BAUD_RATE 19200
//#define BAUD_RATE 57600
#define BAUD_RATE 115200

#define UART_DRIVER &SD5

enum KeyStatus : uint8_t {
    RELEASED = 0,
    PRESSED = 1
};

enum Keys : uint8_t {
    KEY_LEFT = 0,
    KEY_RIGHT = 1 << 1,
    KEY_UP = 2 << 1,
    KEY_DOWN = 3 << 1,
    KEY_A = 4 << 1,
    KEY_B = 5 << 1,
    KEY_START = 6 << 1,
    KEY_SELECT = 7 << 1,
    KEY_L = 8 << 1,
    KEY_R = 9 << 1
};

/*
 * Application entry point.
 */
int main(void)
{
    /*
    * System initializations.
    * - HAL initialization, this also initializes the configured device drivers
    *   and performs the board-specific initializations.
    * - Kernel initialization, the main() function becomes a thread and the
    *   RTOS is active.
    */
    halInit();
    System::init();

    // Initialize the serial module
    sdInit();
    // Setup the serial config with the desired baud rate we are using to transfer the data
    SerialConfig serial_config;
    serial_config.speed = BAUD_RATE;
    serial_config.cr1 = 0;
    serial_config.cr2 = 0;
    serial_config.cr3 = 0;

    // Start the serial driver 5 with our configuration
    sdStart(UART_DRIVER, &serial_config);

    while (true) {
        // Send key states
        // sdWrite(UART_DRIVER, buffer, BUFFER_SIZE);
    }
    return 0;
}
