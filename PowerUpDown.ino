
//
// Turn On Routine
//
byte PowerUpRoutine(int &CurrentBand, byte &RigPortNumber,  byte &RigModel, bool &Act_Byp) {
  //Returns 0 = Passed
  //Returns 1 = Failed to Establish Comms
  //Returns 2 = Failed to Disable K2 Internal Amp
  //Returns 3 = Power Up, NO 50v detected
  //Returns 4 = Invalid Band (could be outside the Ham Bands)

  String RigName = "";

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
  } while ((RigModel == 0) && ((millis() - StartTime) < 5000));

  //Serial.print(F("RigModel Complete = ")); Serial.println(RigModel);

  if (RigModel == 0) {
    //Failed to establish Comms
    Serial.println(F("RigModel Returned 0."));
    return 1;
  }

  //We've established Comms:

  //For the K3, DISABLE the internal 100w Amp to prevent overdrive:
  if (RigModel == 1) {
    //Turn the Internal K3 Amp Off
    if (K3AmpOnOff(RigPortNumber, false)) {
      //Failed to Disable the K3 KPA3 Amp
      Act_Byp = 0;   Bypass(Act_Byp);  //Set to Bypass
      lcd.setCursor(0, 1);
      lcd.print("K3 Amp NOT OFF! ");  //Display the Error
      SendMorse("K3 Amp err ");
      //Return should Set to ModeError:
      return 2;
    }
  }

  //Get the Current Band setting:
  CurrentBand = ReadBand(RigPortNumber);

  //Serial.print(F("CurrentBand Complete = ")); Serial.println(CurrentBand);

  if (CurrentBand > i6m) {  //
    //Invalid Amplifier Band: 60m, 30m, or could be somewhere outside the bands
    //return 4;
    SendMorse("Band Err");
    //Turn On amp anyway.  Operator can adjust band or Freq.
  }

  //Serial.println(F("Past Band Error Check."));

  //Display Contact and PortNumber onbottom line
  lcd.setCursor(0, 1);
  RigName = (RigModel == 1) ? "   K3S on Port " : "   KX3 on Port ";
  lcd.print(RigName + String(RigPortNumber));

  //Serial.println(F("Printed Rig Type."));

  //Turn the power on to the Amplifier
  digitalWrite(PowerSwitchPin, ON);
  Serial.println(F("Turned On Power Switch"));
  delay(2000);  //Allow time for the Power Supply to come up to voltage.
  // (Occasionally get a beep from the "Display" check if not long enough.)

  //Check the reading for the Volt Meter
  float Volts = ReadVoltage();
  if (Volts < 30) {
    //Power Up FAILED, turn the power pin back off.
    digitalWrite(PowerSwitchPin, OFF);
    return 3;  //Sets ErrorString and ModeError.
  }

  //Serial.println(F("Voltage Turn On Successful."));

  //Setup the Band Relays:
  SetupBandOutput(CurrentBand);
  //Set the Power Output for the band.
  if (UpdateBandPower(CurrentBand, RigPortNumber)) {
    //Set for continouse beep for failed UpdateBandPower???

  }
  //On:
  SendMorse("On ");
  return 0;  //OK
}


//
// Shut Down Routine
//
void OffRoutine(byte &Mode, bool DisplayOff) {
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

  if (DisplayOff) {
    //Turn off the Backlight and Display.
    lcd.setBacklight(0); //OFF
    lcd.setCursor(0, 0);
    lcd.noDisplay();
  }
  SendMorse("Off ");
  //Set the Mode to ModeOff
  Mode = ModeOff;

  //WARNING: Do NOT use Serial[1 0r 2].End  It affects the P3 so that it becomes nearly un-responsive!  (???)
}

void RigPowerDownRoutine(int RigPortNumber, byte RigModel) {
  //For the K3, Re-enable the KPA3 internal amplifier.
  //  We only go through this once!!!
  lcd.display();
  lcd.setBacklight(1);  //ON
  if (RigModel == 1) {
    //Turn the K3 Internal Amp Back ON.
    if (K3AmpOnOff(RigPortNumber, true)) {
      lcd.setCursor(0, 0);
      lcd.print("K3 Amp NOT RESET ");  //Display the Error
      SendMorse("No Rig ");
      delay(200);
      //This is not a critical fault, but Setting the K3 KPA3 amp to BYPASS assures us that we can't overdrive the LDMOS amp.
      // In this case, it was not Re-Enabled.

      //Set Tune Power to 10 Watts for the K3.
      if (SetTunePower(10, RigPortNumber)) {
        //SetTunePower Failed
        lcd.setCursor(0, 1);
        lcd.print("Reset Tune Fail ");
        SendMorse("Tune Err");
      }
    }
  }
  else {  //KX3:
    //Set Tune Power to 5 Watts for the KX3.
    if (SetTunePower(5, RigPortNumber)) {
      lcd.setCursor(0, 0);
      lcd.print("Reset Tune Fail ");  //Display the Error
      //SetTunePower Failed
      SendMorse("Tune Err");
      delay(2000);
    }
  }

  //Allow the Rig to NOT be turned Off (Press Left Key)
  lcd.setCursor(0, 0);
  lcd.print("To Keep Rig On  ");
  lcd.setCursor(0, 1);
  lcd.print("Push Left Button");

  //Variables needed for arguments, but not used:
  int CurrentBand = 255;
  byte Mode = ModeOff;
  unsigned long ulTimeout = 0;
  byte bHours = 0;
  byte bMinutes = 0;
  bool Act_Byp = 0;
  //Keep reading the buttons for 10 seconds, see if the user presses the Left button (set CurrentBand=200) to keep the Rig On.
  unsigned long WaitTime = millis();
  do {
    delay(50);
    HandleButtons(Mode, RigPortNumber, CurrentBand, bHours, bMinutes, ulTimeout, Act_Byp);
  } while (((millis() - WaitTime) < 10000) && (CurrentBand == 255));

  //If HandleButtons returned CurrentBand == 200, then DON'T turn off the Rig.
  if (CurrentBand != 200) {
    //When we Timeout, also turn off the radio and close the Comm Ports.
    RigPowerOff(RigPortNumber);
  }

  //Turn off the display.
  lcd.setBacklight(0);
  lcd.setCursor(0, 0);
  lcd.noDisplay();
  lcd.setBacklight(0);
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
      case ModeError: {
          Serial.println(F("Error"));
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
      case ModeSetupTimeout: {
          Serial.println(F("SetupPowerOffTimeout"));
          break;
        }
      case ModeSetupBypOper: {
          Serial.println(F("SetupBypOperMode"));
          break;
        }
    }
    PreviousMode = Mode;
    RunOnce = true;
  }
}
