
boolean CheckBandUpdate(int &CurrentBand, byte RigPortNumber, byte &Mode, boolean AI2Mode, byte AntennaSwitch, boolean &Act_Byp) {
  //Returns true if the band updates.
  //Used every so many seconds to see if the band changed.

  //Problem: With slow Band Changes on K3, the AI2 modes doesn't always seem to keep up, may loose the last band change.

  boolean AntennaSet = false;
  boolean HamBand = false;

  //Check for Band Update:  Takes about 340 ms:
  boolean Changed = false;
  Changed = ReadBand(CurrentBand, RigPortNumber, AI2Mode, HamBand);

  
  //FOR NOW, Set the Act_Byp to the HamBand, we update this every loop, in case the Frequency changed to out of Band...
  Act_Byp = HamBand;
  Bypass(Act_Byp);

  if (Changed) {  //Function just above...
    Serial.println(F("   BAND CHANGED."));
    //Update relays to the new band
    SetupAmpBandOutput(CurrentBand);  //Function just below...

    //Set the Power for the Rig to the Power Value stored in Eeprom for the new band setting.
    if (UpdateBandPower(CurrentBand, RigPortNumber)) {
      //Set for Continous beep for failed UpdateBandPower???
      Mode = ModeError;
    }

    //Serial.print(F("  AntennaSwitch Value: ")); Serial.println(AntennaSwitch);
    //Set the Antenna output also:
    if (AntennaSwitch == 0) {
      //AntennaSwitch is in AUTO mode
      AntennaSet = SetAutoAntenna(CurrentBand);
      //Don't know what else to do with the Antenna fail, don't think it should.
      if (AntennaSet) Serial.println(F("Antenna Failed to Set."));
    }
    //The Band Updated.
    return true;
  }
  else {
    //No Updates.
    return false;
  }
}


