#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef CAT 
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * cat.cpp                                                                                     *
 * CAT controller                                                                              *
 * CAT controller emulating the Yaesu TS2000 CAT protocol                                      *
 *---------------------------------------------------------------------------------------------*
 * Original extract from ADX_CAT_V1.4 - Version release date: 04/09/2023                       *
 * Barb(Barbaros ASUROGLU) - WB2CBA - 2022                                                     *
 * ADX CAT code inspired from Lajos Höss, HA8HL TS2000 CAT implementation for Arduino.         *
 *---------------------------------------------------------------------------------------------*
  This file is part of the RDX Digital Transceiver project for Arduino environment.            *
  Adaptation and integration with RDX project by Dr. Pedro E. Colla (LU7DZ) 2022               *
 *---------------------------------------------------------------------------------------------*
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
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "Wire.h"
#include <stdio.h>
#include <WiFi.h>
#include <Time.h>
#include <stdbool.h>
#include <FS.h>
#include <LittleFS.h>

/*
CAT Protocol definitions 
CAT CONTROL RIG EMULATION: KENWOOD TS2000
SERIAL PORT SETTINGS: 115200 baud,8 bit,1 stop bit
When CAT is active FT4 and JS8 leds will be solid lit.
In CAT mode none of the Switches and leds are active including TX SWITCH in order to avoid different setting clashes except TX LED. 
TX LED WILL BE LIT briefly on/off and then solid during TX WHEN TRANSMITTING IN CAT Mode.
In CAT mode ADX can be controlled ONLY by CAT interface. Frequency and TX can be controlled via CAT.
To get out of CAT mode and to use ADX with Switch and led interface just recycle power. Once activated CAT mode stays active as rig control Until power recycle. 
In CAT mode manual Band setup is deactivated and ADX can be operated on any band as long as the right lpf filter module is plugged in. 
IN CAT MODE MAKE SURE THE CORRECT LPF MODULE IS PLUGGED IN WHEN OPERATING BAND IS CHANGED!!! IF WRONG LPF FILTER MODULE IS PLUGGED IN then PA POWER MOSFETS CAN 
BE DAMAGED!

*/

//------------ CAT Variables

int cat_stat = 0;
int CAT_mode = 2;  

String received;
String receivedPart1;
String receivedPart2;    
String command;
String command2;  
String parameter;
String parameter2; 
String sent;
String sent2;

/*--------------------------------------------------------------------------------------
  CAT_check
  The serial port associated with CAT is checked to see if there are available commands
  to process
 */ 

void CAT_Check() { 
  if (_CAT.available() > 1){
     cat_stat = 1; 
     
     //si5351.set_freq(freq * 100ULL, SI5351_CLK1);
     //si5351.output_enable(SI5351_CLK1, 1);   //RX on
     //digitalWrite(WSPR, LOW); 
     //digitalWrite(JS8, HIGH); 
     //digitalWrite(FT4, HIGH); 
     //digitalWrite(FT8, LOW); 
     _INFO("CAT data available (%d bytes)\n",_CAT.available());
                     
     CAT_Control(); 
     
    
  } else {
    String dummy = Serial.readString(); 
  }
}

/*--------------------------------------------------------------------------------------
  CAT_control
  Main CAT loop reading and command processing
 */ 
void CAT_Control(void) {

  received = Serial.readString();  
  received.toUpperCase();  
  received.replace("\n","");  

  _INFO("Received (%s) len=%d\n",received.c_str(),received.length());
  String data = "";
  int bufferIndex = 0;

  for (int i = 0; i < received.length(); ++i) {
              
    char c = received[i]; 
    _INFO("Character c=%0x bufferIndex=%d\n",byte(c),bufferIndex);
    if (c != ';') {
        data += c;
    } else {
      if (bufferIndex == 0) {  
          data += '\0';
          receivedPart1 = data;
          bufferIndex++;
          data = "";
      } else  {  
          data += '\0';
          receivedPart2 = data;
          bufferIndex++;
          data = "";
      }
    }

  }
    
  command = receivedPart1.substring(0,2);
  command2 = receivedPart2.substring(0,2);    
  parameter = receivedPart1.substring(2,receivedPart1.length());
  parameter2 = receivedPart2.substring(2,receivedPart2.length());

  _INFO("command =%s command2=%s parameter=%s parameter2=%s\n",command.c_str(),command2.c_str(),parameter.c_str(),parameter2.c_str());
  if (command == "FA") { 
     if (parameter != "") {
         freq=parameter.toInt();
     }
    sent = "FA" + String("00000000000").substring(0,11-(String(freq).length())) + String(freq) + ";";     
  }  
  if (command == "PS") {sent = "PS1;";}
  if (command == "TX") {sent = "TX0;";startTX();} //HERE TURN TRANSCEIVER ON
  if (command == "RX") {sent = "RX0;";stopTX();}  //HERE TURN TRANSCEIVER OFF
  if (command == "ID") {sent = "ID019;";}
  if (command == "AI") {sent = "AI0;";}
  if (command == "MD") {sent = "MD2;";}
  if (command == "IF") {
      if (TX_State == 1) {
        sent = "IF" // Return 11 digit frequency in Hz.  
              + String("00000000000").substring(0,11-(String(freq).length()))   
              + String(freq) + "00000" + "+" + "0000" + "0" + "0" + "0" + "00" + "1" + String(CAT_mode) + "0" + "0" + "0" + "0" + "000" + ";"; 
      } else {  
        sent = "IF" // Return 11 digit frequency in Hz.  
             + String("00000000000").substring(0,11-(String(freq).length()))   
             + String(freq) + "00000" + "+" + "0000" + "0" + "0" + "0" + "00" + "0" + String(CAT_mode) + "0" + "0" + "0" + "0" + "000" + ";"; 
      } 
  }
//------------------------------------------------------------------------------      

  if (command2 == "ID") { sent2 = "ID019;"; }
  
  if (bufferIndex == 2) {
      _CAT.print(sent2);
      _INFO("resp2=%s\n",sent2.c_str());
  } else {
      _CAT.print(sent);
      _INFO("resp=%s\n",sent.c_str());
  }  

  if ((command == "RX") or (command = "TX")) delay(50);

  sent = String("");
  sent2 = String("");  
}

#endif //CAT
