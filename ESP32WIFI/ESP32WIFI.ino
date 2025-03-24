/* 
YODA ESP32S3 Motor Controller Board Code
Started: 22.03.2025
Ended: Ongoing

MIT License

Copyright (c) 2025 Borab55

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <nvs_flash.h>

WiFiUDP Udp;
Preferences flash;

  // Dinlenecek UDP portu
char incomingPacket[255];               // Gelen veri için buffer
IPAddress senderIP;
unsigned int senderPort;
unsigned int localUdpPort;

//////////PIN DEFINITONS//////////
#define en_r 0
#define en_l 4
#define pwm_1_r 16
#define pwm_2_r 17
#define pwm_1_l 5
#define pwm_2_l 18

//////////////CONFIG//////////////
const char* ssid = "Borab";      // WiFi ağ adı
const char* password = "cCccCccCc"; // WiFi şifresi
#define DEFAULT_UDP_PORT 4210

//////////////////////////////////

void splitPacket(char seperator, char* packet, int* outputArray, int array_length) {
  int index = 0;
  char* adress = strtok(packet, &seperator); // Find first integers adress

  while (adress != NULL && index < array_length) {
    outputArray[index] = atoi(adress); // Convert char array into integer
    index++;
    adress = strtok(NULL, &seperator);
  }
}

void systemReset(int time){
  Serial.printf("\nSystem restarting in %d seconds ! ",time);
  for(int i=0;i<time;i++)Serial.print("."),delay(1000);
  esp_restart();
}

void sendPacket()
void setNewUdpPort(int newPort){
  flash.begin("config",false);
  flash.putInt("localUdpPort",newPort);
  flash.end();
  systemReset(3);
}

void factoryReset(){
  nvs_flash_erase(); // erase the NVS partition and...
  nvs_flash_init(); // initialize the NVS partition.
  Serial.printf("\nFactory reset! All saved values erased!\nDefault values:\nSSID: %s\nPassword: %s\nUDP Port: %d",ssid,password,DEFAULT_UDP_PORT);
  systemReset(3);
}

void parseMessage(char* packet){

  if(strncmp(packet,"SET:",4) == 0){

    if(strncmp(packet+4,"PORT=",5) == 0){

      int newPort = atoi(packet+9);
      Serial.printf("NEW PORT=%d\n",newPort);
      setNewUdpPort(newPort);
      Udp.beginPacket(senderIP, senderPort);
      Udp.printf("NEW PORT = %d",newPort);
      Udp.endPacket();
    }
  }
  else if(strncmp(packet,"GET:",4) == 0){
    if(strncmp(packet+4,"PORT",4) == 0){

      Serial.printf("GET:PORT=%d\n",senderPort);
      Udp.beginPacket(senderIP, senderPort);
      Udp.printf("SENDER PORT %d",senderPort);
      Udp.endPacket();
    }
  }
  else if(strncmp(packet,"SYS:",4) == 0){
    if(strncmp(packet+4,"RESTART",7) == 0){

      Udp.beginPacket(senderIP, senderPort);
      Udp.printf("RESTARTING!",senderPort);
      Udp.endPacket();
      systemReset(3);
    }
    else if(strncmp(packet+4,"FACTORY_RESET",13) == 0){
      
      factoryReset();
      Udp.beginPacket(senderIP, senderPort);
      Udp.printf("FACTORY",senderPort);
      Udp.endPacket();
      systemReset(3);
    }
    
  }

}

void setup() {
  Serial.begin(115200);

  flash.begin("config",false);

  if(flash.getInt("localUdpPort",-1)<0)flash.putInt("localUdpPort",DEFAULT_UDP_PORT);

  localUdpPort = flash.getInt("localUdpPort",1);

  flash.end();

  // WiFi Bağlantı
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to %s ",ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WIFI!");
  Serial.print("ESP32 Local IP: ");
  Serial.println(WiFi.localIP());

  // UDP başlat
  Udp.begin(localUdpPort);
  Serial.printf("Listening UDP Port %d ...\n", localUdpPort);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI Connection Lost! Reconnecting...");
    WiFi.begin(ssid, password);
    delay(5000); // Yeniden bağlanmayı beklemek için
  }

  // Get packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, sizeof(incomingPacket) - 1);
    if (len > 0) {
      incomingPacket[len] = '\0';
    }
  
  senderIP = Udp.remoteIP();
  senderPort = Udp.remotePort();

  Serial.printf("Incoming UDP (%s:%d): %s\n", senderIP.toString().c_str(), senderPort, incomingPacket);

  static int output[10];
  splitPacket(';',incomingPacket,output,10);
  
  parseMessage(incomingPacket);

  for(int i = 0;i<10;i++){
    Serial.printf("%d , ",output[i]);
  }

  // PING'e cevap ver
  if (strcasecmp(incomingPacket, "ping") == 0) {
    Serial.println("PING recieved, sending PONG...");
    Udp.beginPacket(senderIP, senderPort);
    Udp.print("PONG");
    Udp.endPacket();
  }
  }

  if(Serial.available()){
    int port= Serial.parseInt();
    setNewUdpPort(port);
    Serial.printf("%d is now set as the new UDP port!\nSystem restart in 5 seconds ",port);
    for(int i=0;i<5;i++)Serial.print("."),delay(1000);
    esp_restart();
  }
}