boolean ReadBand(int &CurrentBand, byte RigPortNumber, boolean AI2Mode, boolean &HamBand) {
  //Returns true if the band changed.
  //Returns False if the band did NOT change.

  //Using Frequency:  Returns new band as an integer.
  //  Return 254 for AI2Mode for no update (No Comms from AI2 Mode!)
  //  Returns 255 for invalid Frequency.

  //Store the CurrentBand and LastHamBand:
  static int LastBand;
  static boolean LastHamBand;

  //Serial.print(F(" HamBand coming in is: ")); Serial.print(HamBand); Serial.print(F(" LastHamBand is: ")); Serial.println(LastHamBand);

  unsigned int TargetFreq = 0;
  //Elecraft A2I Mode, Serial Comms Buffer Overflow (Greater than 64 bytes) in AI2 is common.  Typically happens on any Band Change.
  //  BUT I found a way to increase the Comms Buffer so there is no longer an overflow!

  if (RigPortNumber <= 2) { //Elecraft RigPortNumber 1 or 2:
    //If we are in AI2 Mode:
    if (AI2Mode) { //Elecraft
      String AI2Output = RadioAiResponse(RigPortNumber);
      int Length = AI2Output.length();
      if (Length == 0) {
        //No Change
        HamBand = LastHamBand;
        return false;
      }
      else { //There is content to the message:
        //Serial.print(F(" AI2Output Length is ")); Serial.println(AI2Output.length());    //Serial.print(F(" AI2 Output: ")); Serial.println(AI2Output);
        //See Comments about BUFFER OVERRUN on Main File!!!   LEAVE THIS HERE.  PC SPECIFIC MODIFICATION to increase the TX buffer from 64 to 256 bytes.
        if ((Length > 62) && (Length < 65)) {
          Serial.println(F("Check for Buffer OverRun"));
        }

        TargetFreq = ReadFrequencyFromFA(AI2Output);  //Get the "FA" frequency from the response.
        if (TargetFreq > 0) {
          CurrentBand = FreqToBand(TargetFreq, HamBand);
          //Serial.print(F("Found TargetFreq = ")); Serial.print(TargetFreq); Serial.print(F("   LastBand = ")); Serial.print(LastBand); Serial.print(F("  CurrentBand updated to: ")); Serial.print(CurrentBand); Serial.print(F("  HamBand: ")); Serial.println(HamBand);
          if (CurrentBand <= i30m) {
            //Possibly Band Change:
            if (CurrentBand != LastBand) {
              //Band Change
              //HamBand may NOT get set everyloop, store it.
              HamBand = LastHamBand;
              LastBand = CurrentBand;
              return true;
            }
            else {
              LastHamBand = HamBand;
              return false;
            }
          }
          else if (CurrentBand == 255) { //If the FreqToBand Fails, try the ReadTheFrequency manually!
            delay(50);
            Serial.print(F("     AI2 Freq Read Failed on Band Change, Retrying: ")); Serial.println(CurrentBand);
            TargetFreq = ReadTheFrequency(RigPortNumber);
            if (TargetFreq > 0) {
              CurrentBand = FreqToBand(TargetFreq, HamBand);
              LastHamBand = HamBand;
            }
          }
        }
        else {
          //No FA in the string...
          HamBand = LastHamBand;
          return false;
        }
      }
    }
    else { //Else Standard Query Mode:
      //Read the Frequency from the Radio and convert to Band (byte)
      TargetFreq = ReadTheFrequency(RigPortNumber);
      if (TargetFreq > 0) {
        //HamBand gets updated every loop...
        CurrentBand = FreqToBand(TargetFreq, HamBand);
        if (CurrentBand == LastBand) {
          //No Change
          return false;
        }
        Serial.println(F("  This is the Standard Query!"));
        LastBand = CurrentBand;
        return true;
      }
      else {
        //What to do if the Target Frequency Fails?
        //Serial.println(F(" Elecraft Standard Band Query.  Not sure what to do."));
        HamBand = LastHamBand;
        return false;
      }
    }
  }

  else {   //ICOM Transceive Mode (Like AI2) OR Polling Mode: (ESP32 Takes care of both modes)
    String sBand;
    //Read the CurrentBand from the ESP32
    sBand = Serial3Cmd("cbnd");  //Returns 'cband[1/0][2 digit band]

    if (sBand.length() == 7)  {  //Comms sometimees screws up and adds in some extra characters that mess up the decode.  Don't even have to check for: if (sBand != "")
      //Serial.print(F("    sBand String = ")); Serial.print(sBand); Serial.print(F(" and Length = ")); Serial.println(sBand.length());
      CurrentBand = sBand.substring(5, 7).toInt();
      HamBand = sBand.substring(4, 5).toInt();
      //Serial.print(F("  HamBand = '")); Serial.print(HamBand); Serial.print(F("' and CurrentBand = ")); Serial.println(CurrentBand);
      //If it fails, run the "band" command to read and decode the CurrentBand.
      if (CurrentBand == 255) {
        sBand = Serial3Cmd("band");
        if (sBand != "") {
          CurrentBand = sBand.substring(5, 7).toInt();
          HamBand = sBand.substring(4, 5).toInt();
          //Serial.println(F(" Re-read band successfully ??"));
        }
      }
      if (LastBand == CurrentBand) {
        //No Change, HamBand updates every Loop.
        return false;
      }
      else {
        //Band Changed:
        LastBand = CurrentBand;
        return true;
      }
    }
    else {
      //What to do if the sBand.length !== 7???
      Serial.print(F(" ICOM sBand !==7.  Reply is: ")); Serial.println(sBand);
      HamBand = LastHamBand;
      return false;
    }
  }
  Serial.println(F("  ReadBand: Why the end?"));
  return false;
}

