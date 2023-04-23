#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef WEBTOOLS
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * web.cpp                                                                                     *
 * Simple web Web_server based configuration tool                                                *
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
#include <WiFiClient.h>
#include <Webserver.h>
#include <LEAmDNS.h>
#include <SPI.h>
#include <LittleFS.h>
#include "cli.h"

WebServer Web_server(web_port);
FSInfo fs_info;


/*-----------------------------------------------------------
 * This creates a standard body with the form to host all the
 * arguments and the submit of that form as a post for 
 * processing at the web Web_server
 */
void setBody(char *out) {
  char CPU[32];

/*------------------------------------------------------------  
 * Set CPU model
 */
  strcpy(CPU,"Raspberry Pico rp2040");
  #ifdef RP2040_W
  strcpy(CPU,"Raspberry Pico rp2040 W");
  #endif   

/*-------------------------------------------------------------
 * create body structure
 */
  sprintf(out+strlen(out),"<h1 style=\"text-align: center;\"><strong>RDX Digital Transceiver</strong></h1>");
  sprintf(out+strlen(out),"<h2 style=\"text-align: center;\">Web based configuration tool</h2>");
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">%s</div>",CPU);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">LU7DZ (c) 2022</div>"); 
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px;\">Version %s Build(%s)</div>",(char*)VERSION,(char*)BUILD);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Free for amateur uses only</div>");
  sprintf(out+strlen(out),"<hr />");
  sprintf(out+strlen(out),"<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">");
  sprintf(out+strlen(out),"<table style=\"margin-left: auto; margin-right: auto; height: 34px;\" width=\"519\"><tbody>");

}
/*--------------------------------------------
 * This creates a row in the page for each
 * parameter that is need of being configured
 */
void createHTML(char *cmd, char *arg, char *help,char *out) {

    sprintf(out+strlen(out),"<tr><td style=\"width: 150px;\">%s</td>",cmd);
    sprintf(out+strlen(out),"<td style=\"width: 300px;\"><input type=\"text\" name=\"%s\" value=\"%s\"></td>",cmd,arg);
    sprintf(out+strlen(out),"<td style=\"width: 461px;\">%s</td>",help);
    sprintf(out+strlen(out),"</tr>");

}

/*--------------------------------------------
 * Creates a standard HTML Header
 */
void setHeader(char *out) {
    sprintf(out+strlen(out),"<html><head><title>Pico-W Web Web_server POST handling</title>");
    sprintf(out+strlen(out),"<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style></head>"); 
}
/*--------------------------------------------
 * Creates a standard HTML footer with some
 * operational information
 */
void setFooter(char *out) {

/*----------------------------------------------------  
 * Check if the program is operating in debug mode
 * but not allow to change that, only a recompilation
 * can change it.
 */
  bool _debug=false;
  #ifdef DEBUG
     _debug=true;
  #endif   

/*-----------------------------------------------------
 * Running the temperature of the processor, just for 
 * the heck of it.
 */
  float tCPU=analogReadTemp();   

/*------------------------------------------------------  
 * Compute the current usage of the file system
 */
  uint32_t sizep=fs_info.usedBytes*100;
  sizep=sizep/fs_info.totalBytes;
  
  int k1=kMax_candidates;       //KMAX_CANDIDATES       // Original was 120
  int k2=kLDPC_iterations;      //KLDPC_ITERATIONS      // Original was 20
  int k3=kMax_decoded_messages; //KMAX_DECODED_MESSAGES //was 50, change to 14 since there's 14 buttons on the 4x4 membrane keyboard
  int af_frequency=AF_FREQ;

  sprintf(out+strlen(out),"</tbody>");
  
  sprintf(out+strlen(out),"<tr>");
  sprintf(out+strlen(out),"<td><input type=\"submit\" value=\"Update\"></td>");
  sprintf(out+strlen(out),"</form>");
  
  sprintf(out+strlen(out),"<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postsave/\">");
  sprintf(out+strlen(out),"<td><input type=\"submit\" value=\"Save\"></td>");
  sprintf(out+strlen(out),"</form>");
  
  sprintf(out+strlen(out),"<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/posttx/\">");
  char txstr[4];
  if (TX_State==0) {
     strcpy(txstr,"Tx+");
  } else {
     strcpy(txstr,"Tx-");
  }
  sprintf(out+strlen(out),"<td><input type=\"submit\" value=\"%s\"></td>",txstr);
  sprintf(out+strlen(out),"</form>");
  sprintf(out+strlen(out),"</tr>");
  sprintf(out+strlen(out),"</table>");
  sprintf(out+strlen(out),"<hr />");
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">IP address: %s</div>",ip);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">For support see: <a href=\"https://github.com/lu7did/ADX-rp2040/tree/master/src/RDX-rp2040\">Github page</a></div>");
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">CPU Temp %2.1f C</div>",tCPU);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Debug (%s)</div>",BOOL2CHAR(_debug));
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Free memory %d KB</div>",heapLeft()/1024);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">File System: %d/%d KB(%lu %)</div>",fs_info.usedBytes,fs_info.totalBytes,sizep);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Band(%dm) %lu KHz</div>",Bands[Band_slot],freq/1000);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Base AF: %d Hz</div>",af_frequency);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">NTP %s/%s</div>",ntp_server1,ntp_server2);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Magic trio: (%d/%d/%d)</div>",k1,k2,k3);
  sprintf(out+strlen(out),"<p>&nbsp;</p></body></html>");
  
}
/*-------------------------------------------------------------------------------------------------------*
 *                         Web proc handlers                                                             *
 *-------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------
 * This procedure handles the http://rdx.local request
 */
