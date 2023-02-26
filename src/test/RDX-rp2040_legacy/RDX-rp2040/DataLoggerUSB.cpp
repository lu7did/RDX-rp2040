#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef DATALOGGERUSB
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * DataLoggerUSB.cpp                                                                           *
 * Simple logger with USB upload to PC                                                         *
 * Uses SingleFileDrive to export an onboard LittleFS file to the computer                     *
 * The PC can open/copy the file, and then the user can delete it to restart                   *
 *                                                                                             *
 * Released to the public domain,  2022 - Earle F. Philhower, III                              *
 *---------------------------------------------------------------------------------------------*
  Copyright (c) 2022 by Earle F. Philhower, III1. All rights reserved.                     
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
 * This implementation provides a simplified link between a single file into the flash memory  *
 * based filesystem (LittleFS) which is rdx.txt, containing the ADIF logbook if enabled, and a *
 * file which is exported and made visible on the PC calles rdx_logbook.txt                    *
 *---------------------------------------------------------------------------------------------*
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <SingleFileDrive.h>
#include <LittleFS.h>

uint32_t cnt = 0;
bool okayToWrite = true;
bool SingleFileDriveactive = false;
char logbook[32];



/*---------------------------------------------------------------
 * Make the ADIF file and give it a simple header if it doesn't
 * exists previously
 */
void headerADIF() {
  cnt = 0;
}
/*----------------------------------------------------------------
 * Called when the USB stick is connected to the PC and the drive
 * is opened, note that only called when modifying the exported
 * file.
 * This is called from within a USB IRQ so no conflicting printing
 * to serial/USB or other peripherical activity, also needs to keep
 * to a minimum the time 
 */
void plugUSB(uint32_t i) {
  (void) i;
  okayToWrite = false;
}
/*-------------------------------------------------------------------
 * Called when the USB is ejected or removed from a PC
 * This is called from within a USB IRQ so no conflicting printing
 * to serial/USB or other peripherical activity, also needs to keep
 * to a minimum the time 
 */
void unplugUSB(uint32_t i) {
  (void) i;
  okayToWrite = true;
}

/*--------------------------------------------------------------------
 * Called when the PC tries to delete the file
 * This is called from within a USB IRQ so no conflicting printing
 * to serial/USB or other peripherical activity, also needs to keep
 * to a minimum the time 
 */
void deleteADIF(uint32_t i) {
  (void) i;
  LittleFS.remove(adiffile);
  headerADIF();
}
/*--------------------------------------------------------------------
 * When activated initialize the service
 */
void data_setup() {

  SingleFileDriveactive=true;
  LittleFS.begin();

  /*------------------------------------------
   * Setup the USB disk share
   */
  singleFileDrive.onDelete(deleteADIF);
  singleFileDrive.onPlug(plugUSB);
  singleFileDrive.onUnplug(unplugUSB);
  singleFileDrive.begin(adiffile,logbook);

  /*-------------------------------------------
   * Find the end of the data
   */
  File f = LittleFS.open((char*)ADIFFILE, "r");
  if (!f || !f.size()) {
    cnt = 1;
    headerADIF();
  } else {
    if (f.size() > 2048) {
      f.seek(f.size() - 1024);
    }
    do {
      String s = f.readStringUntil('\n');
      sscanf(s.c_str(), "%lu,", &cnt);
    } while (f.available());
    f.close();
    cnt++;
  }
  _INFOLIST("%s USB file (%s) export started\n",__func__,adiffile);
  
}
/*-----------------------------------------------------
 * Complete the USB export
 */
void data_stop() {
  LittleFS.end();
  SingleFileDriveactive=false;
  _INFOLIST("%s USB file export finalized\n",__func__);
}
/*-----------------------------------------------------
 * Loop, mostly improductive but left open for some 
 * RTOS like maintenance
 */
void data_loop() {
  
  noInterrupts();
  if (okayToWrite) {
  }
  interrupts();
  delay(1000);
}
#endif //DATALOGGERUSB
