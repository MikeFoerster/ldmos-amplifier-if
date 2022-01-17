

//Send the command for Serial3 for the IC-705
String Serial3Cmd(String Command) {
  //Send the command thru Serial3
  String Return = "";

  for (unsigned int i = 0; i < Command.length(); i++)  {
    Serial3.write(Command[i]);   // Push each char 1 by 1 on each loop pass
  }

  //Serial.print(F("  Command: ")); Serial.print(Command);
  //Wait for the command to return:
  Return = Serial3Read(true);

  //Serial.print(F(" which Returned: ")); Serial.println(Return);

  //Return a "nak" if there was no return (we expect a return for a command we sent out.)
  if (Return == "") {
    //Serial.print(F("No Response to Command: ")); Serial.println(Command);
    return "nak";
  }
  else {
    return Return;
  }
}

String Serial3Read(boolean Wait) {
  //Read from Serial3 from the ESP32 port
  int Count = 0;
  String Return = "";

  if (Wait) {
    //unsigned long StartWait = millis();
    do {
      //If the TX pin is active, interrupt the read or we will timeout 
      if (gTxFlag) break;
      //Wait for the Return:
      delay(10);
      Count++;
    } while ((Serial3.available() == 0) && (Count < 100));
    //Serial.print(F("      BT Run Time = ")); Serial.print(millis() - StartWait); Serial.println();
  }

  if (Serial3.available()) {
    Return = Serial3.readString();
  }

  if (Return.length() < 2) {
    //Serial.print(F("  Serial3Read DIDN'T return: ")); Serial.println(Return);
    return "";
  }
  else {
    //Serial.print(F("  Serial3Read returned: ")); Serial.println(Return);
    return Return;
  }
}

//OffMode, Up Button, Wake the ESP32 Bluetooth.
void WakeBt(void) {
  digitalWrite(WakePin, 1);
  delay(100);
  digitalWrite(WakePin, 0);
  //Initialize the Display:
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print("ESP32 BT Wake   ");  //Print the message.
  lcd.setCursor(0, 1);
  lcd.print("");
  delay(2000); //Leave the above display up for 2 seconds.
  //Turn off the display.
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.setBacklight(0);
  lcd.noDisplay();
}

//OffMode, Down Button, Put the ESP32 Bluetooth to sleep.
void SleepBt(void) {
  // Initialize Serial Port 3, Bluetooth.
  //Send the Sleep Command:
  Serial.println(F("  SleepBt sending the Sleep Command."));
  Serial3.begin(115200);
  delay(50);
  String Command = "slep";
  for (unsigned int i = 0; i < Command.length(); i++)  {
    Serial3.write(Command[i]);   // Push each char 1 by 1 on each loop pass
  }  
  //Initialize the Display:
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print("ESP32 BT Sleep  ");  //Print the message.
  lcd.setCursor(0, 1);
  lcd.print("");
  delay(2000); //Leave the above display up for 2 seconds.
  //Turn off the display.
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.setBacklight(0);
  lcd.noDisplay();  
}

