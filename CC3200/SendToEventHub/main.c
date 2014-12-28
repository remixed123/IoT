//*****************************************************************************
//
// Application Name        - SendToEventHub
// Application Version     - 1.1.0
// Application Modify Date - 14th of December 2014
// Application Developer   - Glenn Vassallo
// Application Repository  - https://github.com/remixed123/IoT
// Application Overview    - This is a sample application demonstrating the
//                           sending of telemetry (temperature data) from the
//                           CC3200 LaunchPad to Microsoft Azure Event Hub by
//                           utilising Event Hubs REST API over SSL/TLS. The
//                           application also retrieves the time from a SNTP
//                           server periodically, this helps to ensure that
//                           SSL connection will function.
// Application History     - The SendToEventHub example code has been created
//                           through modifying and including code from the SSL
//                           and Get_TIME examples that are provided with the
//                           CC3200 SDK - http://www.ti.com/tool/cc3200sdk
// Application Details     - https://github.com/remixed123/IoT/wiki
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup get_time
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Project Includes
#include "simplelinklibrary.h"

//CC3200 LaunchPad includes
#include "tmp006drv.h"

// simplelink includes
#include "simplelink.h"

// driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"

//Free_rtos/ti-rtos includes
#include "osi.h"

// common interface includes
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "gpio_if.h"
#include "i2c_if.h"
#include "timer_if.h"
#include "udma_if.h"
#include "pinmux.h"
#include "common.h"

//================================

//#define DATE                18    /* Current Date */
//#define MONTH               6     /* Month 1-12 */
//#define YEAR                2014  /* Current year */
//#define HOUR                12    /* Time - hours */
//#define MINUTE              32    /* Time - minutes */
//#define SECOND              0     /* Time - seconds */

//*****************************************************************************
//                       APPLICATION DETAIL DEFINES
//*****************************************************************************

#define APPLICATION_NAME        "SendToEventHub"
#define APPLICATION_VERSION     "1.2.0"

#define OSI_STACK_SIZE          4096

//*****************************************************************************
//                          EVENT HUB SERVER DEFINES
//*****************************************************************************

#define EH_SERVER_NAME		     "swiftsoftware-ns.servicebus.windows.net" // You will need change this to reflect your details
#define SSL_DST_PORT             443

//*****************************************************************************
//                        PACKET DELAY TIME DEFINES
//
// The application uses a simple delay to alter the time intervale between
// packets that are sent to Azure Event Hub. You can set the time by change the
// value below. This will provide an estimate, if you require a more accurate
// delay time, or you wish to do other things during the delay interval then
// you will need to setup a timer.
//
//*****************************************************************************
#define SLEEP_TIME            60 //This is in second, change this to set your delay time
#define SEC_TO_LOOP(x)        ((80000000/5)*x)

//*****************************************************************************
//                       CERTIFICATE LOCATION DEFINE
//
// You will need to include an SSL Certificate Authority file to be able to
// send SSL/TLS packets to Event Hub. Further details can be found at the
// wiki -
//
//*****************************************************************************

#define SL_SSL_CA_CERT_FILE_NAME        "/cert/azurecacert.der"

//*****************************************************************************
//                           SNTP CONFIGURATION DEFINES
//*****************************************************************************

#define TIME2013                3565987200u      /* 113 years + 28 days(leap) */
#define YEAR2013                2013
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              86400

#define SERVER_RESPONSE_TIMEOUT 10
#define GMT_DIFF_TIME_HRS       10
#define GMT_DIFF_TIME_MINS      0

//*****************************************************************************
//                         HTTP POST HEADER DEFINES
//
// 1) You will need to update the POSTHEADER and HOSTHEADER here to reflect
//    your Azure setup.
// 2) You wil also need to change AUTHHEADER by generating a Shared Access
//    Signature that expires in say 10 years. You can do this from a C#
//    application, see the steps at this link -
// 3) Update the DATA portion to reflect the details you wish to send.
//
//*****************************************************************************
#define POSTHEADER "POST /swiftsoftware-eh/messages HTTP/1.1\r\n"
#define HOSTHEADER "Host: swiftsoftware-ns.servicebus.windows.net\r\n"
#define AUTHHEADER "Authorization: SharedAccessSignature sr=swiftsoftware-ns.servicebus.windows.net&sig=6sIkgCiaNbK9R0XEpsKJcQ2Clv8MUMVdQfEVQP09WkM%3d&se=1733661915&skn=EventHubPublisher\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
#define DATA1 "{\"MessageType\":\"CC3200 Sensor\",\"Temp\":"
#define DATA2 ",\"Humidity\":50,\"Location\":\"Glenn's Home\",\"Room\":\"Kitchen\",\"Info\":\"Sent from CC3200 LaunchPad\"}"

