#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <ESP8266WiFi.h>

#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   D3
#define CSN_PIN D4

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8GLIB_SSD1306_128X64 u8g2(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
const byte slaveAddress[5] = {'R', 'x', 'A', 'A', 'A'};

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

char dataToSend[10] = "ON";
char dataToSend_off[10] = "OFF";
char txNum = '0';
char  ackData[10] = "empty"; // to hold the two values coming from the slave
bool newData = false;

int x = 0, y = 25, a;
String temp = "00";
String hum = "00";
String targ = "0";
String data, sensor, temps, t, h, tr;
char ch;
#define pin D8
int pins, current_state ;
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 2000;

void setup(void) {

  WiFi.mode(WIFI_OFF);
  delay(3000);
  Serial.begin(115200);
  u8g2.begin();
  radio.begin();
  radio.setDataRate( RF24_250KBPS );
  radio.enableAckPayload();
  pinMode(pin, INPUT);
  radio.setRetries(3, 5); // delay, count
  radio.openWritingPipe(slaveAddress);
  // Display();
}

void loop(void) {
  //  test();
  if (current_state != (pins = digitalRead(pin))) {
    send();
    current_state = pins;
  }
  // Serial.println(pins);
  // delay(100);
  read_uart();
  loop_Rf();
  Display();

}
