/*
 * GPS class
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

#include <queue>
#include <iomanip>

#include "gps.h"
#include <pico/sync.h>

using namespace std;

static GPS* sg_pGPS = NULL;
static char szSentence[256];
static uint nRead = 0;
static critical_section_t critsec;

static queue<string> sg_sentenceQueue;

// RX interrupt handler
static void on_uart_rx()
{
    uart_inst_t* pUART = sg_pGPS->getUART();
    while (uart_is_readable(pUART))
    {
        char ch             = uart_getc(pUART);
        szSentence[nRead++] = ch;
        if (ch == '\n')
        {
            szSentence[nRead++] = '\0';
            string sentence     = szSentence;
            nRead               = 0;
            critical_section_enter_blocking(&critsec);
            sg_sentenceQueue.push(sentence);
            critical_section_exit(&critsec);
        }
        if (nRead >= sizeof(szSentence))
        {
            nRead = 0;
            break;
        }
    }
}

GPS::GPS(uart_inst_t* pUART)
    : m_pUART(pUART),
      m_bFixTime(false),
      m_bFixPos(false),
      m_bExternalAntenna(true),
      m_bGSVInProgress(false),
      m_pGPSData(NULL),
      m_pSentenceCallBack(nullptr),
      m_pSentenceCtx(nullptr),
      m_pGpsDataCallback(nullptr),
      m_pGpsDataCtx(nullptr)
{
}

GPS::~GPS()
{
    delete m_pGPSData;
}

void GPS::setSentenceCallback(void* pCtx, sentenceCallback pCB)
{
    printf("In GPS::setSentenceCallback()\n");
    m_pSentenceCtx      = pCtx;
    m_pSentenceCallBack = pCB;
}

void GPS::setGpsDataCallback(void* pCtx, gpsDataCallback pCB)
{
    printf("In GPS::setGpsDataCallback()\n");
    m_pGpsDataCtx      = pCtx;
    m_pGpsDataCallback = pCB;
}

void GPS::run()
{
    printf("In GPS::run()\n");
    // Set up GPS
    uart_set_fifo_enabled(m_pUART, true);
    sg_pGPS     = this; // Allow interrupt handler to call us back
    int uartIRQ = m_pUART == uart0 ? UART0_IRQ : UART1_IRQ;
    // Set up and enable the interrupt handler
    irq_set_exclusive_handler(uartIRQ, on_uart_rx);
    irq_set_enabled(uartIRQ, true);
    critical_section_init(&critsec);
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(m_pUART, true, false);

    string strSentence;
    bool bSentAntennaCommands = false;
    while (true)
    {
        // Read sentence from GPS device
        tight_loop_contents();
        // sleep_ms(1);
        bool bSentenceAvailable = false;
        critical_section_enter_blocking(&critsec);
        if (!sg_sentenceQueue.empty())
        {
            strSentence = sg_sentenceQueue.front();
            sg_sentenceQueue.pop();
            bSentenceAvailable = true;
        }
        critical_section_exit(&critsec);

        if (bSentenceAvailable)
        {
            processSentence(strSentence);
            bSentenceAvailable = false;
            if (!bSentAntennaCommands)
            {
                // Write commands to enable reporting external vs internal antenna.  We wait
                // until some data is received to ensure the GPS has finished initializing.
                uart_puts(m_pUART, "$PGCMD,33,1*6C\r\n"); // Enable antenna output for PA6H
                uart_puts(m_pUART, "$CDCMD,33,1*7C\r\n"); // Enable antenna output for PA1616S
                bSentAntennaCommands = true;
            }
        }
    }
}

void GPS::processSentence(string strSentence)
{
    printf("%s", strSentence.c_str());

    // Validate the string
    if (!validateSentence(strSentence))
    {
        printf("Failed to validate NMEA sentence\n");
        return;
    }

    if (NULL != m_pSentenceCallBack)
    {
        (*m_pSentenceCallBack)(m_pSentenceCtx, strSentence);
    }

    if (NULL == m_pGPSData)
    {
        // Guarantee we have an object to update
        m_pGPSData           = new GPSData;
        m_pGPSData->vSatList = m_vSatListPersistent; // restore previous data
    }

    // Split the string
    vector<string> elems;
    stringstream s_stream(strSentence);
    while (s_stream.good())
    {
        string substr;
        getline(s_stream, substr, ','); // get first string delimited by comma
        elems.push_back(substr);
    }

    if (time_us_64() > m_nSatListTime + 30 * 1000 * 1000) // Nothing in 30 seconds, clear vectors
    {
        m_pGPSData->vSatList.clear();
        m_pGPSData->vUsedList.clear();
    }

    if (elems[0] == "$GPGSV")
    {
        // Multipart, clear any previous data and re-gather
        if (elems[2] == "1")
        {
            m_pGPSData->vSatList.clear();
            m_strNumGSV      = elems[1];
            m_bGSVInProgress = true;
        }
        int nNumSatsInGSV = min(4, atoi(elems[3].c_str()) - 4 * (atoi(elems[2].c_str()) - 1));
        if (m_bGSVInProgress)
        {
            for (int i = 4; i < 4 + 4 * nNumSatsInGSV; i += 4)
            {
                if (!elems[i].empty() && !elems[i + 1].empty() && !elems[i + 2].empty())
                {
                    uint num  = atoi(elems[i].c_str());
                    uint el   = atoi(elems[i + 1].c_str());
                    uint az   = atoi(elems[i + 2].c_str());
                    uint rssi = elems[i + 3].empty() ? 0 : atoi(elems[i + 3].c_str());
                    m_pGPSData->vSatList.emplace_back(num, el, az, rssi);
                }
            }
            if (elems[2] == m_strNumGSV) // Last one received
            {
                m_bGSVInProgress = false;
                m_nSatListTime   = time_us_64();
            }
        }
        return;
    }
    else if (m_bGSVInProgress) // Did not complete
    {
        m_pGPSData->vSatList.clear();
        m_pGPSData->vUsedList.clear();
    }

    if (elems[0] == "$GPRMC")
    {
        if (!elems[1].empty())
        {
            string& t              = elems[1];
            m_pGPSData->strGPSTime = t.substr(0, 2) + ":" + t.substr(2, 2) + ":" + t.substr(4, 2) + "Z";
            m_bFixTime             = true;
        }
        else
        {
            m_bFixTime             = false;
            m_pGPSData->strGPSTime = "";
        }
        if (elems[2] == "A")
        {
            if (!elems[3].empty() && !elems[4].empty() && !elems[5].empty() && !elems[6].empty())
            {
                m_bFixPos                = true;
                m_pGPSData->strLatitude  = convertToDegrees(elems[3], 7) + elems[4];
                m_pGPSData->strLongitude = convertToDegrees(elems[5], 8) + elems[6];
            }
            if (!elems[7].empty())
            {
                double dKnots = stod(elems[7].c_str());
                std::stringstream oss;
                if (dKnots < 10.0)
                {
                    oss << fixed << setfill(' ') << setprecision(1) << dKnots << "kn";
                }
                else
                {
                    oss << setfill(' ') << setprecision(0) << dKnots << "kn";
                }
                m_pGPSData->strSpeedKts = oss.str();
            }
        }
        else
        {
            m_bFixPos = false;
        }
    }

    if (elems[0] == "$GPGGA")
    {
        if (!elems[7].empty())
        {
            m_pGPSData->strNumSats = "Sat: " + elems[7];
        }
        if (!elems[9].empty())
        {
            double dMeters = stod(elems[9].c_str());
            std::stringstream oss;
            if (dMeters < 1000.0)
            {
                oss << fixed << setfill(' ') << setprecision(1) << dMeters << "m";
            }
            else
            {
                oss << setfill(' ') << setprecision(0) << dMeters << "m";
            }
            m_pGPSData->strAltitude = oss.str();
        }
    }

    if (elems[0] == "$GPGSA")
    {
        m_pGPSData->vUsedList.clear();
        m_pGPSData->strMode3D = elems[2] + "D Fix";
        if (elems[2] == "1")
        {
            m_pGPSData->strMode3D = "No Fix";
        }
        for (int i = 3; i < 15; ++i)
        {
            if (!elems[i].empty())
            {
                uint satNum = atoi(elems[i].c_str());
                if (satNum != 0)
                {
                    m_pGPSData->vUsedList.push_back(satNum);
                }
            }
            else
            {
                break;
            }
        }
    }

    if (elems[0] == "$GPRMC")
    {
        if (NULL != m_pGpsDataCallback)
        {
            m_vSatListPersistent = m_pGPSData->vSatList; // Persist the list
            (*m_pGpsDataCallback)(m_pGpsDataCtx, m_pGPSData);
            m_pGPSData = NULL; // Caller must free
        }
    }

    if (elems[0] == "$PGTOP") // PA6H
    {
        if (elems[2] == "2")
        {
            m_bExternalAntenna = false;
        }
        if (elems[2] == "3")
        {
            m_bExternalAntenna = true;
        }
    }

    if (elems[0] == "$PCD") // PA1616S
    {
        if (elems[2] == "1")
        {
            m_bExternalAntenna = false;
        }
        if (elems[2] == "2")
        {
            m_bExternalAntenna = true;
        }
    }
}

bool GPS::validateSentence(string& strSentence)
{
    // Validate format and remove checksum and CRLF
    size_t nLen = strSentence.size();
    if (nLen < 1 || strSentence[0] != '$')
    {
        printf("Sentence does not start with $\n");
        return false;
    }
    if (nLen < 6 || strSentence.substr(nLen - 2, 2) != "\r\n" || strSentence[nLen - 5] != '*')
    {
        printf("Sentence does not end with *XX\\r\\n\n");
        return false;
    }
    string specifiedCheck  = strSentence.substr(nLen - 4, 2);
    string calculatedCheck = checkSum(strSentence.substr(1, nLen - 6));
    if (calculatedCheck != specifiedCheck)
    {
        printf("Failed to validate checksum. Specified: %s, Expected: %s\n", specifiedCheck.c_str(), calculatedCheck.c_str());
        return false;
    }

    strSentence = strSentence.substr(0, nLen - 5);

    return true;
}

string GPS::checkSum(const string& strSentence)
{
    uint8_t check = 0;
    for (const char& c : strSentence)
    {
        check ^= (uint8_t)c;
    }
    std::stringstream oss;
    oss << hex << uppercase << setw(2) << setfill('0') << (unsigned int)check;
    return oss.str();
}

string GPS::convertToDegrees(string strRaw, int width)
{
    // Convert (D)DDMM.mmmm to decimal degrees
    double dRawAsDouble = stod(strRaw);
    int firstdigits     = int(dRawAsDouble / 100);
    int nexttwodigits   = dRawAsDouble - double(firstdigits * 100);
    double converted    = double(firstdigits) + nexttwodigits / 60.0;
    std::stringstream oss;
    oss << fixed << setw(width) << setfill(' ') << setprecision(4) << converted;
    return oss.str();
}
