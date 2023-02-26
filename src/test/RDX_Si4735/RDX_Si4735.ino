/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * RDX_Si4735.ino                                                                              *
 * Simple Proof of Concept (PoC) sketch for the RDX transceiver operating with the Si4735 chip *
 * Uses the POC_01 sketch in the PU2CLR Si4735 Library as the starting point                   *
 *                                                                                             *
 * The table below shows the Si4735 and RASPBERRY PICO connections modified for the RDX project*
 *                                                                                             *   
 * | Si4735 pin     |  PICO Pin  |                                                             *
 * | ---------------| ---------- |                                                             *
 * | RESET (pin 15) |     GP16   |                                                             *
 * | SDIO (pin 18)  |     GP0    |                                                             *
 * | CLK (pin 17)   |     GP1    |                                                             *
 * PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI4735/extras/apidoc/html/        *
 * RDX project documentation: 
 *                                                                                             *
 * Released by the public domain By Ricardo Lima Caratti, Nov 2019.                            *
 *---------------------------------------------------------------------------------------------*
  Copyright (c) 2019 by Ricardo Lima Caratti. All rights reserved.                     
  This file is part of the Arduino Pico core board library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *---------------------------------------------------------------------------------------------*
 * Adaptation and integration with RDX project by Dr. Pedro E. Colla (LU7DZ) 2022              *
 * This implementation provides a simplified setup to be used for debugging purposes, the      *
 * final firmware resulting from experimentation will be integrated into the RDX transceiver   *
 *---------------------------------------------------------------------------------------------*
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <SI4735.h>
#include <Arduino.h>
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
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Identification Division                                               *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#define PROGNAME "RDX_Si4735"
#define AUTHOR "Pedro E. Colla (LU7DZ)"
#define VERSION "1.0"
#define BUILD   "00"
/*-------------------------------------------------
 * Macro expansions
 */
#define digitalWrite(x,y) gpio_put(x,y)
#define digitalRead(x)  gpio_get(x)
#define BOOL2CHAR(x)  (x==true ? "True" : "False")
#undef  _NOP
#define _NOP (byte)0

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Pin definitions                                                       *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#define RESET_PIN      16
/*---
   LED
*/
#define WSPR            7  //WSPR LED
#define JS8             6  //JS8 LED
#define FT4             5  //FT4 LED
#define FT8             4  //FT8 LED
#define TX              3  //TX  LED

/*---
   Switches
*/
#define UP             10  //UP Switch
#define DOWN           11  //DOWN Switch
#define TXSW            8  //RX-TX Switch
/*---
    I2C
*/
#define I2C_SDA        16  //I2C SDA
#define I2C_SCL        17  //I2C SCL


/*---
 * RESET pin of Si4735
 */
#define RESET       1

/*---
 * Other defines
 */
#define AM_FUNCTION 1
#define FM_FUNCTION 0
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Global varibles                                                       *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
char hi[128];
uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t bandwidthIdx = 0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};
bool led=false;
SI4735 rx;
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Setup                                                                 *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
void setup()
{
  Serial.begin(115200);
  while(!Serial);

  /*-----------------------------
     Port definitions (pinout, direction and pullups used
  */
  
  /*--------
     Initialize switches
  */
  gpio_init(UP);
  gpio_init(DOWN);
  gpio_init(TXSW);

  /*---
     Initialize LED
  */

  gpio_init(WSPR);
  gpio_init(JS8);
  gpio_init(FT4);
  gpio_init(FT8);
  gpio_init(TX);

  gpio_init(RESET);
  /*-----
     Set direction of input ports
  */
  gpio_set_dir(UP, GPIO_IN);
  gpio_set_dir(DOWN, GPIO_IN);
  gpio_set_dir(TXSW, GPIO_IN);

  /*-----
     Pull-up for input ports
     (Warning! See hardware instructions
  */

  gpio_pull_up(TXSW);
  gpio_pull_up(DOWN);
  gpio_pull_up(UP);

  /*---
     Set output ports
  */
  gpio_set_dir(RESET,GPIO_OUT);
  gpio_set_dir(TX, GPIO_OUT);
  gpio_set_dir(WSPR, GPIO_OUT);
  gpio_set_dir(JS8, GPIO_OUT);
  gpio_set_dir(FT4, GPIO_OUT);
  gpio_set_dir(FT8, GPIO_OUT);
  gpio_pull_up(RESET);


  /*---
     Initialice I2C sub-system
  */

  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  digitalWrite(WSPR,HIGH);
  Serial.println("I/O sub-system initialization completed");

  digitalWrite(RESET_PIN, HIGH);
  Serial.println("AM and FM station tuning test.");

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473x device not found on I2C bus, blocking\n");
    Serial.flush();
    led=true;
    while (true) {
      digitalWrite(JS8,led);
      delay(500);
      led=!led;
    }
  }
  digitalWrite(JS8,HIGH);
  sprintf(hi,"Si473x I2C address is %04x\n",si4735Addr);
  Serial.print(hi);
  delay(500);

  /*--------------------------------------
   * Setup receiver FM band 103.90 MHz
   */
  rx.setup(RESET_PIN, FM_FUNCTION);
  // Starts defaul radio function and band (FM; from 84 to 108 MHz; 103.9 MHz; step 100kHz)
  rx.setFM(8400, 10800, 10390, 10);
  delay(500);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(45);
  digitalWrite(FT4,HIGH);
  led=false;
}
/*-----------------------------------------
 * Show the current receiver status
 */
void showStatus()
{
  // rx.getStatus();
  previousFrequency = currentFrequency = rx.getFrequency();
  rx.getCurrentReceivedSignalQuality();
  sprintf(hi,"f=%8.2f MHz SNR: %8.2f dB Signal: %8.2f dBuV\n",currentFrequency / 100.0,rx.getCurrentSNR(),rx.getCurrentRSSI());
  Serial.print(hi);
}
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Loop                                                                  *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
void loop() {
  delay(1000);
  showStatus();
  led=!led;
  digitalWrite(FT8,led);
}
