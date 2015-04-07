//*****************************************************************************
//
// Application Name        - simplelinklibrary
// Application Version     - 1.3.3
// Application Modify Date - 28th of December 2014
// Application Developer   - Glenn Vassallo
// Application Contact	   - contact@swiftsoftware.com.au
// Application Repository  - https://github.com/remixed123/startproject
//
// Application Overview    - This library provides some of the most common and
//							 useful functions that are required when developing
//						     applications with the CC3100/CC3200.
//
// Application Details     - https://github.com/remixed123/startproject/readme.txt
//
// Further Details         - If you would like to chat about your next CC3200 project
//                           then feel free contact us at contact@swiftsoftware.com.au
//
//*****************************************************************************

#ifndef SIMPLELINKLIBRARY_H_
#define SIMPLELINKLIBRARY_H_

#include "simplelink.h"

 //*****************************************************************************
 // Device Defines
 //*****************************************************************************
#define DEVICE_VERSION			"1.0.0"
#define DEVICE_MANUFACTURE		"SWIFT_SOFTWARE"
#define DEVICE_NAME 			"STARTPROJECT"
#define DEVICE_MODEL			"CC3200"
#define DEVICE_AP_DOMAIN_NAME	"startproject.net"
#define DEVICE_SSID_AP_NAME		"StartProjectAP"

 //*****************************************************************************
 // mDNS Defines
 //*****************************************************************************
#define MDNS_SERVICE  "._control._udp.local"
#define TTL             120
#define UNIQUE_SERVICE  1       /* Set 1 for unique services, 0 otherwise */

 //*****************************************************************************
 // SimpleLink/WiFi Defines
 //*****************************************************************************
#define UDPPORT         4000 /* Port number to which the connection is bound */
#define UDPPACKETSIZE   1024
#define SPI_BIT_RATE    14000000

#define TIMEOUT 5

//*****************************************************************************
// Date and Time Global
//*****************************************************************************
extern SlDateTime_t dateTime;

 //*****************************************************************************
 // Function Declarations
 //*****************************************************************************

char * getMacAddress();
char * getDeviceName();
char * getApDomainName();
char * getSsidName();
int getDeviceTimeDate();

int setDeviceName();
int setApDomainName();
int setSsidName();
int setDeviceTimeDate();

int registerMdnsService();
int unregisterMdnsService();

#endif /* SIMPLELINKLIBRARY_H_ */
