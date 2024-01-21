
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

#include <iostream>
#include <pico/stdlib.h>
#include "hardware/adc.h"

#if defined(RASPBERRYPI_PICO_W)
#include "pico/cyw43_arch.h"
#endif

#include "gps_oled.h"

#define UART_DEVICE    uart_default             // Default is uart0
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

#if !defined(GPSD_GMT_OFFSET)
#define GPSD_GMT_OFFSET 0.0
#endif

int main()
{
    stdio_init_all();
    adc_init();

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

#if defined(RASPBERRYPI_PICO_W)
    cyw43_arch_init();
#endif

    // Create the LED object
    LED::Shared spLED;
#if defined(USE_WS2812_PIN)
    spLED = std::make_shared<LED_neo>(1, USE_WS2812_PIN);
    spLED->Initialize();
    spLED->SetPixel(0, led_green);
#elif defined(PICO_DEFAULT_WS2812_PIN) && !defined(USE_LED_PIN)
    spLED = std::make_shared<LED_neo>(1, PICO_DEFAULT_WS2812_PIN);
    spLED->Initialize();
    spLED->SetPixel(0, led_green);
#elif defined(USE_LED_PIN)
    spLED = std::make_shared<LED_pico>(USE_LED_PIN);
    spLED->SetIgnore({led_red});
#elif defined(PICO_DEFAULT_LED_PIN)
    spLED = std::make_shared<LED_pico>(PICO_DEFAULT_LED_PIN);
    spLED->SetIgnore({led_red});
#elif defined(RASPBERRYPI_PICO_W)
    spLED = std::make_shared<LED_pico_w>(CYW43_WL_GPIO_LED_PIN);
    spLED->SetIgnore({led_red});
#endif

    // Create the GPS object
    GPS::Shared spGPS = std::make_shared<GPS>(UART_DEVICE);

    // Create the display
    SSD1306::Shared spDisplay = std::make_shared<SSD1306_I2C>(128, 64, I2C_DEVICE);

    // Create the GPS_TFT display object
    GPS_OLED::Shared spDevice = std::make_shared<GPS_OLED>(spDisplay, spGPS, spLED, GPSD_GMT_OFFSET);

    spDevice->Initialize();
    // Run the show
    spDevice->Run();

#if defined(RASPBERRYPI_PICO_W)
    cyw43_arch_deinit();
#endif

    return 0;
}
