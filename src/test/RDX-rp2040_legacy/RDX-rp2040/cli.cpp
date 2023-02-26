#include <Arduino.h>
#include "RDX-rp2040.h"
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * cli.cpp                                                                                     *
 * Simple command line based configuration tool                                                *
 *---------------------------------------------------------------------------------------------*
 *  Copyright  RDX project by Dr. Pedro E. Colla (LU7DZ) 2022. All rights reserved             *
 * This implementation provides a simplified command processor that can ge used to either      *
 * browse or modify configuration data stored in EEPROM, so the program configuration can be   *
 * changed without rebuild from sources                                                        *
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
#include "cli.h"

char outBuffer[1024];
char inBuffer[128];
int  ptrBuffer=0;
CALLBACK quitCallBack=NULL;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                                     Generic handlers                                        */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
bool handleStr(int idx, char *_var, char *_arg,char *_out) {
  
     if (strcmp(_arg,"") == 0) {
        sprintf(_out+strlen(_out),"%s\n",_var);
        return false;
     }
     if (langSet[idx].toUpper == true) toupperStr(_arg);
     if (langSet[idx].toLower == true) tolowerStr(_arg);
     strcpy(_var,_arg);
     sprintf(_out+strlen(_out),"%s\n",_var);
     return false;
}

bool handleBool(int idx, bool* _bool, char *_arg,char *_out) {

     if (strcmp(_arg,"") == 0) {
        sprintf(_out+strlen(_out),"%d\n",*_bool);
        return false;
     }
     if (!isNumeric(_arg)){
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
        return false;
     }

     char n[16]="";
     parse(_arg,n);   
     int v=atoi(n);

     if (v==0) {
        *_bool=false;
     } else {
        *_bool=true;
     }
     sprintf(_out+strlen(_out),"%d\n",*_bool);
     return false;
}

bool handleNum(int idx, int* _num, char *_arg,char *_out) {

     if (strcmp(_arg,"") == 0) {
        sprintf(_out+strlen(_out),"%d\n",*_num);
        return false;
     }

     if (!isNumeric(_arg)){
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
        return false;
     }

     char n[16]="";
     parse(_arg,n);   
     int v=atoi(n);

     if (v==0) {
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
        return false;
     }  
     
     if (v>=langSet[idx].min && v<=langSet[idx].max) {
        *_num=v;
        sprintf(_out+strlen(_out),"%d\n",*_num);
     } else {
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
     }
     return false;
}

bool handleByte(int idx, uint8_t* _num, char *_arg,char *_out) {

     if (strcmp(_arg,"") == 0) {
        sprintf(_out+strlen(_out),"%d\n",*_num);
        return false;
     }

     if (!isNumeric(_arg)){
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
        return false;
     }

     char n[16]="";
     parse(_arg,n);   
     int v=atoi(n);

     if (v==0) {
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
        return false;
     }  
     
     if (v>=langSet[idx].min && v<=langSet[idx].max) {
        *_num=v;
        sprintf(_out+strlen(_out),"%d\n",*_num);
     } else {
        sprintf(_out+strlen(_out),"invalid parameter (%s)\n",_arg);   
     }
     return false;

}
/*------------------------------------------------
 * Command token definition table
 * Don't touch unless you really know how to handle
 * C pointers (a lot).
 */
cmdSet langSet[MAXTOKEN] =
                      {{"help","Help for all commands",0x00,false,false,0,0,NULL,NULL},
                      {"list","List EEPROM content",0x00,false,false,0,0,NULL,NULL},
                      {"load","Load EEPROM content",0x00,false,false,0,0,NULL,NULL},
                      {"save","Save EEPROM content",0x00,false,false,0,0,NULL,NULL},
                      {"reset","Reset EEPROM to default",0x00,false,false,0,0,NULL,NULL},                     
                      {"?","List all commands",0x00,false,false,0,0,NULL,NULL},
                      {"call","Station callsign",'a',true,false,0,0,NULL,(void*)&my_callsign},
                      {"grid","Station grid locator",'a',true,false,0,0,NULL,(void*)&my_grid},
                      {"adif","Logbook name",'a',false,true,0,0,NULL,(void*)&adiffile},
                      {"ssid","WiFi AP SSID",'a',false,false,0,0,NULL,(void*)&wifi_ssid},
                      {"psk","WiFi AP password",'a',false,false,0,0,NULL,(void*)&wifi_psk},
                      {"log","USB exported logbook",'a',false,false,0,0,NULL,(void*)&logbook},
                      {"msg","FT8 ADIF message",'a',true,false,0,0,NULL,(void*)&qso_message},
                      {"host","Host name",'a',false,false,0,0,NULL,(void*)&hostname},
                      {"writelog","Enable ADIF log write",'b',false,false,0,0,NULL,(void*)&logADIF},
                      {"autosend","Enable FT8 QSO auto",'b',false,false,0,0,NULL,(void*)&autosend},
                      {"tx","Turn TX on/off",'b',false,false,0,0,NULL,NULL},
                      {"tcpport","Telnet Port",'i',false,false,23,10000,NULL,&tcp_port},
                      {"http","FS HTTP Port",'i',false,false,80,10000,NULL,(void*)&http_port},
                      {"web","Web tool Port",'i',false,false,80,10000,NULL,(void*)&web_port},
                      {"ft8try","FT8 Max tries",'n',false,false,1,12,NULL,(void*)&maxTry},
                      {"ft8tx","FT8 Max tx",'n',false,false,1,12,NULL,(void*)&maxTx},
                      {"tz","TZ offset from UTC",'i',false,false,-12,+12,NULL,(void*)&timezone},
                      {"ip","IP address",0x00,false,false,0,0,NULL,NULL},
                      {"rssi","Signal strength (dBm)",0x00,false,false,0,0,NULL,NULL},
                      {"quit","quit terminal mode",0x00,false,false,0,0,NULL,NULL},
                      {"","",0x00,false,false,0,0,NULL,NULL}};

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                                Parameter specific handlers                                  */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
bool txCmd(int idx, char *_cmd,char *_arg,char *_out) {

     char txstr[8];
     strcpy(txstr,"");
     parse(_arg,txstr);
     if (strcmp(txstr,"1")==0) {
        startTX();
        sprintf(_out+strlen(_out),"TX+\n");
     } else {
        stopTX();
        sprintf(_out+strlen(_out),"TX-\n");
     }
     return false;
}