//*****************************************************************************
//              Application specific status/error codes
//*****************************************************************************
//typedef enum{
//    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
//    SERVER_GET_TIME_FAILED = -0x7D0,
//    DNS_LOOPUP_FAILED = SERVER_GET_TIME_FAILED  -1,
//
//    STATUS_CODE_MAX = -0xBB8
//}e_AppStatusCodes;

typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,
    SERVER_GET_TIME_FAILED = -0x7D0,
    DNS_LOOPUP_FAILED = SERVER_GET_TIME_FAILED  -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
//unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
//unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
//unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = EH_SERVER_NAME;
//const char     pcDigits[] = "0123456789"; /* variable used by itoa function */
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
unsigned short g_usTimerInts;
SlSecParams_t SecurityParams = {0};

//*****************************************************************************
// Globals for Date and Time
//*****************************************************************************
SlDateTime_t dateTime =  {0};

//*****************************************************************************
// Globals for errors
//*****************************************************************************
int errorcount = 0;
int errorcounttime = 0;
int errorcountpost = 0;
int lasterrornumber = 0;

//*****************************************************************************
// Globals for time difference calculations
//*****************************************************************************
int hourSet = 25; // Ensures the timeDifference calc will end up in a minus, so will enter the function on first pass
int timeDifference = 0;


#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//!    ######################### list of SNTP servers ##################################
//!    ##
//!    ##          hostname         |        IP       |       location
//!    ## -----------------------------------------------------------------------------
//!    ##   nist1-nj2.ustiming.org  | 165.193.126.229 |  Weehawken, NJ
//!    ##   nist1-pa.ustiming.org   | 206.246.122.250 |  Hatfield, PA
//!    ##   time-a.nist.gov         | 129.6.15.28     |  NIST, Gaithersburg, Maryland
//!    ##   time-b.nist.gov         | 129.6.15.29     |  NIST, Gaithersburg, Maryland
//!    ##   time-c.nist.gov         | 129.6.15.30     |  NIST, Gaithersburg, Maryland
//!    ##   ntp-nist.ldsbc.edu      | 198.60.73.8     |  LDSBC, Salt Lake City, Utah
//!    ##   nist1-macon.macon.ga.us | 98.175.203.200  |  Macon, Georgia
//!
//!    ##   For more SNTP server link visit 'http://tf.nist.gov/tf-cgi/servers.cgi'
//!    ###################################################################################
const char g_acSNTPserver[30] = "nist1-nj2.ustiming.org"; //Add any one of the above servers

// Tuesday is the 1st day in 2013 - the relative year
const char g_acDaysOfWeek2013[7][3] = {{"Tue"},
                                    {"Wed"},
                                    {"Thu"},
                                    {"Fri"},
                                    {"Sat"},
                                    {"Sun"},
                                    {"Mon"}};

const char g_acMonthOfYear[12][3] = {{"Jan"},
                                  {"Feb"},
                                  {"Mar"},
                                  {"Apr"},
                                  {"May"},
                                  {"Jun"},
                                  {"Jul"},
                                  {"Aug"},
                                  {"Sep"},
                                  {"Oct"},
                                  {"Nov"},
                                  {"Dec"}};

const char g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

const char g_acDigits[] = "0123456789";

struct
{
    unsigned long ulDestinationIP;
    int iSockID;
    unsigned long ulElapsedSec;
    short isGeneralVar;
    unsigned long ulGeneralVar;
    unsigned long ulGeneralVar1;
    char acTimeStore[30];
    char *pcCCPtr;
    unsigned short uisCCLen;
}g_sAppData;

SlSockAddr_t sAddr;
SlSockAddrIn_t sLocalAddr;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
void MainTask(void *pvParameters);
static long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins);
static void DisplayBanner(char * AppName);
long PostEventHubSSL();

