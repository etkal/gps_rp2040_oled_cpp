/*
 * Pico LED class
 *
 * (c) 2023 Erik Tkal
 *
 */

#pragma once

#include <memory>
#include <vector>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>

auto constexpr max_lum = 100;

static inline constexpr uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r * max_lum / 256) << 8) | ((uint32_t)(g * max_lum / 256) << 16) | (uint32_t)(b * max_lum / 256);
}

auto constexpr led_white = urgb_u32(0x80, 0x80, 0x80);
auto constexpr led_on    = urgb_u32(0x80, 0x80, 0x80);
auto constexpr led_black = urgb_u32(0, 0, 0);
auto constexpr led_off   = urgb_u32(0, 0, 0);

auto constexpr led_red     = urgb_u32(0x80, 0, 0);
auto constexpr led_green   = urgb_u32(0, 0x80, 0);
auto constexpr led_blue    = urgb_u32(0, 0, 0x80);
auto constexpr led_cyan    = urgb_u32(0, 0x80, 0x80);
auto constexpr led_magenta = urgb_u32(0x80, 0, 0x80);
auto constexpr led_yellow  = urgb_u32(0x80, 0x80, 0);


class LED
{
public:
    LED(){};
    virtual ~LED(){};

    virtual void init() = 0;
    virtual void on()   = 0;
    virtual void off()  = 0;
    virtual void setPixel(uint idx, uint32_t color){};
    void blink_ms(uint duration = 50, uint32_t color = led_white);
};

class LED_pico : public LED
{
public:
    LED_pico(uint pin = PICO_DEFAULT_LED_PIN);
    virtual ~LED_pico();

    void init(){};
    void on();
    void off();

private:
    uint m_nPin;
};

class LED_neo : public LED
{
public:
    LED_neo(uint numLEDs, uint pin = PICO_DEFAULT_WS2812_PIN, uint powerPin = PICO_DEFAULT_WS2812_POWER_PIN, bool bIsRGBW = false);
    virtual ~LED_neo();

    void init();
    void on();
    void off();
    void setPixel(uint idx, uint32_t color);

private:
    uint m_nPin;
    uint m_nPowerPin;
    uint m_nNumLEDs;
    bool m_bIsRGBW;
    std::vector<uint32_t> m_vPixels;
};
