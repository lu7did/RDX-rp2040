//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//                                              RDX_rp2040                                                 *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
// Pedro (Pedro Colla) - LU7DZ - 2022
//
// Version 2.0
//
// This is a direct port into the rp2040 architecture of the ADX_UNO firmware code (baseline version 1.1).
// Version 2.0 is an enhanced version which extends the capabilities of the ADX-rp2040 firmware
//*********************************************************************************************************
/*---------------------------------------------------------------------------------------------------*
 * Includes and macro definitions for the ADX-rp2040 transceiver firmware                            *
 *---------------------------------------------------------------------------------------------------*/
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       External libraries used                                               *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <si5351.h>
#include "hardware/watchdog.h"
#include "Wire.h"
#include <EEPROM.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include <WiFi.h>
#include <Time.h>
#include <stdbool.h>

/*------------------------------------------------
   IDENTIFICATION DIVISION.
   (just a programmer joke)
*/
#define PROGNAME "RDX"
#define AUTHOR "Pedro E. Colla (LU7DZ)"
#define VERSION "2.0"
#define BUILD   "82"
/*-------------------------------------------------
 * Macro expansions and type definitions
 */
#define digitalWrite(x,y) gpio_put(x,y)
#define digitalRead(x)  gpio_get(x)

#define BOOL2CHAR(x)  (x==true ? "True" : "False")
#undef  _NOP
#define _NOP (byte)0
typedef void (*CALLBACK)();
typedef void (*CALLQSO)(int i);
typedef bool (*CMD)(int idx,char *_cmd,char *_arg,char *_out);
typedef int16_t sigBin[960];


/*-------------------------------------------------------------*
   CPU Configuration parameters
  -------------------------------------------------------------*/

#define MULTICORE       1       //Processing is split between core0 and core1

/*-------------------------------------------------------------*
   LCD TFT Configuration parameters
  -------------------------------------------------------------*/
#define IL9488          1       //Commenting this will assume a non IL9488 LCD and restrict the access to the pen

/*-------------------------------------------------------------*
   Support for the Si473x chipset as a receiver
  -------------------------------------------------------------*/
#define RX_SI473X       1       //UNCOMMENT to use a receiver based on Si473x chipset
/*----------------------
  The following definition is used to force a given I2C address if not found
  by querying the I2C bus
*/

#define BAUD            115200  //Standard Serial port
#define CPU_CLOCK       250000  //CPU Clock (KHz)
#define STACK_SIZE      11000   //Bytes
#define DSP_QUEUEMAX    2       //DSP Queue High Watermark
#define OVERCLOCK       1       //Overclock the processor up to x2

/*-------------------------------------------------------------*
   User defined configuration parameters
  -------------------------------------------------------------*/
#include "RDX-rp2040_User_Setup.h"

/*-------------------------------------------------------------------------------------------------
   Consistency rules for sub-system integration
   Memory constraints and other dependencies are looked after the sub-system definition
   in order to avoid conflicts
*/

/*----------------------------------------------
  If defined MULTICORE then exclude FSBROWSER
  and CLITOOLS and WEBTOOLS
 *----------------------------------------------*/
#ifdef MULTICORE
#undef FSBROWSER
#undef CLITOOLS
#undef WEBTOOLS
#endif //MULTICORE

/*-----------------------------------------------
  If this is a standard Pico (not W) when exclude
  facilities that requires WiFi
 *-----------------------------------------------*/
#ifndef RP2040_W
#undef FSBROWSER
#undef WEBTOOLS
#endif //RP2040_W

/*-----------------------------------------------
  If FSBROWSER has been defined also include 
  CLITOOLS but exclude DATALOGGERUSB and WEBTOOLS
 *-----------------------------------------------*/
#ifdef FSBROWSER
#define CLITOOLS  1
#undef DATALOGGERUSB
#undef WEBTOOLS 
#endif //FSBROWSER

/*-----------------------------------------------
  If WEBTOOLS is defined exclude CLITOOLS
 *-----------------------------------------------*/
#ifdef WEBTOOLS
#define CLITOOLS 1
#endif //WEBTOOLS

/*-----------------------------------------------
  If DATALOGGERUSB is defined then force ADIF log
 *-----------------------------------------------*/
#ifdef DATALOGGERUSB
#define ADIF 1
#endif //DATALOGGERUSB

/*-----------------------------------------------
  If RX_SI473X receiver defined declare constants
 *-----------------------------------------------*/

#ifdef RX_SI473X           //Define RESET pin for SI473x if defined
#define RESET_SI473X    1  //Si473x RESET pin
#define AM_FUNCTION     1
#define RESET_PIN       1
#define LSB             1
#define USB             2

#ifdef DEBUG
//#define TRACE_SI473X    1
#endif //DEBUG

#endif //RX_SI473X
/*----
   Output control lines
*/
#define RX              2  //RX Switch

