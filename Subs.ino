//This file is mostly Sub calls from Main():

void SubOff(byte &RigModel, byte &RigPortNumber, byte &bHours, byte &bMinutes, bool &Act_Byp, int &CurrentBand) {
  //System is off.
  //Power to the Amp is Off.
  //Display is blank.

  //First time thru, Reset Rig Amp and Tune Power to default, Turn off the Rig and clear the variables.
  if (RigModel) {
    //Run the RigPowerDownRoutine to as to turn rig off (in PowerUpDown tab).
    RigPowerDownRoutine(RigPortNumber, RigModel);
    //Finally, turn off some of the variables
    bHours = 0;
    bMinutes = 0;
    Act_Byp = 0;
    CurrentBand = 255;
    RigPortNumber = 0;
    //Clear the RigModel variable so we no longer write to the K3:
    RigModel = 0;
  }
  //WARNING: Do NOT use Serial[1 0r 2].End  It affects the P3 so that it becomes nearly un-responsive!
}

void SubPowerTurnedOn(int &CurrentBand, byte &RigPortNumber, byte &RigModel, byte &Mode, byte &bHours, byte &bMinutes, unsigned long &ulTimeout, bool &Act_Byp) {
  //Power On was just detected on the push buttons.
  //Attempt to establish communications with the radio (either K3 or KX3)
  byte PwrUpResponse = PowerUpRoutine(CurrentBand, RigPortNumber, RigModel, Act_Byp);
  switch (PwrUpResponse) {
    case 1: {
        //FAILED, Didn't establish Comms:
        lcd.setCursor(0, 1);
        lcd.print("No Comm with Rig");
        delay(2000);
        //Turn the Mode back off... (but we didn't call OffRoutine, so the display stays with the message.)
        Mode = ModeOff;
        return;
        break;
      }
    case 2: {
        //Failed to Disable K3 Internal Amp
        //Continue On anyway
        break;
      }
    case 3: {
        //Failed Power Up 50v Test
        //Change back to Off Mode  (but we didn't call OffRoutine, so the display stays with the message.)
        Mode = ModeOff;
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
}


void SubSwrError(bool &SwrFailMessage, bool &Act_Byp) {
  // SWR Error was detected at the top of the Main Loop.
  //The only thing to clear this is to cycle Power (Done in SubSwrErrorReset).

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
  OffRoutine(Mode);

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

void SubReceive(int CurrentBand, bool &Act_Byp, bool &FirstTransmit) {
  //Read Internal Temp, even in the Off Mode:
  //        int InternalTemp = ReadInternalTemp();
  //        if (InternalTemp > 120) {
  //          //Alarm???

  //If the band is invalid, Set to Bypass
  if (CurrentBand > i6m) {
    //INVALID Band, set amp to Bypass Mode
    Act_Byp = 0;
    Bypass(Act_Byp);
  }
  FirstTransmit = true;
}

void SubTransmit(int &FwdPower, int &RefPower, bool &FirstTransmit, float &Swr, unsigned long &ulCommTime) {
  //Make sure that the Transimit Indication is ON.
  // Update the Display for Fwd, Ref
  ReadPower(FwdPower, RefPower, FirstTransmit);

  //Clear the 'FirstTransmit' variable, will be reset in Receive mode:
  FirstTransmit = false;

  Swr = CalculateSwr(FwdPower, RefPower);
  //double AmpTemp = ReadAmpTemp();  //Read the amplifier temp.
  //Reset the start time so we are SURE that we don't change bands.
  ulCommTime = millis();  //Serial.println(F("Hit ulCommWait in ModeTransmit."));
}

//void SubOverTemp(byte OverTemp, byte &Mode) {
//  //OverTemp was detected in the top of the Loop.
//  //  Only thing to do is to wait for Overtemp to clear.  It will clear when the Heat Sink cools down.
//  if (OverTemp == 1) Mode = ModeReceive;
//}

//void SubSetupBandPower() {
//  //User is in Setup Mode: Changing the Per Band Power Output.
//}
