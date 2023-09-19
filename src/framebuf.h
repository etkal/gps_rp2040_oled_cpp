/*
 * Framebuf class
 *
 * (c) 2023 Erik Tkal
 *
 */

#pragma once

#include "pico/stdlib.h"

typedef enum ePixelFormat
{
    MVLSB,  // ssd1306
    RGB565, // ili9341
    MHLSB,
    MHMSB,
} ePixelFormat;

// Q2 Q1
// Q3 Q4
#define ELLIPSE_MASK_FILL (0x10)
#define ELLIPSE_MASK_ALL  (0x0f)
#define ELLIPSE_MASK_Q1   (0x01)
#define ELLIPSE_MASK_Q2   (0x02)
#define ELLIPSE_MASK_Q3   (0x04)
#define ELLIPSE_MASK_Q4   (0x08)

class Framebuf
{
public:
    Framebuf(uint16_t nWidth, uint16_t nHeight, ePixelFormat eFormat, bool bRevBytes = false, uint16_t nStride = 0);
    ~Framebuf();
    void setpixel(int x, int y, uint16_t color);
    uint16_t getpixel(int x, int y);
    void hline(int x, int y, int w, uint16_t color);
    void vline(int x, int y, int h, uint16_t color);
    void rect(int x, int y, int w, int h, uint16_t color, bool bFill = false);
    void fillrect(int x, int y, int w, int h, uint16_t color);
    void fill(uint16_t color);
    void line(int x1, int y1, int x2, int y2, uint16_t color);
    void ellipse(int cx, int cy, int xradius, int yradius, uint16_t color, bool bFill = false, uint8_t mask = ELLIPSE_MASK_ALL);
    void scroll(int xstep, int ystep);
    void text(const char* str, int x, int y, uint16_t color);
    void* buffer()
    {
        return m_pBuf;
    }
    uint16_t width()
    {
        return m_nWidth;
    }
    uint16_t height()
    {
        return m_nHeight;
    }

private:
    void setpixel_checked(int x, int y, uint16_t color, uint8_t mask);
    void fillrect_checked(int x, int y, int h, int w, uint16_t color);
    void ellipse_points(int cx, int cy, int x, int y, uint16_t color, uint8_t mask);
    uint16_t fixcolor(uint16_t color)
    {
        return m_bRevBytes ? __builtin_bswap16(color) : color;
    }

    void* m_pBuf;
    uint16_t m_nWidth;
    uint16_t m_nHeight;
    uint16_t m_nStride;
    ePixelFormat m_eFormat;
    bool m_bRevBytes;
};