void Web_handleRoot() {
  
  char response[8192];
  char strValue[32];  
  strcpy(response,"");
  setHeader(response);
  setBody(response);

  for (int i=0;i<MAXTOKEN;i++) {
    void* p=langSet[i].var;
    if(langSet[i].typearg == 'a' && p != NULL) {
      createHTML((char*)langSet[i].token,static_cast<char*>(langSet[i].var),(char*)langSet[i].help,response);
    }


    if(langSet[i].typearg == 'i' && p != NULL) {
      sprintf(strValue,"%d",*(int *)p);
      createHTML((char*)langSet[i].token,strValue,(char*)langSet[i].help,response);
    }
    if(langSet[i].typearg == 'n' && p != NULL) {
      sprintf(strValue,"%d",*(uint8_t *)p);
      createHTML((char*)langSet[i].token,strValue,(char*)langSet[i].help,response);
    }

    if(langSet[i].typearg == 'b' && p != NULL) {
      sprintf(strValue,"%d",*(bool *)p);
      createHTML((char*)langSet[i].token,strValue,(char*)langSet[i].help,response);
    }

  }

  setFooter(response);
    
  Web_server.send(200, "text/html", response);
}
/*---------------------------------------------------------
 * This procedure handles a form request in plain text
 * (not used)
 */
void Web_handlePlain() {
  
  if (Web_server.method() != HTTP_POST) {
    Web_server.send(405, "text/plain", "Method Not Allowed");
  } else {
    Web_server.send(200, "text/plain", "POST body was:\n" + Web_server.arg("plain"));
  }
}
/*------------------------------------------------------------
 * This procedure handles a form request with a POST call
 * Here the answer is computed and checked to see if any
 * change has been made on variables, is detected make
 * this change
 */
void Web_handleForm() {
  
  char message[1024];
  char strVal[64];
  char strWarn[12];
  
  strcpy(message,"");
  strcpy(strWarn,"");
  
  if (Web_server.method() != HTTP_POST) {
    Web_server.send(405, "text/plain", "Method Not Allowed");
  } else {
    sprintf(message+strlen(message),"POST form was:\n");
    for (uint8_t i = 0; i < Web_server.args(); i++) {
      strcpy(strWarn,"");
      strcpy(strVal,"<nil>");
      cli_commandProcessor((char*)Web_server.argName(i).c_str(),(char*)Web_server.arg(i).c_str(),message);
    }
    
    Web_handleRoot();
  }
}
/*---------------------------------------------------------------
 * This procedure handles all requests that can not be recognized
 */
void Web_handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += Web_server.uri();
  message += "\nMethod: ";
  message += (Web_server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += Web_server.args();
  message += "\n";
  for (uint8_t i = 0; i < Web_server.args(); i++) {
    message += " " + Web_server.argName(i) + ": " + Web_server.arg(i) + "\n";
  }
  Web_server.send(404, "text/plain", message);
}
/*---------------------------------------------------------
 * Handle the Save button to commit changes into EEPROM
 */
void Web_handleSave() {

  updateEEPROM();
  /*
  String message = "Save EEPROM\n\n";
  message += "URI: ";
  message += Web_server.uri();
  message += "\nMethod: ";
  message += (Web_server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += Web_server.args();
  message += "\n";
  for (uint8_t i = 0; i < Web_server.args(); i++) {
    message += " " + Web_server.argName(i) + ": " + Web_server.arg(i) + "\n";
  }
  Web_server.send(404, "text/plain", message);
  */
  Web_handleRoot();

}
/*-----------------------------------------------------------
 * Handle the TX button to turn on/off the transmitter
 */
void Web_handleTx() {

  if (TX_State == 0) {
     startTX();
  } else {
     stopTX();
  }
  /*
  String message = "Turn On/Off Tx\n\n";
  message += "URI: ";
  message += Web_server.uri();
  message += "\nMethod: ";
  message += (Web_server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += Web_server.args();
  message += "\n";
  for (uint8_t i = 0; i < Web_server.args(); i++) {
    message += " " + Web_server.argName(i) + ": " + Web_server.arg(i) + "\n";
  }
  Web_server.send(404, "text/plain", message);
  */
  Web_handleRoot();
  
}

/*-------------------------------------------------------------------------------------------------------*
 *                          setup the web Web_server for the configuration tool                              *
 *-------------------------------------------------------------------------------------------------------*/
void setup_Web(void) {

  adc_set_temp_sensor_enabled(true);
  
  LittleFS.begin();
  LittleFS.info(fs_info);
  LittleFS.end();
  _INFOLIST("%s Web Web_server listening at %s:%d\n",__func__,ip,web_port);

  if (MDNS.begin(hostname)) {
    _INFOLIST("%s MDNS responder started for %s.local\n",__func__,hostname);
  }

  Web_server.on("/", Web_handleRoot);
  Web_server.on("/postplain/", Web_handlePlain);
  Web_server.on("/postform/", Web_handleForm);
  Web_server.on("/postsave/", Web_handleSave);
  Web_server.on("/posttx/", Web_handleTx);

  Web_server.onNotFound(Web_handleNotFound);

  Web_server.begin();
}
/*-------------------------------------------------------------------------------------------------------*
 *                     serve the web Web_server for the configuration tool incoming connections              *
 *-------------------------------------------------------------------------------------------------------*/
void process_Web(void) {
  Web_server.handleClient();
}
#endif //WEBTOOLS
