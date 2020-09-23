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

/*
 * This file has been automatically generated using ChibiStudio board
 * generator plugin. Do not edit manually.
 */

#ifndef BOARD_H
#define BOARD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Setup for ESA board board.
 */

/*
 * Board identifier.
 */
#define BOARD_ESA_BOARD
#define BOARD_NAME                  "ESA board"

/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 * NOTE: HSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0U
#endif

#define STM32_LSEDRV                (3U << 3U)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                0U
#endif

#define STM32_HSE_BYPASS

/*
 * MCU type as defined in the ST header.
 */
#define STM32F030xC

/*
 * IO pins assignments.
 */
#define GPIOA_USART2_CTS            0U
#define GPIOA_USART2_RTS            1U
#define GPIOA_USART2_TX             2U
#define GPIOA_USART2_RX             3U
#define GPIOA_JACK_LEFT             4U
#define GPIOA_JACK_RIGHT            5U
#define GPIOA_LIGHT                 6U
#define GPIOA_LED1_GREEN            7U
#define GPIOA_M2_CTRL               8U
#define GPIOA_M3_CTRL               9U
#define GPIOA_M4_CTRL               10U
#define GPIOA_M1_CTRL               11U
#define GPIOA_GPIO_1                12U
#define GPIOA_SWDAT                 13U
#define GPIOA_SWCLK                 14U
#define GPIOA_GPIO_2                15U

#define GPIOB_LED0_RED              0U
#define GPIOB_BT_SYSTEM_OUT         1U
#define GPIOB_STICK_RIGHT           2U
#define GPIOB_USART5_TX             3U
#define GPIOB_USART5_RX             4U
#define GPIOB_GPIO_5                5U
#define GPIOB_GPIO_4                6U
#define GPIOB_GPIO_6                7U
#define GPIOB_I2C1_SCL              8U
#define GPIOB_I2C1_SDA              9U
#define GPIOB_STICK_CENTER          10U
#define GPIOB_STICK_DOWN            11U
#define GPIOB_STICK_LEFT            12U
#define GPIOB_STICK_UP              13U
#define GPIOB_GPIO_3                14U
#define GPIOB_GPIO_0                15U

#define GPIOC_PIN0                  0U
#define GPIOC_PIN1                  1U
#define GPIOC_PIN2                  2U
#define GPIOC_PIN3                  3U
#define GPIOC_PIN4                  4U
#define GPIOC_PIN5                  5U
#define GPIOC_PIN6                  6U
#define GPIOC_PIN7                  7U
#define GPIOC_PIN8                  8U
#define GPIOC_PIN9                  9U
#define GPIOC_PIN10                 10U
#define GPIOC_PIN11                 11U
#define GPIOC_PIN12                 12U
#define GPIOC_GPIO_7                13U
#define GPIOC_GPIO_8                14U
#define GPIOC_BT_SYSTEM_KEY         15U

#define GPIOD_PIN0                  0U
#define GPIOD_PIN1                  1U
#define GPIOD_PIN2                  2U
#define GPIOD_PIN3                  3U
#define GPIOD_PIN4                  4U
#define GPIOD_PIN5                  5U
#define GPIOD_PIN6                  6U
#define GPIOD_PIN7                  7U
#define GPIOD_PIN8                  8U
#define GPIOD_PIN9                  9U
#define GPIOD_PIN10                 10U
#define GPIOD_PIN11                 11U
#define GPIOD_PIN12                 12U
#define GPIOD_PIN13                 13U
#define GPIOD_PIN14                 14U
#define GPIOD_PIN15                 15U

#define GPIOF_GPIO_9                0U
#define GPIOF_GPIO_10               1U
#define GPIOF_PIN2                  2U
#define GPIOF_PIN3                  3U
#define GPIOF_PIN4                  4U
#define GPIOF_PIN5                  5U
#define GPIOF_PIN6                  6U
#define GPIOF_PIN7                  7U
#define GPIOF_PIN8                  8U
#define GPIOF_PIN9                  9U
#define GPIOF_PIN10                 10U
#define GPIOF_PIN11                 11U
#define GPIOF_PIN12                 12U
#define GPIOF_PIN13                 13U
#define GPIOF_PIN14                 14U
#define GPIOF_PIN15                 15U

