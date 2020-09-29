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

// Whether to use Board v3 or v4
#define EXTENSION_BOARD_V3 // V4_5: EXTENSION_BOARD_V4_5

#define BUFFER_SIZE 1
//#define BAUD_RATE 9600
//#define BAUD_RATE 19200
//#define BAUD_RATE 57600
#define BAUD_RATE 115200

#define UART_DRIVER &SD5

#ifdef EXTENSION_BOARD_V3

#define GPIO_0_PORT GPIOB
#define GPIO_0_PAD GPIOB_GPIO_0

#define GPIO_1_PORT GPIOA
#define GPIO_1_PAD GPIOA_GPIO_1

#define GPIO_2_PORT GPIOA
#define GPIO_2_PAD GPIOA_GPIO_2

#define GPIO_3_PORT GPIOB
#define GPIO_3_PAD GPIOB_GPIO_3

#define GPIO_4_PORT GPIOB
#define GPIO_4_PAD GPIOB_GPIO_4

#define GPIO_5_PORT GPIOB
#define GPIO_5_PAD GPIOB_GPIO_5

#define GPIO_6_PORT GPIOB
#define GPIO_6_PAD GPIOB_GPIO_6

#define GPIO_7_PORT GPIOC
#define GPIO_7_PAD GPIOC_GPIO_7

#define GPIO_8_PORT GPIOC
#define GPIO_8_PAD GPIOC_GPIO_7

#define GPIO_9_PORT GPIOF
#define GPIO_9_PAD GPIOF_GPIO_9

#elif defined(EXTENSION_BOARD_V4_5)

#define GPIO_0_PORT GPIOB
#define GPIO_0_PAD GPIOB_GPIO_0

#define GPIO_1_PORT GPIOB
#define GPIO_1_PAD GPIOB_GPIO_1

#define GPIO_2_PORT GPIOB
#define GPIO_2_PAD GPIOB_GPIO_2

#define GPIO_3_PORT GPIOB
#define GPIO_3_PAD GPIOB_GPIO_3

#define GPIO_4_PORT GPIOC
#define GPIO_4_PAD GPIOC_GPIO_4

#define GPIO_5_PORT GPIOB
#define GPIO_5_PAD GPIOB_GPIO_5

#define GPIO_6_PORT GPIOB
#define GPIO_6_PAD GPIOB_STICK_UP

#define GPIO_7_PORT GPIOB
#define GPIO_7_PAD GPIOB_STICK_DOWN

#define GPIO_8_PORT GPIOB
#define GPIO_8_PAD GPIOB_STICK_LEFT

#define GPIO_9_PORT GPIOB
#define GPIO_9_PAD GPIOB_STICK_RIGHT

#else
#error "Unsupported board version!"
#endif

typedef struct {
    stm32_gpio_t *buttonPort;
    uint32_t pad;
} Key;

static const Key keys[] = {
    {GPIO_0_PORT, GPIO_0_PAD},
    {GPIO_1_PORT, GPIO_1_PAD},
    {GPIO_2_PORT, GPIO_2_PAD},
    {GPIO_3_PORT, GPIO_3_PAD},
    {GPIO_4_PORT, GPIO_4_PAD},
    {GPIO_5_PORT, GPIO_5_PAD},
    {GPIO_6_PORT, GPIO_6_PAD},
    {GPIO_7_PORT, GPIO_7_PAD},
    {GPIO_8_PORT, GPIO_8_PAD},
    {GPIO_9_PORT, GPIO_9_PAD}};

static uint8_t prevState[sizeof(keys) / sizeof(keys[0])];

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

    // Initialize the i/o port abstraction layer
    palInit();

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

    // Configure all required GPIO pins as input
    for (uint32_t idx = 0; idx < sizeof(keys) / sizeof(keys[0]); ++idx) {
        // The Port and Pad to use for this button
        prevState[idx] = PAL_HIGH;

        // Set the pad to input with pull down resistors configured
        palSetPadMode(keys[idx].buttonPort, keys[idx].pad, PAL_MODE_INPUT_PULLUP);
    }

    while (true) {
        // Give ChibiOs some time to handle possible events on buttons
        chThdSleepMilliseconds(10);

        for (uint32_t idx = 0; idx < sizeof(keys) / sizeof(keys[0]); ++idx) {

            if (prevState[idx] != palReadPad(keys[idx].buttonPort, keys[idx].pad)) {
                prevState[idx] = 1 - prevState[idx];
                uint8_t btn = (idx << 1) | prevState[idx];
                sdWrite(SERIAL_DRIVER, &btn, sizeof(btn));
            }
        }
    }

    return 0;
}
