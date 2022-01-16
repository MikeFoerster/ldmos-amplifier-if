//This file is mostly Sub calls from Main():


void SubPowerTurnedOn(boolean &AI2Mode, int &CurrentBand, byte &RigPortNumber, byte &RigModel, byte &Mode, byte &bHours, byte &bMinutes, unsigned long &ulTimeout, boolean &Act_Byp, String &ErrorString) {
  //Power On was just detected on the push buttons.

  //Check the Eeprom iAi2Mode (false = Query Mode (Original), true = AI2 Mode
  int AiMode = EEPROMReadInt(iAI2Mode);
  if (AiMode) AI2Mode = true;
  else AI2Mode = false;

  //Attempt to establish communications with the radio (either K3 or KX3)
  ErrorString = PowerUpRoutine(AI2Mode, CurrentBand, RigPortNumber, RigModel);
  if (ErrorString != "") {
    //We have some Error.  Change to ModeError and return.
    Mode = ModeError;
    return;
  }

  //Power Up Passed:
  //Change the Mode to normal Receive mode.
  if (gManualMode) Mode = ModeManual;
  else            Mode = ModeReceive;
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

void SubPrepareOff(byte &Mode, byte RigPortNumber, byte &RigModel) {

  //Run the RigPowerDownRoutine to as to turn rig off (in PowerUpDown tab).
  RigPowerDownRoutine(RigPortNumber, RigModel);

  //Power the amp off, but allow the CoolDown mode
  // Will set to either ModeCoolDown or ModeOff
  AmpOffRoutine(Mode);
}

void SubCoolDown(byte &Mode) {
  //Read the amplifier temp.
  double AmpTemp = ReadAmpTemp();

  if (AmpTemp > TurnOffTemp) {
    //Update the Fan speed until we reach 75 Deg or less. (Only updates every 10 seconds.)
    UpdateAmpFan(AmpTemp);
    delay(100);  //Wait 0.1 second before retrying. (Not longer, or the buttons won't work to restart it!)
  }
  else {
    //CoolDown Complete, Shut down the Amp.
    //Set the PID to Manual mode:
    analogWrite(iFanOutputPin, 0);

    //Turn Off Power to the Amp.
    digitalWrite(PowerSwitchPin, OFF);

    //Power Down Message
    String Line1 = "Final Power OFF ";
    String Line2 = "                ";
    lcd.setCursor(0, 0);
    lcd.print(Line1);
    lcd.setCursor(0, 1);
    lcd.print(Line2);
    delay(2000);

    //Turn off the display.
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.setBacklight(0);
    lcd.noDisplay();
    delay(100);  //Not sure if this is needed at all... (was 1 sec.)

    //Set the Mode to ModeOff
    Mode = ModeOff;
  }
}

void SubSwrError(boolean &Act_Byp) {
  // SWR Error was detected at the top of the Main Loop.
  //The only thing to clear this is to cycle Power (Done in SubSwrErrorReset).

  //Set to Bypass, and keep it that way... (repeat every cycle).
  Act_Byp = 0;  //Return Act_Byp value of 0!
  Bypass(Act_Byp);

  // Wait for Power Reset Button (Right) command which will then change Mode to ModeSwrErrorReset,
}

void SubSwrErrorReset(byte &Mode) {
  //User pressed the Select button
  // Cycle the Power OFF then ON Again.

  //We turn off the Amp, but we don't actually execute the ModeOff,
  //  so we won't turn off the Rig, because we don't leave this routine.
  AmpOffRoutine(Mode);

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

void SubReceive(int &CurrentBand, boolean &Act_Byp, boolean &FirstTransmit, double AmpTemp, byte RigPortNumber) {
  static byte count;
  //If the band is invalid, Set to Bypass
  if (CurrentBand > i6m) {
    //INVALID Band, set amp to Bypass Mode (By having this here, we KEEP it in Bypass mode)
    
    Act_Byp = 0;
    Bypass(Act_Byp);
  }

  //If the comms is disconnected or not working for a period of time, use SendMorse to notify.
  if (CurrentBand == 255) {
    //Try re-reading the Band
    CurrentBand = ReadBand(RigPortNumber, false); 
    Serial.print(F("  SubRecieve Re-Read CurrentBand as: ")); Serial.println(CurrentBand);
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

  //Update the PID to calculate the Fan Speed (Recieve Mode, Fan is ALWAYS full speed in Transmit).
  UpdateAmpFan(AmpTemp);
}

void SubTransmit(int &FwdPower, int &RefPower, boolean &FirstTransmit, float &Swr, unsigned long &ulCommTime) {
  //Make sure that the Transimit Indication is ON.
  // Update the Display for Fwd, Ref
  ReadPower(FwdPower, RefPower, FirstTransmit, Swr);

  //Clear the 'FirstTransmit' variable, will be reset in Receive mode:
  FirstTransmit = false;

  //Reset the start time so we are SURE that we don't change bands.
  ulCommTime = millis();
}

void SubManual(boolean &FirstTransmit, double AmpTemp) {
  //No update of Current Band!

  //Set to True so our first Transmit see's it as True.
  FirstTransmit = true;

  //Update the PID to calculate the Fan Speed (Recieve Mode, Fan is ALWAYS full speed in Transmit).
  UpdateAmpFan(AmpTemp);
}

