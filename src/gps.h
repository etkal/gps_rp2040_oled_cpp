/*
 * GPS class
 *
 * (c) 2025 Erik Tkal
 *
 */

#pragma once

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

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

typedef std::map<uint, SatInfo> SatList;
typedef std::vector<uint> UsedList;

class GPSData
{
public:
    typedef std::shared_ptr<GPSData> Shared;

    GPSData()
        : bHasPosition(false),
          bExternalAntenna(false)
    {
    }
    ~GPSData() = default;

    bool bHasPosition;
    bool bExternalAntenna;
    std::string strLatitude;
    std::string strLongitude;
    std::string strAltitude;
    std::string strNumSats;
    std::string strGPSTime;
    std::string strMode3D;
    std::string strSpeed;
    SatList mSatList;
    UsedList vUsedList;
};

typedef void (*sentenceCallback)(void* pCtx, std::string strSentence);
typedef void (*gpsDataCallback)(void* pCtx, GPSData::Shared spGPSData);

auto constexpr GPS_BUFSIZE            = 4096; // Circular buffer size

class GPS
{
public:
    typedef std::shared_ptr<GPS> Shared;

    GPS(uart_inst_t* pUART0, uart_inst_t* pUART1 = nullptr);
    ~GPS();

    void SetSentenceCallback(void* pCtx, sentenceCallback pCB);
    void SetGpsDataCallback(void* pCtx, gpsDataCallback pCB);
    void Run();
    uart_inst_t* GetUART()
    {
        return m_pUART0;
    }

private:
    bool processSentence(std::string strSentence);
    bool validateSentence(std::string& strSentence);
    std::string checkSum(const std::string& strSentence);
    std::string convertToDegrees(std::string strRaw, int width);

    uart_inst_t* m_pUART0;
    uart_inst_t* m_pUART1; // output echo

    // RX buffer management
    static char sm_szBuffer[GPS_BUFSIZE];
    static volatile size_t sm_iHead;
    static volatile size_t sm_iNext;
    static volatile size_t sm_nSentences;
    static void on_uart_rx();
    static bool getSentence(std::string& strSentence);

    // GPS object members
    bool m_bExit;
    bool m_bGSVInProgress;
    std::string m_strNumGSV;
    uint64_t m_nSatListTime;
    bool m_bSendGpsData;
    GPSData::Shared m_spGPSData;
    SatList m_mSatListIncoming;
    SatList m_mSatListPersistent;

    sentenceCallback m_pSentenceCallBack;
    void* m_pSentenceCtx;
    gpsDataCallback m_pGpsDataCallback;
    void* m_pGpsDataCtx;
};