//*****************************************************************************
//
//! Gets the current time from the selected SNTP server
//!
//! \brief  This function obtains the NTP time from the server.
//!
//! \param  GmtDiffHr is the GMT Time Zone difference in hours
//! \param  GmtDiffMins is the GMT Time Zone difference in minutes
//!
//! \return 0 : success, -ve : failure
//!
//
//*****************************************************************************
long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins)
{

/*
                            NTP Packet Header:

       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    char cDataBuf[48];
    long lRetVal = 0;
    int iAddrSize;

    //
    // Send a query ? to the NTP server to get the NTP time
    //
    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;
    // the source port
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    // UDP port number for NTP is 123
    sAddr.sa_data[2] = (char)((g_sAppData.ulDestinationIP>>24)&0xff);
    sAddr.sa_data[3] = (char)((g_sAppData.ulDestinationIP>>16)&0xff);
    sAddr.sa_data[4] = (char)((g_sAppData.ulDestinationIP>>8)&0xff);
    sAddr.sa_data[5] = (char)(g_sAppData.ulDestinationIP&0xff);

    lRetVal = sl_SendTo(g_sAppData.iSockID,
                     cDataBuf,
                     sizeof(cDataBuf), 0,
                     &sAddr, sizeof(sAddr));
    if (lRetVal != sizeof(cDataBuf))
    {
        // could not send SNTP request
    	errorcounttime++;
        ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);
    }

    //
    // Wait to receive the NTP time from the server
    //
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = 0;
    sLocalAddr.sin_addr.s_addr = 0;
    if(g_sAppData.ulElapsedSec == 0)
    {
        lRetVal = sl_Bind(g_sAppData.iSockID,
                (SlSockAddr_t *)&sLocalAddr,
                sizeof(SlSockAddrIn_t));
        errorcounttime++;
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_sAppData.iSockID,
                       cDataBuf, sizeof(cDataBuf), 0,
                       (SlSockAddr_t *)&sLocalAddr,
                       (SlSocklen_t*)&iAddrSize);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Confirm that the MODE is 4 --> server
    //
    if ((cDataBuf[0] & 0x7) != 4)    // expect only server response
    {
         ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);  // MODE is not server, abort
    }
    else
    {
        unsigned char iIndex;

        //
        // Getting the data from the Transmit Timestamp (seconds) field
        // This is the time at which the reply departed the
        // server for the client
        //
        g_sAppData.ulElapsedSec = cDataBuf[40];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[41];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[42];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[43];

        //
        // seconds are relative to 0h on 1 January 1900
        //
        g_sAppData.ulElapsedSec -= TIME2013;

        //
        // in order to correct the timezone
        //
        g_sAppData.ulElapsedSec += (ucGmtDiffHr * SEC_IN_HOUR);
        g_sAppData.ulElapsedSec += (ucGmtDiffMins * SEC_IN_MIN);

        g_sAppData.pcCCPtr = &g_sAppData.acTimeStore[0];

        //
        // day, number of days since beginning of 2013
        //
        g_sAppData.isGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        memcpy(g_sAppData.pcCCPtr,
               g_acDaysOfWeek2013[g_sAppData.isGeneralVar%7], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // month
        //
        g_sAppData.isGeneralVar %= 365;
        for (iIndex = 0; iIndex < 12; iIndex++)
        {
            g_sAppData.isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
            if (g_sAppData.isGeneralVar < 0)
                    break;
        }
        if(iIndex == 12)
        {
            iIndex = 0;
        }
        memcpy(g_sAppData.pcCCPtr, g_acMonthOfYear[iIndex], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        // Set the Month Value
        dateTime.sl_tm_mon = iIndex + 1;

        //
        // date
        // restore the day in current month
        //
        g_sAppData.isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
        g_sAppData.uisCCLen = itoa(g_sAppData.isGeneralVar + 1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        // Set the Date
        dateTime.sl_tm_day = g_sAppData.isGeneralVar + 1;

        //
        // time
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec%SEC_IN_DAY;

        // number of seconds per hour
        g_sAppData.ulGeneralVar1 = g_sAppData.ulGeneralVar%SEC_IN_HOUR;

        // number of hours
        g_sAppData.ulGeneralVar /= SEC_IN_HOUR;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';

        // Set the hour
        dateTime.sl_tm_hour = g_sAppData.ulGeneralVar;

        // number of minutes per hour
        g_sAppData.ulGeneralVar = g_sAppData.ulGeneralVar1/SEC_IN_MIN;

        // Set the minutes
        dateTime.sl_tm_min = g_sAppData.ulGeneralVar;

        // number of seconds per minute
        g_sAppData.ulGeneralVar1 %= SEC_IN_MIN;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        //Set the seconds
        dateTime.sl_tm_sec = g_sAppData.ulGeneralVar1;

        //
        // year
        // number of days since beginning of 2013
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        g_sAppData.ulGeneralVar /= 365;
        g_sAppData.uisCCLen = itoa(YEAR2013 + g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;

        *g_sAppData.pcCCPtr++ = '\0';

        //Set the year
        dateTime.sl_tm_year = 2013 + g_sAppData.ulGeneralVar;

        UART_PRINT("response from server: ");
        UART_PRINT((char *)g_acSNTPserver);
        UART_PRINT("\n\r");
        UART_PRINT(g_sAppData.acTimeStore);
        UART_PRINT("\n\r\n\r");

        //Set time of the device for certificate verification.
        lRetVal = setDeviceTimeDate();
        if(lRetVal < 0)
        {
            UART_PRINT("Unable to set time in the device");
            errorcounttime++;
            return lRetVal;
        }

//        //
//        // Send the JSON packet with temperature to Event Hub
//		lRetVal = PostEventHubSSL();
//		if(lRetVal < 0)
//		{
//			ERR_PRINT(lRetVal);
//			errorcounttime++;
//			//break;
//		}

    }

    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
    return SUCCESS;
}
//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void
TimerPeriodicIntHandler(void)
{
    unsigned long ulInts;

    //
    // Clear all pending interrupts from the timer we are
    // currently using.
    //
    ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
    MAP_TimerIntClear(TIMERA0_BASE, ulInts);

    //
    // Increment our interrupt counter.
    //
    g_usTimerInts++;
    if(!(g_usTimerInts & 0x1))
    {
        //
        // Off Led
        //
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    }
    else
    {
        //
        // On Led
        //
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    }
}

//****************************************************************************
//
//! Function to configure and start timer to blink the LED while device is
//! trying to connect to an AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerConfigNStart()
{
    //
    // Configure Timer for blinking the LED for IP acquisition
    //
    Timer_IF_Init(PRCM_TIMERA0,TIMERA0_BASE,TIMER_CFG_PERIODIC,TIMER_A,0);
    Timer_IF_IntSetup(TIMERA0_BASE,TIMER_A,TimerPeriodicIntHandler);
    Timer_IF_Start(TIMERA0_BASE,TIMER_A,PERIODIC_TEST_CYCLES / 10);
}

//****************************************************************************
//
//! Disable the LED blinking Timer as Device is connected to AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerDeinitStop()
{
    //
    // Disable the LED blinking Timer as Device is connected to AP
    //
    Timer_IF_Stop(TIMERA0_BASE,TIMER_A);
    Timer_IF_DeInit(TIMERA0_BASE,TIMER_A);
}


//*****************************************************************************
//
//! This function demonstrates how certificate can be used with SSL.
//! The procedure includes the following steps:
//! 1) connect to an open AP
//! 2) get the server name via a DNS request
//! 3) define all socket options and point to the CA certificate
//! 4) connect to the server via TCP
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  LED1 is turned solid in case of success
//!    LED2 is turned solid in case of failure
//!
//*****************************************************************************
long PostEventHubSSL()
{
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1; //SL_SO_SEC_METHOD_SSLV3;
    unsigned int uiIP, uiCipher = SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA;
    long lRetVal = -1;
    int iSSLSockID;
    char acSendBuff[512];
    char acRecvbuff[1460];
    //char* pcBufData;
    char* pcBufHeaders;
    float fCurrentTemp;

//    //Set time of the device for certificate verification.
//    lRetVal = set_time();
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Unable to set time in the device");
//        return lRetVal;
//    }

    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't retrive the host name \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(SSL_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);
    //
    // opens a secure socket
    //
    iSSLSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSSLSockID < 0 )
    {
        UART_PRINT("Device unable to create secure socket \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    //
    // configure the socket as SSLV3.0
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,\
                               sizeof(ucMethod));
    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }
    //
    //configure the socket as RSA with RC4 128 SHA
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,\
                           sizeof(uiCipher));
    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    //
    //configure the socket with Azure CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
                           SL_SSL_CA_CERT_FILE_NAME, \
                           strlen(SL_SSL_CA_CERT_FILE_NAME));

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't set socket options \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    /* connect to the peer device - Azure server */
    lRetVal = sl_Connect(iSSLSockID, ( SlSockAddr_t *)&Addr, iAddrSize);

    if(lRetVal < 0)
    {
        UART_PRINT("Device couldn't connect to Azure server \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    //
    // Obtains the MCU temperature
    //
	TMP006DrvGetTemp(&fCurrentTemp);
    //float cCurrentTemp = 21.27;
    float cCurrentTemp = (((fCurrentTemp - 32) * 5) / 9) - 15; // Will need to calibrate temperature sensor by adding or subtracting

    char  cTempChar[5];
    sprintf(cTempChar, "%.2f", cCurrentTemp);

    //
    // Puts together the HTTP POST string.
    //

    int dataLength = strlen(DATA1) + 5 + strlen(DATA2);
    char cCLLength[4];

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, AUTHHEADER);
    pcBufHeaders += strlen(AUTHHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATA1);
    pcBufHeaders += strlen(DATA1);
    strcpy(pcBufHeaders , cTempChar);
    pcBufHeaders += strlen(cTempChar);
    strcpy(pcBufHeaders, DATA2);

    int testDataLength = strlen(pcBufHeaders);

    /* send the packet to the server */
    lRetVal = sl_Send(iSSLSockID, acSendBuff, strlen(acSendBuff), 0);

    if(lRetVal < 0)
    {
        UART_PRINT("Post failed \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
    }

    lRetVal = sl_Recv(iSSLSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);

	if(lRetVal < 0)
	{
        UART_PRINT("Received failed \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        errorcountpost++;
        return lRetVal;
	}
	else
	{
		DBG_PRINT("Received HTTP Post response data. \n\r");
	}

    sl_Close(iSSLSockID);

    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return SUCCESS;
}


//****************************************************************************
//
//! Task function implementing the gettime functionality using an NTP server
//!
//! \param none
//!
//! This function
//!    1. Initializes the required peripherals
//!    2. Initializes network driver and connects to the default AP
//!    3. Creates a UDP socket, gets the NTP server IP address using DNS
//!    4. Periodically gets the NTP time and displays the time
//!
//! \return None.
//
//****************************************************************************
void MainTask(void *pvParameters)
{
    int iSocketDesc;
    long lRetVal = -1;

    UART_PRINT("GET_TIME: Test Begin\n\r");

    //
    // Configure LED
    //
    GPIO_IF_LedConfigure(LED1|LED2|LED3);

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);    


    //
    // Reset The state of the machine
    //
    Network_IF_ResetMCUStateMachine();

    //
    // Start the driver
    //
    lRetVal = Network_IF_InitDriver(ROLE_STA);
    if(lRetVal < 0)
    {
       UART_PRINT("Failed to start SimpleLink Device\n\r",lRetVal);
       LOOP_FOREVER();
    }

    // switch on Green LED to indicate Simplelink is properly up
    GPIO_IF_LedOn(MCU_ON_IND);

    // Start Timer to blink Red LED till AP connection
    LedTimerConfigNStart();

    // Initialize AP security params
    SecurityParams.Key = (signed char *)SECURITY_KEY;
    SecurityParams.KeyLen = strlen(SECURITY_KEY);
    SecurityParams.Type = SECURITY_TYPE;

    //
    // Connect to the Access Point
    //
    lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);
    if(lRetVal < 0)
    {
       UART_PRINT("Connection to an AP failed\n\r");
       LOOP_FOREVER();
    }

    //
    // Disable the LED blinking Timer as Device is connected to AP
    //
    LedTimerDeinitStop();

    //
    // Switch ON RED LED to indicate that Device acquired an IP
    //
    GPIO_IF_LedOn(MCU_IP_ALLOC_IND);

    //
    // Create UDP socket
    //
    iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(iSocketDesc < 0)
    {
        ERR_PRINT(iSocketDesc);
        goto end;
    }
    g_sAppData.iSockID = iSocketDesc;

    UART_PRINT("Socket created\n\r");

    //
    // Get the NTP server host IP address using the DNS lookup
    //
    lRetVal = Network_IF_GetHostIP((char*)g_acSNTPserver, \
                                    &g_sAppData.ulDestinationIP);
    
    if( lRetVal >= 0)
    {
    
        struct SlTimeval_t timeVal;
        timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
        timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
        lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                        (unsigned char*)&timeVal, sizeof(timeVal)); 
        if(lRetVal < 0)
        {
           ERR_PRINT(lRetVal);
           LOOP_FOREVER();
        }
        
        ////////////////////////////////////

        //
        // Create UDP socket - Perhaps move to seperate function
        //
        iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(iSocketDesc < 0)
        {
            ERR_PRINT(iSocketDesc);
            //goto end;
        }
        g_sAppData.iSockID = iSocketDesc;

        UART_PRINT("Socket created\n\r");

        //
        // Get the NTP server host IP address using the DNS lookup
        //
        lRetVal = Network_IF_GetHostIP((char*)g_acSNTPserver, \
                                        &g_sAppData.ulDestinationIP);

        if( lRetVal >= 0)
        {

            struct SlTimeval_t timeVal;
            timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
            timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
            lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                            (unsigned char*)&timeVal, sizeof(timeVal));
            if(lRetVal < 0)
            {
               ERR_PRINT(lRetVal);
               LOOP_FOREVER();
            }
        }

        ///////////////////////////////////

        while(1)
        {
            //
            // Get the NTP time to use with the SSL process
        	// Only call this every 4 hours to update the RTC
        	// As we do not want to be calling the NTP server too often
            //
        	getDeviceTimeDate();
        	timeDifference = dateTime.sl_tm_hour - hourSet; // This roughly works, after midnight it resets.

        	if (timeDifference > 1 || timeDifference < 0)
        	{
        		hourSet = dateTime.sl_tm_hour; // Set to current hour

				lRetVal = GetSNTPTime(GMT_DIFF_TIME_HRS, GMT_DIFF_TIME_MINS);
				if(lRetVal < 0)
				{
					UART_PRINT("Server Get Time failed\n\r");
					//break;
					errorcount++;
					lasterrornumber = lRetVal;
				}
        	}

            //
            // Send the JSON packet with temperature to Event Hub
    		lRetVal = PostEventHubSSL();
    		if(lRetVal < 0)
    		{
    			ERR_PRINT(lRetVal);
    			errorcounttime++;
    			//break;
    		}

            //
            // Wait a while before resuming
            //
            MAP_UtilsDelay(SEC_TO_LOOP(SLEEP_TIME));
        }
    }
    else
    {
        UART_PRINT("DNS lookup failed. \n\r");
    }

    //
    // Close the socket
    //
    close(iSocketDesc);
    UART_PRINT("Socket closed\n\r");

