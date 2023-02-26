#include <Arduino.h>
#include "RDX-rp2040.h"
#ifdef RP2040_W
#include "wifi.h"
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                   TCP/IP STACK   SUBSYSTEM                                                  *
//* Manage connectivity, NTP sync, TCP and UDP processing                                       *
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

#include <WiFiUdp.h>

WiFiUDP udp;                           //UDP server


bool ntpok=false;
bool pingit=false;
char wifi_ssid[40];
char wifi_psk[16];
char ipstr[16];
unsigned long rssi=0;
int tz=TIMEZONE;
char ipAddress[16];


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                            Infraestructure routines                                 */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*-------------
   dumpString()
   dumps a string from position p of a buffer
*/
void dumpString(char *buffer, int p, int len) {

  for (int i = 0; i < 10; i++) {
    if ((p + i) > len) {
      return;
    }
    if (buffer[p + i] >= 0x20 && buffer[p + i] <= 0x7e) {
      sprintf(hi, "%c", buffer[p + i]);
      _SERIAL.print(hi);
    } else {
      _SERIAL.print(".");
    }
  }

}

/*-----------------
   dumpHex()
   dump a buffer with a hex representation
*/
void dumpHex(char *buffer, int len) {

  if (len == 0) {
    return;
  }
  int i = 0;
  while (i < len) {
    sprintf(hi, "|%04d| ", i);
    _SERIAL.print(hi);
    for (int j = 0; j < 10; j++) {
      if (i + j > len) {
        _SERIAL.println(" ");
        return;
      }
      sprintf(hi, "%02x ", buffer[i + j]);
      _SERIAL.print(hi);
    }
    sprintf(hi, "|");
    _SERIAL.print(hi);
    dumpString(buffer, i, len);
    sprintf(hi, "|\n");
    _SERIAL.print(hi);
    i = i + 10;
  }
}
/*------------------------
   Take an IPAddress object and returns a String-ified version of it
   for printing and display purposes
*/
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

/*------
 * transform an IP address into a C string
 */
void Ip2Str(const IPAddress& ipAddress,char *i) {
  sprintf(i,"%03d.%03d.%03d.%03d",ipAddress[0],ipAddress[1],ipAddress[2],ipAddress[3]);
  return;
}
/*-----
 *  transform a string into a lower case
 */
void toLowerCase(char *s) {

  char z[128];
  strcpy(z,"");
  for (int i=0;i<strlen(s);i++) {
    z[i]=tolower((char)s[i]);
  }
  z[strlen(s)]=0x00;
  #ifdef DEBUG
     _INFOLIST("%s entry(%s) exit(%s)\n",__func__,s,z); 
  #endif //DEBUG
  strcpy(s,z);
  return;
}
/*------
 * replace a character from orig to rep into a C string
 */
