//This file is mostly Sub calls from Main():

void SubOff(byte &RigModel, byte &RigPortNumber) {
  //Called ONLY if RigModel is > 0.
  //System is off.
  //Power to the Amp is Off.
  //Display is blank.

  //Run the RigPowerDownRoutine to as to turn rig off (in PowerUpDown tab).
  RigPowerDownRoutine(RigPortNumber, RigModel);

}

void SubPowerTurnedOn(int &CurrentBand, byte &RigPortNumber, byte &RigModel, byte &Mode, byte &bHours, byte &bMinutes, unsigned long &ulTimeout, bool &Act_Byp, String &ErrorString) {
  //Power On was just detected on the push buttons.
  //Attempt to establish communications with the radio (either K3 or KX3)
  byte PwrUpResponse = PowerUpRoutine(CurrentBand, RigPortNumber, RigModel, Act_Byp);
  switch (PwrUpResponse) {
    case 1: {
        //FAILED, Didn't establish Comms:
        ErrorString = "No Comm with Rig";
        Mode = ModeError;
        return;
        break;
      }
    case 2: {
        //Failed to Disable K3 Internal Amp
        //Change to Error Mode
        ErrorString = "K3 Internal Amp";
        Mode = ModeError;
        return;
        break;
      }
    case 3: {
        //Failed Power Up 50v Test
        ErrorString ="50 Volt Fail";
        Mode = ModeError;
        return;
        break;
      }
    case 4: {
        //Not a valid band, Outside Ham Bands
        //For now, continue on...  Will be forced into Bypass mode later...
        break;
      }
  }

  //Power Up Passed:
  //Change the Mode to normal Receive mode.
  Mode = ModeReceive;
  //Initialize the Time:
  bHours = EEPROMReadInt(iHoursEeprom);
  bMinutes = 0;
  //Calculate the Timeout Time with the current Hours and Minutes:
  CalculateTimeout(bHours, bMinutes, ulTimeout);


  if (CurrentBand > i6m) {
    //Invalid Band:
    lcd.setCursor(0, 1);
    lcd.print("Invalid Band    ");
    delay(2000);
    Act_Byp = 0;
    Bypass(Act_Byp);
  }
  else {
    //Setup Bypass/Operate Mode per stored value:
    //Read the data in Eeprom, should we start in Bypass(0) or Operate(1) mode?
    Act_Byp = bool(EEPROMReadInt(iBypassModeEeprom));
    Bypass(Act_Byp);
  }

  //Turn the Mode back off...
  Mode = ModeReceive;
  //Do I want to try to get the AI2 working???
  //Set Rig to Mode "AI2" to indicate Freq & Power Changes:
  //if (SetRigAi2Mode(RigPortNumber))

  //When Time Expires, will change to OFF mode.
  TimeUpdate(bHours, bMinutes, ulTimeout, Mode);
}


void SubSwrError(bool &SwrFailMessage, bool &Act_Byp) {
  // SWR Error was detected at the top of the Main Loop.
  //The only thing to clear this is to cycle Power (Done in SubSwrErrorReset).

  //I don't think this 'if' is really necessary, nor is the SwrFailMessage!!!
  if (!(SwrFailMessage)) {
    //Indicate the error.  This should beep pretty much continously, each time through the loop.
    // ERR
    SendMorse("Swr Err ");

    //  Set the Error Message:
    SwrFailMessage = true;
  }

  //Set to Bypass, and keep it that way... (repeat every cycle).
  Act_Byp = 0;
  Bypass(Act_Byp);

  // Wait for Power Reset Button (Right) command which will then change Mode to ModeSwrErrorReset,
  //   Power Down, Wait for Voltage to drop to near Zero (about 5 seconds), then Power up again.
}

void SubSwrErrorReset(int &CurrentBand, byte &Mode, bool &Act_Byp, byte &RigPortNumber, byte &RigModel) {
  //User pressed the Select button
  // Cycle the Power OFF then ON Again.

  //We turn off the Amp, but we don't actually execute the ModeOff,
  //  so we won't turn off the Rig, because we don't leave this routine.
  OffRoutine(Mode, 0);

  //Make sure we are in Bypass mode.
  Act_Byp = 0;
  Bypass(Act_Byp);

  //Wait 5 seconds
  delay(5000);
  byte PowerUpResponse = PowerUpRoutine(CurrentBand, RigPortNumber, RigModel, Act_Byp);
  if (PowerUpResponse == 1) {
    //FAILED, Didn't establish Comms again:
    lcd.setCursor(0, 1);
    lcd.print("No Comm with Rig");
    delay(2000);
    //Turn the Mode back off...
    Mode = ModeOff;
    return;
  }

  //Finally, change the mode to Receive.
  Mode = ModeReceive;  //?? Is there something else that should change the Mode to Receive?????
}

void SubError() {
  Bypass(0);  //Keep forcing Bypass mode.
  //Keep repeating an 'e' just as a warning!
  SendMorse("e");
}

void SubReceive(int CurrentBand, bool &Act_Byp, bool &FirstTransmit) {
  //If the band is invalid, Set to Bypass
  if (CurrentBand > i6m) {
    //INVALID Band, set amp to Bypass Mode (By having this here, we KEEP it in Bypass mode)
    Act_Byp = 0;
    Bypass(Act_Byp);
  }
  //Set to True so our first Transmit see's it as True.
  FirstTransmit = true;
}

void SubTransmit(int &FwdPower, int &RefPower, bool &FirstTransmit, float &Swr, unsigned long &ulCommTime) {
  //Make sure that the Transimit Indication is ON.
  // Update the Display for Fwd, Ref
  ReadPower(FwdPower, RefPower, FirstTransmit);

  //Clear the 'FirstTransmit' variable, will be reset in Receive mode:
  FirstTransmit = false;

  Swr = CalculateSwr(FwdPower, RefPower);

  //Reset the start time so we are SURE that we don't change bands.
  ulCommTime = millis();  //Serial.println(F("Hit ulCommWait in ModeTransmit."));
}
