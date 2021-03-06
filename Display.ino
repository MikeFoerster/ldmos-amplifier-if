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


void UpdateDisplay(byte Mode, byte CurrentBand, int Fwd, int Ref, float SWR, float Volts, bool Act_Byp, double AmpTemp, byte bHours, byte bMinutes, String ErrorString) {
  String Line1 = "";
  String Line2 = "";
  static byte count;

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

    case ModeError: {
        Line1 = Line1 + "Error Mode ";

        count ++;
        if (count < 10) {
        Line2 = ErrorString;
        }
        else if (count < 20) {
          Line2 = F("Select to Retry ");
        }
        else count = 0; //Reset the count.
        break;
      }

    case ModeReceive: {
        Line1 = Line1 + "Receive";
        //If the Amp is in Bypass, indicate by adding the "Byp" to the end of the string.
        if (Act_Byp == false) {
          Line1 = Line1 + " Byp";
        }
        
        //Line 2, Build the normal Recieve String:  Volts, AmpTemp, TimeRemaining... (char(223) is the Degree symbol)
        Line2 = String(Volts) +  "v  " + String(int(AmpTemp)) + char(223) + " " + String(int(bHours));
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
        Line2 =  String(Volts) + " F" + FZeros + String(Fwd) + " R" + RZeros + String(Ref);
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