int strrepl(char *str, char orig, char rep) {
    char *ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

/*-------------------------
   setClock()
   synchronize system clock with an NTP server using internet connectivity
   return
      (true)        Synchronization was successful
      (false)       Synchronization failed
*/
bool getClock(char* n1, char* n2) {

  _INFOLIST("%s NTP server pool ntp1(%s) ntp2(%s)\n",__func__,n1,n2);
  NTP.begin(n1, n2);
  bool ntp_rc = NTP.waitSet(wifi_tout * 1000);

  if (ntp_rc) {
    _INFOLIST("%s rc(%s)\n",__func__,BOOL2CHAR(ntp_rc));
    return ntp_rc;
  }
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  _INFOLIST("%s NTP time synchronization completed now is %s", __func__, asctime(&timeinfo));
  return ntp_rc;
}

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                                      MDNS server                                    */
/* This server, if enabled, allows for the IP address of the board to be discovered as */
/* {localhost}.local (currently it's pdx.local but can be changed as a parameter)      */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#include <MDNS_Generic.h>
MDNS mdns(udp);
//char hostname[32];
//int  tcp_port;

/*----
 * this is a callback function addressed by the mDNS library when a name
 * is revolved
 */
// This function is called when a name is resolved via mDNS/Bonjour. We set
// this up in the setup() function above. The name you give to this callback
// function does not matter at all, but it must take exactly these arguments
// (a const char*, which is the hostName you wanted resolved, and a const
// byte[4], which contains the IP address of the host on success, or NULL if
// the name resolution timed out).
void nameFound(const char* name, IPAddress ip)
{
  if ( (ip[0] != 0) || (ip[1] != 0) || (ip[2] != 0) || (ip[3] != 0) )
  {
     char ipstr[24];
     Ip2Str(ip,ipstr);
     _INFOLIST("%s IP for %s is %s\n",__func__,name,ipstr);
  }
  else if (ip[0] == 0)
  {
    _INFOLIST("%s Resolving %s error\n",__func__,name);
  }
  else
  {
    _INFOLIST("%s Resolving %s timeout\n",__func__,name);
  }
}
/*---------
 * initialization of the mDNS service, needs to be called once at
 * startup
 */
void init_mdns() {
  
  //strcpy(hostname,HOSTNAME);
  toLowerCase(hostname);
  strrepl(hostname,' ','-');
  strrepl(hostname,'_','-');
  
  _INFOLIST("%s Registering mDNS hostname(%s)\n",__func__,hostname);
  _INFOLIST("%s To access using (%s.local)\n",__func__,hostname);

  /*----
   * init the mDNS library to respond with the current IP address to any
   * request to resolver {hostname}.local
   */
  mdns.begin(WiFi.localIP(), hostname);
  mdns.setNameResolvedCallback(nameFound);  
  _INFOLIST("%s mDNS service started ip(%s)\n",__func__,IpAddress2String(WiFi.localIP()).c_str());

}
/*-------
 * mDNS service needs to be called periodically for name requests to
 * be honored and answered on the local network
 */
void run_mdns() {
  /*----
   * Periodic call to the library to resolve pending requests
   */
  //_INFOLIST("%s probe\n",__func__); 
  //mdns.run();
 
}

/*------
   doping(host)
   check connectivity to a given host
*/
int doping(char *host) {
  return WiFi.ping(host);
}
/*------------
    checkInet()
    Check if there is actual internet connectivity by sending a successful ping to a known host
*/
int checkInet(char* hostName) {
  int pingResult = WiFi.ping(hostName);
  _INFOLIST("%s Ping host(%s) rtt=%d msec\n", __func__, hostName, pingResult);
  return pingResult;
}

/*-------------
    checkAP()
    Check if there is an AP and try to connect with it
*/
int checkAP(char* s, char* p) {

  if (WiFi.status() == WL_CONNECTED) {
    _INFOLIST("%s Already connected to ssid(%s) IP(%s)\n", __func__, s, IpAddress2String(WiFi.localIP()).c_str());
    return WL_CONNECTED;
  }


  WiFi.mode(WIFI_STA);
  
#ifdef FIXED_IP  
  IPAddress ip(192,168,0,151);     
  IPAddress gateway(192,168,0,1);   
  IPAddress subnet(255,255,255,0); 
  IPAddress dnsserver(8,8,8,8);
  WiFi.config(ip, dnsserver,gateway, subnet);
  _INFOLIST("%s defining fixed IP\n",__func__);
#endif //FIXED_IP  

  WiFi.begin(s, p);
  delay(1000);

  uint32_t t = time_us_32();

  while (WiFi.status() != WL_CONNECTED) {
    if (time_us_32() - t > wifi_tout * 1000000) {

      _INFOLIST("%s Failed to connect to ssid(%s)\n", __func__, s);

      return WiFi.status();
    }
  }
  _INFOLIST("%s Connected to ssid(%s) IP(%s)\n", __func__, s, IpAddress2String(WiFi.localIP()).c_str());
  delay(200);

  rssi=WiFi.RSSI();
  
  sprintf(ipstr,"%s    ",IpAddress2String(WiFi.localIP()).c_str());
  tft_setIP(ipstr);
  return (int) WL_CONNECTED;
}
/*-------------------------------------------------------
 * Disconnect from WiFi AP
 */
void resetAP() {
  WiFi.disconnect();
  sprintf(ipstr," Disconnected");
  tft_setIP(ipstr);
  _INFOLIST("%s WiFi disconnected\n",__func__);
}
/*---------------------------------------------
   setup_wifi()
   Connect to an AP, verify navigation and sync time
      SYNC_OK       Connectivity and all set services ok
      SYNC_NO_INET  No Internet connectivity
      SYNC_NO_AP    No WiFi AP 
      SYNC_NO_NTP   No NTP server
*/

int setup_wifi() {

  strcpy(ntp_server1,(char*)NTP_SERVER1);    //Server defined to sync time (primary)
  strcpy(ntp_server2,(char*)NTP_SERVER2);    //Server defined to sync time (secondary)
  strcpy(inet_server,(char*)INET_SERVER);    //Server defined to validate Inet connectivity
  return 0;
  
}

#endif //RP2040_W
