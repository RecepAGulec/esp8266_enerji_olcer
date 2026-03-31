#ifndef HESAP_H
#define HESAP_H
#include <ESP8266WiFi.h>

const char* ssid = "wi-fi ağınızın ismi";
const char* password = "wi-fi ağınızın şifresi";

// Eğer statik IP istemiyorsanız bu kısmı silebilirsiniz. 
IPAddress local_IP(192, 168, 1, 171); 
IPAddress gateway(192, 168, 1, 1);    
IPAddress subnet(255, 255, 252, 0);   
IPAddress primaryDNS(8, 8, 8, 8);     
IPAddress secondaryDNS(8, 8, 4, 4);
// Statik IP için silinecek kısmın sonu
#endif