byte FreqToBand(unsigned int TargetFreq, boolean & HamBand) {
  //Returns Band (as a byte variable) and HamBand so that we can disable the Amp when outside the HamBands (using the Bypass() function.
  //Convert the Frequency to Band

  //For efficiency, run through the Ham Bands First:
  if ((TargetFreq >= 1800) && (TargetFreq < 2000)) {  //Valid 160m
    HamBand = true;
    return  i160m;
  }
  else if ((TargetFreq >= 3500) && (TargetFreq < 4000)) { //Valid 80m
    HamBand = true;
    return  i80m;
  }
  //Currently, 60M antenna is NOT connected, the case below will bypass the 60M enable below, Amp not allowed anyway!
  else if ((TargetFreq >= 5300) && (TargetFreq < 5500)) { //Valid 60m
    HamBand = false;  //Disable Amp on 60M
    return  i60m;
  }
  else if ((TargetFreq >= 7000) && (TargetFreq < 7300))  { //Valid 40m
    HamBand = true;
    return  i40m;
  }
  else if ((TargetFreq >= 10100) && (TargetFreq < 10150)) { //Valid 30m
    HamBand = false;  //Amp NOT allowed on 30m
    return  i30m;  //30m enables the 40m band anyway...
  }
  else if ((TargetFreq >= 14000) && (TargetFreq < 14350)) { //Valid 20m
    HamBand = true;
    return  i20m;
  }
  else if ((TargetFreq >= 18068) && (TargetFreq < 18168)) { //Valid 17m
    HamBand = true;
    return  i17m;
  }
  else if ((TargetFreq >= 21000) && (TargetFreq < 21450)) { //Valid 15m
    HamBand = true;
    return  i15m;
  }
  else if ((TargetFreq >= 24890) && (TargetFreq < 24990)) { //Valid 12m
    HamBand = true;
    return  i12m;
  }
  else if ((TargetFreq >= 28000) && (TargetFreq < 29700)) { //Valid 10m
    HamBand = true;
    return  i10m;
  }
  else if ((TargetFreq >= 50100) && (TargetFreq < 54000)) { //Valid 6m
    HamBand = true;
    return  i6m;
  }

  //Out of Band Frequenies:
  else if (TargetFreq < 1800) {
    HamBand = false;
    return i160m;
  }
  else if ((TargetFreq >= 2000) && (TargetFreq < 3000)) {
    HamBand = false;
    return  i160m;
  }
  else if ((TargetFreq >= 3000) && (TargetFreq < 3500)) {
    HamBand = false;
    return  i80m;
  }
  else if ((TargetFreq >= 4000) && (TargetFreq < 6000)) {
    HamBand = false;
    return  i80m;
  }
  else if ((TargetFreq >= 6000) && (TargetFreq < 7000)) {
    HamBand = false;
    return  i40m;
  }
  else if ((TargetFreq >= 7300) && (TargetFreq < 10100)) {
    HamBand = false;
    return  i40m;
  }
  else if ((TargetFreq >= 10150) && (TargetFreq < 12000)) {
    HamBand = false;
    return  i40m;
  }
  else if ((TargetFreq >= 12000) && (TargetFreq < 14000)) {
    HamBand = false;
    return  i20m;
  }
  else if ((TargetFreq >= 14350) && (TargetFreq < 18068)) {
    HamBand = false;
    return  i20m;
  }
  else if ((TargetFreq >= 18168) && (TargetFreq < 21000)) {
    HamBand = false;
    return  i17m;
  }
  else if ((TargetFreq >= 12450) && (TargetFreq < 24890)) {
    HamBand = false;
    return  i15m;
  }
  else if ((TargetFreq >= 24990) && (TargetFreq < 28000)) {
    HamBand = false;
    return  i12m;
  }
  else if ((TargetFreq >= 29700) && (TargetFreq < 50100)) {
    HamBand = false;
    return  i10m;
  }
  else if (TargetFreq >= 54000)  {
    HamBand = false;
    return  i6m;
  }
  else return 255;
}



