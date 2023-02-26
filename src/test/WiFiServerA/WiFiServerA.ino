// Placed in the public domain by Earle F. Philhower, III, 2022

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>

#include <WiFi.h>

#ifndef STASSID
#define STASSID "Fibertel WiFi996 2.4GHz"
#define STAPSK "00413322447"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

int port = 4242;
bool flagConnected=false;

WiFiServer server(port);

void setup() {
  Serial.begin(115200);
  while(!Serial);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("RDX");
  Serial.printf("Connecting to '%s' with '%s'\n", ssid, password);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.printf("\nConnected to WiFi\n\nConnect to server at %s:%d\n", WiFi.localIP().toString().c_str(), port);

  server.begin();
}

WiFiClient client;
String req;
char inBuffer[1024];
char outBuffer[1024];
char answer[1024];
char inChar;

int  inPtr=0;

void wipeChar(char *out) {
  char temp[strlen(out)+5];
  int  k=0;
  if (strlen(out)==0) return;
  for (int i=0;i<strlen(out);i++) {
      if (out[i] != '\n' && out[i] != '\r') {
         temp[k++]=out[i];
      }
  }
  temp[k]=0x00;
  strcpy(out,temp);
  return;
}
void loop() {

  if (!flagConnected) {
     client = server.available();
     if (!client) {
        return;
     }
     flagConnected=true;
     client.println("RDX server test");
     //client.flush();
  }

  if (!client.connected() || !client) {
     Serial.println("Client disconnected");
     flagConnected=false;
     return;
  }


  if (!client.available()) {
     if (client.status() != 4) {
        Serial.print("Client status ");
        Serial.println(client.status());
        client.stop();
        Serial.println("connection terminated by timeout");
        return;     
     }
  }

  //String req= client.readStringUntil('\n');
  while (client.available()) {
     inChar=client.read();
     if (inChar != '\r' && inChar != '\n') {
        inBuffer[inPtr++]=inChar;
        inBuffer[inPtr]=0x00;       
     } else {
        if (inChar == '\n') {
           wipeChar(inBuffer);
           if (strcmp(inBuffer,"q")==0) {
              Serial.print("Client disconnected by user\n");
              client.stop();
              flagConnected=false;
              return;
           }

           if (strcmp(TinBuffer,"q")==0) {
              Serial.print("Client disconnected by user\n");
              client.stop();
              flagConnected=false;
              return;
           }

           strcpy(answer,"Answer");
           sprintf(outBuffer,"Response to req(%s) is (%s)\n\r",inBuffer,answer); 
           client.printf(outBuffer);
           Serial.println(outBuffer);
           inPtr=0;
           strcpy(inBuffer,"");
        }
     }   
  }

  return;
}
