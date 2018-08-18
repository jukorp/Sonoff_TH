void Display(){
  
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenR24_tn); // choose a suitable font
  u8g2.setCursor(x, y);
  width = u8g2.getUTF8Width(temp);
  u8g2.print(temp);
  u8g2.drawCircle(width + 10, 4, 3, U8G2_DRAW_ALL);
  u8g2.setFont(u8g2_font_ncenR18_tn);
  u8g2.setCursor(82, y);
  u8g2.print(hum);
  u8g2.setFont(u8g2_font_helvR12_tf);
  u8g2.setCursor(113, y);
  u8g2.print("%");
  u8g2.setFont(u8g2_font_ncenR18_te);  // choose a suitable font
  u8g2.setCursor(10, 60);
  u8g2.print("off");
  u8g2.setFont(u8g2_font_courB08_tf);  // choose a suitable font
  u8g2.setCursor(58, 50);
  u8g2.print("Heat");
  u8g2.setCursor(58, 60);
  u8g2.print("To;");
  u8g2.setFont(u8g2_font_ncenR18_tn);
  u8g2.setCursor(90, 60);
  u8g2.print(targ);
  u8g2.sendBuffer();          // transfer internal memory to the display   \xB0
  temp = "28.9";
  delay(3000);
}
