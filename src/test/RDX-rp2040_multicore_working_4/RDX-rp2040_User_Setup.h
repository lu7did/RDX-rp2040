/*------ User setup
 * 
 */

 /*--------------------------------------------------
 * Program configuration parameters
 */
#define DEBUG                1  //Uncomment to activate debugging traces (_INFOLIST(...) statements thru _SERIAL

#define RP2040_W             1  //Comment if running on a standard Raspberry Pico (non Wireless)
#if defined(RP2040_W)
//#define FSBROWSER            1  //Comment out if a File System browser is not needed
//#define CLITOOLS             1  //Terminal
//#define WEBTOOLS             1  //Web parameters
#endif //RP2040_W

#define ADIF                 1  //Comment out if an ADIF logging is not needed
//#define DATALOGGERUSB        1  //Enable log export as a single file thru USB when active
#undef  UART                    //define for other than USB serial (Serial1 thru UART)

/*----------------------------------------------------
 * ft8 definitions
 */

#define MY_CALLSIGN "VU2IIA"
#define MY_GRID "MK69"

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                               WiFi Access Point credentials                                           *
//* Replace the AP SSID and password of your choice, if not modified the firmware won't be able to sync   *
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#ifndef WIFI_SSID

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//The ap.h file must contain valid Wifi credentials following the format
//#define WIFI_SSID                  "Your WiFi SSID"
//#define WIFI_PSK                   "0123456789"
//You might replace that include with a couple of #define for WIFI_SSID and WIFI_PSK for your Wifi AP
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

#if __has_include("ap.h")
#include "ap.h"
#else
#include "Y:\Documents\GitHub\ap.h"        //This is a trick to provide credentials on a file outside of the GitHub package
                                           //This is an horrendous practice that should be avoided, the only reason is
                                           //not to forget and publish my WiFi credentials (I test with)
                                           //Once you create your own ap.h file just place it at the folder where the rest
                                           //of the firmware is and replace the include with
                                           //     #include "ap.h"
#endif                                           
#endif //WIFI_SSID

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                               TCP/IP related areas                                                    *
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#define HOSTNAME              PROGNAME
#define NTP_SERVER1           "132.163.97.1"     //time.nist.gov in case of a faulty DNS server
#define NTP_SERVER2           "pool.ntp.org"     //NTP server secondary      
#define INET_SERVER           "www.google.com"   //Check some relevant host or IP address of interest to check connectivity
#define TIMEZONE              0                 //Buenos Aires, Argentina (GMT-3), change it accordingly
#define ADIFFILE              "/rdx.txt"         //ADIF Logbook internal FS name
#define LOGBOOK               "rdx_logbook.txt"  //ADIF Logbook USB exported name
#define QSO_MESSAGE           "73 and GL"        //Courtesy message to be added as a comment on a QSO registered into the ADIF Logbook
#define TCP_PORT               9000              //TCP Port to listen for connections
#define HTTP_PORT                80              //Web Filesystem browser
#define WEB_PORT               8000              //Web configuration tool

/*-------------------------------------------------------------
 * This are the FT8 algorithm tune parameters
 * Lowering the parameters will make the FT8 decoding to be 
 * faster and deafer, on a crowded band it might drive the 
 * algorithm not to detect our own QSO party if it's weaker
 * than the crocs around.
 *                   ** Warning **
 * Touch only is you know exactly what you're looking for as
 * wrong values might render the FT8 algorithm useless
 */
#define KMAX_CANDIDATES       30 //Defines the ability to pick candidate signals, on a PC is 120, with IL9488 is 30, lower for speed (with caution)
#define KLDPC_ITERATIONS      10 //Defines how deep the decoding process is, on a PC was 20, with IL9488 is 10, lower for speed (with caution)
#define KMAX_DECODED_MESSAGES 14 //Defines how many messages are actually decoded, on a PC was 50, with IL9488 was 14, lower for speed 
