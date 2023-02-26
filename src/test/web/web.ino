#include <Arduino.h>
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * web.cpp                                                                                     *
 * Simple web based  configuration tool                                                *
 *---------------------------------------------------------------------------------------------*
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "hardware/watchdog.h"
#include "Wire.h"
#include <EEPROM.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/uart.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <LEAmDNS.h>
#include <SPI.h>

#ifndef STASSID
#define STASSID "Fibertel WiFi996 2.4GHz"
#define STAPSK "00413322447"
#endif
#define BOOL2CHAR(x)  (x==true ? "True" : "False")


const char* ssid = STASSID;
const char* password = STAPSK;
WebServer server(80);
const int led = LED_BUILTIN;
char hi[128];

#include <LittleFS.h>
 
  FSInfo fs_info;

//const char* fsName = "LittleFS";
//FS* fileSystem = &LittleFS;
//LittleFSConfig fileSystemConfig = LittleFSConfig();

#define VERSION "2.0"
#define BUILD "63"
#define RP2040_W  1
#define USE_LITTLEFS


/*---------------------------------------------------------------------------------------------------
 * Define the page
 */
const String postForms = "<html>\
  <head>\
    <title>Pico-W Web Server POST handling</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>POST plain text to /postplain/</h1><br>\
    <form method=\"post\" enctype=\"text/plain\" action=\"/postplain/\">\
      <input type=\"text\" name=\'{\"hello\": \"world\", \"trash\": \"\' value=\'\"}\'><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
    <h1>POST form data to /postform/</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      <input type=\"text\" name=\"hello\" value=\"world\"><br>\
      <input type=\"text\" name=\"mycall\" value=\"lu2eic\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

const String headerHTML = "<html>\
  <head>\
    <title>Pico-W Web Server POST handling</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>";

const String postHTML = "<h1 style=\"text-align: center;\"><strong>RDX Digital Transceiver</strong></h1>\
<h2 style=\"text-align: center;\">Web based configuration tool</h2>\
<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Raspberry Pico rp2040-W</div><div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">LU7DZ (c) 2022</div>\
<hr />\
<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
<table style=\"margin-left: auto; margin-right: auto; height: 34px;\" width=\"519\">\
<tbody>";

const String footHTML = "</tbody>\
<tr><td>\
<input type=\"submit\" value=\"Submit\">\
</td></tr>\
</table>\
</form>\
<hr />\
<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">IP address: 192.168.0.22</div><div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">For support: <a href=\"http://www.github.com/lu7did/ADX-rp2040\">http://www.github.com/lu7did/ADX-rp2040</a></div>\
<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Free for amateur uses only</div><div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Debug (active)</div>\
<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Free memory 1000000 KB</div><div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Free space 100000 KB</div>\
<p>&nbsp;</p>\
</body>\
</html>";

/*---------------------------------------------------------------------------------------------
 * Check size of heap memory to validate for memory leaks
 */
int heapLeft() {
  char *p = (char*)malloc(256);   // try to avoid undue fragmentation
  int left = &__StackLimit - p;
  free(p);
  return left;
}
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

void setBody(char *out) {
  char CPU[32];
  strcpy(CPU,"Raspberry Pico rp2040");
  #ifdef RP2040_W
  strcpy(CPU,"Raspberry Pico rp2040 W");
  #endif   

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
void createHTML(char *cmd, char *arg, char *help,char *out) {

    sprintf(out+strlen(out),"<tr><td style=\"width: 150px;\">%s</td>",cmd);
    sprintf(out+strlen(out),"<td style=\"width: 300px;\"><input type=\"text\" name=\"%s\" value=\"%s\"></td>",cmd,arg);
    sprintf(out+strlen(out),"<td style=\"width: 461px;\">%s</td>",help);
    sprintf(out+strlen(out),"</tr>");

}

void setHeader(char *out) {
    sprintf(out+strlen(out),"<html><head><title>Pico-W Web Server POST handling</title>");
    sprintf(out+strlen(out),"<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style></head>"); 
}
void setFooter(char *out) {
  
  char IP[16];
  bool _debug=false;
  #ifdef DEBUG
     _debug=true;
  #endif   

  float tCPU=analogReadTemp();   
  Serial.print(hi);

  uint32_t sizep=fs_info.usedBytes*100;
  sizep=sizep/fs_info.totalBytes;
  
  sprintf(IP,IpAddress2String(WiFi.localIP()).c_str());
  sprintf(out+strlen(out),"</tbody><tr><td><input type=\"submit\" value=\"Submit\"></td></tr></table></form><hr />");
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">IP address: %s</div>",IP);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">For support: <a href=\"https://github.com/lu7did/ADX-rp2040/tree/master/src/RDX-rp2040\">Github page</a></div>");
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">CPU Temp %2.1f C</div>",tCPU);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">Debug (%s)</div>",BOOL2CHAR(_debug));
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: left; padding-left: 8px; float: left;\">Free memory %d KB</div>",heapLeft()/1024);
  sprintf(out+strlen(out),"<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">File System: %d/%d KB(%lu %)</div>",fs_info.usedBytes,fs_info.totalBytes,sizep);
  sprintf(out+strlen(out),"<p>&nbsp;</p></body></html>");

  LittleFS.end();
}
/*-------------------------------------------------------------------------------------------------------*
 *                                    Handler                                                            *
 *-------------------------------------------------------------------------------------------------------*/
void handleRoot() {
  digitalWrite(led, 1);
  char response[8192];
  
  strcpy(response,"");
  setHeader(response);
  setBody(response);
  createHTML("call","lu7dz","Station callsign",response);
  createHTML("grid","GF05","Station grid locator",response);
  createHTML("ssid","Fibertel WiFi996 2.4GHz","WiFi AP name",response);
  createHTML("psk","00413322447","Wifi AP password",response);
  createHTML("tz","-3","Time zone (GMT +/-)",response);
  createHTML("log","./rdx.txt","ADIF log file name",response);
  createHTML("msg","73 GL","ADIF record greeting text",response);
  createHTML("hostname","rdx","hostname",response);
  createHTML("tcp","9000","port for TCP access",response);
  createHTML("write","1","ADIF log generation",response);
  createHTML("auto","0","FT8 automatic QSO mode",response);  
  setFooter(response);
    
  server.send(200, "text/html", response);
  digitalWrite(led, 0);
}

void handlePlain() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "POST body was:\n" + server.arg("plain"));
    digitalWrite(led, 0);
  }
}

void handleForm() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(200, "text/plain", message);
    digitalWrite(led, 0);
  }
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

/*-------------------------------------------------------------------------------------------------------*
 *                                    setup                                                              *
 *-------------------------------------------------------------------------------------------------------*/
void setup(void) {
  pinMode(led, OUTPUT);
  adc_set_temp_sensor_enabled(true);
  digitalWrite(led, 0);
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Web Server started");
  WiFi.begin(ssid, password);
  Serial.println("");

  LittleFS.begin();
  LittleFS.info(fs_info);
  LittleFS.end();


  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("rdx")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/postplain/", handlePlain);
  server.on("/postform/", handleForm);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}
/*-------------------------------------------------------------------------------------------------------*
 *                                    loop                                                              *
 *-------------------------------------------------------------------------------------------------------*/
void loop(void) {
  server.handleClient();
}
