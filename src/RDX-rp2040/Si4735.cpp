#include <BluetoothHIDMaster.h>

#include <SI4735.h>

#include <SI4735.h>

#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef RX_SI473X
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * RDX_Si4735.ino                                                                              *
 * The table below shows the Si4735 and RASPBERRY PICO connections modified for the RDX project*                                                                                           *   
 * | ---------------| ---------- |                                                             *
 * | Si4735 pin     |  PICO Pin  |                                                             *
 * | ---------------| ---------- |                                                             *
 * | RESET (pin 15) |     GP16   |                                                             *
 * | SDIO (pin 18)  |     GP0    |                                                             *
 * | CLK (pin 17)   |     GP1    |                                                             *
 * | ---------------| ---------- |                                                             *
 * PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI4735/extras/apidoc/html/        *
 * RDX project documentation: http://www.github.com/lu7did/RDX-rp2040                          *
 *                                                                                             *
 * Released by the public domain By Ricardo Lima Caratti, Nov 2019.                            *
 * Released to the public domain By Dr. Pedro E. Colla, Apr 2023.                              *
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
 * Adaptation and integration with RDX project by Dr. Pedro E. Colla (LU7DZ) 2022,2023         *
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
bool SI473x_enabled=false;
bool disableAgc=false;
//bool avc_en=true;

SI4735 rx;

int currentBFO = 975;
uint16_t currentFrequency;
uint8_t currentStep = 1;
uint8_t currentBFOStep = 25;
uint8_t bandwidthIdx = 2;   //Was 2 for 3.0 KHz, now it is 3 for 4.0 KHz
const char *bandwidth[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};
uint8_t currentAGCAtt = 0;  // currentAGCAtt == 0)
uint8_t si473x_rssi   = 0;
uint32_t rssitimer = 0;

/*------------------------------------------------------------------------------------------
 * set the Si473x chip frequency 
  *-----------------------------------------------------------------------------------------*/
void SI473x_setFrequency(int s) {

  uint16_t b=Bands[s-1];
  uint16_t i=Band2Idx(b);

  uint16_t minimumFreq=uint16_t(slot[i][1]/1000);
  uint16_t maximumFreq=uint16_t(slot[i][2]/1000);
  uint16_t currentFreq=uint16_t(slot[i][0]/1000);
  uint16_t currentStep=1;
  uint16_t currentSSB=USB;
   
  rx.setSSB(minimumFreq,maximumFreq,currentFreq,currentStep,currentSSB);
  delay(10);
  currentFrequency = rx.getCurrentFrequency();
  String freqDisplay;
  freqDisplay = String((float)currentFrequency / 1000, 3);
  _INFO("Frequency set to %s. KHz\n",freqDisplay.c_str());
  rx.setI2CFastModeCustom(100000); // Increase the transfer I2C speed


}
/*------------------------------------------------------------------------------------------
 * setup the Si473x sub-system, find out the Si473x chip I2C address and connect to it
 * then initialize the frequency of operation and other parameters needed.
  *-----------------------------------------------------------------------------------------*/
