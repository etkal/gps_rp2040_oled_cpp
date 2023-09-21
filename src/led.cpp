/*
 * Pico LED class
 *
 * (c) 2023 Erik Tkal
 *
 */

#include "pico/stdlib.h"
#if defined(RASPBERRYPI_PICO_W)
#include "pico/cyw43_arch.h"
#endif
#include "led.h"
#include "ws2812.pio.h"

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

void LED::blink_ms(uint duration, uint32_t color)
{
    on();
    sleep_ms(duration);
    off();
}


LED_pico::LED_pico(uint pin)
    : m_nPin(pin),
      m_nColor(led_white)
{
    gpio_init(m_nPin);
    gpio_set_dir(m_nPin, GPIO_OUT);
    off();
}

LED_pico::~LED_pico()
{
    off();
    gpio_deinit(m_nPin);
}

void LED_pico::on()
{
    for (auto i : m_vIgnore)
    {
        if (i == m_nColor)
        {
            return;
        }
    }
    gpio_put(m_nPin, LED_ON);
}

void LED_pico::off()
{
    gpio_put(m_nPin, LED_OFF);
}

void LED_pico::setPixel(uint idx, uint32_t color)
{
    m_nColor = color;
}

void LED_pico::setIgnore(std::vector<uint32_t> vIgnore)
{
    m_vIgnore = vIgnore;
}


LED_neo::LED_neo(uint numLEDs, uint pin, uint powerPin, bool bIsRGBW)
    : m_nPin(pin),
      m_nPowerPin(powerPin),
      m_nNumLEDs(numLEDs),
      m_bIsRGBW(bIsRGBW)
{
    off();
}

LED_neo::~LED_neo()
{
    off();
    if (0 != m_nPowerPin)
    {
        gpio_put(m_nPowerPin, 0);
        gpio_deinit(m_nPowerPin);
    }
}

void LED_neo::init()
{
    PIO pio     = pio0;
    int sm      = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, m_nPin, 800000, m_bIsRGBW);

    if (0 != m_nPowerPin)
    {
        gpio_init(m_nPowerPin);
        gpio_set_dir(m_nPowerPin, GPIO_OUT);
        gpio_put(m_nPowerPin, 1);
    }

    m_vPixels.resize(m_nNumLEDs);
}

void LED_neo::on()
{
    for (size_t i = 0; i < m_nNumLEDs; ++i)
    {
        put_pixel(m_vPixels[i]);
    }
}

void LED_neo::off()
{
    for (size_t i = 0; i < m_nNumLEDs; ++i)
    {
        put_pixel(0);
    }
}

void LED_neo::setPixel(uint idx, uint32_t color)
{
    m_vPixels[idx] = color;
}

#if defined(RASPBERRYPI_PICO_W)
LED_pico_w::LED_pico_w(uint pin)
    : LED_pico(pin)
{
    off();
}

LED_pico_w::~LED_pico_w()
{
    off();
}

void LED_pico_w::on()
{
    for (auto i : m_vIgnore)
    {
        if (i == m_nColor)
        {
            return;
        }
    }
    cyw43_arch_gpio_put(m_nPin, 1);
}

void LED_pico_w::off()
{
    cyw43_arch_gpio_put(m_nPin, 0);
}
#endif
