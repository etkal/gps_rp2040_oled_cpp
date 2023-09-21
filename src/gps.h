/*
 * GPS class
 *
 * (c) 2023 Erik Tkal
 *
 */

#pragma once

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <string>
#include <vector>

using namespace std;

class SatInfo
{
public:
    SatInfo(uint num = 0, uint el = 0, uint az = 0, uint rssi = 0)
    {
        m_num  = num;
        m_el   = el;
        m_az   = az;
        m_rssi = rssi;
    }
    ~SatInfo()
    {
    }

    uint m_num;
    uint m_el;
    uint m_az;
    uint m_rssi;
};

typedef vector<SatInfo> SatList;
typedef vector<uint> UsedList;

class GPSData
{
public:
    GPSData()
    {
    }
    ~GPSData()
    {
    }

    string strLatitude;
    string strLongitude;
    string strAltitude;
    string strNumSats;
    string strGPSTime;
    string strMode3D;
    string strSpeedKts;
    SatList vSatList;
    UsedList vUsedList;
};

typedef void (*sentenceCallback)(void* pCtx, string strSentence);
typedef void (*gpsDataCallback)(void* pCtx, GPSData* pGPSData);

class GPS
{
public:
    GPS(uart_inst_t* m_pUART);
    ~GPS();

    void setSentenceCallback(void* pCtx, sentenceCallback pCB);
    void setGpsDataCallback(void* pCtx, gpsDataCallback pCB);

    void run();
    uart_inst_t* getUART()
    {
        return m_pUART;
    }
    void processSentence(string strSentence);
    bool validateSentence(string& strSentence);
    string checkSum(const string& strSentence);
    string convertToDegrees(string strRaw, int width);
    bool hasTime()
    {
        return m_bFixTime;
    }
    bool hasPosition()
    {
        return m_bFixPos;
    }
    bool externalAntenna()
    {
        return m_bExternalAntenna;
    }

private:
    uart_inst_t* m_pUART;

    bool m_bFixTime;
    bool m_bFixPos;
    bool m_bExternalAntenna;
    bool m_bGSVInProgress;
    string m_strNumGSV;
    uint64_t m_nSatListTime;

    GPSData* m_pGPSData;
    SatList m_vSatListPersistent;

    sentenceCallback m_pSentenceCallBack;
    void* m_pSentenceCtx;
    gpsDataCallback m_pGpsDataCallback;
    void* m_pGpsDataCtx;
};