/*---
   LED
*/
#define WSPR            7  //WSPR LED
#define JS8             6  //JS8 LED
#define FT4             5  //FT4 LED
#define FT8             4  //FT8 LED

#define TX              3  //TX LED

/*---
   Switches
*/
#define UP             10  //UP Switch
#define DOWN           11  //DOWN Switch
#define TXSW            8  //RX-TX Switch


/*---
 * 
 */
  
#define BUTTON_TX       0
#define BUTTON_CQ       1
#define BUTTON_AUTO     2
#define BUTTON_BAND     3
  
#define BUTTON_END      4


/*---
   Signal input pin
*/

#define FSKpin         27  //Frequency counter algorithm, signal input PIN (warning changes must also be done in freqPIO)
#define ADC0           26  //Audio received (centered at Vcc/2)
#define ADC_CHANNEL     0
/*---
    I2C
*/
#define I2C_SDA        16  //I2C SDA
#define I2C_SCL        17  //I2C SCL

#define SI5351_REF     25000000UL  //change this to the frequency of the crystal on your si5351’s PCB, usually 25 or 27 MHz

/*----
   Autocalibration pin
*/
#define CAL             9      //Automatic calibration entry


//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                      GENERAL PURPOSE GLOBAL DEFINITIONS                                     *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#define BANDS          4            //Max number of bands allowed
#define MAXBAND        9
#define AF_FREQ     1500
#define MAXTRY         5
#define DELAY        250
#define MAXTX          6

/*-----------------------------------------------------
   Definitions for autocalibration

*/
#define AUTOCAL             1
#define CAL_STEP          500           //Calibration factor step up/down while in calibration (sweet spot experimentally found by Barb)
#define CAL_COMMIT         12
#define CAL_ERROR           1


/*--------------------------------------------------------------
 * Magic numbers
 * Do not even think to touch them
 * 
 */
#define kMin_score            10 // Minimum sync score threshold for candidates
#define kMax_candidates       KMAX_CANDIDATES       // Original was 120
#define kLDPC_iterations      KLDPC_ITERATIONS      // Original was 20
#define kMax_decoded_messages KMAX_DECODED_MESSAGES //was 50, change to 14 since there's 14 buttons on the 4x4 membrane keyboard

/*------------------------------------------------------
 * EEPROM Address Map
 */
#define EEPROM_ADDR_CAL     10       //CALB  int32_t
#define EEPROM_ADDR_TEMP    30
#define EEPROM_ADDR_MODE    40       //MODE  int
#define EEPROM_ADDR_SLOT    50       //SLOT  int
#define EEPROM_ADDR_BUILD   60       //BUILD  int
#define EEPROM_ADDR_MYCALL  70       //MYCALL  char[16] -- 10
#define EEPROM_ADDR_MYGRID  80       //GRID  char[8]    -- 10
#define EEPROM_ADDR_SSID    90       //SSID  char[40]   -- 40
#define EEPROM_ADDR_PSK    130       //PSK   char[16]   -- 10
#define EEPROM_ADDR_MAXTX  150       //TX    uint8_t    -- 2
#define EEPROM_ADDR_MAXTRY 160       //TRY   uint8_t    -- 2
#define EEPROM_ADDR_AUTO   170       //AUTO  bool       -- 2
#define EEPROM_ADDR_WRITE  180       //WRITE bool       -- 2
#define EEPROM_ADDR_HTTP   190       //HTTP  int        -- 2
#define EEPROM_ADDR_HOST   200       //HOST  char[16]   -- 10
#define EEPROM_ADDR_WEB    220       //WEB   int        -- 2
#define EEPROM_ADDR_PORT   230       //PORT  int        -- 2
#define EEPROM_ADDR_TZ     240       //TZ    int        -- 2
#define EEPROM_ADDR_ADIF   250       //ADIF  char[16]   -- 20
#define EEPROM_ADDR_LOG    270       //LOG   char[32]   -- 30
#define EEPROM_ADDR_MSG    310       //QSO   char[16]   -- 20
#define EEPROM_ADDR_END    330

/*------------------------------------------------------------
 * GUI Icon enumeration
 */
#define MAXICON  10

#define WIFIICON 0
#define TERMICON 1
#define CALICON  2
#define CNTICON  3
#define CATICON  4
#define WSJTICON 5
#define QUADICON 6
#define MUTEICON 7
#define SPKRICON 8
#define TUNEICON 9


#define SLOT_1               1
#define SLOT_2               2
#define SLOT_3               3
#define SLOT_4               4

#define SYNC_OK               0
#define SYNC_NO_INET          1
#define SYNC_NO_AP            2
#define SYNC_NO_NTP           3


#define WIFI_TOUT            20        //WiFi connection timeout (in Secs)
#define UDP_PORT           2237        //UDP Port to listen
#define UDP_BUFFER          256


