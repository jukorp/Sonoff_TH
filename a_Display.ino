void Display() {
  a = temp.length();
  if (a > 3) {
    a = 70;
  } else {
    a = 45;
  }
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenR24_tn); // choose a suitable font
//  u8g2.drawStr(x, y, "23.3");
  u8g2.setCursor(x, y);
 // String str = "123";
  u8g2.print(temp);
  //  width = u8g2.getUTF8Width(temp);

  u8g2.drawCircle(a, 4, 3, U8G2_DRAW_ALL);
  u8g2.setFont(u8g2_font_osr18_tf);
  u8g2.setCursor(95, 60);
  u8g2.print(targ);
  // u8g2.setFont(u8g2_font_osr18_tf);
  u8g2.setCursor(82, y);
  u8g2.print(hum);
  

  u8g2.setCursor(5, 60);
  u8g2.print("off");

  u8g2.setFont(u8g2_font_courB10_tf);  // choose a suitable font
  u8g2.drawStr(53, 50, "Heat");
  u8g2.drawStr(53, 62, "To;");
  u8g2.setCursor(115, y);
  u8g2.print("%");


  // u8g2.print("%");
  u8g2.sendBuffer();          // transfer internal memory to the display   \xB0
  //temp = "28.9";
  // delay(3000);
}