/*
 * IO lines assignments.
 */
#define LINE_USART2_CTS             PAL_LINE(GPIOA, 0U)
#define LINE_USART2_RTS             PAL_LINE(GPIOA, 1U)
#define LINE_USART2_TX              PAL_LINE(GPIOA, 2U)
#define LINE_USART2_RX              PAL_LINE(GPIOA, 3U)
#define LINE_JACK_LEFT              PAL_LINE(GPIOA, 4U)
#define LINE_JACK_RIGHT             PAL_LINE(GPIOA, 5U)
#define LINE_LIGHT                  PAL_LINE(GPIOA, 6U)
#define LINE_LED1_GREEN             PAL_LINE(GPIOA, 7U)
#define LINE_M2_CTRL                PAL_LINE(GPIOA, 8U)
#define LINE_M3_CTRL                PAL_LINE(GPIOA, 9U)
#define LINE_M4_CTRL                PAL_LINE(GPIOA, 10U)
#define LINE_M1_CTRL                PAL_LINE(GPIOA, 11U)
#define LINE_GPIO_1                 PAL_LINE(GPIOA, 12U)
#define LINE_SWDAT                  PAL_LINE(GPIOA, 13U)
#define LINE_SWCLK                  PAL_LINE(GPIOA, 14U)
#define LINE_GPIO_2                 PAL_LINE(GPIOA, 15U)
#define LINE_LED0_RED               PAL_LINE(GPIOB, 0U)
#define LINE_BT_SYSTEM_OUT          PAL_LINE(GPIOB, 1U)
#define LINE_STICK_RIGHT            PAL_LINE(GPIOB, 2U)
#define LINE_USART5_TX              PAL_LINE(GPIOB, 3U)
#define LINE_USART5_RX              PAL_LINE(GPIOB, 4U)
#define LINE_GPIO_5                 PAL_LINE(GPIOB, 5U)
#define LINE_GPIO_4                 PAL_LINE(GPIOB, 6U)
#define LINE_GPIO_6                 PAL_LINE(GPIOB, 7U)
#define LINE_I2C1_SCL               PAL_LINE(GPIOB, 8U)
#define LINE_I2C1_SDA               PAL_LINE(GPIOB, 9U)
#define LINE_STICK_CENTER           PAL_LINE(GPIOB, 10U)
#define LINE_STICK_DOWN             PAL_LINE(GPIOB, 11U)
#define LINE_STICK_LEFT             PAL_LINE(GPIOB, 12U)
#define LINE_STICK_UP               PAL_LINE(GPIOB, 13U)
#define LINE_GPIO_3                 PAL_LINE(GPIOB, 14U)
#define LINE_GPIO_0                 PAL_LINE(GPIOB, 15U)
#define LINE_GPIO_7                 PAL_LINE(GPIOC, 13U)
#define LINE_GPIO_8                 PAL_LINE(GPIOC, 14U)
#define LINE_BT_SYSTEM_KEY          PAL_LINE(GPIOC, 15U)
#define LINE_GPIO_9                 PAL_LINE(GPIOF, 0U)
#define LINE_GPIO_10                PAL_LINE(GPIOF, 1U)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))