//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Pin definitions (SI4735)                                              *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#define AM_FUNCTION           1
#define FM_FUNCTION           0
#define FT8_BANDWIDTH_IDX     2         //define a 3 KHz bandwidth
#define FT8_USB               2         //define USB as the mode
#define FT8_STEP              1         //Step 1KHz (required but not used as there is no tuning)
#define FT8_VOLUME           45

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                         External References                                              *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
/*-----------------------------------------------------
 * External references to freqPIO variables and methods
 */
extern Si5351 si5351;

extern char my_callsign[16];
extern char my_grid[8];
extern uint8_t nTry;
extern uint8_t maxTry;
extern uint8_t maxTx;
extern int Band_slot;
extern const uint16_t Bands[BANDS];
extern char hostname[16];
extern char qso_message[16];
extern char logbook[32];
extern char adiffile[16];

extern char wifi_ssid[40];
extern char wifi_psk[16];

extern int  tcp_port;
extern int  http_port;
extern int  web_port;

extern const unsigned long slot[MAXBAND][3];


/*------------------------------------------------------------------
 * System global state variables
 */
extern bool autosend;
extern bool stopCore1;
extern bool qwait;
extern bool triggerCQ;
extern bool triggerCALL;
extern bool enable_adc;
extern bool DSP_flag;
extern bool logADIF;
extern bool endQSO;
extern bool enableEEPROM;


extern char hi[128];
extern unsigned long freq;
extern char timestr[12];
extern int magint[960];
extern int TX_State;
extern char programname[12];
extern char version[6];
extern char build[6];
extern char ip[16];
extern uint8_t ft8_state;
extern int timezone;
extern unsigned long rssi;


extern uint16_t call_af_frequency;
extern int8_t call_self_rx_snr;
extern char call_station_callsign[8];
extern char call_grid_square[4];
extern uint16_t call_qsowindow;

extern bool okayToWrite;
extern bool SingleFileDriveactive;

extern int af_frequency;

/*---------------------------------------------------
 * Time related variables
 */
extern time_t t_ofs;
extern time_t now;
extern struct tm timeinfo;        //current time
extern struct tm timeprev;        //epoch time


extern char ntp_server1[32];    //Server defined to sync time (primary)
extern char ntp_server2[32];    //Server defined to sync time (secondary)
extern char inet_server[32];    //Server defined to validate Inet connectivity

extern CALLBACK fftReady;
extern CALLQSO  qsoReady;
extern CALLBACK fftEnd;
extern CALLBACK  txIdle;
extern CALLBACK quitCallBack;

extern void setup_FSBrowser();
extern void loop_FSBrowser();
extern void TCP_Process();

extern void tft_FSBrowser();
extern void tft_autocal();
extern void tft_error(uint16_t e);
extern void tft_print(char *t);
extern void tft_process();
extern void tft_updatewaterfall(int m[]);
extern void tft_setup();
extern void tft_run(bool pen);
extern void tft_endoftime();
extern void tft_checktouch();
extern void tft_resetBar();
extern void tft_endQSO();
extern void tft_init();
extern void tft_cli();
extern void tft_setBar(int colour);
extern void tft_setIP(char *ip_address);
extern void tft_storeQSO(uint16_t qsowindow, uint16_t _qso,char *s,uint16_t af_frequency,int8_t self_rx_snr,char *station_callsign,char *grid_square);
extern void tft_updateBand();
extern void tft_setBarTx();
extern void tft_set(int btnIndex,int v);
extern void tft_ADIF();
extern void tft_syncNTP();
extern void tft_quad();
extern void tft_ADIF();
extern void tft_footupdate();
extern void tft_DataLoggerUSB();
extern void tft_iconState(int _icon,bool _state);
extern void tft_iconSet(int _icon,bool _enabled);
extern void tft_iconActive(int _icon,bool _active);
extern void tft_Web();


extern bool popBang(char *s,char *t,const char delimiter);
extern bool parse(char *s,char *t);
extern int heapLeft();
extern int cliFind(char *cmd);
extern void wipeChar(char *out);
extern void ltrimStr(char *out);
extern void rtrimStr(char *out);
extern void tolowerStr(char *s);
extern void toupperStr(char *s);
extern bool isNumeric(char *s);

extern int checkAP(char* s, char* p);
extern void resetAP();
extern int setup_wifi();
extern bool getClock(char* n1, char* n2);

extern int writeQSO(char *adifFile,char *call,char *grid, char *mode, char *rst_sent,char *rst_rcvd,char *qso_date,char *time_on, char *band, char *freq, char *mycall, char *mygrid, char *message);
extern void setup_adif();

extern void data_stop();
extern void data_setup();
extern void cli_command();
extern void cli_prompt(char *_out);

