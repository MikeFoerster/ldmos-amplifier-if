
//
// Turn On Routine
//
String PowerUpRoutine(boolean &AI2Mode, int &CurrentBand, byte &RigPortNumber,  byte &RigModel, boolean &Act_Byp) {
  //Returns 0 = Passed
  //Returns 1 = Failed to Establish Comms
  //Returns 2 = Failed to Disable K2 Internal Amp
  //Returns 3 = Power Up, NO 50v detected
  //Returns 4 = Invalid Band (could be outside the Ham Bands)
  //Returns 5 = SetupAmpBandOutput failed.
  String ErrorString = "";
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

  //Make sure the ports are closed (becomes a problem when reloading code thru IDE)
  //  Serial1.flush();
  //  Serial2.flush();
  //  Serial3.flush();
  //  Serial1.end();
  //  Serial2.end();
  //  Serial3.end();
  //  delay(100);

  // Set the data rate for Serial Port 1
  Serial1.begin(38400);
  // And for Serial Port 2.
  Serial2.begin(38400);
  // And for Serial Port 3 for Bluetooth.
  Serial3.begin(115200);
  //
  //Establish Comms, Get the Current Band:
  //
  unsigned long StartTime = millis();  //Set Start time for 'do' loop

  //Initialize the RigPortNumber so once we enter the loop, we will start with Port 1.
  RigPortNumber = 0;

  //Make sure that the TX Inhibit is OFF to the ESP32.
  digitalWrite(TxInhibit, 0);

  //Wake the ESP32 from sleep, so it can detect if the IC-705 is connected:
  digitalWrite(WakePin, 1);
  delay(100);
  digitalWrite(WakePin, 0);
  delay(2000);
  String Response = Serial3Read(true);
  if (Response == "wake") {
    Serial.println(F(" Got the Wake signal from the ESP32 BT."));
  }
  else {
    Serial.println(F(" No Wake signal from the ESP32 BT."));
  }


  // IF IC-705, Need to Connect Bluetooth!!!
  Serial.println(F("Connect IC-705 Bluetooth now!"));
  delay(3000); // Allow the ESP32 to start up (may have to play with this value



  do {
    delay(100);
    //Try to read the Band from the Radio, trying both comm ports: (Using If/Else construct)
    RigPortNumber++;
    if (RigPortNumber == 4) RigPortNumber = 1;

    RigModel = GetRigModel(RigPortNumber);  //Returns 1 or 2 or 3 when we establish comms

  } while ((RigModel == 0) && ((millis() - StartTime) < 30000));

  if (RigModel == 0) {
    //Failed to establish Comms
    return  "No Comm with Rig";
  }

  //We've established Comms:

  //For the K3, DISABLE the internal 100w Amp to prevent overdrive:
  if (RigModel == 1) {
    //Turn the Internal K3 Amp Off
    if (K3AmpOnOff(RigPortNumber, false)) {
      //Failed to Disable the K3 KPA3 Amp
      SendMorse("K3 Amp err ");
      //Return should Set to ModeError:
      return "K3 Internal Amp";
    }
  }

  //For ALL - K3 and KX3 & IC-705 Turn on AI2 Mode. (Value is read in SubPowerTurnedOn().
  if (AI2Mode) {
    //Turn the AI mode to AI2;
    if (SetAiOnOff(2, RigPortNumber)) {
      //Change the AI2Mode back to FALSE!!!
      AI2Mode = false;
      //SetAiOff failed:
      SendMorse("AI On err ");
      //Return should Set to ModeError:
      return "AI On Error";
    }
  }
  else {
    //Disable the AI (Auto-Info) mode:
    if (SetAiOnOff(0, RigPortNumber)) {
      //SetAiOff failed:
      SendMorse("AI On err ");
      //Return should Set to ModeError:
      return "AI Off Error";
    }
  }

  if (RigModel == 3) {
    //IC-705 has Transceive Mode (similar to AI Mode on Elecraft)
    //For the IC-705, it currently works MUCH better with the AI2 Off.

    //Try closing the other two Serial Ports:
    Serial1.end();
    Serial2.end();

    RigPortNumber = 3;  //Set the PortNumber to 3.
    //Serial.println(F("Detected ICOM on Port 3."));
  }

  //  Get the Current Band setting using the Query mode (Ignore AI2Mode!)
  boolean TmpHamBand;  //Will be set correctly once we start running.
  ReadBand(CurrentBand, RigPortNumber, false, TmpHamBand);
    Act_Byp = TmpHamBand;
    Bypass(Act_Byp);
  

  //Not using the ICOM, Put the ESP32 Bluetooth back to sleep:
  if (RigModel != 3) {
    Serial3Cmd("slep");
  }

  Serial.print(F("CurrentBand in PowerOn() returned: ")); Serial.println(CurrentBand);

  if (CurrentBand > i6m) {
    //Invalid Amplifier Band: 60m, 30m, or could be somewhere outside the bands
    SendMorse("Band Err");
    //Turn On amp anyway.  Operator can adjust band or Freq.
  }

  //Display Contact and PortNumber onbottom line
  lcd.setCursor(0, 1);

  switch (RigModel) {
    case 1: {
        RigName = "   K3S on Port ";
        break;
      }
    case 2: {
        RigName = "   KX3 on Port ";
        break;
      }
    case 3: {
        RigName = "   IC-705 Port ";
        break;
      }
  }

  lcd.print(RigName + String(RigPortNumber));

  //Turn the power on to the Amplifier
  digitalWrite(PowerSwitchPin, ON);
  delay(2000);  //Allow time for the Power Supply to come up to voltage.
  // (Occasionally get a beep from the "Display" check if not long enough.)

  //Check the reading for the Volt Meter
  float Volts = ReadVoltage();
  if (Volts < 30) {
    //Power Up FAILED, turn the power pin back off.
    digitalWrite(PowerSwitchPin, OFF);
    return "50 Volt Fail";  //Sets ErrorString and ModeError.
  }

  //Setup the Band Relays:
  SetupAmpBandOutput(CurrentBand);
  //Set the Power Output for the band.
  if (UpdateBandPower(CurrentBand, RigPortNumber)) {
    //Set for continouse beep for failed UpdateBandPower???
    return "Power Seting Err";
  }

  //Setup the Antenna Output:
  SetAutoAntenna(CurrentBand);

  //On:
  SendMorse("On ");
  return ErrorString;  //OK
}