boolean UpdateBandPower(int CurrentBand, byte RigPortNumber) {
  //Returns true if it fails.
  //Used from the Buttons functions to update the Power Setting for each individual band.

  byte ErrorCount = 0;
  boolean Response;

  //The byte that each band is represented by, is also the Eeprom address where the power value is stored.
  // i.e: i160m = 0, i80m = 2, i40m = 4, etc...
  int PowerValue = EEPROMReadInt(CurrentBand);

  //Get the Maximum Power for this band:
  int MaxPower = GetMaxPower(CurrentBand);

  //Check for valid value in Eeprom
  if ((PowerValue <= 0) || (PowerValue > MaxPower))  {
    if (CurrentBand <= i6m) {  //Valid Band (Otherwise it would do this for 30 & 60m)
      //No valid value, set to 1 watt
      PowerValue = 1;
      //Write the value to the Eeprom location for this band.
      EEPROMWriteInt(CurrentBand, PowerValue);
    }
  }

  Serial.print(F("      UpdateBandPower Setting Watts to: ")); Serial.println(PowerValue);

  //Set the Power to the Rig using the "PCxxx" command.
  do {
    delay(100);
    Response = SetPower(PowerValue, RigPortNumber);
    //Serial.print(F(" SetPower Response returned: ")); Serial.println(Response);
    if (Response) {
      ErrorCount++;
      Serial.print(F(" PowerValue Error: ")); Serial.println(ErrorCount);
    }
  } while ((Response == true) && (ErrorCount < 5));
  //Failed 5 times!!!
  if (Response) {
    //Command Failed...
    SendMorse("Pwr Err");
    return Response;
  }

  if (RigPortNumber != 3)  //Can't set Tune Power on the IC-705.
  {
    ErrorCount = 0; //Reset ErrorCount

    //Set the Tune Power to 1 WATT using the "MPxxx" command.
    do {
      delay(100);
      Response = SetTunePower(1, RigPortNumber);
      if (Response) {
        //Command Failed...
        ErrorCount++;
        //Serial.print(F(" TuneValue Error: ")); Serial.println(ErrorCount);
      }
    } while ((Response == true) && (ErrorCount < 5));
    //Failed 5 times!!!
    if (Response) {
      //Command Failed...
      SendMorse("Tune Err");
      return Response;
    }
  }

  //  delay(100);
  //  //Recheck the Band Power Setting (Found that Power was at 10W, and should NOT have been, this is a safety Recheck!!!)
  //  if (ReadThePower(RigPortNumber) != PowerValue) {
  //    boolean Response = SetPower(PowerValue, RigPortNumber);
  //    if (Response) {
  //      //Command Failed...
  //      SendMorse("Pwr Err");
  //      return Response;
  //    }
  //  }

  //Check the Eeprom setting, set to Operate or Bypass
  Bypass(EEPROMReadInt(iBypassModeEeprom)); //Set to Operate(1) or Bypass (0) Mode

  return false;
}


void SetupAmpBandOutput(int CurrentBand) {
  //Turn off all Band Outputs (There are only 5, 160m is all others off.)
  digitalWrite(i80mBandPin, OFF);
  digitalWrite(i40mBandPin, OFF);
  digitalWrite(i20mBandPin, OFF);
  digitalWrite(i10mBandPin, OFF);
  digitalWrite(i6mBandPin, OFF);

  //Turn on ONLY the active band:
  switch (CurrentBand) {
    case i160m: /*Do Nothing, All off is 160m */ break;
    case i80m:  digitalWrite(i80mBandPin, ON); break;
    case i40m:  digitalWrite(i40mBandPin, ON); break;
    case i20m:  digitalWrite(i20mBandPin, ON); break;
    case i17m:  digitalWrite(i20mBandPin, ON); break;
    case i15m:  digitalWrite(i10mBandPin, ON); break;
    case i12m:  digitalWrite(i10mBandPin, ON); break;
    case i10m:  digitalWrite(i10mBandPin, ON); break;
    case i6m:   digitalWrite(i6mBandPin, ON); break;
  }
}

