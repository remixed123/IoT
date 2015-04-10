//*****************************************************************************
//
// Application Name        - MSP432_Azure
// Application Version     - 1.0.0
// Application Modify Date - 9th of April 2015
// Application Developer   - Glenn Vassallo
// Application Contact	   - contact@swiftsoftware.com.au
// Application Repository  - https://github.com/remixed123/IoT
// Application Overview    - This is a sample application demonstrating the
//                           sending of telemetry (temperature data) from the
//                           MSP432 LaunchPad and the CC3100 BoosterPack
//							 to Microsoft Azure Event Hub by utilising
//                           Event Hubs REST API over SSL/TLS. The application
//                           also retrieves the time from a SNTP server
//                           periodically, this helps to ensure that
//                           SSL/TLS connection will function.
// Application History     - The MSP432_Azure example code has been created
//                           through modifying and including code from the SSL
//                           and Get_TIME examples that are provided with the
//                           CC3100 SDK - http://www.ti.com/tool/cc3100sdk
// Application Details     - https://github.com/remixed123/IoT/wiki
//
//*****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "driverlib.h"
#include "simplelinklibrary.h"
#include "simplelink.h"
#include "sl_common.h"

/*
 * Values for below macros shall be modified per the access-point's (AP) properties
 * SimpleLink device will connect to following AP when the application is executed
 */
#define SSID_NAME       "YourSSID"       /* Access point name to connect to. */
#define SEC_TYPE        SL_SEC_TYPE_WPA_WPA2     /* Security type of the Access piont */
#define PASSKEY         "YourPasscode"   /* Password in case of secure AP */
#define PASSKEY_LEN     pal_Strlen(PASSKEY)      /* Password length in case of secure AP */

#define APPLICATION_VERSION "1.0.0"

#define SL_STOP_TIMEOUT        0xFF

//*****************************************************************************
//                       APPLICATION DETAIL DEFINES
//*****************************************************************************

#define APPLICATION_NAME        "MSP432_Azure"
//#define APPLICATION_VERSION     "1.5.1"

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
#define SLEEP_TIME            30 //This is in seconds (approximately), change this to set your delay time

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
#define DATA1 "{\"MessageType\":\"MSP432 Sensor\",\"Temp\":"
#define DATA2 ",\"Humidity\":50,\"Location\":\"Sydney, Australia\",\"Room\":\"Kitchen\",\"Info\":\"Sent from MSP432 LaunchPad\"}"

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    HTTP_SEND_ERROR = DEVICE_NOT_IN_STATION_MODE - 1,
    HTTP_RECV_ERROR = HTTP_SEND_ERROR - 1,
    HTTP_INVALID_RESPONSE = HTTP_RECV_ERROR -1,
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//#define min(X,Y) ((X) < (Y) ? (X) : (Y))


/*
 * GLOBAL VARIABLES -- Start
 */
/* Button debounce state variables */
volatile unsigned int S1buttonDebounce = 0;
volatile unsigned int S2buttonDebounce = 0;
volatile int publishID = 0;

//char macStr[18];        // Formatted MAC Address String
//char uniqueID[9];       // Unique ID generated from TLV RAND NUM and MAC Address

_u32  g_Status = 0;

signed char    *g_Host = EH_SERVER_NAME;

//*****************************************************************************
// Globals for Date and Time
//*****************************************************************************
SlDateTime_t dateTime =  {0};

//*****************************************************************************
// Globals for time difference calculations
//*****************************************************************************
int hourSet = 25; // Ensures the timeDifference calc will end up in a minus, so will enter the function on first pass or SNTP error
int timeDifference = 0;

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
//!    ##   Or search Internet for SNTP servers in your region
//!    ###################################################################################
const char g_acSNTPserver[30] = "tic.ntp.telstra.net"; //Add any SNTP server

// Tuesday is the 1st day in 2013 - the relative year
const char g_acDaysOfWeek2013[7][3] = {{"Tue"},{"Wed"},{"Thu"},{"Fri"},{"Sat"},{"Sun"},{"Mon"}};
const char g_acMonthOfYear[12][3] = {{"Jan"},{"Feb"},{"Mar"},{"Apr"},{"May"},{"Jun"},{"Jul"},{"Aug"},{"Sep"},{"Oct"},{"Nov"},{"Dec"}};
const char g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
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



