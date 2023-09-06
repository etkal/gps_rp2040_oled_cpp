
/*
 * Copyright (c) 2023 Erik Tkal
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <memory>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>

#include "gps_oled.h"

#define UART_DEVICE    uart_default
#define PIN_UART_TX    PICO_DEFAULT_UART_TX_PIN // Default is 0
#define PIN_UART_RX    PICO_DEFAULT_UART_RX_PIN // Default is 1
#define UART_BAUD_RATE 9600
#define DATA_BITS      8
#define STOP_BITS      1
#define PARITY         UART_PARITY_NONE

#define I2C_DEVICE     i2c_default
#define PIN_SDA        PICO_DEFAULT_I2C_SDA_PIN
#define PIN_SCL        PICO_DEFAULT_I2C_SCL_PIN

// #define USE_WS2812_PIN 12 // Override
// #define USE_LED_PIN 16    // Override

int main()
{
    stdio_init_all();

    // Set up UART for GPS device
    uart_init(UART_DEVICE, UART_BAUD_RATE);
    gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_DEVICE, false, false);
    uart_set_format(UART_DEVICE, DATA_BITS, STOP_BITS, PARITY);

    // Set up the OLED display
    i2c_init(I2C_DEVICE, 400 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

#if defined(SEEED_XIAO_RP2040)
    // Clear LED(s) on XIAO (default on)
    LED_pico ledBlue(25);  // blue
    LED_pico ledGreen(16); // green
    LED_pico ledRed(17);   // red
#endif

#if defined(USE_WS2812_PIN)
    LED* pLED = new LED_neo(1, USE_WS2812_PIN);
    pLED->init();
    pLED->setPixel(0, led_green);
#elif defined(PICO_DEFAULT_WS2812_PIN) && !defined(USE_LED_PIN)
    LED* pLED = new LED_neo(1, PICO_DEFAULT_WS2812_PIN);
    pLED->init();
    pLED->setPixel(0, led_green);
#elif defined(USE_LED_PIN)
    LED* pLED = new LED_pico(USE_LED_PIN);
    pLED->setIgnore({led_red});
#elif defined(PICO_DEFAULT_LED_PIN)
    LED* pLED = new LED_pico(PICO_DEFAULT_LED_PIN);
    pLED->setIgnore({led_red});
#else
    LED* pLED = nullptr;
#endif
    GPS* pGPS             = new GPS(UART_DEVICE, -5.0);
    SSD1306_I2C* pDisplay = new SSD1306_I2C(128, 64, MVLSB, I2C_DEVICE);
    GPS_OLED* pDevice     = new GPS_OLED(pDisplay, pGPS, pLED);

    pDevice->init();
    pDevice->run();

    return 0;
}
