#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef RX_SI4735
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
SI4735 rx;

/*------------------------------------------------------------------------------------------
 * load the SSB patch and reset the receiver upon initial load and at every band change
 * (not sure if when a setSSB function is called the patch has to be loaded again )
 * pending of research for future optimization
 *-----------------------------------------------------------------------------------------*/
void SI4735_loadSSB(int s) {

if (!enabled) return;

rx.setup(SI4735_RESET, 0, 1, SI473X_ANALOG_AUDIO); // Starts FM mode and ANALOG audio mode.
delay(500);

rx.setup(SI4735_RESET, AM_FUNCTION);
delay(500);

rx.loadPatch(ssb_patch_content, size_content, FT8_BANDWIDTH_IDX);
delay(100);

rx.setTuneFrequencyAntennaCapacitor(1);
delay(100);

uint16_t b=Bands[s-1];
uint16_t i=Band2Idx(b);
 
rx.setSSB(minFreq(i),maxFreq(i),currFreq(i),FT8_STEP, FT8_USB);
delay(100);

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

#ifdef SI4732
  digitalWrite(SI4735_RESET, LOW);
  _INFO("AM and FM station tuning test, looking for a Si4732 chip\n");
#else
  digitalWrite(SI4735_RESET, HIGH);
  _INFO("AM and FM station tuning test, looking for a Si4735 chip\n");
#endif //SI4732

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
  _INFO("Si473x I2C address is %04x\n",si4735Addr);
  delay(500);

  SI4735_loadSSB(Band_slot);
  _INFO("Si473x SSB mode enabled\n");

  rx.setVolume(FT8_VOLUME);
  _INFO("Si473x default volume %d\n",rx.getVolume());

  enabled=true;
}
/*-----------------------------------------
 * Show the current receiver status
 */
void SI4735_Status() {

  if (!enabled) return;
  uint16_t currentFrequency = rx.getFrequency();
  rx.getCurrentReceivedSignalQuality();
  _INFO("f=%d MHz SNR: %8.2f dB Signal: %8.2f dBuV\n",currentFrequency / 100.0,rx.getCurrentSNR(),rx.getCurrentRSSI());

}
#endif //RX_SI4735