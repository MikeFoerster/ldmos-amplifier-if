
//
// Turn On Routine
//
bool PowerUpRoutine(int &CurrentBand, byte &RigPortNumber,  byte &RigModel) {

  Serial.println(F("Running PowerUpRoutine"));


  //Clear the Antenna Output Relays to Off Mode
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print(" W0IH LDMOS Amp ");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_ID);
  delay(1000); //Leave the above display up for 1 second.

  //Overwrite the second line:
  lcd.setCursor(0, 1);
  lcd.print("Reading Rig Freq");

  //
  //On Startup, Establish Comms, Get the Current Band:
  //
  unsigned long StartTime = millis();  //Set for 10 sec timeout

  //Initialize the RigPortNumber so once we enter the loop, we will start with Port 1.
  RigPortNumber = 2;

  do {
    delay(100);
    //Try to read the Band from the Radio, trying both comm ports:
    if (RigPortNumber == 1) RigPortNumber = 2;
    else RigPortNumber = 1;

    Serial.print(F("Testing RigPortNumber: ")); Serial.println(RigPortNumber);
    RigModel = GetRigModel(RigPortNumber);
    Serial.print(F("PowerUpRoutine RigModel Returned:  ")); Serial.println(RigModel);
  } while ((RigModel == 0) && ((millis() - StartTime) < 5000));

  if (RigModel > 0) {
    //Get the Current Band setting:
    CurrentBand = ReadBand(RigPortNumber);

    //If Valid Band or even Invalid Band (30 or 60m):
    if (CurrentBand <= i30m) {

      lcd.setCursor(0, 1);
      lcd.print("  Contact Port " + String(RigPortNumber));

      //Serial.print(F("Valid Band:")); Serial.println(CurrentBand);
      //Serial.print(F("RigPortNumber Found Rig on:")); Serial.println(RigPortNumber);

      //Turn the power on to the Amplifier
      digitalWrite(PowerSwitchPin, ON);
      //Serial.println(F("                    Power Switch Set to ON."));
      //Setup the Band Relays:
      SetupBandOutput(CurrentBand);
      //Set the Power Output for the band.
      UpdateBandPower(CurrentBand, RigPortNumber);

      //On:
      SendMorse("On ");

      return false;  //OK
    }
    else return true; //Failed to read band.
  }
  else {
    //ERR
    SendMorse("Rig Err ");
    return true;
  }
}


//
// Shut Down Routine
//
void OffRoutine(int &CurrentBand, byte &Mode) {
  //Clear the Antenna Output Relays to Off Mode (Set to 160M which turns off all band relays.

  CurrentBand = i160m;
  SetupBandOutput(CurrentBand);
  lcd.setBacklight(0);  //Turn Off The Display.
  lcd.setCursor(0, 0);
  lcd.print("Off             ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  //Do Stuff...
  Bypass(0);
  digitalWrite(PowerSwitchPin, OFF);

  //Turn off the Backlight and Display.
  lcd.setBacklight(0); //OFF
  lcd.noDisplay();
  //OFF
  SendMorse("Off ");
  //Set the Mode to ModeOff
  Mode = ModeOff;
  //Off Routine Complete.
}


void PrintTheMode(byte Mode) {
  //This is a Debug Routine...
  static bool RunOnce;
  static byte PreviousMode;
  if ((Mode != PreviousMode) || (RunOnce == false)) {
    Serial.print(F("Mode="));
    switch (Mode) {
      case ModeOff: {
          Serial.println(F("Off"));
          break;
        }
      case ModePowerTurnedOn: {
          Serial.println(F("PowerTurnOn"));
          break;
        }
      case ModeSwrError: {
          Serial.println(F("SwrError"));
          break;
        }
      case ModeSwrErrorReset: {
          Serial.println(F("SwrErrorReset"));
          break;
        }
      case ModeReceive: {
          Serial.println(F("Receive"));
          break;
        }
      case ModeTransmit: {
          Serial.println(F("Transmit"));
          break;
        }
      case ModeOverTemp: {
          Serial.println(F("OverTemp"));
          break;
        }
      case ModeSetupBandPower: {
          Serial.println(F("SetupBandPower"));
          break;
        }
      case ModeSetupPwrTimeout: {
          Serial.println(F("SetupPowerOffTimeout"));
          break;
        }
      case ModeSetupBypOperMode: {
          Serial.println(F("SetupBypOperMode"));
          break;
        }
    }
    PreviousMode = Mode;
    RunOnce = true;
  }
}
