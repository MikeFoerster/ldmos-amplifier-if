//
// Handle the Button Presses
//
void HandleButtons(byte &Mode, byte RigPortNumber, int &CurrentBand, byte &bHours, byte &bMinutes, unsigned long &ulTimeout, boolean &Act_Byp, boolean &AI2Mode, byte &AntennaSwitch) {
  boolean bWriteComplete = false;  //Used for Long Key Presses
  static int Hours;  //WHY IS THIS HERE????
  static int EepromBypMode;
  static int PowerSetting;  //Store the updated PowerSetting in Setup Band Power mode.
  static int StartSetting;  //Store the PowerSetting for when we enter the PowerSetting mode.

  /*
      Read the Buttons and update the Upper Display as required:
  */
  uint8_t buttons = lcd.readButtons();

  PrintTheMode(Mode, "Entering HandleButtons");

  //If there was a button pushed, handle it...
  if (buttons) {

    /*  We use (buttons & BUTTON_SELECT) in case 2 buttons are pressed, makes sure that there is ONLY 1 button pressed.
      the single "&" or's the two together:
      0001  but:  0001
      &0001       &0011
      =0001       =0001 !!!
      So you can use this for TWO buttons:  UP AND DOWN
      0101
      &0101
      =0101
    */

    //Setup Cycle:  From Receive > SetupBandPower > SetupPwrTimeout > BypOperMode > Receive...

    //SELECT
    if (buttons & BUTTON_SELECT) {

      switch (Mode) {
        //Cycle through the different Setup Modes:
        case ModeOff: {
            //ModeOff : Change to ModePowerTurnedOn so we start the Power Up sequence.
            Mode = ModePowerTurnedOn;
            break;
          }
        case ModeCoolDown: {
            //ModeCoolDown : Change to ModePowerTurnedOn so we re-start the Power Up sequence.
            Mode = ModePowerTurnedOn;
            break;
          }
        case ModeError: {
            //We are in ModeError, pressing Select button will move to ModePowerTurnedOn
            Mode = ModePowerTurnedOn;
            break;
          }
        case ModeReceive: {
            //Select button pressed while in ModeReceive, Change to ModeSetupBandPower:

            //This function just READs the current setting and displays ONLY the Error Messages such as "Already Max Power"
            //  on the second line while in the ModeSetupBandPower mode.
            // Current Setting stored in Eeprom.  Max Power allowed per band is hard coded.
            byte Read = 1;
            PowerSetting = BumpPowerSetting(Read, CurrentBand);
            StartSetting = PowerSetting;
            Mode = ModeSetupBandPower;
            break;
          }

        //        case ModeManual: {
        //            //Select button pressed while in Manual Mode, Change to ModeSetupTimeout:
        //            //This function just READs the current setting and displays ONLY the Error Messages such as "Already Max Power"
        //            //  on the second line while in the ModeSetupBandPower mode.
        //            // Current Setting stored in Eeprom.  Max Power allowed per band is hard coded.
        //            //Need to initialize the Hours for the ModeSetupTimeout mode.
        //            bHours = EEPROMReadInt(iHoursEeprom);
        //            Mode = ModeSetupTimeout;
        //            break;
        //          }

        case ModeSetupBandPower: {
            //Select button pressed while in SetupBandPower Mode, Change to ModeSetupTimeout:
            //Before entering next mode (ModeSetupTimeout),
            //  Write the new value to Eeprom (if it changed):
            if (PowerSetting != StartSetting) {

              //Write the new value to the Rig:
              if (SetPower(PowerSetting, RigPortNumber)) {
                //Command Failed...
                SendMorse("Pwr Err");
              }
              //Also, set the "Tune" power to the same value
              if (SetTunePower(PowerSetting, RigPortNumber)) {
                //SetTunePower Failed
                SendMorse("Tune Err");
              }
            }

            //Need to initialize the Hours for the ModeSetupTimeout mode.
            Hours = EEPROMReadInt(iHoursEeprom);
            Mode = ModeSetupTimeout;
            break;
          }
        case ModeSetupTimeout: {
            //Select Key, we are in ModeSetupTimeout, change to ModeSetupBypOper.
            //Before changing Modes, write new Hours:
            if (Hours != StartSetting) {
              //Must write the Hours value when changed (rather than here) because the Display uses the Eeprom value to display the changes.
              bHours = Hours;
              bMinutes = 0;
              //Recalculate the Timeout after the adjustment.
              CalculateTimeout(bHours, bMinutes, ulTimeout);
            }
            //Read the EepromBypMode setting (It's either a 0 or 1).
            StartSetting = EEPROMReadInt(iBypassModeEeprom);
            EepromBypMode = StartSetting;

            //Change to the next Setup Mode:
            Mode = ModeSetupBypOper;
            break;
          }

        case ModeSetupBypOper: {
            //Select Key, we are in ModeSetupBypOper, change to ModeSetupAi2Mode.
            //The Opr/Byp is written when the value is changed (rather than here) because the Display uses the Eeprom value to display the changes.
            Mode = ModeSetupAi2Mode;
            break;
          }

        case ModeSetupAi2Mode: {
            //Select Key, we are in ModeSetupAi2Mode, change to ModeSetupAntenna.
            //Change Modes:
            Mode = ModeSetupAntenna;
            break;
          }

        case ModeSetupAntenna: {
            //Select Key, we are in ModeSetupAntenna, change to:
            // if gManual set: ModeManual
            // OR ModeReceive.

            //If we changed back to the AUTO setting, reset to the CurrentBand.
            if (AntennaSwitch == 0) SetAutoAntenna(CurrentBand);

            if (gManualMode == true) {
              //Read the Last Band from Eeprom:
              //CurrentBand = EEPROMReadInt(iManBand);  I don't think this is needed!
              Mode = ModeSetupManualBand;
            }
            else Mode = ModeReceive;
            break;
          }

        case ModeSetupManualBand: { //Should be the LAST in the string so that pressing the Select, gets you out to Receive next...
            //Select button pressed while in ModeSetManualBand, Change to ModeReceive:
            //Before Reseting back to ModeManual, write the Manual CurrentBand to Eeprom
            EEPROMWriteInt(iManBand, CurrentBand);

            //Reset back to Recieve Mode:
            Mode = ModeReceive;
            break;
          }
      } //End of switch Mode

      //NOTE: ModePowerTurnOn, ModeReceive, ModeSwrError, ModeOverTemp:  Hold button for 3 seconds or greater, Turn Off
    }  //End of Select button

    //UP
    if (buttons & BUTTON_UP) {
      //Off Mode and AutoMode, Ignore

      //If The Up button is pressed, change from Bypass to Amplify
      switch (Mode) {
        case ModeReceive: {
            //Up Button, Change from Bypass to Operate (Valid operating band only!)
            if (CurrentBand <= i6m) {
              Act_Byp = 1;
              Bypass(Act_Byp);  //Set from Bypass to Operate
            }
            else {
              SendMorse("T ");
            }
            break;
          }
        case ModeSetupBandPower: {
            //Up Button pressed, Increement the Power by 1 watt:
            byte Increment = 2;  //Setup to Increment the BumpPowerSetting using the "2", Also writes to Eeprom.
            PowerSetting = BumpPowerSetting(Increment, CurrentBand);   //Write the vale that was changed to Eeprom
            break;
          }
        case ModeSetupTimeout: {
            //Adjust Timeout Hours Up if less than 9  (Don't Let Hours go greater than 9, messes up!)
            if (Hours < 9) Hours += 1;  //Adjust Hours Up
            //else { //Already at Max: }
            //Write the new value to Eeprom so the Display Updates too!
            EEPROMWriteInt(iHoursEeprom, Hours);
            break;
          }
        case ModeSetupBypOper: {
            //Setup Mode, change from Bypass to Operate:
            EepromBypMode = 1;
            //Write the new value to Eeprom so the Display Updates too!
            EEPROMWriteInt(iBypassModeEeprom, EepromBypMode);
            break;
          }
        case ModeSetupAi2Mode: {
            EEPROMWriteInt(iAI2Mode, 1);
            AI2Mode = EEPROMReadInt(iAI2Mode);

            break;
          }
        case ModeSetupAntenna: {
            AntennaSwitch = ManualBandIncDec(true); //True will increment by 1
            break;
          }
        case ModeSetupManualBand: {
            if (CurrentBand < 32)  CurrentBand = CurrentBand + 2;
            else CurrentBand = 0;
            //Update relays to the new band
            SetupAmpBandOutput(CurrentBand);
            break;
          }
      }
    }  //End Up button

    //DOWN
    if (buttons & BUTTON_DOWN) {
      //Off Mode and AutoMode, Ignore
      switch (Mode) {
        case ModeReceive: {
            //If In Bypass Mode, Change back to Operate
            Act_Byp = 0;
            Bypass(Act_Byp);  //Set from Operate to Bypass
            break;
          }
        case ModeSetupBandPower: {
            //Down Button pressed, Decreement the Power by 1 watt:
            byte Decrement = 3;  //Setup to Decrement the BumpPowerSetting using the "3". Also writes to Eeprom.
            PowerSetting = BumpPowerSetting(Decrement, CurrentBand);
            break;
          }
        case ModeSetupTimeout: {
            //Don't Let Hours go less than 1
            if (Hours > 1) Hours -= 1; //Adjust Hours Down
            //else { //Already at Min: }
            //Write the new value to Eeprom so the Display Updates too!
            EEPROMWriteInt(iHoursEeprom, Hours);
            break;
          }
        case ModeSetupBypOper: {
            EepromBypMode = 0;
            //Write the new value to Eeprom so the Display Updates too!
            EEPROMWriteInt(iBypassModeEeprom, EepromBypMode);
            break;
          }
        case ModeSetupAi2Mode: {
            EEPROMWriteInt(iAI2Mode, 0);
            AI2Mode = EEPROMReadInt(iAI2Mode);
            break;
          }
        case ModeSetupAntenna: {
            AntennaSwitch = ManualBandIncDec(false); //False will Decrement by 1
            break;
          }
        case ModeSetupManualBand: {
            if (CurrentBand > 0)  CurrentBand = CurrentBand - 2;
            else CurrentBand = 32;
            //Update relays to the new band
            SetupAmpBandOutput(CurrentBand);
            break;
          }
      }
    }  //End of 'if (DOWN)'

    //LEFT
    if (buttons & BUTTON_LEFT) {
      switch (Mode) {
        case ModeOff: {
            //Use the CurrentBand set to 200 to indicate to NOT turn off the Rig in "Subs".
            CurrentBand = 200;
            break;
          }
        case ModeSwrError: {
            //User pressed the SELECT button, change mode to Reset the Amp.
            Mode = ModeSwrErrorReset;
            break;
          }
        case ModeSwrErrorReset: {
            //Do Nothing (Keeps from Repeating the "SendMorse").
            break;
          }
        case ModeSetupBandPower: {
            //Cycle the Band Up to 6m, then drop back down to 160.
            if (CurrentBand > i160m)CurrentBand = CurrentBand - 2;
            else CurrentBand = i6m;
            break;
          }
        case ModeError: {  //Left Button Changes to ModeManual!!!!
            gManualMode = true;
            CurrentBand = EEPROMReadInt(iManBand);
            Mode = ModeSetupManualBand;
            SendMorse("MAN");

            //We need to take care of some of the stuff from the Power Up routines
            //Turn the power on to the Amplifier
            digitalWrite(PowerSwitchPin, ON);
            delay(2000);  //Allow time for the Power Supply to come up to voltage.
            // (Occasionally get a beep from the "Display" check if not long enough.)

            //Check the reading for the Volt Meter
            float Volts = ReadVoltage();
            if (Volts < 30) {
              //Power Up FAILED, turn the power pin back off.
              digitalWrite(PowerSwitchPin, OFF);
              //return "50 Volt Fail";  //Sets ErrorString and ModeError.
            }

            //We are in Manual Mode, Read the LAST Manual Band from Eeprom and Update the Band relays:
            CurrentBand = EEPROMReadInt(iManBand);
            SetupAmpBandOutput(CurrentBand);

            //RESET the Timeout:
            bHours = EEPROMReadInt(iHoursEeprom);
            bMinutes = 0;
            //Recalculate the Timeout after the adjustment.
            CalculateTimeout(bHours, bMinutes, ulTimeout);

            break;
          }
        default: {
            //Call SendMorse with blank string to Repeat the last Error message:
            SendMorse("");
            break;
          }
      }
    }


    //RIGHT
    if (buttons & BUTTON_RIGHT) {
      if ((Mode == ModeReceive) || (Mode == ModeManual)) {
        //RESET the Timeout:
        bHours = EEPROMReadInt(iHoursEeprom);
        bMinutes = 0;
        //Recalculate the Timeout after the adjustment.
        CalculateTimeout(bHours, bMinutes, ulTimeout);
      }
      if (Mode == ModeSetupBandPower) {
        //Cycle the Band Up to 6m, then drop back down to 160.
        if (CurrentBand < i6m)        CurrentBand = CurrentBand + 2;
        else CurrentBand = i160m;
      }
    }

  }  //End of if (buttons)

  //Wait for the Key to be released, so we don't get mulitple Entries (and check for OFF!)
  unsigned long StartTime = millis();
  while (buttons != 0) {
    buttons = lcd.readButtons();

    //Loop here until the button is released.  We can detect if the User wants to turn the system off.
    if (bWriteComplete == false) {
      if ((millis() - StartTime > 1500) && (buttons & BUTTON_SELECT)) {
        //2 Second SELECT button turns the unit OFF.
        lcd.setCursor(0, 0);
        lcd.print("Power OFF       ");
        lcd.setCursor(0, 1);
        lcd.print("Check Cool Down ");
        bWriteComplete = true;
        Mode = ModePrepareOff;
      }
    }
  }

  //Set switch to tell that the Long Key Presses are Complete.
  bWriteComplete = false;
}
