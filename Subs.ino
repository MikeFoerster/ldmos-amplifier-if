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
  ErrorString = PowerUpRoutine(CurrentBand, RigPortNumber, RigModel);
  if (ErrorString != "") {
    //We have some Error.  Change to ModeError and return.
    Mode = ModeError;
    return;
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

  //When Time Expires, will change to OFF mode.
  TimeUpdate(bHours, bMinutes, ulTimeout, Mode);
}


void SubSwrError(bool &Act_Byp) {
  // SWR Error was detected at the top of the Main Loop.
  //The only thing to clear this is to cycle Power (Done in SubSwrErrorReset).

  //Set to Bypass, and keep it that way... (repeat every cycle).
  Act_Byp = 0;
  Bypass(Act_Byp);

  // Wait for Power Reset Button (Right) command which will then change Mode to ModeSwrErrorReset,
}

void SubSwrErrorReset(byte &Mode) {
  //User pressed the Select button
  // Cycle the Power OFF then ON Again.

  //We turn off the Amp, but we don't actually execute the ModeOff,
  //  so we won't turn off the Rig, because we don't leave this routine.
  OffRoutine(Mode);

  //Wait 5 seconds
  delay(5000);

  //Go back through the Power Up Routines
  Mode = ModePowerTurnedOn;
}

void SubError() {
  static byte i;
  Bypass(0);  //Keep forcing Bypass mode.

  //Periodically (not quite every loop) keep repeating an 'e' just as a warning!
  i++;
  if (i > 10) {
  SendMorse("e");
  i = 0; //Reset i variable.
  }
}

void SubReceive(int CurrentBand, bool &Act_Byp, bool &FirstTransmit) {
  static byte count;
  //If the band is invalid, Set to Bypass
  if (CurrentBand > i6m) {
    //INVALID Band, set amp to Bypass Mode (By having this here, we KEEP it in Bypass mode)
    Act_Byp = 0;
    Bypass(Act_Byp);
  }

  //If the comms is disconnected or not working for a period of time, use SendMorse to notify.
  if (CurrentBand == 255) {
    //Increment the counter
    count += 1;
    //If no Band Update for a while, we must have lost comms, start sending an error.
    if (count > 25) SendMorse("T ");
  }
  else {
    if (count) count = 0;
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
  ulCommTime = millis();
}
