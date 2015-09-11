#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <Wire.h>
//#include <AM2321.h>
//#include <math.h>
#include <ST7032.h>


#define kNAME "2F_B"
const char* ssid = "G808";
const char* password = "12345678";


WiFiUDP udp;
ST7032 lcd;
//AM2321 ac;

float temperature;
float humidity;
float dewpoint;
int itemp;
int ihum;
int idew;
int loopcount = 0;


void xAMRead(void);

// the setup function runs once when you press reset or power the board
void setup() {

  pinMode(16, OUTPUT);  //LED
  Serial.begin(115200);
  delay(100);
  
  Wire.begin(12, 14);



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

  delay(100);
  lcd.begin(16, 2);
  lcd.setContrast(30);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("local ip");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());



  udp.begin(4040);




}
double dewPoint(double celsius, double humidity) {
  // RATIO was originally named A0, possibly confusing in Arduino context
  /*  double RATIO = 373.15 / (273.15 + celsius);
    double SUM = -7.90298 * (RATIO - 1);
    SUM += 5.02808 * log10(RATIO);
    SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
    SUM += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
    SUM += log10(1013.246);
    double VP = pow(10, SUM - 3) * humidity;
    double T = log(VP/0.61078);   // temp var
    return (241.88 * T) / (17.558 - T);
  */
  return 0;
}
double dewPointFast(double celsius, double humidity) {
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity / 100);
  double Td = (b * temp) / (a - temp);
  return Td;
}
// the loop function runs over and over again forever
void loop() {
  uint8_t packetBuffer[128] = {0};
  char buf[128];

  digitalWrite(16, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(16, LOW);    // turn the LED off by making the voltage LOW
  delay(500);// wait for a second

  //温度取得
  xAMRead();  //i2cが落ち着くまで待つので10秒かかる
  //ac.read();

  //wifiに出力
  itemp = (int)(temperature * 10);
  ihum = (int)(humidity * 10);
  idew = (int)(dewpoint * 10);
  //ipress=(int)(pressure*10);

  //送信パケット作成
  sprintf(buf, "name,%s,count,%d,temp,%d,hum,%d,dew,%d\r\n"
          , kNAME, loopcount, itemp, ihum, idew);
  Serial.println(buf);

  //UDP送信
  if((loopcount%10)==0){
    udp.beginPacket("192.168.43.170", 4126); //NTP requests are to port 123
    udp.write((const uint8_t*)buf, strlen(buf));
    udp.endPacket();
  }
  //受信テスト。シリアルに出力するだけ。
  int size = udp.parsePacket();
  Serial.print("size=");
  Serial.println(size);
  if (size > 0) {
    int rxlen = udp.read(packetBuffer, 128);
    Serial.print("rxlen=");
    Serial.println(rxlen);
    Serial.print("Received:[");
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    for (uint32_t i = 0; i < rxlen; i++) {
      Serial.print((char)packetBuffer[i]);
      lcd.print((char)packetBuffer[i]);
    }
    Serial.print("]\r\n");
  }
  
  loopcount++;

  //LCDに出力
 
  sprintf(buf,"%4.1C %4.1f\% %4.1fC",temperature,humidity,dewpoint); 
  lcd.setCursor(0, 0);
  dtostrf(temperature,4,1,buf);lcd.print(buf);lcd.print("C ");
  dtostrf(humidity,4,1,buf);lcd.print(buf);lcd.print("\% ");
  dtostrf(dewpoint,4,1,buf);lcd.print(buf);lcd.print("C");
  
}



#define I2C_ADDR_AM2321 (0xB8 >> 1)
#define PARAM_AM2321_READ 0x03
#define REG_AM2321_HUMIDITY_MSB 0x00
#define REG_AM2321_HUMIDITY_LSB 0x01
#define REG_AM2321_TEMPERATURE_MSB 0x02
#define REG_AM2321_TEMPERATURE_LSB 0x03
#define REG_AM2321_DEVICE_ID_BIT_24_31 0x0B

void xAMRead(void)
{

  // Wakeup
  //
  Wire.beginTransmission(I2C_ADDR_AM2321);
  Wire.endTransmission();

  //
  // Read Command
  //
  Wire.beginTransmission(I2C_ADDR_AM2321);
  Wire.write(PARAM_AM2321_READ);
  Wire.write(REG_AM2321_HUMIDITY_MSB);
  Wire.write(4);
  Wire.endTransmission();

  //
  // Waiting
  //
  //delayMicroseconds(1600); //>1.5ms
  delay(2);
  //
  // Read
  //
  Wire.requestFrom(I2C_ADDR_AM2321, 8); // COMMAND + REGCOUNT + DATA + CRCLSB + CRCMSB
  //int i = 0;
  uint8_t buf[6];
  for (int i = 0; i < 6; ++i)
    buf[i] = Wire.read();

  unsigned short crc = 0;
  crc  = Wire.read();     //CRC LSB
  crc |= Wire.read() << 8;//CRC MSB

  // if (crc == crc16(buf, i))
  //     return true;
  // return false;
  //Wire.endTransmission(true);
  Serial.println(Wire.available());
  ihum     = buf[2] << 8;
  ihum    += buf[3];
  itemp  = buf[4] << 8;
  itemp += buf[5];
  temperature = ((float)itemp) / 10.0;
  humidity = ((float)ihum) / 10.0;
  dewpoint = dewPointFast(temperature, humidity);

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.print("\t Humidity = ");
  Serial.print(humidity);
  Serial.print("\t dewpoint = ");
  Serial.println(dewpoint);
  delay(10000);
}



