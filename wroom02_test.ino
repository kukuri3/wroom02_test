#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>

const char* ssid = "G808";
const char* password = "12345678";

WiFiUDP udp;

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(16, OUTPUT);  //LED
 Serial.begin(115200);//このシリアル通信はモニター用
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());

  //NTPサーバーでタイムを取得
  udp.begin(4040);
  //WiFi.hostByName(ntpServerName, timeServerIP); 
  //setSyncProvider(getNtpTime);
  delay(3000);

 
}

// the loop function runs over and over again forever
void loop() {
  uint8_t packetBuffer[128]={0};
  
  digitalWrite(16, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(16, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);// wait for a second

  udp.beginPacket("192.168.43.170", 4126); //NTP requests are to port 123
  udp.write("abc", 3);
  udp.endPacket();

  int size = udp.parsePacket();
  Serial.print("size=");
  Serial.println(size);
            if (size > 0) {
            int rxlen=udp.read(packetBuffer, 128);
            Serial.print("rxlen=");
            Serial.print(rxlen);
            Serial.print("Received:[");
            for(uint32_t i = 0; i < rxlen; i++) {
              Serial.print((char)packetBuffer[i]);
            }
            Serial.print("]\r\n");
          }


}
