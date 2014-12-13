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

//*****************************************************************************
//
// Application Name     - Get Weather
// Application Overview - Get Weather application connects to "openweathermap.org"
//                        server, requests for weather details of the specified
//                        city, process data and displays it on the Hyperterminal.
//
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Info_Center_Get_Weather_Application
// or
// docs\examples\CC32xx_Info_Center_Get_Weather_Application.pdf
//
//*****************************************************************************


//****************************************************************************
//
//! \addtogroup get_weather
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// simplelink includes
#include "device.h"

// driverlib includes
#include "hw_memmap.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"
#include "pinmux.h"

//Free_rtos/ti-rtos includes
#include "osi.h"

// common interface includes
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "gpio_if.h"
#include "timer_if.h"
#include "network_if.h"
#include "udma_if.h"
#include "common.h"


//****************************************************************************
//                          LOCAL DEFINES                                   
//****************************************************************************
#define APPLICATION_VERSION     "1.1.0"
#define APP_NAME                "Get Weather"

#define SERVER_RESPONSE_TIMEOUT 10
#define SLEEP_TIME              8000000
#define SUCCESS                 0
#define OSI_STACK_SIZE          3000

#define PREFIX_BUFFER "GET /data/2.5/weather?q="
#define POST_BUFFER "&mode=xml&units=imperial HTTP/1.1\r\nHost: api.openweathermap.org\r\nAccept: */"
#define POST_BUFFER2 "*\r\n\r\n"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SERVER_GET_WEATHER_FAILED = -0x7D0,
    WRONG_CITY_NAME = SERVER_GET_WEATHER_FAILED - 1,
    NO_WEATHER_DATA = WRONG_CITY_NAME - 1,
    DNS_LOOPUP_FAILED = WRONG_CITY_NAME  -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

unsigned long g_ulTimerInts;   //  Variable used in Timer Interrupt Handler
SlSecParams_t SecurityParams = {0};  // AP Security Parameters
const char g_ServerAddress[30] = "openweathermap.org"; // Weather info provider server

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//****************************************************************************
static int CreateConnection(unsigned long ulDestinationIP);
static long GetWeather(int iSockID, char *pcCityName);
void GetWeatherTask(void *pvParameters);
static void BoardInit();
static void DisplayBanner(char * AppName);


