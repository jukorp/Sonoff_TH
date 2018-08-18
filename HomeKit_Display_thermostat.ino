#include <SoftwareSerial.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int x = 0, y = 25;
u8g2_uint_t width;
char *temp = "48";
String hum = "45";
String targ = "37";

void setup(void) {
  Serial.begin(115200);
  u8g2.begin();
}

void loop(void) {
  Display();
}
