#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef CAT
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * cat.cpp                                                                                     *
 * CAT protocol implementation emulating the Kenwood TS2000 protocol                           *
 *---------------------------------------------------------------------------------------------*
 *  Copyright  RDX project by Dr. Pedro E. Colla (LU7DZ) 2022. All rights reserved             *
 * This implementation provides a simplified CAT processor that can be used to control the     *
 * transceiver from another computer with a suitable program such as fldigi                    *
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
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
 
#include <stddef.h>
#include "cat.h"
/*
ID;
*/
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//                                     CAT Support functions                                                  *
//                partial implementation of the Kenwood TS2000 protocol (115200 8N2).                         *
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
void CAT_warning() {

      if (millis()-CATT1 >=500 && CATT2 !=0 && cat_stat != 0) {

       flipLED=!flipLED;
       digitalWrite(WSPR, flipLED); 
       digitalWrite(FT8, flipLED);
       CATT1=millis();

       if (millis()-CATT2 >= 10000) {

          CATT1=0;
          CATT2=0;
          digitalWrite(WSPR, LOW); 
          digitalWrite(FT8, LOW);

       } 

      }
}      
/*------------------------------
  CAT_process
  receives a string with a CAT command (delimited by "";"") and reacts with the
  proper answer to that command.
  Only FA,IF,TX and RX commands are properly implemented, the rest of the commands produces
  mocked up responses to satisfy the logic of (mostly) WSJT-X
*/
bool CAT_process(char *c,char *r,char *cmd,char *arg){

  char *q;
  strcpy(cmd,"");
  strcpy(arg,"");

  cmd[0]=c[0];
  cmd[1]=c[1];
  cmd[2]=0x00;

  if (strlen(cmd) != strlen(c)) {
     strcpy(arg,&c[2]);
  } else {
     strcpy(arg,"");
  }

  if (strlen(cmd)<2) {
    _INFO("malformed command, ignored\n");
    return false;
  }

  if (strcmp(cmd,"FA")==0) {  
      if (strcmp(arg,"") != 0) {
        unsigned long fx=strtol(arg, &q, 10);
        freq=fx;
        
        CATT1=millis();
        CATT2=millis();
        stopTX();
      }    
      uint32_t fa=freq;
      String sent = "FA" // Return 11 digit frequency in Hz.  
          + String("00000000000").substring(0,11-(String(fa).length()))   
          + String(fa) + ";";     
      strcpy(r,sent.c_str());
      return true;
  }
  

  if (strcmp(cmd,"PS")==0) {
      strcpy(r,"PS1;");
      return true;
  }

  if (strcmp(cmd,"TX")==0)  {   
      strcpy(r,"TX0;");
      startTX();
      return true;
  } 

  if (strcmp(cmd,"RX")==0) {  
    strcpy(r,"RX0;");
    stopTX();
    return true;       
  }

  if (strcmp(cmd,"ID")==0) {  
      strcpy(r,"ID019;");
      return true;
  }

  if (strcmp(cmd,"AI")==0) {
      strcpy(r,"AI0;"); 
      return true;
  }

  if (strcmp(cmd,"IF")==0) {

      if (TX_State == 1) {  
          String sent = "IF" // Return 11 digit frequency in Hz.  
                  + String("00000000000").substring(0,11-(String((long int)freq).length()))   
                  + String((long int)freq) + "0000" + "+" + "00000" + "0" + "0" + "0" + "00" + "12" + "0000000;"; 
          strcpy(r,sent.c_str());        
      } else {  
          String sent = "IF" // Return 11 digit frequency in Hz.  
                  + String("00000000000").substring(0,11-(String((long int)freq).length()))   
                  + String((long int)freq) + "0000" + "+" + "00000" + "0" + "0" + "0" + "00" + "02" + "0000000;"; 
          strcpy(r,sent.c_str());
      } 
      return true;
  }


  if (strcmp(cmd,"MD")==0) {  
      strcpy(r,"MD2;");
      return true;
  }
  strcpy(r,"ID019;");   //dummy answer trying not to disrupt the CAT protocol flow
  _INFO("***ERROR*** Entry(%s) not processed response(%s)\n",c,r);
  return false;

}



