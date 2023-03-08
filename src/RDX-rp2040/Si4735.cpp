#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef RX_SI473X
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * RDX_Si4735.ino                                                                              *
 * Simple Proof of Concept (PoC) sketch for the RDX transceiver operating with the Si4735 chip *
 * Uses the POC_01 sketch in the PU2CLR Si4735 Library as the starting point                   *
 *                                                                                             *
 * The table below shows the Si4735 and RASPBERRY PICO connections modified for the RDX project*
 *                                                                                             *   
 * | ---------------| ---------- |                                                             *
 * | Si4735 pin     |  PICO Pin  |                                                             *
 * | ---------------| ---------- |                                                             *
 * | RESET (pin 15) |     GP16   |                                                             *
 * | SDIO (pin 18)  |     GP0    |                                                             *
 * | CLK (pin 17)   |     GP1    |                                                             *
 * | ---------------| ---------- |                                                             *
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
 * Pinout adapted to the pin allocation of the RDX project blueprint                           *
 *                                                                                             *
 * | ---------------| ---------- |                                                             *
 * | Si4735 pin     |   RDX Pin  |                                                             *
 * | ---------------| ---------- |                                                             *
 * | RESET (pin 15) |     GP1    |                                                             *
 * | SDIO (pin 18)  |     GP16   | (same than Si5351 sharing the I2C bus)                      *
 * | CLK (pin 17)   |     GP17   | (same than Si5351 sharing the I2C bus)                      *
 * | ---------------| ---------- |                                                             *
 
 *---------------------------------------------------------------------------------------------*
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <SI4735.h>
#include "patch_init.h" // SSB patch for whole SSBRX initialization string

const uint16_t size_content = sizeof ssb_patch_content; // see patch_init.h
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Global varibles                                                       *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
bool led=false;
bool enabled=false;
bool disableAgc=false;
bool avc_en=true;
uint8_t currentAGCAtt=0;

SI4735 rx;

/*------------------------------------------------------------------------------------------
 * load the SSB patch and reset the receiver upon initial load and at every band change
 * (not sure if when a setSSB function is called the patch has to be loaded again )
 * pending of research for future optimization
 *-----------------------------------------------------------------------------------------*/
void SI4735_loadSSB(int s) {

if (!enabled) return;

rx.setI2CFastModeCustom(500000); // Increase the transfer I2C speed
rx.loadPatch(ssb_patch_content, size_content, FT8_BANDWIDTH_IDX);
rx.setI2CFastModeCustom(100000); // Increase the transfer I2C speed
rx.setI2CFastMode();
delay(10);
_INFO("SSB patch loaded\n");

rx.setTuneFrequencyAntennaCapacitor(1);
delay(10);

uint16_t b=Bands[s-1];
uint16_t i=Band2Idx(b);
 
rx.setSSB(minFreq(i),maxFreq(i),currFreq(i),FT8_STEP, FT8_USB);
delay(10);
_INFO("Frequency set[%d,%d,%d] getFrequency=%d\n",minFreq(i),maxFreq(i),currFreq(i),rx.getFrequency());

rx.setFrequencyStep(FT8_STEP);
rx.setSSBAudioBandwidth(FT8_USB);
rx.setSSBSidebandCutoffFilter(1);
rx.setSSBBfo(0);     //try 975
rx.setAutomaticGainControl(disableAgc, currentAGCAtt);
rx.setSSBAutomaticVolumeControl(avc_en);
rx.setAvcAmMaxGain(65);    // Sets the maximum gain for automatic volume control on AM/SSB mode (from 12 to 90dB)
rx.setVolume(45);    //MV

rx.setSSBAutomaticVolumeControl(1);
delay(100);

rx.setAutomaticGainControl(false, 0);
delay(100);

rx.setAmSoftMuteMaxAttenuation(0); // Soft Mute for AM or SSB
_INFO("Si473x f=%d fmin=%d fmax=%d step=%d mode=%d\n",currFreq(i),minFreq(i),maxFreq(i),FT8_STEP,FT8_USB);

return;

}
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                       Setup                                                                 *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
void SI4735_setup()
{

  digitalWrite(SI4735_RESET, HIGH);
  _INFO("looking for a Si473x chip\n");

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(SI4735_RESET);
  if ( si4735Addr == 0 ) {
    _INFO("Si473x device not found on I2C bus, blocking\n");
    led=true;
     while (true) {
       digitalWrite(JS8,led);
       delay(500);
       led=!led;
     }
  }
  digitalWrite(JS8,HIGH);
  _INFO("Si473x device found I2C address is %04x\n",si4735Addr);
  delay(500);

  rx.setup(SI4735_RESET, AM_FUNCTION);
  delay(10);
  _INFO("Si473x device reset completed\n");
  enabled=true;

  SI4735_loadSSB(Band_slot);
  _INFO("Si473x SSB mode enabled\n");


}
/*-----------------------------------------
 * Show the current receiver status
 */
void SI4735_Status() {

  if (!enabled) return;
  uint16_t currentFrequency = rx.getFrequency();
  rx.getCurrentReceivedSignalQuality();
  _INFO("f=%d KHz SNR: %8.2f dB Signal: %8.2f dBuV\n",currentFrequency,rx.getCurrentSNR(),rx.getCurrentRSSI());

}
#endif //RX_SI473X