//
// Shut Down Routine
//
void AmpOffRoutine(byte &Mode) {
  //Clear the Antenna Output Relays to Off Mode (Set to 160M which turns off all band relays.

  lcd.print("Off             ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  delay(500);

  //Make sure that the TXInhibit is off.
  digitalWrite(TxInhibit, 0);
  delay(100);

  //Send the ESP32 BT Sleep command, we don't know (in this function) which Rig was in use, but if it WASN'T the IC-705, the command will do nothing anyway...
  if ((Mode != 0) && (Mode != ModeSwrErrorReset)) {  //No need to try the 'slep' when called from the setup() function.
    Serial.println(F("                            AmpOffRoutine executing 'slep'."));
    Serial3Cmd("slep");  //No Response expected.
  }

  //Set to i160m, will turn off all relays.
  SetupAmpBandOutput(i160m);
  //Clear the Antenna Outputs:
  Clear_Outputs();
  //Set Bypass output to off.
  Bypass(0);

  SendMorse("Off ");

  Mode = ModeCoolDown;
}

void RigPowerDownRoutine(int RigPortNumber, byte &RigModel) {

  //For the K3, Re-enable the KPA3 internal amplifier.
  //  We only go through this once!!!
  lcd.display();
  lcd.setBacklight(1);  //ON

  if (!gManualMode) {
    //Turn the AI2 to Off (if it's already Off, that's OK...
    if (SetAiOnOff(0, RigPortNumber)) {
      //SetAiOff failed:
      lcd.print("AI OFF Error ");  //Display the Error
      SendMorse("AI Off Err");
      delay(4000);
    }
    delay(100);

    if (RigModel == 1) {
      //Turn the K3 Internal Amp Back ON.
      if (K3AmpOnOff(RigPortNumber, true)) {
        lcd.setCursor(0, 0);
        lcd.print("K3 Amp NOT RESET ");  //Display the Error
        SendMorse("No Rig ");
        delay(4000);
        //This is not a critical fault, but Setting the K3 KPA3 amp to BYPASS assures us that we can't overdrive the LDMOS amp.
        // In this case, it was not Re-Enabled.
      }
      //Set Tune Power to 10 Watts for the K3.
      if (SetTunePower(10, RigPortNumber)) {
        //SetTunePower Failed
        lcd.setCursor(0, 1);
        lcd.print("Reset Tune Fail ");
        SendMorse("Tune Err");
        delay(4000);
      }
      //Reset Power to 28 Watts:
      if (SetPower(28, RigPortNumber)) {
        //SetPower Failed
        lcd.setCursor(0, 1);
        lcd.print("Reset Power Fail ");
        SendMorse("Pwr Err");
        delay(4000);
      }
    }
    else if (RigModel == 2) {  //KX3:
      //Set Tune Power to 3 Watts for the KX3.
      if (SetTunePower(3, RigPortNumber)) {
        lcd.setCursor(0, 0);
        lcd.print("Reset Tune Fail ");  //Display the Error
        //SetTunePower Failed
        SendMorse("Tune Err");
        delay(4000);
      }
      //Reset Power to 10 Watts:
      if (SetPower(10, RigPortNumber)) {
        //SetPower Failed
        lcd.setCursor(0, 1);
        lcd.print("Reset Power Fail ");
        SendMorse("Pwr Err");
        delay(4000);
      }
    }
    else if (RigModel == 3) { //IC-705
      // NOTE: We don't have adjustments for Tune Power on IC-705.
      //Reset the Power to 9 Watts:
      SetPower(9, RigPortNumber);
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
    boolean Act_Byp = 0;
    boolean AI2Mode = 0;
    byte AntennaSwitch = 0;

    //Keep reading the buttons for 4 seconds, see if the user presses the Left button (set CurrentBand=200) to keep the Rig On.
    unsigned long WaitTime = millis();
    do {
      delay(50);
      HandleButtons(Mode, RigPortNumber, CurrentBand, bHours, bMinutes, ulTimeout, Act_Byp, AI2Mode, AntennaSwitch);
    } while (((millis() - WaitTime) < 4000) && (CurrentBand == 255));

    //If HandleButtons returned CurrentBand == 200, then DON'T turn off the Rig.
    if (CurrentBand != 200) {
      lcd.setCursor(0, 0);
      lcd.print("Turning Rig Off ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      //When we Timeout, also turn off the radio and close the Comm Ports.
      RigPowerOffCmd(RigPortNumber);
    }

    //Reset RigModel to 0.
    RigModel = 0;

  }
  //Turn off the gManualMode:
  else gManualMode = false;

  //DON'T Flush & End the serial Ports, this causes the P3 on the K3 to tie up comms, and P3 stops responding!
  // Instead, make sure they are initialized:
  // Initialize the Serial Ports, especially for the Setup() routine when the RigPowerDownRoutine() is called:
  Serial1.begin(38400);
  // And for Serial Port 2.
  Serial2.begin(38400);
  // And for Serial Port 3, Bluetooth.
  Serial3.begin(115200);
}


void PrintTheMode(byte Mode, String Optional) {
  //This is a Debug Routine Only, Print out the Mode as a string...
  static boolean RunOnce;
  static byte PreviousMode;
  if ((Mode != PreviousMode) || (RunOnce == false)) {
    Serial.print(F("Mode="));
    switch (Mode) {
      case ModeOff: {
          Serial.print(F("ModeOff"));
          break;
        }
      case ModePowerTurnedOn: {
          Serial.print(F("ModePowerTurnedOn"));
          break;
        }
      case ModeSwrError: {
          Serial.print(F("ModeSwrError"));
          break;
        }
      case ModePrepareOff: {
          Serial.print(F("ModePrepareOff"));
          break;
        }
      case ModeCoolDown: {
          Serial.print(F("ModeCoolDown"));
          break;
        }
      case ModeSwrErrorReset: {
          Serial.print(F("ModeSwrErrorReset"));
          break;
        }
      case ModeError: {
          Serial.print(F("ModeError"));
          break;
        }
      case ModeReceive: {
          Serial.print(F("ModeReceive"));
          break;
        }
      case ModeTransmit: {
          Serial.print(F("ModeTransmit"));
          break;
        }
      case ModeManual: {
          Serial.print(F("ModeManual"));
          break;
        }
      case ModeOverTemp: {
          Serial.print(F("ModeOverTemp"));
          break;
        }
      case ModeSetupBandPower: {
          Serial.print(F("ModeSetupBandPower"));
          break;
        }
      case ModeSetupTimeout: {
          Serial.print(F("ModeSetupTimeout"));
          break;
        }
      case ModeSetupBypOper: {
          Serial.print(F("ModeSetupBypOper"));
          break;
        }
      case ModeSetupAi2Mode: {
          Serial.print(F("ModeSetupAi2Mode"));
          break;
        }
      case ModeSetupManualBand: {
          Serial.print(F("ModeSetupManualBand"));
          break;
        }
      case ModeSetupAntenna: {
          Serial.print(F("ModeSetupAntenna"));
          break;
        }
    }
    Serial.println(" " + Optional);
    PreviousMode = Mode;
    RunOnce = true;
  }
}
