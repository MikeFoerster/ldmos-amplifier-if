
//
// Turn On Routine
//
byte PowerUpRoutine(int &CurrentBand, byte &RigPortNumber,  byte &RigModel, bool &Act_Byp) {
  //Returns 0 = Passed
  //Returns 1 = Failed to Establish Comms
  //Returns 2 = Failed to Disable K2 Internal Amp
  //Returns 3 = Power Up, NO 50v detected
  //Returns 4 = Invalid Band (could be outside the Ham Bands)

  //Initialize the Display:
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

  // Set the data rate for Serial Port 1
  Serial1.begin(38400);
  // And for Serial Port 2.
  Serial2.begin(38400);

  //
  //Establish Comms, Get the Current Band:
  //
  unsigned long StartTime = millis();  //Set Start time for 'do' loop

  //Initialize the RigPortNumber so once we enter the loop, we will start with Port 1.
  RigPortNumber = 2;

  do {
    delay(100);
    //Try to read the Band from the Radio, trying both comm ports:
    if (RigPortNumber == 1) RigPortNumber = 2;
    else RigPortNumber = 1;

    RigModel = GetRigModel(RigPortNumber);  //Returns 1 or 2 when we establish comms
    //Serial.print(F("PowerUpRoutine RigModel Returned:  ")); Serial.println(RigModel);
  } while ((RigModel == 0) && ((millis() - StartTime) < 5000));


  if (RigModel == 0) {
    //Failed to establish Comms
    return 1;
  }

  //We've established Comms:

  //For the K3, DISABLE the internal 100w Amp to prevent overdrive:
  if (RigModel == 1) {
    //Turn the Internal K3 Amp Off
    if (K3AmpOnOff(RigPortNumber, ON)) {
      //Failed to Disable the K3 KPA3 Amp
      Act_Byp = 0;   Bypass(Act_Byp);  //Set to Bypass
      lcd.setCursor(0, 1);
      lcd.print("K3 Amp NOT OFF! ");  //Display the Error
      SendMorse("K3 Amp err ");
      delay(1000);
      //Display a "Continuing anyway" message for 1 second.
      lcd.setCursor(0, 1);
      lcd.print("Continuing (Byp)");
      delay(1000);
      //This is not a critical fault, but Setting the K3 KPA3 amp to BYPASS helps
      //   to assures us that we can't overdrive the LDMOS amp.
      return 2;
    }
  }

  //Get the Current Band setting:
  CurrentBand = ReadBand(RigPortNumber);

  if (CurrentBand > i6m) {  //
    //Invalid Amplifier Band: 60m, 30m, or could be somewhere outside the bands
    //return 4;
    SendMorse("Err");
    //Turn On amp anyway.  Operator can adjust band or Freq.
  }

  //Display Contact and PortNumber onbottom line
  lcd.setCursor(0, 1);
  lcd.print("  Contact Port " + String(RigPortNumber));

  //Turn the power on to the Amplifier
  digitalWrite(PowerSwitchPin, ON);
  delay(2000);  //Allow time for the Power Supply to come up to voltage.  
                // (Occasionally get a beep from the "Display" check if not long enough.)

  //Check the reading for the Volt Meter
  float Volts = ReadVoltage();
  if (Volts < 30) {
    //Power Up FAILED
    lcd.setCursor(0, 1);  //Second Line
    lcd.print("Pwr Fail:NoVolts");
    SendMorse("Pwr Volts");
    delay(2000);
    digitalWrite(PowerSwitchPin, OFF);
    return 3;
  }

  //Setup the Band Relays:
  SetupBandOutput(CurrentBand);
  //Set the Power Output for the band.
  UpdateBandPower(CurrentBand, RigPortNumber);

  //On:
  SendMorse("On ");
  return 0;  //OK
}


//
// Shut Down Routine
//
void OffRoutine(byte &Mode) {
  //Clear the Antenna Output Relays to Off Mode (Set to 160M which turns off all band relays.

  lcd.print("Off             ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  delay(500);

  //Set to i160m, will turn off all relays.
  SetupBandOutput(i160m);

  //Set Bypass output to off.
  Bypass(0);
  //Turn Off Power to the Amp.
  digitalWrite(PowerSwitchPin, OFF);
  delay(1000);

  //Turn off the Backlight and Display.
  lcd.setBacklight(0); //OFF
  lcd.setCursor(0, 0);
  lcd.noDisplay();
  SendMorse("Off ");
  //Set the Mode to ModeOff
  Mode = ModeOff;
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