void SI473x_Setup()
{

  digitalWrite(RESET_SI473X, HIGH);
  //digitalWrite(INT_SI473X,LOW);

  delay(10);
  _INFO("Loading Si473x library (c) PU2CLR in SSB mode reset(%d)\n",int16_t(RESET_SI473X));
  
  /*-------------------------------------
    get the Si473x I2C buss address
   */
  int16_t si473xAddr = rx.getDeviceI2CAddress(RESET_SI473X);
  if ( si473xAddr == 0 ) {
    _INFO("Si473X chip not found, processing is blocked!\n");
    while (1);
  } else {
    _INFO("Si473X chip found at address 0x%X\n",si473xAddr);

  }
/*
  uint32_t trst=time_us_32();
  while(time_us_32()-trst<1);
  digitalWrite(INT_SI473X,HIGH);
*/

  rx.setup(RESET_SI473X, AM_FUNCTION);
  //rx.setup(RESET_SI473X,0,AM_FUNCTION,SI473X_ANALOG_AUDIO,XOSCEN_CRYSTAL,0);
  delay(10);
  _INFO("Device reset completed...\n");

  long et1 = millis();
  rx.setI2CFastModeCustom(500000); // Increase the transfer I2C speed
  rx.loadPatch(ssb_patch_content, size_content); // It is a legacy function. See loadCompressedPatch 
  //rx.setI2CFastMode();
  rx.setI2CFastModeCustom(100000); // Increase the transfer I2C speed

  long et2 = millis();
 
  _INFO("SSB patch loaded successfully (%ld msec)\n",(et2-et1));
  delay(10);
  
  rx.setTuneFrequencyAntennaCapacitor(1); // Set antenna tuning capacitor for SW.
  _INFO("Antenna Capacitor set\n");

  SI473x_setFrequency(Band_slot);
  _INFO("Frequency set slot(%d)\n",Band_slot);
  
  currentStep = 1;

  delay(10); 
  rx.setFrequencyStep(currentStep);  
  _INFO("set FrequencyStep as %d\n",currentStep);
/*----------------
  Set the Audio bandwidth to 4 KHz
*/
  rx.setSSBAudioBandwidth(bandwidthIdx);
  _INFO("Bandwidth index(%d)\n",bandwidthIdx);
  
  //rx.setSSBSidebandCutoffFilter(1);
  rx.setSSBSidebandCutoffFilter(0);
  _INFO("Cutoff filter set\n");

  rx.setSSBBfo(currentBFO);  
  _INFO("set BFO as %d\n",currentBFO);
/*-----------------
  Switch on/off AGC disableAGC=1 disable, AGC Index=0. Minimum attenuation (max gain)
*/  
  rx.setAutomaticGainControl(disableAgc, currentAGCAtt);  
  _INFO("set AGC as %d==%d\n",disableAgc,currentAGCAtt);

  rx.setSSBAutomaticVolumeControl(false);  
  _INFO("Set automatic volume control\n");

  //rx.setSSBAutomaticVolumeControl(true);       //Very small audio signal if enabled *beware of the drunken sailor*
  //rx.setSSBAutomaticVolumeControl(avc_en);  
  //_INFO("set Volume as %d\n",avc_en);

/*------------------
  Maximum gain for automatic volume control on AM/SSB mode (12 to 90dB)
*/
  //rx.setVolume(45);    //MV
  rx.setVolume(255);    //MV
  _INFO("Set volume\n");
  delay(10);

  currentFrequency = rx.getCurrentFrequency();
  String freqDisplay;
  freqDisplay = String((float)currentFrequency / 1000, 3);
   _INFO("current frequency read from Si473x is %s\n",freqDisplay.c_str());
/*
  _INFO("Performing Jitanjanfora mode\n");
  for (int k=0;k<30;k++){
  uint32_t trst=time_us_32();
  while(time_us_32()-trst<1000000);
  currentFrequency = rx.getCurrentFrequency();
  }

  _INFO("End of Jitanjanfora mode\n");
*/


  SI473x_enabled=true;
  SI473x_Status(); //for test purpose only



} //SI473x_setup() end

/*-----------------------------------------
 * Show the current receiver status
 */
void SI473x_Status() {

if (!SI473x_enabled) {
   _INFO("SI473X chip not enabled, return\n");
   return;
}

  rx.getAutomaticGainControl();
  rx.getCurrentReceivedSignalQuality();

  String bfo;
  if (currentBFO > 0)
    bfo = "+" + String(currentBFO);
  else
    bfo = String(currentBFO);

  //currentFrequency = rx.getCurrentFrequency();
  String freqDisplay = getFrequency();
  //freqDisplay = String((float)currentFrequency / 1000, 3);

  _INFO("|AGC:%s|LNA GAIN index:%d/%d|BW:%s KHz|S=%d|SNR %d|RSSI %d dBuV|Volume: %d|BFO %s|BFOStep: %d|freq=%s|Step: %d|\n", \
       (rx.isAgcEnabled() ? "AGC ON" : "AGC OFF"), \
        rx.getAgcGainIndex(), \
        currentAGCAtt, \
        bandwidth[bandwidthIdx], \
        getSignal(), \
        getSNR(), \
        getRSSI(), \
        getVolume(), \
        bfo.c_str(), \
        currentBFOStep, \
        freqDisplay.c_str(), \
        currentStep);

  rx.setI2CFastModeCustom(100000); // Increase the transfer I2C speed

  
}
int getRSSI() {

    if (time_us_32()-rssitimer > 1000000) {
       rx.getCurrentReceivedSignalQuality();
       rssitimer=time_us_32();
    }   
    return rx.getCurrentRSSI();
}
int getSignal() {
    int rssi = rx.getCurrentRSSI();
    int rssiAux = 0;
    if (rssi < 2) rssiAux = 4;
    else 
       if (rssi < 4) rssiAux = 5;
       else 
          if (rssi < 12) rssiAux = 6;
          else 
             if (rssi < 25) rssiAux = 7;
             else 
                if (rssi < 50) rssiAux = 8;
                else
                  rssiAux = 9;
   return rssiAux;
}
String getFrequency() {
   int currentFrequency = rx.getCurrentFrequency();
   String freqDisplay;
   freqDisplay = String((float)currentFrequency / 1000, 3);
   return freqDisplay;
}
int getSNR() {
   int snr=rx.getCurrentSNR();   //Needs to previously call getRSSI to have a fresh reading of the signal level
   return snr;
}   
int getVolume() {
   int Vol=rx.getCurrentVolume();
   return Vol;

}
void setVolume() {
  rx.setVolume(255);
}
#endif //RX_SI473X