/*-----------------------------------------------
  CAT_check
  Read the serial port and parses the input for
  non-confirming structures
  parse the input stream and tokenize the commands
  found on it.
  Take into consideration some protocol deviations
  (oddities) required by WSJT-X (or HamLib, I don't know)
  to properly operate
*/
void CAT_check(void) {

bool flagRXTX=false;
char cmd[4];
char arg[16];

/*----------
  Handle the brief frequency change
*/
CAT_warning();

/*---------- ID;
  Check if data is available on the serial port
*/
int nread=_CAT.available();
if (nread > 0){
   if (cat_stat == 0) {
       cat_stat=1;
       digitalWrite(WSPR, LOW); 
       digitalWrite(FT8, HIGH);
       digitalWrite(JS8, HIGH);
       digitalWrite(FT4, HIGH);
       stopTX();
       _INFO("CAT sub-system initialized wait(%d)\n",nread);
   }
} else {
  return;
}

/*-----------
  Too small as a packet, perhaps fragmentation is occuring
*/
if (nread < 3) { 
  uint8_t z=_CAT.read();
  return;
}

/*-----------
  Read the serial port buffer
*/
int rc=_CAT.readBytes(buf,nread);
if (rc <= 1) {return;}
buf[rc]=0x0;

/*------------
  Look after spurious contents
*/
int k=strlen(serialBuf);
for (int i=0;i<strlen(buf);i++){
    char c=buf[i];
    if (c>= 0x20 && c<=0x5f && c!=0x0a && c!=0x0d) {
       serialBuf[k++]=c;
    }
}
serialBuf[k]=0x00;
    
/*-------------
  Fragmentation might occur
*/
if (serialBuf[strlen(serialBuf)-1] != ';') {
   return;
}

/*--------------
  Look for oddities from WSJT-X, if this string is
  received the action to turn the TX on is expected
  but only the answer to the ID command needs to 
  be sent
*/
//_INFO("CAT Command(%s) len(%d)\n",serialBuf,strlen(serialBuf));

if (strcmp(serialBuf,"TX;ID;") == 0) {

    startTX();
    strcpy(CATResp,"ID019;");
    _CAT.print(CATResp);
    //_INFO("CAT Command(%s) len(%d)\n",CATResp,strlen(CATResp));

    strcpy(CATResp,"");
    strcpy(serialBuf,"");      
    return;

}

/*-------------------
  More oddities, now with the receiving part
*/
    if (strcmp(serialBuf,"RX;ID;") == 0) {      
       stopTX();
       strcpy(CATResp,"ID019;");
       _CAT.print(CATResp);
       //("CAT Command(%s) len(%d)\n",CATResp,strlen(CATResp));

       strcpy(CATResp,"");
       strcpy(serialBuf,"");      
       return;

    }

/*----------------------
  Parse the command using the ";" as the
  token delimiter
*/
    int j=0;
    strcpy(CATCmd,"");
    strcpy(resp,"");
    int last=0;

    for(int i=0;i<strlen(serialBuf);i++) {
       char data=serialBuf[i];
       if (data==';') {
          last=i;
          strcpy(cmd,"");
          strcpy(arg,"");         

          /* EOT mark found --> process CAT command */
          
          if (!CAT_process(CATCmd,CATResp,cmd,arg)) {
            _CAT.print(CATResp);
            //_INFO("CAT Command(%s) len(%d)\n",CATResp,strlen(CATResp));
             strcpy(serialBuf,"");
             strcpy(CATCmd,"");
             strcpy(CATResp,"");
             return;
          }

          /*--- Yet another WSJT-X oddity ---*/

          if (strcmp(cmd,"FA")==0) {
             if (ignoreFA==true) {
                ignoreFA=false;
                strcpy(CATCmd,"");
                strcpy(CATResp,"");
             } else {
                _CAT.print(CATResp);
                //_INFO("CAT Command(%s) len(%d)\n",CATResp,strlen(CATResp));
                strcpy(resp,"");
                ignoreFA=true;
             }   
          } else {
             strcat(resp,CATResp);
             ignoreFA=false;
          }   
          strcpy(CATCmd,"");
          strcpy(CATResp,"");
          j=0;
       } else {

         /*--- Between tokens store the incoming data */
         CATCmd[j++]= data;
         CATCmd[j]=0x00;
       }
    }

    /*------
     Decide whether fragmentation happened 
     */
    if (last != strlen(serialBuf)) {
       strcpy(serialBuf,&serialBuf[last+1]);
    } else {
       strcpy(serialBuf,"");
    }       

    /*-------
     Reply to any pending message
     */
    if (strlen(resp)>0) {
       _CAT.print(resp);      
       //_INFO("CAT Command(%s) len(%d)\n",resp,strlen(resp));

       if (strcmp(cmd,"RX")==0 || strcmp(cmd,"TX") == 0 ) delay(50);
    }

    /*-------- 
      Clean up buffers
    */
    strcpy(resp,"");
    strcpy(CATCmd,"");
    strcpy(CATResp,"");
    return;
}
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//                                         end of CAT Protocol handler                                        *
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#endif //CAT