/*
 * GPIOA setup:
 *
 * PA0  - USART2_CTS                (alternate 1).
 * PA1  - USART2_RTS                (alternate 1).
 * PA2  - USART2_TX                 (alternate 1).
 * PA3  - USART2_RX                 (alternate 1).
 * PA4  - JACK_LEFT                 (analog).
 * PA5  - JACK_RIGHT                (analog).
 * PA6  - LIGHT                     (analog).
 * PA7  - LED1_GREEN                (output pushpull minimum).
 * PA8  - M2_CTRL                   (output pushpull minimum).
 * PA9  - M3_CTRL                   (output pushpull minimum).
 * PA10 - M4_CTRL                   (output pushpull minimum).
 * PA11 - M1_CTRL                   (output pushpull minimum).
 * PA12 - GPIO_1                    (output pushpull minimum).
 * PA13 - SWDAT                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - GPIO_2                    (output pushpull minimum).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_ALTERNATE(GPIOA_USART2_CTS) | \
                                     PIN_MODE_ALTERNATE(GPIOA_USART2_RTS) | \
                                     PIN_MODE_ALTERNATE(GPIOA_USART2_TX) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_USART2_RX) |  \
                                     PIN_MODE_ANALOG(GPIOA_JACK_LEFT) |     \
                                     PIN_MODE_ANALOG(GPIOA_JACK_RIGHT) |    \
                                     PIN_MODE_ANALOG(GPIOA_LIGHT) |         \
                                     PIN_MODE_OUTPUT(GPIOA_LED1_GREEN) |    \
                                     PIN_MODE_OUTPUT(GPIOA_M2_CTRL) |       \
                                     PIN_MODE_OUTPUT(GPIOA_M3_CTRL) |       \
                                     PIN_MODE_OUTPUT(GPIOA_M4_CTRL) |       \
                                     PIN_MODE_OUTPUT(GPIOA_M1_CTRL) |       \
                                     PIN_MODE_OUTPUT(GPIOA_GPIO_1) |        \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDAT) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_OUTPUT(GPIOA_GPIO_2))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_USART2_CTS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USART2_RTS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USART2_TX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USART2_RX) |  \
                                     PIN_OTYPE_OPENDRAIN(GPIOA_JACK_LEFT) | \
                                     PIN_OTYPE_OPENDRAIN(GPIOA_JACK_RIGHT) |\
                                     PIN_OTYPE_OPENDRAIN(GPIOA_LIGHT) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LED1_GREEN) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_M2_CTRL) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_M3_CTRL) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_M4_CTRL) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_M1_CTRL) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_GPIO_1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDAT) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_GPIO_2))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOA_USART2_CTS) | \
                                     PIN_OSPEED_VERYLOW(GPIOA_USART2_RTS) | \
                                     PIN_OSPEED_VERYLOW(GPIOA_USART2_TX) |  \
                                     PIN_OSPEED_VERYLOW(GPIOA_USART2_RX) |  \
                                     PIN_OSPEED_VERYLOW(GPIOA_JACK_LEFT) |  \
                                     PIN_OSPEED_VERYLOW(GPIOA_JACK_RIGHT) | \
                                     PIN_OSPEED_VERYLOW(GPIOA_LIGHT) |      \
                                     PIN_OSPEED_VERYLOW(GPIOA_LED1_GREEN) | \
                                     PIN_OSPEED_VERYLOW(GPIOA_M2_CTRL) |    \
                                     PIN_OSPEED_VERYLOW(GPIOA_M3_CTRL) |    \
                                     PIN_OSPEED_VERYLOW(GPIOA_M4_CTRL) |    \
                                     PIN_OSPEED_VERYLOW(GPIOA_M1_CTRL) |    \
                                     PIN_OSPEED_VERYLOW(GPIOA_GPIO_1) |     \
                                     PIN_OSPEED_HIGH(GPIOA_SWDAT) |         \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_VERYLOW(GPIOA_GPIO_2))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_FLOATING(GPIOA_USART2_CTS) | \
                                     PIN_PUPDR_FLOATING(GPIOA_USART2_RTS) | \
                                     PIN_PUPDR_FLOATING(GPIOA_USART2_TX) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_USART2_RX) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_JACK_LEFT) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_JACK_RIGHT) | \
                                     PIN_PUPDR_FLOATING(GPIOA_LIGHT) |      \
                                     PIN_PUPDR_PULLDOWN(GPIOA_LED1_GREEN) | \
                                     PIN_PUPDR_PULLDOWN(GPIOA_M2_CTRL) |    \
                                     PIN_PUPDR_PULLDOWN(GPIOA_M3_CTRL) |    \
                                     PIN_PUPDR_PULLDOWN(GPIOA_M4_CTRL) |    \
                                     PIN_PUPDR_PULLDOWN(GPIOA_M1_CTRL) |    \
                                     PIN_PUPDR_PULLDOWN(GPIOA_GPIO_1) |     \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDAT) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK) |      \
                                     PIN_PUPDR_PULLDOWN(GPIOA_GPIO_2))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_USART2_CTS) |       \
                                     PIN_ODR_HIGH(GPIOA_USART2_RTS) |       \
                                     PIN_ODR_HIGH(GPIOA_USART2_TX) |        \
                                     PIN_ODR_HIGH(GPIOA_USART2_RX) |        \
                                     PIN_ODR_HIGH(GPIOA_JACK_LEFT) |        \
                                     PIN_ODR_HIGH(GPIOA_JACK_RIGHT) |       \
                                     PIN_ODR_HIGH(GPIOA_LIGHT) |            \
                                     PIN_ODR_LOW(GPIOA_LED1_GREEN) |        \
                                     PIN_ODR_LOW(GPIOA_M2_CTRL) |           \
                                     PIN_ODR_LOW(GPIOA_M3_CTRL) |           \
                                     PIN_ODR_LOW(GPIOA_M4_CTRL) |           \
                                     PIN_ODR_LOW(GPIOA_M1_CTRL) |           \
                                     PIN_ODR_LOW(GPIOA_GPIO_1) |            \
                                     PIN_ODR_HIGH(GPIOA_SWDAT) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_LOW(GPIOA_GPIO_2))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_USART2_CTS, 1U) |    \
                                     PIN_AFIO_AF(GPIOA_USART2_RTS, 1U) |    \
                                     PIN_AFIO_AF(GPIOA_USART2_TX, 1U) |     \
                                     PIN_AFIO_AF(GPIOA_USART2_RX, 1U) |     \
                                     PIN_AFIO_AF(GPIOA_JACK_LEFT, 0U) |     \
                                     PIN_AFIO_AF(GPIOA_JACK_RIGHT, 0U) |    \
                                     PIN_AFIO_AF(GPIOA_LIGHT, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_LED1_GREEN, 0U))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_M2_CTRL, 0U) |       \
                                     PIN_AFIO_AF(GPIOA_M3_CTRL, 0U) |       \
                                     PIN_AFIO_AF(GPIOA_M4_CTRL, 0U) |       \
                                     PIN_AFIO_AF(GPIOA_M1_CTRL, 0U) |       \
                                     PIN_AFIO_AF(GPIOA_GPIO_1, 0U) |        \
                                     PIN_AFIO_AF(GPIOA_SWDAT, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_GPIO_2, 0U))

/*
 * GPIOB setup:
 *
 * PB0  - LED0_RED                  (output pushpull minimum).
 * PB1  - BT_SYSTEM_OUT             (input floating).
 * PB2  - STICK_RIGHT               (input pullup).
 * PB3  - USART5_TX                 (alternate 4).
 * PB4  - USART5_RX                 (alternate 4).
 * PB5  - GPIO_5                    (output pushpull minimum).
 * PB6  - GPIO_4                    (output pushpull minimum).
 * PB7  - GPIO_6                    (output pushpull minimum).
 * PB8  - I2C1_SCL                  (alternate 1).
 * PB9  - I2C1_SDA                  (alternate 1).
 * PB10 - STICK_CENTER              (input pullup).
 * PB11 - STICK_DOWN                (input pullup).
 * PB12 - STICK_LEFT                (input pullup).
 * PB13 - STICK_UP                  (input pullup).
 * PB14 - GPIO_3                    (output pushpull minimum).
 * PB15 - GPIO_0                    (output pushpull minimum).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_OUTPUT(GPIOB_LED0_RED) |      \
                                     PIN_MODE_INPUT(GPIOB_BT_SYSTEM_OUT) |  \
                                     PIN_MODE_INPUT(GPIOB_STICK_RIGHT) |    \
                                     PIN_MODE_ALTERNATE(GPIOB_USART5_TX) |  \
                                     PIN_MODE_ALTERNATE(GPIOB_USART5_RX) |  \
                                     PIN_MODE_OUTPUT(GPIOB_GPIO_5) |        \
                                     PIN_MODE_OUTPUT(GPIOB_GPIO_4) |        \
                                     PIN_MODE_OUTPUT(GPIOB_GPIO_6) |        \
                                     PIN_MODE_ALTERNATE(GPIOB_I2C1_SCL) |   \
                                     PIN_MODE_ALTERNATE(GPIOB_I2C1_SDA) |   \
                                     PIN_MODE_INPUT(GPIOB_STICK_CENTER) |   \
                                     PIN_MODE_INPUT(GPIOB_STICK_DOWN) |     \
                                     PIN_MODE_INPUT(GPIOB_STICK_LEFT) |     \
                                     PIN_MODE_INPUT(GPIOB_STICK_UP) |       \
                                     PIN_MODE_OUTPUT(GPIOB_GPIO_3) |        \
                                     PIN_MODE_OUTPUT(GPIOB_GPIO_0))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_LED0_RED) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_BT_SYSTEM_OUT) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_STICK_RIGHT) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_USART5_TX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_USART5_RX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GPIO_5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GPIO_4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GPIO_6) |     \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_I2C1_SCL) |  \
                                     PIN_OTYPE_OPENDRAIN(GPIOB_I2C1_SDA) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_STICK_CENTER) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_STICK_DOWN) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_STICK_LEFT) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_STICK_UP) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GPIO_3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_GPIO_0))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOB_LED0_RED) |   \
                                     PIN_OSPEED_HIGH(GPIOB_BT_SYSTEM_OUT) | \
                                     PIN_OSPEED_VERYLOW(GPIOB_STICK_RIGHT) |\
                                     PIN_OSPEED_VERYLOW(GPIOB_USART5_TX) |  \
                                     PIN_OSPEED_VERYLOW(GPIOB_USART5_RX) |  \
                                     PIN_OSPEED_VERYLOW(GPIOB_GPIO_5) |     \
                                     PIN_OSPEED_VERYLOW(GPIOB_GPIO_4) |     \
                                     PIN_OSPEED_VERYLOW(GPIOB_GPIO_6) |     \
                                     PIN_OSPEED_HIGH(GPIOB_I2C1_SCL) |      \
                                     PIN_OSPEED_HIGH(GPIOB_I2C1_SDA) |      \
                                     PIN_OSPEED_VERYLOW(GPIOB_STICK_CENTER) |\
                                     PIN_OSPEED_VERYLOW(GPIOB_STICK_DOWN) | \
                                     PIN_OSPEED_VERYLOW(GPIOB_STICK_LEFT) | \
                                     PIN_OSPEED_VERYLOW(GPIOB_STICK_UP) |   \
                                     PIN_OSPEED_VERYLOW(GPIOB_GPIO_3) |     \
                                     PIN_OSPEED_VERYLOW(GPIOB_GPIO_0))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_PULLDOWN(GPIOB_LED0_RED) |   \
                                     PIN_PUPDR_FLOATING(GPIOB_BT_SYSTEM_OUT) |\
                                     PIN_PUPDR_PULLUP(GPIOB_STICK_RIGHT) |  \
                                     PIN_PUPDR_FLOATING(GPIOB_USART5_TX) |  \
                                     PIN_PUPDR_FLOATING(GPIOB_USART5_RX) |  \
                                     PIN_PUPDR_PULLDOWN(GPIOB_GPIO_5) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOB_GPIO_4) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOB_GPIO_6) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_I2C1_SCL) |   \
                                     PIN_PUPDR_FLOATING(GPIOB_I2C1_SDA) |   \
                                     PIN_PUPDR_PULLUP(GPIOB_STICK_CENTER) | \
                                     PIN_PUPDR_PULLUP(GPIOB_STICK_DOWN) |   \
                                     PIN_PUPDR_PULLUP(GPIOB_STICK_LEFT) |   \
                                     PIN_PUPDR_PULLUP(GPIOB_STICK_UP) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOB_GPIO_3) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOB_GPIO_0))
#define VAL_GPIOB_ODR               (PIN_ODR_LOW(GPIOB_LED0_RED) |          \
                                     PIN_ODR_HIGH(GPIOB_BT_SYSTEM_OUT) |    \
                                     PIN_ODR_HIGH(GPIOB_STICK_RIGHT) |      \
                                     PIN_ODR_HIGH(GPIOB_USART5_TX) |        \
                                     PIN_ODR_HIGH(GPIOB_USART5_RX) |        \
                                     PIN_ODR_LOW(GPIOB_GPIO_5) |            \
                                     PIN_ODR_LOW(GPIOB_GPIO_4) |            \
                                     PIN_ODR_LOW(GPIOB_GPIO_6) |            \
                                     PIN_ODR_HIGH(GPIOB_I2C1_SCL) |         \
                                     PIN_ODR_HIGH(GPIOB_I2C1_SDA) |         \
                                     PIN_ODR_HIGH(GPIOB_STICK_CENTER) |     \
                                     PIN_ODR_HIGH(GPIOB_STICK_DOWN) |       \
                                     PIN_ODR_HIGH(GPIOB_STICK_LEFT) |       \
                                     PIN_ODR_HIGH(GPIOB_STICK_UP) |         \
                                     PIN_ODR_LOW(GPIOB_GPIO_3) |            \
                                     PIN_ODR_LOW(GPIOB_GPIO_0))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_LED0_RED, 0U) |      \
                                     PIN_AFIO_AF(GPIOB_BT_SYSTEM_OUT, 0U) | \
                                     PIN_AFIO_AF(GPIOB_STICK_RIGHT, 0U) |   \
                                     PIN_AFIO_AF(GPIOB_USART5_TX, 4U) |     \
                                     PIN_AFIO_AF(GPIOB_USART5_RX, 4U) |     \
                                     PIN_AFIO_AF(GPIOB_GPIO_5, 0U) |        \
                                     PIN_AFIO_AF(GPIOB_GPIO_4, 0U) |        \
                                     PIN_AFIO_AF(GPIOB_GPIO_6, 0U))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_I2C1_SCL, 1U) |      \
                                     PIN_AFIO_AF(GPIOB_I2C1_SDA, 1U) |      \
                                     PIN_AFIO_AF(GPIOB_STICK_CENTER, 0U) |  \
                                     PIN_AFIO_AF(GPIOB_STICK_DOWN, 0U) |    \
                                     PIN_AFIO_AF(GPIOB_STICK_LEFT, 0U) |    \
                                     PIN_AFIO_AF(GPIOB_STICK_UP, 0U) |      \
                                     PIN_AFIO_AF(GPIOB_GPIO_3, 0U) |        \
                                     PIN_AFIO_AF(GPIOB_GPIO_0, 0U))

/*
 * GPIOC setup:
 *
 * PC0  - PIN0                      (input pullup).
 * PC1  - PIN1                      (input pullup).
 * PC2  - PIN2                      (input pullup).
 * PC3  - PIN3                      (input pullup).
 * PC4  - PIN4                      (input pullup).
 * PC5  - PIN5                      (input pullup).
 * PC6  - PIN6                      (input pullup).
 * PC7  - PIN7                      (input pullup).
 * PC8  - PIN8                      (output pushpull maximum).
 * PC9  - PIN9                      (output pushpull maximum).
 * PC10 - PIN10                     (input pullup).
 * PC11 - PIN11                     (input pullup).
 * PC12 - PIN12                     (input pullup).
 * PC13 - GPIO_7                    (output pushpull minimum).
 * PC14 - GPIO_8                    (output pushpull minimum).
 * PC15 - BT_SYSTEM_KEY             (output pushpull maximum).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_INPUT(GPIOC_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN7) |           \
                                     PIN_MODE_OUTPUT(GPIOC_PIN8) |          \
                                     PIN_MODE_OUTPUT(GPIOC_PIN9) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN12) |          \
                                     PIN_MODE_OUTPUT(GPIOC_GPIO_7) |        \
                                     PIN_MODE_OUTPUT(GPIOC_GPIO_8) |        \
                                     PIN_MODE_OUTPUT(GPIOC_BT_SYSTEM_KEY))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_GPIO_7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOC_GPIO_8) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOC_BT_SYSTEM_KEY))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOC_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN3) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN4) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN7) |       \
                                     PIN_OSPEED_HIGH(GPIOC_PIN8) |          \
                                     PIN_OSPEED_HIGH(GPIOC_PIN9) |          \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN10) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_GPIO_7) |     \
                                     PIN_OSPEED_VERYLOW(GPIOC_GPIO_8) |     \
                                     PIN_OSPEED_HIGH(GPIOC_BT_SYSTEM_KEY))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_PULLUP(GPIOC_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN7) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_PIN8) |       \
                                     PIN_PUPDR_FLOATING(GPIOC_PIN9) |       \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN12) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOC_GPIO_7) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOC_GPIO_8) |     \
                                     PIN_PUPDR_FLOATING(GPIOC_BT_SYSTEM_KEY))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN7) |             \
                                     PIN_ODR_LOW(GPIOC_PIN8) |              \
                                     PIN_ODR_LOW(GPIOC_PIN9) |              \
                                     PIN_ODR_HIGH(GPIOC_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN12) |            \
                                     PIN_ODR_LOW(GPIOC_GPIO_7) |            \
                                     PIN_ODR_LOW(GPIOC_GPIO_8) |            \
                                     PIN_ODR_LOW(GPIOC_BT_SYSTEM_KEY))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN7, 0U))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_GPIO_7, 0U) |        \
                                     PIN_AFIO_AF(GPIOC_GPIO_8, 0U) |        \
                                     PIN_AFIO_AF(GPIOC_BT_SYSTEM_KEY, 0U))

/*
 * GPIOD setup:
 *
 * PD0  - PIN0                      (input pullup).
 * PD1  - PIN1                      (input pullup).
 * PD2  - PIN2                      (input pullup).
 * PD3  - PIN3                      (input pullup).
 * PD4  - PIN4                      (input pullup).
 * PD5  - PIN5                      (input pullup).
 * PD6  - PIN6                      (input pullup).
 * PD7  - PIN7                      (input pullup).
 * PD8  - PIN8                      (input pullup).
 * PD9  - PIN9                      (input pullup).
 * PD10 - PIN10                     (input pullup).
 * PD11 - PIN11                     (input pullup).
 * PD12 - PIN12                     (input pullup).
 * PD13 - PIN13                     (input pullup).
 * PD14 - PIN14                     (input pullup).
 * PD15 - PIN15                     (input pullup).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_INPUT(GPIOD_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN15))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOD_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN15))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOD_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN3) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN4) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN8) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN9) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN10) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN15))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_PULLUP(GPIOD_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN15))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(GPIOD_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN15))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN7, 0U))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(GPIOD_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN15, 0U))

/*
 * GPIOF setup:
 *
 * PF0  - GPIO_9                    (output pushpull minimum).
 * PF1  - GPIO_10                   (output pushpull minimum).
 * PF2  - PIN2                      (input pullup).
 * PF3  - PIN3                      (input pullup).
 * PF4  - PIN4                      (input pullup).
 * PF5  - PIN5                      (input pullup).
 * PF6  - PIN6                      (input pullup).
 * PF7  - PIN7                      (input pullup).
 * PF8  - PIN8                      (input pullup).
 * PF9  - PIN9                      (input pullup).
 * PF10 - PIN10                     (input pullup).
 * PF11 - PIN11                     (input pullup).
 * PF12 - PIN12                     (input pullup).
 * PF13 - PIN13                     (input pullup).
 * PF14 - PIN14                     (input pullup).
 * PF15 - PIN15                     (input pullup).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_OUTPUT(GPIOF_GPIO_9) |        \
                                     PIN_MODE_OUTPUT(GPIOF_GPIO_10) |       \
                                     PIN_MODE_INPUT(GPIOF_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOF_PIN15))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOF_GPIO_9) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_GPIO_10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN15))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOF_GPIO_9) |     \
                                     PIN_OSPEED_VERYLOW(GPIOF_GPIO_10) |    \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN3) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN4) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN8) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN9) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN10) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN15))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_PULLDOWN(GPIOF_GPIO_9) |     \
                                     PIN_PUPDR_PULLDOWN(GPIOF_GPIO_10) |    \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN15))
#define VAL_GPIOF_ODR               (PIN_ODR_LOW(GPIOF_GPIO_9) |            \
                                     PIN_ODR_LOW(GPIOF_GPIO_10) |           \
                                     PIN_ODR_HIGH(GPIOF_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOF_PIN15))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(GPIOF_GPIO_9, 0U) |        \
                                     PIN_AFIO_AF(GPIOF_GPIO_10, 0U) |       \
                                     PIN_AFIO_AF(GPIOF_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN7, 0U))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(GPIOF_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_PIN15, 0U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
