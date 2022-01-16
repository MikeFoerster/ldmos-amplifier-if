// Update Display Routine
// |______________|
//Receive:
//  160m Receive Byp    Or:  160m 104 Receive  Bypass is Optional
//  50.4V 100^ 2:59        /Display the Voltage, Temp and Timeout Time remaining.
//Transmit:
//  160m TR SWR:1.50  (Replace "XMT" with current Temp?
//  50.02 F1200 R100  Voltage, Forward Power, Reflected Power
// |_______________|
//SWR Error:
// 160m SWR ERROR
// Left Key Clear (This would cycle power to the amp)
//OVER TEMP:
// 160m OVER TEMP
// Wait for Clear  (Must wait for the temp to drop to below error value)


void UpdateDisplay(byte Mode, int CurrentBand, int Fwd, int Ref, float SWR, float Volts, boolean Act_Byp, double AmpTemp, byte bHours, byte bMinutes, byte AntennaSwitch, String ErrorString) {
  String Line1 = "";
  String Line2 = "";
  static byte count;
  //static byte X;
  static byte Y;

  if (Mode >= ModeSwrError) {
    //For Modes other than Off & PowerUp, Get the BandString to start Line1.
    //Get the Current Band:
    Line1 = BandString(CurrentBand);
  }

  switch (Mode) {
    case ModeOff: {
        //Do nothing else:
        break;
      }
    case ModePowerTurnedOn: {
        Line1 = "Pwr Up Detected!";
        Line2 = "Trying Rig Comms";
        break;
      }

    case ModeSwrError: {
        Line1 = Line1 + "SWR ERROR";
        Line2 = "Left Key Clear";
        break;
      }

    case ModeCoolDown: {
        Line1 = "CoolDown Mode";
        //Read the amplifier temp. (It's no longer updated from the Main Loop.)
        double dAmpTemp = ReadAmpTemp();
        Line2 = "Tmp:" + String(int(dAmpTemp)) + " Fan:";
        //Use the global FanOutput to calculate the FanPercentage.
        int FanPercent =  map(FanOutput, 0, 255, 0, 99);
        //If Fan is running, then Print out the Fan Power Percentage.
        Line2 = Line2 + " " + String(FanPercent) + "%";
        break;
      }

    case ModeError: {
        static boolean Toggle;
        static unsigned long DisplayTime;
        Line1 = Line1 + "Error Mode ";

        count ++;
        if (count < 10) {
          Line2 = ErrorString;
        }
        else if (count < 20) {
          if (Toggle) {
            Line2 = F("Left Manual Mode");
            if ((millis() - DisplayTime) > 2000) {
              Toggle = false;
              DisplayTime = millis();
            }
          }
          else {
            Line2 = F("Select to Retry ");
            delay(500);
            if ((millis() - DisplayTime) > 2000) {
              Toggle = true;
              DisplayTime = millis();
            }
          }
        }
        else count = 0; //Reset the count.
        break;
      }

    case ModeReceive: {
        //Normal Temp, use "Receive ", OverTemp use "AmpTemp ".
        if (AmpTemp < 108) Line1 = Line1 + "Receive ";
        else Line1 = Line1 + "AmpTemp ";

        //If the Amp is in Bypass, indicate by adding the "Byp" to the end of the string.
        if (Act_Byp == false) {
          Line1 = Line1 + " Byp";
        }
        else {
          //Use the global FanOutput to calculate the FanPercentage.
          int FanPercent =  map(FanOutput, 0, 255, 0, 99);
          //If Fan is running, then Print out the Fan Power Percentage to the Receive Top lne.
          if (FanPercent > 0) {
            if (FanPercent < 10) Line1 = Line1 + " " + String(FanPercent) + "%";
            else Line1 = Line1 + String(FanPercent) + "%";
          }
        }

        //Line 2, Build the normal Recieve String:  Volts, AmpTemp, TimeRemaining... (char(223) is the Degree symbol)
        if (AmpTemp < 100) {
          Line2 = String(Volts) +  "v  " + String(int(AmpTemp)) + char(223) + " " + String(int(bHours));
        }
        else {
          //Keep the Minutes digits from dropping off when temp > 100:
          Line2 = String(Volts) +  "v " + String(int(AmpTemp)) + char(223) + " " + String(int(bHours));
        }                         // ^ removed a space
        if (bMinutes < 10) {
          Line2 += ":0" + String(int(bMinutes));
        }
        else {
          Line2 += ":" + String(int(bMinutes));
        }
        break;
      }

    case ModeManual: {
        //Normal Temp, use "Receive ", OverTemp use "AmpTemp ".
        if (AmpTemp < 108) Line1 = Line1 + "Receive ";
        else Line1 = Line1 + "AmpTemp ";

        //If the Amp is in Bypass, indicate by adding the "Byp" to the end of the string.
        if (Act_Byp == false) {
          Line1 = Line1 + " Byp";
        }
        else {
          //Use the global FanOutput to calculate the FanPercentage.
          int FanPercent =  map(FanOutput, 0, 255, 0, 99);
          //If Fan is running, then Print out the Fan Power Percentage to the Receive Top lne.
          if (FanPercent > 0) {
            if (FanPercent < 10) Line1 = Line1 + " " + String(FanPercent) + "%";
            else Line1 = Line1 + String(FanPercent) + "%";
          }
        }
        //Line 2, Build the normal Recieve String:  Volts, AmpTemp, TimeRemaining... (char(223) is the Degree symbol)
        if (AmpTemp < 100) {
          Line2 = String(Volts) +  "v  " + String(int(AmpTemp)) + char(223) + " " + String(int(bHours));
        }
        else {
          //Keep the Minutes digits from dropping off when temp > 100:
          Line2 = String(Volts) +  "v " + String(int(AmpTemp)) + char(223) + " " + String(int(bHours));
        }                         // ^ removed a space
        if (bMinutes < 10) {
          Line2 += ":0" + String(int(bMinutes));
        }
        else {
          Line2 += ":" + String(int(bMinutes));
        }
        break;
      }

    case ModeTransmit: {
        String FZeros = "";
        if (Fwd < 10) FZeros = "   ";
        else if (Fwd < 100) FZeros = "  ";
        else if (Fwd < 1000) FZeros = " ";

        String RZeros = "";
        if (Ref < 10) RZeros = "  ";
        else if (Ref < 100) RZeros = " ";

        Line1 = Line1 + "TR SWR: " + String(SWR);
        if (AmpTemp < 110) {
          Line2 =  String(Volts) + " F" + FZeros + String(Fwd) + " R" + RZeros + String(Ref);
        }
        else {
          Y < 10 ? Y += 1 : Y = 0;
          //Alternate between Voltage and "Temp"
          if (Y < 5) {
            Line2 =  String(Volts) + " F" + FZeros + String(Fwd) + " R" + RZeros + String(Ref);
          }
          else {
            Line2 =  "Hi Temp" + FZeros + String(Fwd) + " R" + RZeros + String(Ref);
          }
        }
        break;
      }

    case ModeOverTemp: {
        Line1 = Line1 + "OVER TEMP: " + String(int(AmpTemp)) + char(223);
        Line2 = "Wait for Clear";
        break;
      }

    case ModeSetupBandPower: {
        Line1 = Line1 + "Band Power";
        if (CurrentBand <= i6m) Line2 = String(EEPROMReadInt(CurrentBand)) + " Watts";
        else Line2 = "Invalid Band    ";
        break;
      }

    case ModeSetupTimeout: {
        Line1 = Line1 + "Timeout";
        Line2 = String(EEPROMReadInt(iHoursEeprom)) + " Hours";
        break;
      }

    case ModeSetupBypOper: {
        Line1 = Line1 + "Byp/Op Mode";
        //Not sure if this is correct!
        if ((EEPROMReadInt(iBypassModeEeprom)) == 0)  Line2 = "Chng Mode=Bypass";
        else Line2 = "ChngMode=Operate";
        break;
      }
    case ModeSetupAi2Mode: {
        int AiMode = EEPROMReadInt(iAI2Mode);
        Line1 = Line1 + "Set AI2 Mode";
        Line2 = "Value: " + String(AiMode) + "  Up/Dn ";
        break;
      }
    case ModeSetupManualBand: {
        Line1 = Line1 + "Set Manual Band ";
        Line2 = "Value: " + String(CurrentBand) + "  Up/Dn ";
        break;
      }
    case ModeSetupAntenna: {
        Line1 = Line1 + "Set Antenna ";
        //We displaly the current Antenna Switch Position, 0=Auto, 1=160M, 2=80M, etc...
        Line2 = (ManualAntennaString(AntennaSwitch)) + "  Up/Dn ";
        break;
      }

      //default:
  }  //End of switch(Mode)

  //Print out the display lines:
  // NOTE: Each line print takes ~95ms!
  if (Mode != ModeOff) {
    // lcd.clear();
    lcd.setCursor(0, 0);
    while (Line1.length() < 16) {
      Line1 += " ";
    }
    lcd.print(Line1);

    lcd.setCursor(0, 1);
    while (Line2.length() < 16) {
      Line2 += " ";
    }
    lcd.print(Line2);
  }
}