bool listCmd(int idx, char *_cmd,char *_arg,char *_out) {
    strcpy(_out,"\n");
    int i = EEPROM_ADDR_CAL;
    while (!getEEPROM(&i,_out));
    sprintf(_out+strlen(_out),"\n>");
    _INFOLIST("%s list(%s)\n",__func__,_out);

    return false;
}
bool quitCmd(int idx,char *_cmd,char *_arg,char *_out) {
  if (quitCallBack!=NULL) quitCallBack();
  return true;
}

bool loadCmd(int idx, char *_cmd,char *_arg,char *_out) {

   readEEPROM();
   sprintf(_out+strlen(_out),"EEPROM values restored\n>");
   return false;
}
bool saveCmd(int idx, char *_cmd,char *_arg,char *_out) {

   updateEEPROM();
   return false;
}

bool ipCmd(int idx, char *_cmd,char *_arg,char *_out) {
   char ipstr[16];
   strcpy(ipstr,ip);
   rtrimStr(ipstr);
   ltrimStr(ipstr);
   sprintf(_out+strlen(_out),"%s\n",ipstr);
   _INFOLIST("%s ip=%s\n",ip);
   return false;
}  
bool rssiCmd(int idx, char *_cmd,char *_arg,char *_out) {
   sprintf(_out+strlen(_out),"%ul",rssi);
   _INFOLIST("%s rssi=%ul\n",rssi);
   return false;
}  
bool resetCmd(int idx, char *_cmd,char *_arg,char *_out) {

   resetEEPROM();
   sprintf(_out+strlen(_out),"EEPROM values reset to default values\n>");
   return false;
}

bool helpCmd(int idx, char *_cmd,char *_arg,char *_out) {

    for (int i=0;i<MAXTOKEN;i++) {
      if (strcmp(langSet[i].token,"") == 0) {
         _INFOLIST("%s help(%s)\n",__func__,_out);
         delay(200);
         strcpy(_out,"Help system\n");
         return false;
      }
      sprintf(_out+strlen(_out),"(%s) - %s\n",langSet[i].token,langSet[i].help);
      delay(200);
    }
    _INFOLIST("%s help(%s)\n",__func__,_out);
    strcpy(_out,"Help system\n");
    delay(200);
    return false;
}
bool shortCmd(int idx, char *_cmd,char *_arg,char *_out) {
    strcpy(_out,"\n");
    for (int i=0;i<MAXTOKEN;i++) {
      if (strcmp(langSet[i].token,"") == 0) {
         sprintf(_out+strlen(_out),"\n");
         return false;
      }
      sprintf(_out+strlen(_out),"%s ",langSet[i].token);
    }
    sprintf(_out+strlen(_out),"\n");
    _INFOLIST("%s short(%s)\n",__func__,_out);

    return false;
}
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                                Command processor                                            */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*---------------------------------------------------
 * find the entry for a given command
 */
int cliFind(char *cmd) {
  for (int i=0;i<MAXTOKEN;i++) {
    if (strcmp(cmd,langSet[i].token)==0) {
       return i;
    }
  }
  return -1;
}

/*----------------------------------------------------
 * Either process the command, display a value or 
 * modifies it
 */