//*****************************************************************************
//
//! CreateConnection
//!
//! \brief  Creating an endpoint for TCP communication and initiating 
//!         connection on socket
//!
//! \param  The server hostname
//!
//! \return SocketID on Success or < 0 on Failure.        
//!
//
//*****************************************************************************
static int CreateConnection(unsigned long ulDestinationIP)
{
    int iLenorError;
    SlSockAddrIn_t  sAddr;
    int iAddrSize;
    int iSockIDorError = 0;

    sAddr.sin_family = SL_AF_INET;
    sAddr.sin_port = sl_Htons(80);

    //Change the DestinationIP endianity , to big endian
    sAddr.sin_addr.s_addr = sl_Htonl(ulDestinationIP);

    iAddrSize = sizeof(SlSockAddrIn_t);

    iSockIDorError = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    ASSERT_ON_ERROR(iSockIDorError);

    iLenorError = sl_Connect(iSockIDorError, ( SlSockAddr_t *)&sAddr, iAddrSize);
    ASSERT_ON_ERROR(iLenorError);

    DBG_PRINT("Socket Id: %d was created.",iSockIDorError);

    return iSockIDorError;//success, connection created
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
    g_ulTimerInts++;
    if(!(g_ulTimerInts & 0x1))
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
//! GetWeather
//!
//! \brief  Obtaining the weather info for the specified city from the server
//!
//! \param  iSockID is the socket ID
//! \param  pcCityName is the pointer to the name of the city
//!
//! \return none.        
//!
//
//*****************************************************************************
static long GetWeather(int iSockID, char *pcCityName)
{
    int iTXStatus;
    int iRXDataStatus;
    char *pcIndxPtr;
    char *pcEndPtr;
    char acSendBuff[512];
    char acRecvbuff[1460];
    char* pcBufLocation;

    memset(acRecvbuff, 0, sizeof(acRecvbuff));

    //
    // Puts together the HTTP GET string.
    //
    pcBufLocation = acSendBuff;
    strcpy(pcBufLocation, PREFIX_BUFFER);
    pcBufLocation += strlen(PREFIX_BUFFER);
    strcpy(pcBufLocation , pcCityName);
    pcBufLocation += strlen(pcCityName);
    strcpy(pcBufLocation, POST_BUFFER);
    pcBufLocation += strlen(POST_BUFFER);
    strcpy(pcBufLocation, POST_BUFFER2);

    //
    // Send the HTTP GET string to the open TCP/IP socket.
    //
    iTXStatus = sl_Send(iSockID, acSendBuff, strlen(acSendBuff), 0);
    if(iTXStatus < 0)
    {
        ASSERT_ON_ERROR(SERVER_GET_WEATHER_FAILED);
    }
    else
    {
        DBG_PRINT("Sent HTTP GET request. \n\r");
    }

    DBG_PRINT("Return value: %d \n\r", iTXStatus);

    //
    // Store the reply from the server in buffer.
    //
    iRXDataStatus = sl_Recv(iSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(iRXDataStatus < 0)
    {
        ASSERT_ON_ERROR(SERVER_GET_WEATHER_FAILED);
    }
    else
    {
        DBG_PRINT("Received HTTP GET response data. \n\r");
    }

    DBG_PRINT("Return value: %d \n\r", iRXDataStatus);

    //
    // Get city name
    //
    pcIndxPtr = strstr(acRecvbuff, "name=");
    if(pcIndxPtr == NULL)
    {
        ASSERT_ON_ERROR(WRONG_CITY_NAME);
    }

    DBG_PRINT("\n\r****************************** \n\r\n\r");
    DBG_PRINT("City: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("name=") + 1;
        pcEndPtr = strstr(pcIndxPtr, "\">");
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
    }
    else
    {
        DBG_PRINT("N/A\n\r");
        return NO_WEATHER_DATA;
    }

    //
    // Get temperature value
    //
    pcIndxPtr = strstr(pcEndPtr+1, "temperature value");
    DBG_PRINT("Temperature: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("temperature value") + 2;
        pcEndPtr = strstr(pcIndxPtr, "\" ");
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
    }
    else
    {
        DBG_PRINT("N/A\n\r");
        return NO_WEATHER_DATA;
    }
    
    
    //
    // Get weather condition
    //
    pcIndxPtr = strstr(pcEndPtr+1, "weather number");
    DBG_PRINT("Weather Condition: ");
    if( NULL != pcIndxPtr )
    {
        pcIndxPtr = pcIndxPtr + strlen("weather number") + 14;
        pcEndPtr = strstr(pcIndxPtr, "\" ");
        if( NULL != pcEndPtr )
        {
           *pcEndPtr = 0;
        }
        DBG_PRINT("%s\n\r",pcIndxPtr);
    }
    else
    {
        DBG_PRINT("N/A\n\r");
        return NO_WEATHER_DATA;
    }
   
    DBG_PRINT("\n\r****************************** \n\r");
    return SUCCESS;
}


//****************************************************************************
//
//! Task function implementing the getweather functionality
//!
//! \param none
//! 
//! This function  
//!    1. Initializes the required peripherals
//!    2. Initializes network driver and connects to the default AP
//!    3. Creates a TCP socket, gets the server IP address using DNS
//!    4. Gets the weather info for the city specified
//!
//! \return None.
//
//****************************************************************************
void GetWeatherTask(void *pvParameters)
{
    int iSocketDesc, iRetVal;
    char acCityName[32];
    long lRetVal = -1;
    
    unsigned long ulDestinationIP;

    DBG_PRINT("GET_WEATHER: Test Begin\n\r");

    //
    // Configure LED
    //
    GPIO_IF_LedConfigure(LED1|LED3);

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
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
       UART_PRINT("Failed to start SimpleLink Device\n\r");
       return;
    }

    // Switch on Green LED to indicate Simplelink is properly UP
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    //
    // Configure Timer for blinking the LED for IP acquisition
    //
    LedTimerConfigNStart();

    // Initialize AP security params
    SecurityParams.Key = (signed char *)SECURITY_KEY;
    SecurityParams.KeyLen = strlen(SECURITY_KEY);
    SecurityParams.Type = SECURITY_TYPE;

    //
    // Connect to the Access Point
    //
    lRetVal = Network_IF_ConnectAP(SSID_NAME,SecurityParams);
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
    // Get the serverhost IP address using the DNS lookup
    //
    lRetVal = Network_IF_GetHostIP((char*)g_ServerAddress, &ulDestinationIP);
    if(lRetVal < 0)
    {
        UART_PRINT("DNS lookup failed. \n\r",lRetVal);
        goto end;
    }

    //
    // Create a TCP connection to the server
    //
    iSocketDesc = CreateConnection(ulDestinationIP);
    if(iSocketDesc < 0)
    {
        DBG_PRINT("Socket creation failed.\n\r");
        goto end;
    }
    
    struct SlTimeval_t timeVal;
    timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
    timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
    lRetVal = sl_SetSockOpt(iSocketDesc,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                    (unsigned char*)&timeVal, sizeof(timeVal)); 
    if(lRetVal < 0)
    {
       ERR_PRINT(lRetVal);
       LOOP_FOREVER();
    }

    while(1)
    {
        //
        // Get the city name over UART to get the weather info
        //
        UART_PRINT("\n\rEnter city name, or QUIT to quit: ");
        iRetVal = GetCmd(acCityName, sizeof(acCityName));
        if(iRetVal > 0)
        {
            if (!strcmp(acCityName,"QUIT") || !strcmp(acCityName,"quit"))
            {
                break;
            }
            else
            {
                //
                // Get the weather info and display the same
                //
                lRetVal = GetWeather(iSocketDesc, &acCityName[0]);
                if(lRetVal == SERVER_GET_WEATHER_FAILED)
                {
                    UART_PRINT("Server Get Weather failed \n\r");
                    LOOP_FOREVER();
                }
                else if(lRetVal == WRONG_CITY_NAME)
                {
                    UART_PRINT("Wrong input\n\r");

                }
                else if(lRetVal == NO_WEATHER_DATA)
                {
                    UART_PRINT("Weather data not available\n\r");

                }
                else
                {

                }

                //
                // Wait a while before resuming
                //
                MAP_UtilsDelay(SLEEP_TIME);
            }
        }
    }

    //
    // Close the socket
    //
    lRetVal = close(iSocketDesc);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
    DBG_PRINT("Socket closed\n\r");

end:
    //
    // Stop the driver
    //
    lRetVal = Network_IF_DeInitDriver();
    if(lRetVal < 0)
    {
       UART_PRINT("Failed to stop SimpleLink Device\n\r");
       LOOP_FOREVER();
    }

    //
    // Switch Off RED & Green LEDs to indicate that Device is
    // disconnected from AP and Simplelink is shutdown
    //
    GPIO_IF_LedOff(MCU_IP_ALLOC_IND);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    DBG_PRINT("GET_WEATHER: Test Complete\n\r");

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
#if defined(ccs)
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
//!    2. Invokes the GetWeather Task
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
    // Pinmux for UART
    //
    PinMuxConfig();

    //
    // Initializing DMA
    //
    UDMAInit();
#ifndef NOTERM
    //
    // Configuring UART
    //
    InitTerm();
#endif
    //
    // Display Application Banner
    //
    DisplayBanner(APP_NAME);

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
    // Start the GetWeather task
    //
    lRetVal = osi_TaskCreate(GetWeatherTask,
                    (const signed char *)"Get Weather",
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
