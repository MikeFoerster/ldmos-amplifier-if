byte ReadBand(byte RigPortNumber) {
  //Using Frequency:  Returns new band
  byte CurrentBand;

  //Read the Frequency from the Radio and convert to Band (byte)
  unsigned int TargetFreq = ReadTheFrequency(RigPortNumber);  //Through RigComms
  //Serial.print(F("Target Freq: ")); Serial.println(TargetFreq);
  if (TargetFreq > 0) {
    CurrentBand = FreqToBand(TargetFreq);
  }

//TRIED the "BN;" command, but reading the Frequency allows knowing when we are out of the valid Ham Bands
//  //Read the BAND ("BN;") from the Radio and convert to Band (byte)
//  unsigned int Band = ReadTheBand(RigPortNumber);  //Through RigComms
//  //Serial.print(F("Target Freq: ")); Serial.println(TargetFreq);
//  if (Band <= 10) {
//    CurrentBand = BandNumberToBand(Band);
//  }

  else {
    CurrentBand = 255; //Not a valid Frequency!!
    //Serial.println(F("  Not a Valid Ham Band Frequency   "));
    //Don't SendMorse here, we may be just passing through invalid freq...
  }
  return CurrentBand;
}

//int BandNumberToBand(byte BandNumber) {
//  switch (BandNumber) {
//    case 0: { return i160m; break; }
//    case 1: { return i80m; break; }
//    case 2: { return i60m; break; }
//    case 3: { return i40m; break; }
//    case 4: { return i60m; break; }
//    case 5: { return i20m; break; }
//    case 6: { return i17m; break; }
//    case 7: { return i15m; break; }
//    case 8: { return i12m; break; }
//    case 9: { return i10m; break; }
//    case 10: { return i6m; break; }
//    default: { return 255; break;}
//  }
//}

byte FreqToBand(unsigned int TargetFreq) {
  //Returns Band (as a byte variable)
  //Convert the Frequency to Band
  if ((TargetFreq >= 1800) && (TargetFreq <= 2000))        return  i160m;
  else if ((TargetFreq >= 3500) && (TargetFreq <= 4000))   return  i80m;
  else if ((TargetFreq >= 5300) && (TargetFreq <= 5500))   return  i60m;
  else if ((TargetFreq >= 7000) && (TargetFreq <= 7300))   return  i40m;
  else if ((TargetFreq >= 10100) && (TargetFreq <= 10150)) return  i30m;
  else if ((TargetFreq >= 14000) && (TargetFreq <= 14350)) return  i20m;
  else if ((TargetFreq >= 18068) && (TargetFreq <= 18168)) return  i17m;
  else if ((TargetFreq >= 21000) && (TargetFreq <= 21450)) return  i15m;
  else if ((TargetFreq >= 24890) && (TargetFreq <= 24990)) return  i12m;
  else if ((TargetFreq >= 28000) && (TargetFreq <= 29700)) return  i10m;
  else if ((TargetFreq >= 50000) && (TargetFreq <= 54000)) return  i6m;
  else return 255;
}

bool CheckBandUpdate(int &CurrentBand, byte RigPortNumber) {
  //Returns true if the band updates.
  //Usede every so many seconds to see if the band changed.

  //Store the CurrentBand:
  int LastBand = CurrentBand;

  //Check for Band Update:  Takes about 340 ms:
  CurrentBand = ReadBand(RigPortNumber);  //Function just above...

  //If there was no change, we're done here...
  if (CurrentBand == LastBand) return false;

  //There must have been a band change detected (Need to Verify)
  if (CurrentBand <= i30m) {  //Only update for the Valid & Invalid Bands (Not for 255!)
    //Verify the band change:
    delay(100);
    byte CurrentBand2 = ReadBand(RigPortNumber);
    if (CurrentBand == CurrentBand2) {

      //Serial.print(F("CheckBandUpdate Reseting Band to: ")); Serial.println(CurrentBand);
      //Update relays to the new band
      SetupBandOutput(CurrentBand);  //Function just below...

      //Serial.println(F("Calling UpdateBandPower!"));
      //Set the Power for the Rig to the Power Value stored in Eeprom
      UpdateBandPower(CurrentBand, RigPortNumber);
      return true;
    }
  }
  //If we get this far, Band didn't change.
  return false;
}

bool UpdateBandPower(byte CurrentBand, byte RigPortNumber) {
  //Returns true if it fails.
  //Used from the Buttons functions to update the Power Setting for each individual band.
  //The byte that each band is represented by, is also the Eeprom address where the power value is stored.
  // i.e: i160m = 0, i80m = 2, i40m = 4, etc...
  int PowerValue = EEPROMReadInt(CurrentBand);

  //Get the Maximum Power for this band:
  int MaxPower = GetMaxPower(CurrentBand);

  //Serial.print(F("  Changing Band to: ")); Serial.println(CurrentBand);
  //Serial.print(F("  PowerValue: ")); Serial.println(PowerValue);
  //Check for valid value in Eeprom
  if ((PowerValue < 0) || (PowerValue > MaxPower))  {
    if (CurrentBand <= i6m) {  //Valid Band (Otherwise it would do this for 30 & 60m)
      //No valid value, set to 1 watt
      PowerValue = 1;
      //Write the value to the Eeprom location for this band.
      EEPROMWriteInt(CurrentBand, PowerValue);
      //Serial.print(F("  Failed PowerValue: ")); Serial.println(PowerValue);
    }
    return false;
  }

  //Set the Power to the Rig using the "PCxxx" command.
  //Serial.println(F("  Setting PowerValue"));
  if (SetThePower(PowerValue, RigPortNumber)) {
    //Command Failed...
    SendMorse("Pwr Err");
  }
  if (SetTunePower(PowerValue, RigPortNumber)) {
    //SetTunePower Failed
    //Serial.println(F("SetTunePower Failed from Band file."));
    SendMorse("Tune Err");
  }

  //Check the Eeprom setting, set to Operate or Bypass
  if (EEPROMReadInt(iBypassModeEeprom) == 0) Bypass(0); //Set to Bypass Mode
  else Bypass(1);  //Set to Operate Mode

  return false;
}


void SetupBandOutput(byte CurrentBand) {
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
    default: delay(100);
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
    //For Increment, get Maximum Power setting for this band.
    int MaxPower = GetMaxPower(CurrentBand);
    //Verify that the new setting is NOT already Maximum value
    if (PowerSetting == GetMaxPower(CurrentBand)) {
      return "Already Max Power"; //Return Error Message
    }
    else {
      PowerSetting++;  //Increment the value
    }
  }
  else if (ReadIncDec == 3) { //Decrement
    //Down
    //Verify that the new setting is NOT already Minimum value
    if (PowerSetting == 0) {
      return "Already Min Power"; //Return Error Message
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


//Return the Maximum Power for the Current Band (Store in Eeprom???):
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
    case i6m:   return  9; break; // 9 watts output, NO ATTENUATOR!
    default:    return  0; break; //Invalid Band.
  }
}
