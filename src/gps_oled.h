/*
 * GPS using OLED display
 *
 * (c) 2023 Erik Tkal
 *
 */

#pragma once

#include "ssd1306.h"
#include "gps.h"
#include "led.h"

// GPS_OLED class
//
// This combines an OLED display, GPS module and LED.
// The devices are initialized here and then callbacks are set up in order
// to receive data from the GPS, and the resulting data is displayed
// and the LED can be used to indicate a position lock and/or other
// status.
//
class GPS_OLED
{
public:
    GPS_OLED(SSD1306* pDisplay, GPS* pGPS, LED* pLED);
    ~GPS_OLED();

    void init();
    void run();

private:
    static void sentenceCB(void* pCtx, string strSentence);
    static void gpsDataCB(void* pCtx, GPSData* pGPSData);

    void UpdateUI(GPSData* pGPSData);
    void drawSatGrid(uint x, uint y, uint width, uint height, uint nRings = 3);
    void drawSatGridRadial(uint xCenter, uint yCenter, uint radius, uint nRings = 3);
    void drawBarGraph(uint x, uint y, uint width, uint height);
    void drawClock(uint x, uint y, uint radius, string strTime);
    void drawCircleSat(uint gridCenterX,
                       uint gridCenterY,
                       uint nGridRadius,
                       float elrad,
                       float azrad,
                       uint satRadius,
                       uint16_t color     = oled_white,
                       uint16_t fillColor = oled_white);
    int linePos(int nLine);
    void drawText(int nLine, string strText, uint16_t color = oled_white, bool bRightAlign = true, uint nRightPad = 0);

    SSD1306* m_pDisplay;
    GPS* m_pGPS;
    LED* m_pLED;

    GPSData* m_pGPSData;
};