/*
 * GLOBAL VARIABLES -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 establishConnectionWithAP();
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static void displayBanner();
long GetCurrentTime();
long PostEventHubSSL();



/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

//*****************************************************************************
//
//! itoa
//!
//!    @brief  Convert integer to ASCII in decimal base
//!
//!     @param  cNum is input integer number to convert
//!     @param  cString is output string
//!
//!     @return number of ASCII parameters
//!
//!
//
//*****************************************************************************
unsigned short itoa(short cNum, char *cString)
{
    char* ptr;
    short uTemp = cNum;
    unsigned short length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = g_acDigits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

//*****************************************************************************
//
//! getHostIP
//!
//! \brief  This function obtains the server IP address using a DNS lookup
//!
//! \param[in]  pcHostName        The server hostname
//! \param[out] pDestinationIP    This parameter is filled with host IP address.
//!
//! \return On success, +ve value is returned. On error, -ve value is returned
//!
//
//*****************************************************************************
long getHostIP(char* pcHostName, unsigned long * pDestinationIP)
{
    long lStatus = 0;

    lStatus = sl_NetAppDnsGetHostByName((signed char *) pcHostName,
                                            strlen(pcHostName),
                                            pDestinationIP, SL_AF_INET);
    return lStatus;
}

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
    // Send a query to the NTP server to get the NTP time
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

    lRetVal = sl_SendTo(g_sAppData.iSockID, cDataBuf, sizeof(cDataBuf), 0, &sAddr, sizeof(sAddr));
    if (lRetVal != sizeof(cDataBuf))
    {
        // could not send SNTP request
        hourSet = 25; // This will ensure we try to fetch time again
        //ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);
    }

    //
    // Wait to receive the NTP time from the server
    //
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = 0;
    sLocalAddr.sin_addr.s_addr = 0;
    if(g_sAppData.ulElapsedSec == 0)
    {
        lRetVal = sl_Bind(g_sAppData.iSockID, (SlSockAddr_t *)&sLocalAddr, sizeof(SlSockAddrIn_t));
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_sAppData.iSockID, cDataBuf, sizeof(cDataBuf), 0, (SlSockAddr_t *)&sLocalAddr, (SlSocklen_t*)&iAddrSize);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Confirm that the MODE is 4 --> server
    //
    if ((cDataBuf[0] & 0x7) != 4)    // expect only server response
    {
        hourSet = 25; // This will ensure we try to fetch time again
        //ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);  // MODE is not server, abort
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
        memcpy(g_sAppData.pcCCPtr, g_acDaysOfWeek2013[g_sAppData.isGeneralVar%7], 3);
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
        g_sAppData.uisCCLen = itoa(g_sAppData.isGeneralVar + 1, g_sAppData.pcCCPtr);
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
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar, g_sAppData.pcCCPtr);
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
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar, g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar1, g_sAppData.pcCCPtr);
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
        g_sAppData.uisCCLen = itoa(YEAR2013 + g_sAppData.ulGeneralVar, g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;

        *g_sAppData.pcCCPtr++ = '\0';

        //Set the year
        dateTime.sl_tm_year = 2013 + g_sAppData.ulGeneralVar;

        CLI_Write("Response from server: ");
        CLI_Write((unsigned char *)g_acSNTPserver);
        CLI_Write("\n\r");
        CLI_Write((unsigned char *)g_sAppData.acTimeStore);
        CLI_Write("\n\r\n\r");

        //Set time of the device for certificate verification.
        lRetVal = setDeviceTimeDate();
        if(lRetVal < 0)
        {
            CLI_Write("Unable to set time in the device.\n\r");
            return lRetVal;
        }
    }

    return SUCCESS;
}

//*****************************************************************************
//
//! This function obtains the current time from a SNTP server if required due
//! to not having current time (when booting up) or periodically to update
//! the time
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  Error Number of failure
//
//*****************************************************************************
long GetCurrentTime()
{
    int iSocketDesc;
    long lRetVal = -1;

	//
	// Get the time and date currently stored in the RTC
	//
	getDeviceTimeDate();

	//
	// Calculate time difference between the last time we obtained time from a NTP server
	//
	timeDifference = dateTime.sl_tm_hour - hourSet; // This roughly works, it does however reset after midnight.

	//
	// Get the NTP time to use with the SSL process. Only call this every 6 hours to update the RTC
	// As we do not want to be calling the NTP server too often
	//
	if (timeDifference > 6 || timeDifference < 0)
	{
		//
		// Create UDP socket
		//
		iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(iSocketDesc < 0)
		{
			CLI_Write("Could not create UDP socket.\n\r");
			close(iSocketDesc);
			return iSocketDesc;
		}
		else
		{
			CLI_Write("Socket successfully created\n\r");
		}
		g_sAppData.iSockID = iSocketDesc;

		//
		// Get the NTP server host IP address using the DNS lookup
		//
		lRetVal = getHostIP((char*)g_acSNTPserver, &g_sAppData.ulDestinationIP);
		if( lRetVal >= 0)
		{
			//
			// Configure the recieve timeout
			//
			struct SlTimeval_t timeVal;
			timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
			timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
			lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO, (unsigned char*)&timeVal, sizeof(timeVal));
			if(lRetVal < 0)
			{
			   CLI_Write("Could not configure socket option (receive timeout).\n\r");
			   close(iSocketDesc);
			   return lRetVal;
			}
		}
		else
		{
			CLI_Write("DNS lookup failed.");
		}

		//
		// Get current time from the SNTP server
		//
		CLI_Write("Fetching Time From SNTP Server\n\r");
		lRetVal = GetSNTPTime(GMT_DIFF_TIME_HRS, GMT_DIFF_TIME_MINS);
		if(lRetVal < 0)
		{
			CLI_Write("Server Get Time failed.\n\r");
			close(iSocketDesc);
			return lRetVal;
		}
		else
		{
			hourSet = dateTime.sl_tm_hour; // Set to current hour as we did get the time successfully
			CLI_Write("Server Get Time Successful\n\n\r");
		}

		//
		// Close the socket
		//
		close(iSocketDesc);
	}

	return 0;
}

//*****************************************************************************
//
//! This function POST to Event Hub REST API using TLS
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  Error number on Failure
//
//*****************************************************************************
long PostEventHubSSL()
{
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char ucMethod = SL_SO_SEC_METHOD_TLSV1; //SL_SO_SEC_METHOD_SSLv3_TLSV1_2; //SL_SO_SEC_METHOD_TLSV1_2; //SL_SO_SEC_METHOD_SSLV3;
    unsigned int uiIP;
    unsigned int uiCipher = SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA;
    long lRetVal = -1;
    int iSSLSockID;
    char acSendBuff[512];
    char acRecvbuff[1460];
    //char* pcBufData;
    char* pcBufHeaders;

    //
    // Retrieve IP from Hostname
    //
    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host), (unsigned long*)&uiIP, SL_AF_INET);
    if(lRetVal < 0)
    {
        CLI_Write("Could not retrive the IP Address for Azure Server.\n\r");
        //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(SSL_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);

    //
    // Opens a secure socket
    //
    iSSLSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSSLSockID < 0 )
    {
        CLI_Write("Unable to create secure socket.\n\r");
        //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //
    // Configure the socket as TLS (SSLV3.0 does not work because of POODLE - http://en.wikipedia.org/wiki/POODLE)
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod, sizeof(ucMethod));
    if(lRetVal < 0)
    {
        CLI_Write("Couldn't set socket option (TLS).\n\r");
        sl_Close(iSSLSockID);
        //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    //
    // Configure the socket as RSA with RC4 128 SHA
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher, sizeof(uiCipher));
    if(lRetVal < 0)
    {
        CLI_Write("Couldn't set socket option (RSA).\n\r");
        sl_Close(iSSLSockID);
        return lRetVal;
    }

    //
    // Configure the socket with Azure CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME, SL_SSL_CA_CERT_FILE_NAME, strlen(SL_SSL_CA_CERT_FILE_NAME));
    if(lRetVal < 0)
    {
        CLI_Write("Couldn't set socket option (CA Certificate).\n\r");
        sl_Close(iSSLSockID);
        return lRetVal;
    }

    //
    // Configure the recieve timeout
    //
    struct SlTimeval_t timeVal;
    timeVal.tv_sec = SERVER_RESPONSE_TIMEOUT;    // In Seconds
    timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
    lRetVal = sl_SetSockOpt(iSSLSockID, SL_SOL_SOCKET, SL_SO_RCVTIMEO, (unsigned char*)&timeVal, sizeof(timeVal));
    if(lRetVal < 0)
    {
       CLI_Write("Couldn't set socket option (Receive Timeout).\n\r");
       sl_Close(iSSLSockID);
       return lRetVal;
    }

    //
    // Connect to the peer device - Azure server */
    //
    lRetVal = sl_Connect(iSSLSockID, ( SlSockAddr_t *)&Addr, iAddrSize);
    if(lRetVal < 0)
    {
        CLI_Write("Couldn't connect to Azure server.\n\r");
        sl_Close(iSSLSockID);
        //GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    // Set temperature, you could have a sensor providing this.
    char  cTempChar[5] = "24.56";
    //sprintf(cTempChar, "%.2f", cCurrentTemp);

    //
    // Creates the HTTP POST string.
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

    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iSSLSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0)
    {
        CLI_Write("POST failed.\n\r");
        sl_Close(iSSLSockID);
        return lRetVal;
    }

    //
    // Receive response packet from the server */
    //
    lRetVal = sl_Recv(iSSLSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
	if(lRetVal < 0)
	{
        CLI_Write("Received failed.\n\r");
        sl_Close(iSSLSockID);
        return lRetVal;
	}
	else
	{
		CLI_Write("HTTP POST Successful. Telemetry successfully logged\n\r");
	}

    sl_Close(iSSLSockID);

    return SUCCESS;
}

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */

/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
        CLI_Write(" [WLAN EVENT] NULL Pointer Error \n\r");
    
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

            /*
             * Information about the connected AP (like name, MAC etc) will be
             * available in 'slWlanConnectAsyncResponse_t' - Applications
             * can use it if required
             *
             * slWlanConnectAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
             *
             */
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                CLI_Write(" Device disconnected from the AP on application's request \n\r");
            }
            else
            {
                CLI_Write(" Device disconnected from the AP on an ERROR..!! \n\r");
            }
        }
        break;

        default:
        {
            CLI_Write(" [WLAN EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles events for IP address acquisition via DHCP
           indication

    \param[in]      pNetAppEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
        CLI_Write(" [NETAPP EVENT] NULL Pointer Error \n\r");
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            /*
             * Information about the connected AP's IP, gateway, DNS etc
             * will be available in 'SlIpV4AcquiredAsync_t' - Applications
             * can use it if required
             *
             * SlIpV4AcquiredAsync_t *pEventData = NULL;
             * pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
             * <gateway_ip> = pEventData->gateway;
             *
             */
        }
        break;

        default:
        {
            CLI_Write(" [NETAPP EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles callback for the HTTP server events

    \param[in]      pHttpEvent - Contains the relevant event information
    \param[in]      pHttpResponse - Should be filled by the user with the
                    relevant response information

    \return         None

    \note

    \warning
*/
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    /*
     * This application doesn't work with HTTP server - Hence these
     * events are not handled here
     */
    CLI_Write(" [HTTP EVENT] Unexpected event \n\r");
}

/*!
    \brief This function handles general error events indication

    \param[in]      pDevEvent is the event passed to the handler

    \return         None
*/
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /*
     * Most of the general errors are not FATAL are are to be handled
     * appropriately by the application
     */
    CLI_Write(" [GENERAL EVENT] \n\r");
}

/*!
    \brief This function handles socket events indication

    \param[in]      pSock is the event passed to the handler

    \return         None
*/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
        CLI_Write(" [SOCK EVENT] NULL Pointer Error \n\r");

    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
        {
            /*
            * TX Failed
            *
            * Information about the socket descriptor and status will be
            * available in 'SlSockEventData_t' - Applications can use it if
            * required
            *
            * SlSockEventData_t *pEventData = NULL;
            * pEventData = & pSock->EventData;
            */
            switch( pSock->EventData.status )
            {
                case SL_ECLOSE:
                    CLI_Write(" [SOCK EVENT] Close socket operation failed to transmit all queued packets\n\r");
                break;


                default:
                    CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
                break;
            }
        }
        break;

        default:
            CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
        break;
    }
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */


/*
 * Application's entry point
 */
int main(int argc, char** argv)
{
    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    initClk();


    /* Configure command line interface */
    CLI_Configure();

    displayBanner();

    /*
     * Following function configures the device to default state by cleaning
     * the persistent settings stored in NVMEM (viz. connection profiles &
     * policies, power policy etc)
     *
     * Applications may choose to skip this step if the developer is sure
     * that the device is in its default state at start of application
     *
     * Note that all profiles and persistent settings that were done on the
     * device will be lost
     */
    retVal = configureSimpleLinkToDefaultState();
    if(retVal < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == retVal)
            CLI_Write(" Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }

    CLI_Write(" Device is configured in default state \n\r");

    /*
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        CLI_Write(" Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device started as STATION \n\r");

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP();
    if(retVal < 0)
    {
        CLI_Write(" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ AP and IP is acquired \n\r");

    //
    // Main Program Loop
	//
    while(1)
	{

		//
		// Get the current time from an SNTP server
		//
    	retVal = GetCurrentTime();
		if(retVal < 0)
		{
			CLI_Write(" Failed to GetCurrentTime.\n\r");
		}

		//
		// Send the JSON packet with temperature to Event Hub
		//
		retVal = PostEventHubSSL();
		if(retVal < 0)
		{
			CLI_Write(" Failed to PostEventHubSSL.\n\r");
		}

		//
		// Wait a while before resuming.
		// This is an estimate, for more accuracy other methods should be implemented.
		//
		Delay(SLEEP_TIME);
	}
}

/*!
    \brief This function configure the SimpleLink device in its default state. It:
           - Sets the mode to STATION
           - Configures connection policy to Auto and AutoSmartConfig
           - Deletes all the stored profiles
           - Enables DHCP
           - Disables Scan policy
           - Sets Tx power to maximum
           - Sets power policy to normal
           - Unregisters mDNS services
           - Remove all filters

    \param[in]      none

    \return         On success, zero is returned. On error, negative is returned
*/
static _i32 configureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    _u8           val = 1;
    _u8           configOpt = 0;
    _u8           configLen = 0;
    _u8           power = 0;

    _i32          retVal = -1;
    _i32          mode = -1;

    mode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(mode);

    /* If the device is not in station-mode, try configuring it in station-mode */
    if (ROLE_STA != mode)
    {
        if (ROLE_AP == mode)
        {
            /* If the device is in AP mode, we need to wait for this event before doing anything */
            while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
        }

        /* Switch to STA role and restart */
        retVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(retVal);

        /* Check if the device is in station again */
        if (ROLE_STA != retVal)
        {
            /* We don't want to proceed if the device is not coming up in station-mode */
            ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
        }
    }

    /* Get the device's version-information */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (_u8 *)(&ver));
    ASSERT_ON_ERROR(retVal);

    /* Set connection policy to Auto + SmartConfig (Device's default connection policy) */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove all profiles */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);

    /*
     * Device in station-mode. Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if(0 == retVal)
    {
        /* Wait */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }

    /* Enable DHCP client*/
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    ASSERT_ON_ERROR(retVal);

    /* Disable scan */
    configOpt = SL_SCAN_POLICY(0);
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Set Tx power level for station mode
       Number between 0-15, as dB offset from max power - 0 will set maximum power */
    power = 0;
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
    ASSERT_ON_ERROR(retVal);

    /* Set PM policy to normal */
    retVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Unregister mDNS services */
    retVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove  all 64 filters (8*8) */
    pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    retVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(retVal);

    retVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(retVal);

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    return retVal; /* Success */
}

/*!
    \brief Connecting to a WLAN Access point

    This function connects to the required AP (SSID_NAME).
    The function will return once we are connected and have acquired IP address

    \param[in]  None

    \return     0 on success, negative error-code on error

    \note

    \warning    If the WLAN connection fails or we don't acquire an IP address,
                We will be stuck in this function forever.
*/
static _i32 establishConnectionWithAP()
{
    SlSecParams_t secParams = {0};
    _i32 retVal = 0;

    secParams.Key = PASSKEY;
    secParams.KeyLen = PASSKEY_LEN;
    secParams.Type = SEC_TYPE;

    retVal = sl_WlanConnect(SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) { _SlNonOsMainLoopTask(); }

    return SUCCESS;
}

/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    //pal_Memset(&g_AppData, 0, sizeof(g_AppData));

    return SUCCESS;
}

/*!
    \brief This function displays the application's banner

    \param      None

    \return     None
*/
static void displayBanner()
{
    CLI_Write("\n\r\n\r");
    CLI_Write("MSP432 - CC3100 - Azure Event Hub Example ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