bool cli_commandProcessor(char *cmd,char *arg, char *response) {

   _INFOLIST("%s cmd(%s) arg(%s)\n",__func__,cmd,arg);
   
   int idx=cliFind(cmd);
   if (idx == -1) {
      sprintf(response,"invalid command\n");
      return false;
   }

   _INFOLIST("%s cmd(%s) idx(%d)\n",__func__,cmd,idx);
   
   if (langSet[idx].typearg == 0x00) {
       bool rc=langSet[idx].handlerCmd(idx,cmd,arg,response);
       _INFOLIST("%s 0x00 order response(%s)\n",__func__,response);
       delay(200);
       return rc;
   }

   if (langSet[idx].typearg=='a') {
      handleStr(idx,static_cast<char*>(langSet[idx].var),arg,response);
      _INFOLIST("%s 'a' order response(%s)\n",__func__,response);

      return false;
    }

   if (langSet[idx].typearg=='i') {
      handleNum(idx,(int *)langSet[idx].var,arg,response);
      _INFOLIST("%s 'i' order response(%s)\n",__func__,response);
      return false;
    }

   if (langSet[idx].typearg=='n') {
      handleByte(idx,(uint8_t *)langSet[idx].var,arg,response);
      _INFOLIST("%s 'n' order response(%s)\n",__func__,response);
      return false;
    }

   if (langSet[idx].typearg=='b') {
      handleBool(idx,(bool *)langSet[idx].var,arg,response);
      _INFOLIST("%s 'b' order response(%s)\n",__func__,response);
      return false;
    }

    sprintf(response,"invalid command\n");
    _INFOLIST("%s invalid command\n",__func__);
    return false;
}
/*----------------------------------
 * Main command dispatcher
 * the parsed command is contained in the area pointed by c
 * whilst the rest of the arguments in the area pointed by a
 * depending on the command process the argument in different
 * ways
 */
bool cli_execute(char *buffer, char *outbuffer) {

char cmd[128]="";
char argv[128]="";
bool exitcode=false;

     _INFOLIST("%s received cmd(%s)\n",__func__,buffer);

     parse(buffer,cmd);
     strcpy(argv,buffer);
     tolowerStr(cmd);

     if (strcmp(cmd,"")==0) {
        cli_prompt(outbuffer);
        return false;
     }
     
     _INFOLIST("%s parsed between cmd(%s) arg(%s)\n",__func__,cmd,argv);    
     cli_commandProcessor(cmd,argv,outbuffer);
     _INFOLIST("%s processed command cmd(%s) arg(%s) response(%s)\n",__func__,cmd,argv,outbuffer);
     delay(200);
 
   
     cli_prompt(outbuffer);
     _INFOLIST("%s processed command cmd(%s) arg(%s) response(%s)\n",__func__,cmd,argv,outbuffer);
     delay(200);     
     return exitcode;
}
/*--------------------------------------------
 * Generate the prompt with current time
 */
void cli_prompt(char *_out) {
  
  time_t now = time(nullptr) - t_ofs;
  gmtime_r(&now, &timeinfo);

  int tzhour=timeinfo.tm_hour;
  if (timezone != 0) {
     tzhour=tzhour+timezone;
     if (tzhour>23) tzhour=tzhour-24;
     if (tzhour<0)  tzhour=tzhour+24;
  }
  sprintf(_out+strlen(_out), "[%02d:%02d:%02d] >", tzhour, timeinfo.tm_min, timeinfo.tm_sec);
}
/*----------------------------------------------
 * Define pointers to the specific variable
 * handlers (need to improve) this is far from
 * elegant
 */
void cli_setHandlers() {

  langSet[0].handlerCmd=helpCmd;
  langSet[1].handlerCmd=listCmd;
  langSet[2].handlerCmd=loadCmd;
  langSet[3].handlerCmd=saveCmd;
  langSet[4].handlerCmd=resetCmd;
  langSet[5].handlerCmd=shortCmd;
  langSet[16].handlerCmd=txCmd; 
  langSet[23].handlerCmd=ipCmd;
  langSet[24].handlerCmd=rssiCmd;
  langSet[25].handlerCmd=quitCmd;

}
/*--------------------------
 * sub-system initialization
 */
void cli_init(char *_out) {
  
  cli_setHandlers();
  sprintf(_out+strlen(_out), "\n\rRDX %s build(%s) command interpreter\n\r", VERSION, BUILD);
  strcpy(inBuffer,"");
  ptrBuffer=0;
  cli_prompt(_out);

  
}
/*----------------------------------
 * Serial_processor
 * Collect the command from the serial input
 * on LF parse the command line and tokenize into a command
 * followed by arguments (one to many)
 */

bool Serial_processor() {
  
  tft_checktouch();
  bool eoc=false;
  while (_SERIAL.available()) {
     char c = _SERIAL.read();
     if ((byte)c=='\r') {
     }
     if ((byte)c=='\n') {
        _SERIAL.println(inBuffer);
        eoc=cli_execute(inBuffer,outBuffer);
        ptrBuffer=0;
        strcpy(inBuffer,"");
        _SERIAL.print(outBuffer);
        strcpy(outBuffer,"");
        if (eoc) break;
     }
     if ((byte)c!='\n' && (byte)c!='\r') {
        inBuffer[ptrBuffer++]=c;               
        inBuffer[ptrBuffer]=0x00;
     }   
  }
  return eoc;
 
}
/*-------------------------------------------
 * cli_command
 * loop to collect input from serial terminal
 * and execute
 */
void cli_command() {

  cli_init(outBuffer);
  _SERIAL.print(outBuffer);
  strcpy(outBuffer,"");
  
  while (!Serial_processor());
    
}