extern uint16_t Band2Idx(uint16_t b);
extern uint16_t minFreq(uint16_t i);
extern uint16_t maxFreq(uint16_t i);
extern uint16_t currFreq(uint16_t i);



extern struct semaphore spc;      //Semaphore to protect multithread access to the signal queue

#ifdef RX_SI473X
/*-------------------------------------
 * Definitions for the Si473x chipset
 */

extern void SI473x_Setup();
extern void SI473x_Status(); 
extern void SI473x_setFrequency(int s);

#endif //RX_SI473X

#ifdef MULTICORE

#define QMAX  3                   //Maximum number of members in the received signal queue extern sigBin fresh_signal;       //Most recent signal capture buffer
extern sigBin old_signal;         //signal capture buffer being analyzed
extern sigBin queued_signal[QMAX];//Actual signal capture queue
extern uint16_t queueR;           //Signal capture queue read pointer
extern uint16_t queueW;           //Signal capture queue write pointer

extern struct semaphore ipc;      //Semaphore to protect multithread access to the signal queue



/*-------------------------------------
 * Special structure to protect EEPROM
 * updates
 */
extern struct semaphore epc;
extern queue_t edata;
extern bool stallEEPROM;


extern queue_t qdata;
extern queue_t sdata;

/*----------------------------------
 * Non-generic methods to handle the
 * signal queue buffer
 */
extern void pushSignal();
extern void popSignal();
extern bool availSignal();
extern int sizeSignal();

extern void process_adc();
extern int num_adc_blocks;

#endif //MULTICORE


extern void initSi5351();

extern void AutoCalibration();
extern void timeSync();
extern int getQSOwindow();

extern void readEEPROM();
extern void resetEEPROM();
extern void updateEEPROM();
extern bool getEEPROM(int *i,char *buffer);

extern void setCALL();
extern void INIT();
extern void Calibration();

extern void Band_Select();
extern void Band2Str(char *str);
extern void ManualTX();
extern void Band_assign();
extern void Freq_assign();
extern void Mode_assign();
extern void checkButton();
extern unsigned long Slot2Freq(int s);

extern void startTX();
extern void stopTX();

extern void setup_Web();
extern void process_Web();

extern bool cli_commandProcessor(char *cmd,char *arg, char *response);
extern void cli_init(char *out);
bool cli_execute(char *buffer, char *outbuffer);

/*-------------------------------------------------------
 * Debug and development aid tracing
 * only enabled if DEBUG is defined previously
 */
#ifdef DEBUG

#ifdef  UART    //Can test with the IDE, USB based, serial port or the UART based external serial port
#define _SERIAL Serial1
#else
#define _SERIAL Serial
#endif //UART

#define _INFOLIST(...) \
  do { \
    now = time(0) - t_ofs;  \
    gmtime_r(&now, &timeinfo);  \
    sprintf(timestr,"[%02d:%02d:%02d] ",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);   \
    strcpy(hi,timestr); \
    strcat(hi,"@");  \
    sprintf(hi+strlen(hi),__VA_ARGS__); \
    _SERIAL.write(hi); \
    _SERIAL.flush(); \
  } while (false)

#define _INFO(...) \
  do { \
    while(!sem_try_acquire(&spc)); \
    now = time(0) - t_ofs;  \
    gmtime_r(&now, &timeinfo);  \
    sprintf(timestr,"[%02d:%02d:%02d] ",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);   \
    strcpy(hi,timestr); \
    strcat(hi,"@");  \
    sprintf(hi+strlen(hi),"%s ",__func__); \
    sprintf(hi+strlen(hi),__VA_ARGS__); \
    _SERIAL.write(hi); \
    _SERIAL.flush(); \
    sem_release(&spc); \
  } while (false)

/*-----------------------------------------------
   IPC protected serial write to be used when
   debugging MULTICORE
*/

#ifdef MULTICORE
#define _print(...) \
  do { \
    while(!sem_try_acquire(&spc)); \
    rp2040.idleOtherCore(); \
    _SERIAL.print(__VA_ARGS__); \
    _SERIAL.flush(); \
    sem_release(&spc); \
    rp2040.resumeOtherCore(); \
  } while (false)
#define _println(...) \
  do { \
    while(!sem_try_acquire(&spc)); \
    rp2040.idleOtherCore(); \
    _SERIAL.println(__VA_ARGS__); \
    _SERIAL.flush(); \
    sem_release(&spc); \
    rp2040.resumeOtherCore(); \
  } while (false)
#endif //MULTICORE

#else //!DEBUG
#define _INFOLIST(...)
#define _INFO(...)
#define _SERIAL Serial
#define _println(...)
#define _print(...)

#endif

/*----------------------------------------------------------
 * Extern functions across the different sub-systems
 */

extern uint16_t ft8_crc(const uint8_t message[], int num_bits);
