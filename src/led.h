/*
 * Pico LED class
 *
 * (c) 2023 Erik Tkal
 *
 */

#pragma once

#include <vector>

#if PICO_DEFAULT_LED_PIN_INVERTED
auto constexpr LED_ON  = 0;
auto constexpr LED_OFF = 1;
#else
auto constexpr LED_ON  = 1;
auto constexpr LED_OFF = 0;
#endif

#if defined(PICO_DEFAULT_WS2812_POWER_PIN)
#define WS2812_POWER_PIN PICO_DEFAULT_WS2812_POWER_PIN
#else
#define WS2812_POWER_PIN 0
#endif

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

    virtual void init()                             = 0;
    virtual void on()                               = 0;
    virtual void off()                              = 0;
    virtual void setPixel(uint idx, uint32_t color) = 0;
    virtual void setIgnore(std::vector<uint32_t> vIgnore){};
    void blink_ms(uint duration = 50, uint32_t color = led_white);
};

class LED_pico : public LED
{
public:
    LED_pico(uint pin);
    virtual ~LED_pico();

    void init() override{};
    void on() override;
    void off() override;
    void setPixel(uint idx, uint32_t color) override;
    void setIgnore(std::vector<uint32_t> vIgnore) override;

protected:
    uint m_nPin;
    uint32_t m_nColor;
    std::vector<uint32_t> m_vIgnore;
};

class LED_neo : public LED
{
public:
    LED_neo(uint numLEDs, uint pin, uint powerPin = WS2812_POWER_PIN, bool bIsRGBW = false);
    virtual ~LED_neo();

    void init() override;
    void on() override;
    void off() override;
    void setPixel(uint idx, uint32_t color) override;

private:
    uint m_nPin;
    uint m_nPowerPin;
    uint m_nNumLEDs;
    bool m_bIsRGBW;
    std::vector<uint32_t> m_vPixels;
};

#if defined(RASPBERRYPI_PICO_W)
class LED_pico_w : public LED_pico
{
public:
    LED_pico_w(uint pin);
    virtual ~LED_pico_w();

    void on();
    void off();
};
#endif
