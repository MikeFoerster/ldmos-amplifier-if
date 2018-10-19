//
// Handle the Button Presses
//
byte HandleButtons(byte &Mode, byte RigPortNumber, int &CurrentBand, int &PowerSetting, byte &bHours, byte &bMinutes, unsigned long &ulTimeout, bool &Act_Byp) {

  static int LastButton;
  static long InitialButtonTime;
  //Initial State needs more thought... FixMe
  static boolean bAdjustMinutes = true; // Which to adjust, True=Minutes, False=Hours
  boolean bWriteComplete = false;  //Used for Long Key Presses
  static int Hours;
  static bool EepromBypMode;
  static int StartSetting;  //Store the PowerSetting for when we enter the PowerSetting mode.

  /*
      Read the Buttons and update the Upper Display as required:
  */
  uint8_t buttons = lcd.readButtons();

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

        case ModeReceive: {
            //Select button pressed while in Recieve Mode, Change to ModeSetupBandPower:
            //  BUT FIRST, we need to set things up:
            //Store the StartPowerSetting so we know if it changed.
            //   (Power Setting is passed in from the main();
            StartSetting = EEPROMReadInt(CurrentBand);

            //This function just READs the current setting and displays ONLY the Error Messages such as "Already Max Power"
            //  on the second line while in the ModeSetupBandPower mode.
            // Current Setting stored in Eeprom.  Max Power allowed per band is hard coded.
            byte Read = 1;
            PowerSetting = BumpPowerSetting(Read, CurrentBand);
            Mode = ModeSetupBandPower;
            break;
          }
        case ModeSetupBandPower: {
            //Before entering next mode (ModeSetupPwrTimeout),
            //  Write the new value to Eeprom (if it changed):
            if (PowerSetting != StartSetting) {
              //Print a Wait Message while the writes take place.
              lcd.setCursor(0, 1);
              lcd.print("Wait...");
              EEPROMWriteInt(CurrentBand, PowerSetting);
              //Write the new value to the Rig:
              //Serial.println(F("  Setting PowerValue"));
              if (SetThePower(PowerSetting, RigPortNumber)) {
                //Command Failed...
                SendMorse("Pwr Err");
                //Serial.println(F("SetThePower Failed from Buttons file."));
              }
              //Also, set the "Tune" power to the same value
              if (SetTunePower(PowerSetting, RigPortNumber)) {
                //SetTunePower Failed
                //Serial.println(F("SetTunePower Failed from Buttons file."));
                SendMorse("Tune Err");
              }
            }

            //Need to initialize the Hours for the ModeSetupPwrTimeout mode.
            Hours = EEPROMReadInt(iHoursEeprom);
            StartSetting = Hours;
            //Before starting to change the Power Down Timeout, Read the current Hours stored in Eeprom:
            Mode = ModeSetupPwrTimeout;
            break;
          }
        case ModeSetupPwrTimeout: {
            //Select Key, we are in ModeSetupPwrTimeout, change to ModeSetupBypOperMode.
            //Before changing Modes, write new Hours:
            if (Hours != StartSetting) {
              //If the Hours changed, write the new value to Eeprom.
              EEPROMWriteInt(iHoursEeprom, Hours);
              bHours = Hours;
              bMinutes = 0;
              //Recalculate the Timeout after the adjustment.
              CalculateTimeout(bHours, bMinutes, ulTimeout);
            }

            //Read the EepromBypMode setting (It's either a 0 or 1).
            StartSetting = EEPROMReadInt(iBypassModeEeprom);
            if (StartSetting == 0) EepromBypMode = false;
            else EepromBypMode = true;
            //Change to the next Setup Mode:
            Mode = ModeSetupBypOperMode;
            break;
          }

        case ModeSetupBypOperMode: {
            //Before changing modes, write the new Opr/Byp value to Eeprom:
            if ((StartSetting == 0) && (EepromBypMode == true) ||
                (StartSetting == 1) && (EepromBypMode == false)) {
              EEPROMWriteInt(iBypassModeEeprom, 0);
            }
            //Select Key, we are in ModeSetupBypOperMode, change back to ModeReceive.
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
              SendMorse("Band Err ");
            }
            break;
          }
        case ModeSetupBandPower: {
            //Up Button pressed, Increement the Power by 1 watt:
            byte Increment = 2;  //Setup to Increment the BumpPowerSetting using the "2"
            PowerSetting = BumpPowerSetting(Increment, CurrentBand);
            break;
          }
        case ModeSetupPwrTimeout: {
            //Adjust Timeout Hours Up if less than 9  (Don't Let Hours go greater than 9, messes up!)
            if (Hours < 9) {
              Hours += 1;
            }
            //else { //Already at Max: }
            break;
          }
        case ModeSetupBypOperMode: {
            //Setup Mode, change from Bypass to Operate:
            EepromBypMode = true;
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
            byte Decrement = 3;  //Setup to Decrement the BumpPowerSetting using the "3"
            PowerSetting = BumpPowerSetting(Decrement, CurrentBand);
            break;
          }
        case ModeSetupPwrTimeout: {
            //Don't Let Hours go less than 1
            if (Hours > 1) {
              //Adjust Hours down:
              Hours -= 1;
            }
            //else { //Already at Min: }
            break;
          }
          case ModeSetupBypOperMode: {
            EepromBypMode = false;
            break;
          }
      }
    }  //End of 'if (DOWN)'

    //LEFT
    if (buttons & BUTTON_LEFT) {
      if (Mode == ModeSwrError) {
        //User pressed the SELECT button, change mode to Reset the Amp.
        Mode = ModeSwrErrorReset;
      }
      else {
        //Repeat the last Error from the SendMorse Error message:
        SendMorse("", true);
      }
    }

    //RIGHT
    if (buttons & BUTTON_RIGHT) {
      if (Mode == ModeReceive) {
        //RESET the Timeout:
        bHours = EEPROMReadInt(iHoursEeprom);
        bMinutes = 0;
        //Recalculate the Timeout after the adjustment.
        CalculateTimeout(bHours, bMinutes, ulTimeout);
      }
    }
  }  //End of if (buttons)


  //Wait for the Key to be released, so we don't get mulitple Entries (and check for OFF!)
  unsigned long StartTime = millis();
  while (buttons != 0) {
    buttons = lcd.readButtons();

    if (bWriteComplete == false) {
      if ((millis() - StartTime > 2000) && (buttons & BUTTON_SELECT)) {
        //2 Second SELECT button turns the unit OFF.
        //Serial.println(F("OffRoutine from Buttons-Button_Select"));
        OffRoutine(CurrentBand, Mode);
        //Reset the Mode to Off.
        Mode = ModeOff;
        bWriteComplete = true;
      }
    }
  }

  //Set switch to tell that the Long Key Presses are Complete.
  bWriteComplete = false;

  return Mode;
}
