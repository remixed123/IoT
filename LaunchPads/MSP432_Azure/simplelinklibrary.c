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
// Further Details         - If you would like to chat about your next CC3200 project
//                           then feel free contact us at contact@swiftsoftware.com.au
//
//*****************************************************************************

#include "simplelinklibrary.h"

#include <stdio.h>

//*****************************************************************************
// Globals used by mDNS
//*****************************************************************************
static char mdnsServiceName[40] = "";
static char mdnsText[70] = "";

//*****************************************************************************
//! getMacAddress
//!
//! Returns the MAC Address as string
//!
//****************************************************************************
char * getMacAddress()
{
	int i;

	unsigned char macAddressVal[SL_MAC_ADDR_LEN];
	unsigned char macAddressLen = SL_MAC_ADDR_LEN;

	sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&macAddressLen,(unsigned char *)macAddressVal);

	char macAddressPart[2];
	static char macAddressFull[18]; //18

	for (i = 0 ; i < 6 ; i++)
	{
		sprintf(macAddressPart, "%02X", macAddressVal[i]);
		strcat(macAddressFull, (char *)macAddressPart);
		strcat(macAddressFull, ":");
	}

	macAddressFull[17] = '\0'; // Replace the the last : with a zero termination

	return macAddressFull;
}

//*****************************************************************************
//! getDeviceName
//!
//! Returns the Device Name as a string
//!
//****************************************************************************
char * getDeviceName()
{
	static char strDeviceName[35];
	sl_NetAppGet (SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, (unsigned char *)strlen(strDeviceName), (unsigned char *)strDeviceName);
	return strDeviceName;
}

//*****************************************************************************
//! getApDomainName
//!
//! Returns the Access Point Domain Name as a string
//!
//****************************************************************************
char * getApDomainName()
{
	static char strDomainName[35];
	sl_NetAppGet (SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME, (unsigned char *)strlen(strDomainName), (unsigned char *)strDomainName);
	return strDomainName;
}

//*****************************************************************************
//! getSsidName
//!
//! Returns the SSID Name for the device when in Access Point Mode
//!
//****************************************************************************
char * getSsidName()
{
	static char ssidName[32];
	unsigned short len = 32;
	unsigned short  config_opt = WLAN_AP_OPT_SSID;
	sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char *)ssidName);
	return (char *)ssidName;
}

//*****************************************************************************
//! getDeviceTimeDate
//!
//! Gets the device time and date
//!
//! Returns: On success, zero is returned. On error, -1 is returned
//!
//****************************************************************************
int getDeviceTimeDate()
{
	int iretVal;
	//dateTime =  {0};
	unsigned char configLen = (unsigned char)sizeof(SlDateTime_t);
	unsigned char configOpt = (unsigned char)SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME;
	iretVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (unsigned char *)&dateTime);
	return iretVal;
}

//*****************************************************************************
//! setDeviceName
//!
//! Sets the name of the Device
//!
//! Returns: On success, zero is returned. On error, -1 is returned
//!
//****************************************************************************
int setDeviceName()
{
	int iretVal;
	unsigned char strDeviceName[32] = DEVICE_NAME;
	iretVal = sl_NetAppSet (SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, strlen((const char *)strDeviceName), (unsigned char *) strDeviceName);
	return iretVal;
}

//*****************************************************************************
//! setApDomainName
//!
//! Sets the name of the Access Point's Domain Name
//!
//! Returns: On success, zero is returned. On error, -1 is returned
//!
//****************************************************************************
int setApDomainName()
{
	int iretVal;
	unsigned char strDomain[32] = DEVICE_AP_DOMAIN_NAME;
	unsigned char lenDomain = strlen((const char *)strDomain);
	iretVal = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME, lenDomain, (unsigned char*)strDomain);
	return iretVal;
}

//*****************************************************************************
//! setSsidName
//!
//! Sets the SSID name for AP mode
//!
//! Returns: On success, zero is returned. On error one of the following error codes returned:
//!    CONF_ERROR (-1)
//!    CONF_NVMEM_ACCESS_FAILED (-2)
//!    CONF_OLD_FILE_VERSION (-3)
//!    CONF_ERROR_NO_SUCH_COUNTRY_CODE (-4)
//!
//****************************************************************************
int setSsidName()
{
	int iretVal;
	unsigned char  str[33] = "StartProjectAP";
	unsigned short  length = strlen((const char *)str);
	iretVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, length, str);
	return iretVal;
}


//*****************************************************************************
//! setDeviceTimeDate
//!
//! Gets the device time and date
//!
//! Returns: On success, zero is returned. On error, -1 is returned
//!
//****************************************************************************
int setDeviceTimeDate()
{
	int iretVal;
	iretVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION, SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME, sizeof(SlDateTime_t), (_u8 *)(&dateTime));
	return iretVal;
}

//*****************************************************************************
//! registerMdnsService
//!
//! Registers the mDNS Service.
//!
//! Service settings for type and port are set in simplelinklibrary.h
//!
//! Returns: On success returns 0
//!
//****************************************************************************
int registerMdnsService()
{
	int iretVal;
	unsigned int i;

	// Create mDNS Service Name
	for (i = 0; i < 40; i++)
		mdnsServiceName[i] = 0x00;

	// Obtain the device name
	char * deviceName = getDeviceName();

	strcat(mdnsServiceName, (const char *)deviceName);
	strcat(mdnsServiceName, MDNS_SERVICE);

	// Create mDNS Text
	for (i = 0; i < 50; i++)
		mdnsText[i] = 0x00;

	// Obtain the MAC Address
	char * macAddress = getMacAddress();

	strcat(mdnsText, "mac=");
	strcat(mdnsText, macAddress);
	strcat(mdnsText, ";ver=");
	strcat(mdnsText, DEVICE_VERSION);
	strcat(mdnsText, ";man=");
	strcat(mdnsText, DEVICE_MANUFACTURE);
	strcat(mdnsText, ";mod=");
	strcat(mdnsText, DEVICE_MODEL);
	strcat(mdnsText, "\0");

	int strSvrLength = strlen(mdnsServiceName);
	int strTxtLength = strlen(mdnsText);

	//Unregisters the mDNS service.
	unregisterMdnsService();

	//Registering for the mDNS service.
	iretVal = sl_NetAppMDNSRegisterService((const signed char *)mdnsServiceName,strlen(mdnsServiceName),
			(const signed char *)mdnsText,strlen(mdnsText)+1, UDPPORT, TTL, UNIQUE_SERVICE);

	return iretVal;
}

//*****************************************************************************
//! unregisterMdnsService
//!
//! Unregisters the mDNS Service.
//!
//! Returns: On success returns 0
//!
//****************************************************************************
int unregisterMdnsService()
{
	int iretVal;
	iretVal = sl_NetAppMDNSUnRegisterService((const signed char *)mdnsServiceName,strlen(mdnsServiceName));
	return iretVal;
}





