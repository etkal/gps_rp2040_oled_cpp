/*
 * Framebuffer implementation, rewritten from MicroPython modframebuf.c
 *
 * This version (c) 2023 Erik Tkal
 *
 * Original copyright:
 *
 * This file is part of the MicroPython project, http://micropython.org/
 * The MIT License (MIT)
 * Copyright (c) 2016 Damien P. George
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

#include <algorithm>
#include "framebuf.h"
#include "font_petme128_8x8.h"

using std::max;
using std::min;
using std::size_t;

Framebuf::Framebuf(uint16_t nWidth, uint16_t nHeight, ePixelFormat eFormat, bool bRevBytes, uint16_t nStride)
{
    m_nWidth    = nWidth;
    m_nHeight   = nHeight;
    m_eFormat   = eFormat;
    m_bRevBytes = bRevBytes;
    if (0 == nStride)
    {
        m_nStride = m_nWidth;
    }
    switch (m_eFormat)
    {
    case MVLSB:
    case MHLSB:
    case MHMSB:
        m_pBuf    = new uint8_t[m_nWidth * m_nHeight / 8];
        m_nStride = (m_nStride + 7) & ~7;
        break;
    case RGB565:
        m_pBuf = new uint16_t[m_nWidth * m_nHeight];
        break;
    default:
        m_pBuf = NULL;
        break;
    }
}

Framebuf::~Framebuf()
{
    switch (m_eFormat)
    {
    case MVLSB:
    case MHLSB:
    case MHMSB:
        delete[] (uint8_t*)m_pBuf;
        break;
    case RGB565:
        delete[] (uint16_t*)m_pBuf;
        break;
    default:
        break;
    }
}

void Framebuf::setpixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight)
    {
        return;
    }
    switch (m_eFormat)
    {
    case MVLSB:
    {
        size_t index              = (y >> 3) * m_nStride + x;
        uint8_t offset            = y & 0x07;
        ((uint8_t*)m_pBuf)[index] = (((uint8_t*)m_pBuf)[index] & ~(0x01 << offset)) | ((color != 0) << offset);
    }
    break;
    case RGB565:
    {
        ((uint16_t*)m_pBuf)[x + y * m_nStride] = fixcolor(color);
    }
    break;
    default:
        return;
        break;
    }
}

void Framebuf::setpixel_checked(int x, int y, uint16_t color, uint8_t mask)
{
    if (mask && 0 <= x && x < m_nWidth && 0 <= y && y < m_nHeight)
    {
        setpixel(x, y, color);
    }
}

uint16_t Framebuf::getpixel(int x, int y)
{
    if (0 > x || x >= m_nWidth || 0 > y || y >= m_nHeight)
    {
        return 0;
    }
    switch (m_eFormat)
    {
    case MVLSB:
        return (((uint8_t*)m_pBuf)[(y >> 3) * m_nStride + x] >> (y & 0x07)) & 0x01;
        break;
    case RGB565:
        return ((uint16_t*)m_pBuf)[x + y * m_nStride];
        break;
    default:
        return 0;
        break;
    }
}

void Framebuf::fillrect(int x, int y, int w, int h, uint16_t color)
{
    if (h < 1 || w < 1 || x + w <= 0 || y + h <= 0 || y >= m_nHeight || x >= m_nWidth)
    {
        // No operation needed.
        return;
    }

    // clip to the framebuffer
    int xend = min((int)m_nWidth, x + w);
    int yend = min((int)m_nHeight, y + h);
    x        = max(x, 0);
    y        = max(y, 0);

    fillrect_checked(x, y, xend - x, yend - y, color);
}

void Framebuf::fillrect_checked(int x, int y, int w, int h, uint16_t color)
{
    switch (m_eFormat)
    {
    case MVLSB:
        while (h--)
        {
            uint8_t* b     = &((uint8_t*)m_pBuf)[(y >> 3) * m_nStride + x];
            uint8_t offset = y & 0x07;
            for (unsigned int ww = w; ww; --ww)
            {
                *b = (*b & ~(0x01 << offset)) | ((color != 0) << offset);
                ++b;
            }
            ++y;
        }
        break;
    case RGB565:
    {
        uint16_t* b = &((uint16_t*)m_pBuf)[x + y * m_nStride];
        while (h--)
        {
            for (unsigned int ww = w; ww; --ww)
            {
                *b++ = fixcolor(color);
            }
            b += m_nStride - w;
        }
    }
    break;
    default:
        break;
    }
}


void Framebuf::fill(uint16_t color)
{
    fillrect(0, 0, m_nWidth, m_nHeight, color);
}


void Framebuf::hline(int x, int y, int w, uint16_t color)
{
    fillrect(x, y, w, 1, color);
}

void Framebuf::vline(int x, int y, int h, uint16_t color)
{
    fillrect(x, y, 1, h, color);
}


void Framebuf::rect(int x, int y, int w, int h, uint16_t color, bool bFill)
{
    if (bFill)
    {
        fillrect(x, y, w, h, color);
    }
    else
    {
        fillrect(x, y, w, 1, color);
        fillrect(x, y + h - 1, w, 1, color);
        fillrect(x, y, 1, h, color);
        fillrect(x + w - 1, y, 1, h, color);
    }
}

void Framebuf::line(int x1, int y1, int x2, int y2, uint16_t color)
{
    int dx = x2 - x1;
    int sx;
    if (dx > 0)
    {
        sx = 1;
    }
    else
    {
        dx = -dx;
        sx = -1;
    }

    int dy = y2 - y1;
    int sy;
    if (dy > 0)
    {
        sy = 1;
    }
    else
    {
        dy = -dy;
        sy = -1;
    }

    bool steep;
    if (dy > dx)
    {
        int temp;
        temp  = x1;
        x1    = y1;
        y1    = temp;
        temp  = dx;
        dx    = dy;
        dy    = temp;
        temp  = sx;
        sx    = sy;
        sy    = temp;
        steep = true;
    }
    else
    {
        steep = false;
    }

    int e = 2 * dy - dx;
    for (int i = 0; i < dx; ++i)
    {
        if (steep)
        {
            if (0 <= y1 && y1 < m_nWidth && 0 <= x1 && x1 < m_nHeight)
            {
                setpixel(y1, x1, color);
            }
        }
        else
        {
            if (0 <= x1 && x1 < m_nWidth && 0 <= y1 && y1 < m_nHeight)
            {
                setpixel(x1, y1, color);
            }
        }
        while (e >= 0)
        {
            y1 += sy;
            e -= 2 * dx;
        }
        x1 += sx;
        e += 2 * dy;
    }

    setpixel(x2, y2, color);
}

void Framebuf::ellipse(int cx, int cy, int xradius, int yradius, uint16_t color, bool bFill, uint8_t mask)
{
    if (bFill)
    {
        mask |= ELLIPSE_MASK_FILL;
    }
    int two_asquare   = 2 * xradius * xradius;
    int two_bsquare   = 2 * yradius * yradius;
    int x             = xradius;
    int y             = 0;
    int xchange       = yradius * yradius * (1 - 2 * xradius);
    int ychange       = xradius * xradius;
    int ellipse_error = 0;
    int stoppingx     = two_bsquare * xradius;
    int stoppingy     = 0;
    while (stoppingx >= stoppingy)
    { // 1st set of points,  y' > -1
        ellipse_points(cx, cy, x, y, color, mask);
        y += 1;
        stoppingy += two_asquare;
        ellipse_error += ychange;
        ychange += two_asquare;
        if ((2 * ellipse_error + xchange) > 0)
        {
            x -= 1;
            stoppingx -= two_bsquare;
            ellipse_error += xchange;
            xchange += two_bsquare;
        }
    }
    // 1st point set is done start the 2nd set of points
    x             = 0;
    y             = yradius;
    xchange       = yradius * yradius;
    ychange       = xradius * xradius * (1 - 2 * yradius);
    ellipse_error = 0;
    stoppingx     = 0;
    stoppingy     = two_asquare * yradius;
    while (stoppingx <= stoppingy)
    { // 2nd set of points, y' < -1
        ellipse_points(cx, cy, x, y, color, mask);
        x += 1;
        stoppingx += two_bsquare;
        ellipse_error += xchange;
        xchange += two_bsquare;
        if ((2 * ellipse_error + ychange) > 0)
        {
            y -= 1;
            stoppingy -= two_asquare;
            ellipse_error += ychange;
            ychange += two_asquare;
        }
    }
    return;
}

void Framebuf::ellipse_points(int cx, int cy, int x, int y, uint16_t color, uint8_t mask)
{
    if (mask & ELLIPSE_MASK_FILL)
    {
        if (mask & ELLIPSE_MASK_Q1)
        {
            fillrect(cx, cy - y, x + 1, 1, color);
        }
        if (mask & ELLIPSE_MASK_Q2)
        {
            fillrect(cx - x, cy - y, x + 1, 1, color);
        }
        if (mask & ELLIPSE_MASK_Q3)
        {
            fillrect(cx - x, cy + y, x + 1, 1, color);
        }
        if (mask & ELLIPSE_MASK_Q4)
        {
            fillrect(cx, cy + y, x + 1, 1, color);
        }
    }
    else
    {
        setpixel_checked(cx + x, cy - y, color, mask & ELLIPSE_MASK_Q1);
        setpixel_checked(cx - x, cy - y, color, mask & ELLIPSE_MASK_Q2);
        setpixel_checked(cx - x, cy + y, color, mask & ELLIPSE_MASK_Q3);
        setpixel_checked(cx + x, cy + y, color, mask & ELLIPSE_MASK_Q4);
    }
}

void Framebuf::scroll(int xstep, int ystep)
{
    int sx, y, xend, yend, dx, dy;
    if (xstep < 0)
    {
        sx   = 0;
        xend = m_nWidth + xstep;
        if (xend <= 0)
        {
            return;
        }
        dx = 1;
    }
    else
    {
        sx   = m_nWidth - 1;
        xend = xstep - 1;
        if (xend >= sx)
        {
            return;
        }
        dx = -1;
    }
    if (ystep < 0)
    {
        y    = 0;
        yend = m_nHeight + ystep;
        if (yend <= 0)
        {
            return;
        }
        dy = 1;
    }
    else
    {
        y    = m_nHeight - 1;
        yend = ystep - 1;
        if (yend >= y)
        {
            return;
        }
        dy = -1;
    }
    for (; y != yend; y += dy)
    {
        for (int x = sx; x != xend; x += dx)
        {
            setpixel(x, y, getpixel(x - xstep, y - ystep));
        }
    }
}

void Framebuf::text(const char* str, int x, int y, uint16_t color)
{
    // loop over chars
    for (; *str; ++str)
    {
        // get char and make sure its in range of font
        int chr = *(uint8_t*)str;
        if (chr < 32 || chr > 127)
        {
            chr = 127;
        }
        // get char data
        const uint8_t* chr_data = &font_petme128_8x8[(chr - 32) * 8];
        // loop over char data
        for (int j = 0; j < 8; j++, x++)
        {
            if (0 <= x && x < m_nWidth)
            {                                  // clip x
                uint vline_data = chr_data[j]; // each byte is a column of 8 pixels, LSB at top
                for (int y1 = y; vline_data; vline_data >>= 1, y1++)
                { // scan over vertical column
                    if (vline_data & 1)
                    { // only draw if pixel set
                        if (0 <= y1 && y1 < m_nHeight)
                        { // clip y
                            setpixel(x, y1, color);
                        }
                    }
                }
            }
        }
    }
}
