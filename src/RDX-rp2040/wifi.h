#include <Arduino.h>
#include "RDX-rp2040.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
   wifi.h
   definitions for the wifi.cpp program

  =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#ifndef __WIFI_H__
#define __WIFI_H__

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

/*----------------------------------------------------
 * Definitions for rp2040-w (wireless) Raspberry Pico
 * 
 */
#ifdef RP2040_W


struct EEPROM_WIFI_SSID{
  char ssid[30];
  char password[20];
};

/*--------------------
 * General definitions
 *--------------------*/

char ntp_server1[32];    //Server defined to sync time (primary)
char ntp_server2[32];    //Server defined to sync time (secondary)
char inet_server[32];    //Server defined to validate Inet connectivity

uint16_t    wifi_tout   = WIFI_TOUT;      //TCPIP connectivity timeout
EEPROM_WIFI_SSID         wifi;        //Structure to held WiFi AP credentials

#endif //RP2040_W





/*
#ifdef __cplusplus
}
#endif
*/

#endif
