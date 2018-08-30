

void read_uart() {

  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      ch = Serial.read();
      if (ch == '@') {
        sensor = reads();
        temp = sensor;
      } else if (ch == '$') {
        sensor = reads().toInt();
        hum = sensor;
      } else  if (ch == '*') {
        sensor = reads();
        targ = sensor;
      }
    }
  }
}

String reads() {
  // for (int i = 0; i < 4; i++) {
  while (Serial.available() > 0) {
    ch = Serial.read();
    if (ch != '\n') {
      data.concat(ch);
    } else {
      break;
    }

  }
  String res = data;
  data = "";
  return res;

}