end:
    //
    // Stop the driver
    //
//    lRetVal = Network_IF_DeInitDriver();
//    if(lRetVal < 0)
//    {
//       UART_PRINT("Failed to stop SimpleLink Device\n\r");
//       LOOP_FOREVER();
//    }

    //
    // Switch Off RED & Green LEDs to indicate that Device is
    // disconnected from AP and Simplelink is shutdown
    //
    GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    UART_PRINT("GET_TIME: Test Complete\n\r");

    //
    // Loop here
    //
    LOOP_FOREVER();
}


//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{

    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t      CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS

    //
    // Set vector table base
    //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif

    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//
//! Main function
//!
//! \param none
//!
//! This function
//!    1. Invokes the SLHost task
//!    2. Invokes the GetNTPTimeTask
//!
//! \return None.
//
//****************************************************************************
void main()
{
    long lRetVal = -1;

    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Enable and configure DMA
    //
    UDMAInit();

    //
    // Pinmux for UART
    //
    PinMuxConfig();

    //
    // Configuring UART
    //
    InitTerm();

    //
    // I2C Init
    //
    lRetVal = I2C_IF_Open(I2C_MASTER_MODE_FST);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //Init Temprature Sensor
    lRetVal = TMP006DrvOpen();
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Display Application Banner
    //
    DisplayBanner(APPLICATION_NAME);

    //
    // Start the SimpleLink Host
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the GetNTPTime task
    //
    lRetVal = osi_TaskCreate(MainTask,
                    (const signed char *)"Main Task",
                    OSI_STACK_SIZE,
                    NULL,
                    1,
                    NULL );

    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //
    osi_start();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
