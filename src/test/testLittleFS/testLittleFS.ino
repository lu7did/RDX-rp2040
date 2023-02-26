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
char hi[128];

/*
 

ADIF Export
<adif_ver:5>3.1.1
<created_timestamp:15>20220108 234130
<programid:6>WSJT-X
<programversion:5>2.3.0
<eoh>
<call:6>JA0EOK <gridsquare:0> <mode:3>FT8 <rst_sent:3>-06 <rst_rcvd:3>-20 <qso_date:8>20220108 <time_on:6>211800 <qso_date_off:8>20220108 
 <time_off:6>234130 <band:3>10m <freq:9>28.076331 <station_callsign:5>LU7DZ <my_gridsquare:6>GF11FX <comment:25>FT8  Sent: -06  Rcvd: -20 <eor>

*/
void addHeader(char *str,char *programid,char *version) {
  sprintf(str+strlen(str),"ADIF Export\n<adif_ver:5>3.1.1\n<created_timestamp:15>20220108 234130\n<programid:%d>%s\n<programversion:%d>%s\n<eoh>\n",strlen(programid),programid,strlen(version),version);
}
void addItem(char *str,char *token,char *item) {
  sprintf(str+strlen(str),"<%s:%d>%s ",token,strlen(item),item);
}
void addFooter(char *str) {
  sprintf(str+strlen(str),"<eor>\n");

}

int writeQSO(char *call,char *grid, char *mode, char *rst_sent,char *rst_rcvd,char *qso_date,char *time_on, char *band, char *freq, char *mycall, char *mygrid, char *message) {

char adifStr[512];
  
  strcpy(adifStr,"");
  if(!LittleFS.exists("/rdx6.adif")) {
     addHeader(adifStr,(char*)"RDX",(char*)"v2.0");
  }


  File f=LittleFS.open("/rdx6.adif","a");
  if (!f) {
    sprintf(hi,"File open error\n");
    Serial.print(hi);
    return -1; 
  }
  
  addItem(adifStr,(char*)"call",call);
  addItem(adifStr,(char*)"grid",grid);
  addItem(adifStr,(char*)"mode",mode);
  addItem(adifStr,(char*)"rst_sent",rst_sent);
  addItem(adifStr,(char*)"rst_rcvd",rst_rcvd);
  addItem(adifStr,(char*)"qso_date",qso_date);
  addItem(adifStr,(char*)"time_on",time_on);
  addItem(adifStr,(char*)"qso_date_off",qso_date);
  addItem(adifStr,(char*)"time_off",time_on); 
  addItem(adifStr,(char*)"band",band);
  addItem(adifStr,(char*)"freq",freq);
  addItem(adifStr,(char*)"station_callsign",mycall);
  addItem(adifStr,(char*)"my_gridsquare",mygrid);
  addItem(adifStr,(char*)"comment",message);
  addFooter(adifStr);
  
  size_t x=f.write(adifStr,strlen(adifStr));
  
  sprintf(hi,"%s",adifStr);
  Serial.print(hi);
  
  f.close();  
  return 0;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);
  Serial.flush();
  sprintf(hi,"testLittleFS evaluation firmware\n");
  Serial.print(hi);
  LittleFS.begin();
  writeQSO((char*)"JA3SPO",(char*)"XX20",(char*)"FT8",(char*)"-20",(char*)"-12",(char*)"20230108",(char*)"003000",(char*)"10m",(char*)"28074",(char*)"LU7DZ",(char*)"GF05",(char*)"Test QSO");
  writeQSO((char*)"JA1ABC",(char*)"XX21",(char*)"FT8",(char*)"-20",(char*)"-12",(char*)"20230108",(char*)"003000",(char*)"10m",(char*)"28074",(char*)"LU7DZ",(char*)"GF05",(char*)"Test QSO");
  writeQSO((char*)"K5SE",(char*)"NN21",(char*)"FT8",(char*)"-20",(char*)"-12",(char*)"20230108",(char*)"003000",(char*)"10m",(char*)"28074",(char*)"LU7DZ",(char*)"GF05",(char*)"Test QSO");
  writeQSO((char*)"LT7H",(char*)"FF78",(char*)"FT8",(char*)"-20",(char*)"-12",(char*)"20230108",(char*)"003000",(char*)"10m",(char*)"28074",(char*)"LU7DZ",(char*)"GF05",(char*)"Test QSO");
  writeQSO((char*)"PY2RON/PY1",(char*)"FA34",(char*)"FT8",(char*)"-20",(char*)"-12",(char*)"20230108",(char*)"003000",(char*)"10m",(char*)"28074",(char*)"LU7DZ",(char*)"GF05",(char*)"Test QSO");
  LittleFS.end();
}

void loop() {
  // put your main code here, to run repeatedly:

}
