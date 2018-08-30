
//=============

void loop_Rf() {

  currentMillis = millis();
  if (currentMillis - prevMillis >= txIntervalMillis) {
    send();

  }
  showData();
}

//================

void send() {

  bool rslt;
  
  if (pins == 1) {
    rslt = radio.write( &dataToSend, sizeof(dataToSend) );
    Serial.print("Data Sent ");
    Serial.print(dataToSend);
  } else {
    rslt = radio.write( &dataToSend_off, sizeof(dataToSend_off) );
    Serial.print("Data Sent ");
    Serial.print(dataToSend_off);
  }

  if (rslt) {
    if ( radio.isAckPayloadAvailable() ) {
      radio.read(&ackData, sizeof(ackData));
      newData = true;
    }
    else {
      Serial.println("  Acknowledge but no data ");
    }

  }
  else {
    Serial.println("  Tx failed");
  }

  prevMillis = millis();
}


//=================

void showData() {
  if (newData == true) {
    Serial.print("  Acknowledge data ");
    Serial.print(String(ackData));

    Serial.println();
    newData = false;
  }
}

//================