String BandString(int CurrentBand) {
  //Return the String Name of the Current Band
  switch (CurrentBand) {
    //Always 5 chars
    case i160m: return "160m "; break;
    case i80m:  return "80m  "; break;
    case i60m:  return "60m  "; break;
    case i40m:  return "40m  "; break;
    case i30m:  return "30m  "; break;
    case i20m:  return "20m  "; break;
    case i17m:  return "17m  "; break;
    case i15m:  return "15m  "; break;
    case i12m:  return "12m  "; break;
    case i10m:  return "10m  "; break;
    case i6m:   return "6m   "; break;
    default:    return "Unkn "; break;
  }
}


int BumpPowerSetting(byte ReadIncDec, int CurrentBand) {
  //  Returns PowerSetting.

  //This changes the Power Setting by 1 Watt, either Up=1 or Down=0.
  // Reads the current setting set into Eeprom using CurrentBand.
  //  Reads and Verifies Power
  //  Writes the new Value to the Radio.
  //  Writes the new Value to Eeprom for the current Band.

  // KX3: 160 thru 20 has 15w max, 17 thru 10 has 12 watt max, 6m is 10 watt max.
  // K3 doesn't have these limitations.
  //  KX3 and K3 both have 1 watt resolution in the PC; command that writes the power to the radio.
  // Amplifier has a 6 db Attenuator
  //Input  Output after Attenuator
  //  15  3.77  160-20m Max
  //  14  3.52
  //  13  3.26
  //  12  3.02  17-10m Max
  //  11  2.77
  //  10  2.52  6m Max
  //   9  2.27
  //   8  2.02
  //   7  1.77
  //   6  1.52
  //   5  1.27
  //   4  1.02
  //   3  0.75
  //   2  0.5
  //   1  0.25

  //For Read, Inc or Dec, get current Power Setting for this band.
  //  Note: Invalid Values (un-initialized) are handled in UpdateBandPower
  int PowerSetting = EEPROMReadInt(CurrentBand);

  //Check Up/Down for Max/Min range or Inc/Dec PowerSetting
  if (ReadIncDec == 2) { //Increment

    //Verify that the new setting is NOT already Maximum value
    if (PowerSetting == GetMaxPower(CurrentBand)) {
      //Error Message
      lcd.setCursor(0, 1);
      lcd.print("Already Max Power");
      delay(1000);
      return PowerSetting;
    }
    else {
      PowerSetting++;  //Increment the value
    }
  }
  else if (ReadIncDec == 3) { //Decrement
    //Down
    //Verify that the new setting is NOT already Minimum value
    if (PowerSetting == 1) {
      //Error Message
      lcd.setCursor(0, 1);
      lcd.print("Already Min Power");
      delay(1000);
      return PowerSetting;
    }
    else {
      PowerSetting--; //Decrement the value
    }
  }

  //For Inc or Dec:
  if (ReadIncDec >= 2) {
    //Update the Eeprom Value
    EEPROMWriteInt(CurrentBand, PowerSetting);
  }

  //Complete, return the PowerSetting:
  return PowerSetting;
}


//Return the Maximum Power for the Current Band:
int GetMaxPower(int CurrentBand) {
  //Define a MAXIMUM Power (considering the 6 db attenuator)
  // NOTE that the Attenuator is bypassed in 6m band, we need to do this to get 4 watts out on 6m.
  //  Values must be in whole numbers.  (8 watts from radio is 2 watts into amplifier input)
  switch (CurrentBand) {
    case i160m: return 12; break; // 3 watts output with 6db attenuator
    case i80m:  return 12; break; // 3 watts output
    case i40m:  return 12; break; // 3 watts output
    case i20m:  return 12; break; // 3 watts output
    case i17m:  return 12; break; // 3 watts output
    case i15m:  return 12; break; // 3 watts output
    case i12m:  return 12; break; // 3 watts output
    case i10m:  return 12; break; // 3 watts output
    case i6m:   return  8; break; // 8 watts output, NO ATTENUATOR!
    default:    return  0; break; //Invalid Band.
  }
}

