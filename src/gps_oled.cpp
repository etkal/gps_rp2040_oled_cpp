/*
 * GPS using OLED display
 *
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

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "ssd1306.h"
#include "framebuf.h"
#include "gps.h"
#include "led.h"
#include "gps_oled.h"
#include <pico/double.h>
#include <math.h>

#define SAT_ICON_RADIUS 2

#define CHAR_WIDTH      8
#define CHAR_HEIGHT     8
#define LINE_HEIGHT     (CHAR_HEIGHT + 1)
#define COL_WIDTH       (CHAR_WIDTH)
#define PAD_CHARS       0

static const double pi = 3.14159265359;


GPS_OLED::GPS_OLED(SSD1306* pDisplay, GPS* pGPS, LED* pLED)
    : m_pDisplay(pDisplay),
      m_pGPS(pGPS),
      m_pLED(pLED),
      m_pGPSData(NULL)
{
}

GPS_OLED::~GPS_OLED()
{
    delete m_pDisplay;
    delete m_pGPS;
    delete m_pLED;
}

void GPS_OLED::init()
{
    printf("In GPS_OLED::init()\n");
    m_pDisplay->init();
    m_pDisplay->contrast(0x10);
    m_pGPS->setSentenceCallback(this, sentenceCB);
    m_pGPS->setGpsDataCallback(this, gpsDataCB);
}

void GPS_OLED::run()
{
    m_pDisplay->fill(oled_off);
    m_pDisplay->show();
    m_pGPS->run();
}

void GPS_OLED::sentenceCB(void* pCtx, string strSentence)
{
    printf("sentenceCB received: %s\n", strSentence.c_str());
}

void GPS_OLED::gpsDataCB(void* pCtx, GPSData* pGPSData)
{
    printf("gpsDataCB received time: %s\n", pGPSData->strGPSTime.c_str());
    GPS_OLED* pThis = reinterpret_cast<GPS_OLED*>(pCtx);
    pThis->UpdateUI(pGPSData);
    delete pGPSData; // We must delete
}

void GPS_OLED::UpdateUI(GPSData* pGPSData)
{
    m_pGPSData = pGPSData;

    // This code assumes OLED display 128x64 mono
    uint16_t nWidth  = m_pDisplay->width();
    uint16_t nHeight = m_pDisplay->height();
    m_pDisplay->fill(oled_black);

    // Draw satellite grid
    drawSatGrid(0, 0, nWidth / 2, nHeight, 2);

    // Draw upper right text
    drawText(3, pGPSData->strNumSats, oled_white, true, 0);
    drawText(4, pGPSData->strMode3D, oled_white, true, 0);

    if (m_pLED)
    {
        if (m_pGPS->hasPosition())
        {
            m_pLED->setPixel(0, m_pGPS->externalAntenna() ? led_blue : led_green);
            m_pLED->blink_ms(20);
        }
        else
        {
            m_pLED->setPixel(0, led_red);
            m_pLED->blink_ms(20);
        }
    }
    if (!pGPSData->strGPSTime.empty())
    {
        drawText(-1, pGPSData->strGPSTime, oled_white, true, 0);
    }
    if (!pGPSData->strLatitude.empty())
    {
        drawText(0, pGPSData->strLatitude, oled_white, true, 0);
        drawText(1, pGPSData->strLongitude, oled_white, true, 0);
        drawText(2, pGPSData->strAltitude, oled_white, true, 0);
        // drawText(5, pGPSData->strSpeedKts, oled_white, true, 0);
    }

    // Draw clock
    // if (!pGPSData->strGPSTime.empty())
    // {
    //     drawClock(nWidth/2, LINE_HEIGHT*3, LINE_HEIGHT*7/2, pGPSData->strGPSTime);
    // }

    // Draw bar graph
    // drawBarGraph(nWidth/2, nHeight/2,
    //              nWidth/2 - (PAD_CHARS*CHAR_WIDTH), nHeight/2 - (PAD_CHARS*CHAR_HEIGHT));

    m_pGPSData = NULL;

    // blit the framebuf to the display
    m_pDisplay->show();
}


void GPS_OLED::drawSatGrid(uint x, uint y, uint width, uint height, uint nRings)
{
    uint radius  = min(width, height) / 2 - CHAR_HEIGHT / 2;
    uint xCenter = x + width / 2;
    uint yCenter = y + height / 2;
    drawSatGridRadial(xCenter, yCenter, radius, nRings);
}

void GPS_OLED::drawSatGridRadial(uint xCenter, uint yCenter, uint radius, uint nRings)
{
    for (uint i = 1; i <= nRings; ++i)
    {
        m_pDisplay->ellipse(xCenter, yCenter, radius * i / nRings, radius * i / nRings, oled_white);
    }

    m_pDisplay->vline(xCenter, yCenter - radius - 2, 2 * radius + 5, oled_white);
    m_pDisplay->hline(xCenter - radius - 2, yCenter, 2 * radius + 5, oled_white);
    // m_pDisplay->text("^", xCenter-CHAR_WIDTH/2, yCenter-radius-CHAR_HEIGHT/2, oled_white);
    m_pDisplay->text("'", xCenter - 6, yCenter - radius - CHAR_HEIGHT / 2, oled_white);
    m_pDisplay->text("`", xCenter - 2, yCenter - radius - CHAR_HEIGHT / 2, oled_white);

    int satRadius = SAT_ICON_RADIUS / 2;
    if (!m_pGPSData->strLatitude.empty())
    {
        satRadius = SAT_ICON_RADIUS;
    }
    for (auto oSat : m_pGPSData->vSatList)
    {
        double elrad = oSat.m_el * pi / 180;
        double azrad = oSat.m_az * pi / 180;
        drawCircleSat(xCenter, yCenter, radius, elrad, azrad, satRadius, oled_white, oled_black);
        for (auto nSat : m_pGPSData->vUsedList)
        {
            if (oSat.m_num == nSat)
            {
                drawCircleSat(xCenter, yCenter, radius, elrad, azrad, satRadius, oled_white, oled_white);
                break;
            }
        }
    }
}

void GPS_OLED::drawCircleSat(uint gridCenterX,
                             uint gridCenterY,
                             uint nGridRadius,
                             float elrad,
                             float azrad,
                             uint satRadius,
                             uint16_t color,
                             uint16_t fillColor)
{
    // Draw satellite (fill first, then draw open circle)
    int dx = (nGridRadius - SAT_ICON_RADIUS) * cos(elrad) * sin(azrad);
    int dy = (nGridRadius - SAT_ICON_RADIUS) * cos(elrad) * -cos(azrad);
    int x  = gridCenterX + dx;
    int y  = gridCenterY + dy;
    m_pDisplay->ellipse(x, y, satRadius, satRadius, fillColor, true); // Clear area with fill
    m_pDisplay->ellipse(x, y, satRadius, satRadius, color);           // Draw circle without fill
}

void GPS_OLED::drawBarGraph(uint x, uint y, uint width, uint height)
{
    // static const uint nMaxSats = 14;
    // uint barDelta = width/nMaxSats;
    // uint barWidth = barDelta-2;
    // uint barPosX = x + width - (m_vSatList.size() * barDelta);
    // uint barHeightMax = height - LINE_HEIGHT*2;

    // for (auto pSat : m_vSatList)
    // {
    //     uint rssi = pSat->m_rssi;
    //     uint barHeight = (int)((double)barHeightMax * (double)rssi/64);
    //     uint baseLineY = y + barHeightMax;
    //     m_pFBuf->hline(barPosX, baseLineY, barDelta, COLOUR_WHITE);
    //     uint nSat = pSat->m_num;
    //     std::stringstream oss;
    //     oss << fixed << setw(2) << setfill('0') << setprecision(0) << nSat;
    //     string strSatNum = oss.str();
    //     uint charPosX = barPosX + (barDelta - CHAR_WIDTH)/2;
    //     m_pFBuf->text(strSatNum.substr(0,1).c_str(), charPosX, baseLineY+2, COLOUR_WHITE);
    //     m_pFBuf->text(strSatNum.substr(1,1).c_str(), charPosX, baseLineY+CHAR_HEIGHT+2, COLOUR_WHITE);
    //     if (barHeight > 0)
    //     {
    //         m_pFBuf->rect(barPosX+1, baseLineY-barHeight+1, barWidth, barHeight, COLOUR_WHITE);
    //     }
    //     for (auto pUsedSat : m_vUsedSatList)
    //     {
    //         if (pSat->m_num == pUsedSat->m_num)
    //         {
    //             if (barHeight > 0)
    //             {
    //                 m_pFBuf->rect(barPosX+2, baseLineY-barHeight+2, barWidth-2, barHeight-2, COLOUR_BLUE, true);
    //             }
    //             break;
    //         }
    //     }
    //     barPosX += barDelta;
    // }
}

void GPS_OLED::drawClock(uint x, uint y, uint radius, string strTime)
{
    // uint xCenter = x + radius;
    // uint yCenter = y + radius;
    // uint nHour = atoi(strTime.substr(0,2).c_str());
    // float hour = (float)(nHour % 12) + m_GMToffset;
    // hour = (hour < 0) ? hour + 12 : hour;
    // hour = (hour >= 12) ? hour - 12 : hour;
    // float minute = (float)atoi(strTime.substr(3,2).c_str());
    // float second = (float)atoi(strTime.substr(6,2).c_str());
    // uint16_t faceColor = COLOUR_BLACK;
    // uint16_t handColor = COLOUR_WHITE;
    // uint16_t secondHandColor = COLOUR_RED;
    // double handLenHour = radius * 0.4;
    // double handLenMinute = radius * 0.7;
    // double handLenSecond = radius * 0.8;
    // double radiansHour = 2 * pi * (((hour * 3600.0) + (minute * 60.0) + second) / (12.0 * 60.0 * 60.0));
    // double radiansMinute = 2 * pi * (((minute * 60.0) + second) / (60.0 * 60.0));
    // double radiansSecond = 2 * pi * (second / 60.0);
    // int dxh = int(handLenHour * sin(radiansHour));
    // int dyh = int(handLenHour * -cos(radiansHour));
    // int dxm = int(handLenMinute * sin(radiansMinute));
    // int dym = int(handLenMinute * -cos(radiansMinute));
    // int dxs = int(handLenSecond * sin(radiansSecond));
    // int dys = int(handLenSecond * -cos(radiansSecond));

    // // Draw the face
    // m_pFBuf->ellipse(xCenter, yCenter, radius-1, radius-1, COLOUR_GREEN, true);
    // m_pFBuf->ellipse(xCenter, yCenter, radius-2, radius-2, faceColor, true);
    // // Draw quarter dots
    // for (uint degDot = 0; degDot<360; degDot += 30)
    // {
    //     uint dxDot = int((radius-3) * sin(degDot * pi / 180));
    //     uint dyDot = int((radius-3) * -cos(degDot * pi / 180));
    //     uint16_t colDot = (degDot%90 == 0) ? COLOUR_BLUE : COLOUR_GRAY;
    //     uint16_t sizDot = (degDot%90 == 0) ? 2 : 1;
    //     m_pFBuf->ellipse(xCenter+dxDot, yCenter+dyDot, sizDot, sizDot, colDot, true);
    // }
    // // Draw the hands
    // m_pFBuf->line(xCenter, yCenter, xCenter+dxs, yCenter+dys, secondHandColor);
    // m_pFBuf->line(xCenter, yCenter, xCenter+dxh, yCenter+dyh, handColor);
    // m_pFBuf->line(xCenter, yCenter, xCenter+dxm, yCenter+dym, handColor);
    // m_pFBuf->ellipse(xCenter, yCenter, 1, 1, faceColor);
}

int GPS_OLED::linePos(int nLine)
{
    if (nLine >= 0)
        return nLine * LINE_HEIGHT;
    else
        return m_pDisplay->height() + 1 + (nLine * LINE_HEIGHT);
}

void GPS_OLED::drawText(int nLine, string strText, uint16_t color, bool bRightAlign, uint nRightPad)
{
    int x = (!bRightAlign) ? 0 : m_pDisplay->width() - (strText.length() * COL_WIDTH);
    int y = linePos(nLine);
    x     = x - nRightPad;
    m_pDisplay->text(strText.c_str(), x, y, color);
}
