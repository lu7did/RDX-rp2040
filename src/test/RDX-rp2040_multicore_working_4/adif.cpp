#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef ADIF 
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * adif.cpp                                                                                    *
 * ADIF logging facility                                                                       *
 * Creation of an ADIF file with all QSO performed by the transceiver                          *
 *---------------------------------------------------------------------------------------------*
 
  Copyright (c) 2023 Dr. Pedro E. Colla (LU7DZ). All rights reserved.                     
  This file is part of the RDX Digital Transceiver project for Arduino environment.

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
  Adaptation and integration with RDX project by Dr. Pedro E. Colla (LU7DZ) 2022               *
 *---------------------------------------------------------------------------------------------*
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "Wire.h"
#include <stdio.h>
#include <WiFi.h>
#include <time.h>
#include <stdbool.h>
#include <FS.h>
#include <LittleFS.h>

/*
ADIF format template 

<quote>

ADIF Export
<adif_ver:5>3.1.1
<created_timestamp:15>yyyymmdd hhmmss
<programid:3>RDX
<programversion:6>2.0.53
<eoh>

<call:6>JA0EOK <gridsquare:0> <mode:3>FT8 <rst_sent:3>-06 <rst_rcvd:3>-20 <qso_date:8>20220108 
<time_on:6>211800 <qso_date_off:8>20220108 
<time_off:6>234130 <band:3>10m <freq:9>28.076331 <station_callsign:5>LU7DZ 
<my_gridsquare:6>GF11FX <comment:25>FT8  Sent: -06  Rcvd: -20 
<eor>

</quote>
*/

class adifFile {
  public:
  
  adifFile();
  void addHeader(char *str,char *programid, char *version);
  void addItem(char *str,char *token,char *item);
  void addFooter(char *str);

  private:
    uint16_t zambo;

};

/*------------------------------------------
 * Constructor
 * Store the log file name
 */
adifFile::adifFile() {
  return;
  //_INFOLIST("%s ADIF Export file set",__func__);
}
/*-------------------------------------------
 * ADIF Header
 * only once per file
 */
void adifFile::addHeader(char *str,char *programid,char *version) {
    char tstr[16];
    now = time(0) - t_ofs;  \
    gmtime_r(&now, &timeinfo);  \
    sprintf(tstr,"%04d%02d%02d %02d%02d%02d",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec); 
    sprintf(str+strlen(str),"ADIF Export\n<adif_ver:5>3.1.1\n<created_timestamp:%d>%s\n<programid:%d>%s\n<programversion:%d>%s\n<eoh>\n",strlen(tstr),tstr,strlen(programid),programid,strlen(version),version);
}

/*----------------------------------------------
 * ADIF element added
 * Called as many times as elements on the ADIF
 * record are included, this aggregates the item
 * into the area pointed by str
 */
void adifFile::addItem(char *str,char *token,char *item) {
  sprintf(str+strlen(str),"<%s:%d>%s ",token,strlen(item),item);
}
/*-----------------------------------------------
 * ADIF record completion
 * Write the full string with the ADIF record and
 * adds the end of record marker
 */
void adifFile::addFooter(char *str) {
  sprintf(str+strlen(str),"<eor>\n");
}


adifFile adif=adifFile();
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
int writeQSO(char *adifFile, char *call,char *grid, char *mode, char *rst_sent,char *rst_rcvd,char *qso_date,char *time_on, char *band, char *freq, char *mycall, char *mygrid, char *message) {

char adifStr[512];
char adifHdr[24];

#ifdef DATALOGGERUSB
  if (!okayToWrite || SingleFileDriveactive) {
     _INFOLIST("%s USB device busy, write request rejected\n",__func__);
     return -1;
  }
#endif //DATALOGGERUSB

  LittleFS.begin();
  strcpy(adifStr,"");

  /*------------------
   * If the file !exists then create it and include a haader
   */
  if(!LittleFS.exists(adifFile)) {
     sprintf(adifHdr,"v%s.%s",(char*)VERSION,(char*)BUILD);    
     adif.addHeader(adifStr,(char*)PROGNAME,adifHdr);
  }

  /*-------------------
   * Open the file for update, new records are appended at
   * the end of the file
   */
  File f=LittleFS.open(adifFile,"a");
  if (!f) {
    _INFOLIST("%s File (%s) open error\n",__func__,adifFile);
    return -1; 
  }

  /*--------------------
   * Create an ADIF record
   */
  adif.addItem(adifStr,(char*)"call",call);
  adif.addItem(adifStr,(char*)"grid",grid);
  adif.addItem(adifStr,(char*)"mode",mode);
  adif.addItem(adifStr,(char*)"rst_sent",rst_sent);
  adif.addItem(adifStr,(char*)"rst_rcvd",rst_rcvd);
  adif.addItem(adifStr,(char*)"qso_date",qso_date);
  adif.addItem(adifStr,(char*)"time_on",time_on);
  adif.addItem(adifStr,(char*)"qso_date_off",qso_date);
  adif.addItem(adifStr,(char*)"time_off",time_on); 
  adif.addItem(adifStr,(char*)"band",band);
  adif.addItem(adifStr,(char*)"freq",freq);
  adif.addItem(adifStr,(char*)"station_callsign",mycall);
  adif.addItem(adifStr,(char*)"my_gridsquare",mygrid);
  adif.addItem(adifStr,(char*)"comment",message);

  /*---------------------
   * Close the ADIF record and write it
   */
  adif.addFooter(adifStr);
  size_t x=f.write(adifStr,strlen(adifStr));
  _INFOLIST("%s ADIF(%s)\n",__func__,adifStr);
  
  f.close();  
  LittleFS.end();

  return 0;
}
void setup_adif() {

  //_INFOLIST("%s \n",__func__);
  tft_iconState(CALICON,false);
  
}

#endif //RP2040_W && FSBROWSER && ADIF
