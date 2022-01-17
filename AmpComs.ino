// For the LDMOS Amplifier, Simulate Communications (Uses RadioCommandResponse(String InputCommand, byte PortNumber)) from RemoteHams.com

//KPA500 code emulation.
// Commands:
// FL: Fault Value   ^FLC; Clears the current fault.  response: ^FL; sends: ^FLnn where nn = current fault identifier.  nn=00 = No Faults active.
// BN: Band Selection   ^BNnn; 00=160, 01=80, 02=60, 03=40, 04=30, 05=20, 06-17, 07=15, 08=12, 09=10, 10=6
// SP: Speaker On/Off   ^SPn;   n=0 OFF, n=1 ON
// FC: Fan Control      ^FCn; n=fan minimum speed, range of 0 (off) to 6 (high).
// PJ: Power Adjustment  ^PJnnn;  nnn=power adjustment setting range of 80 to 120  Poewr adjustment value is saved on a per-band basis for current band.  Response is for current band.

//Missing:
//  ^AL (ALC Threshold, GET/SET)  NOT USED
//   SET/RSP format: ^ALnnn; where nnn is the ALC setting with range of 0 to 210. The ALC value is saved on a per-band basis for the current band. The response indicates the setting for the current band.
//  ^AR (Attenuator Fault Release Time, GET/SET)  NOT USED
//    SET/RSP format: ^ARnnnn; where nnnn is the attenuation timeout value in milliSeconds with range of 1400 to 5000 milliSeconds.
//  ^BC (STBY on Band Change, GET/SET)
//    SET/RSP format: ^BCn; where n = 1 to stay in STBY after band change, or n = 0 to return to previous state prior to band change (STBY or OPER).
//  ^BN (Band Selection, GET/SET)  NOT USED
//   SET/RSP format: ^BNnn; where nn = the band. nn can have the following values:
//    00 = 160m, 01 = 80m, 02 = 60m, 03 = 40m, 04 = 30m, 05 = 20m, 06 = 17m, 07 = 15m, 08 = 12m, 09 = 10m, 10 = 6m  All other values are ignored.
//
//Amp communications never originates from here, but only answers queries from RemoteHams.
//  Using Serial2 for Amplifier
//    Commands emulate the KPA500 comms.

//Act_Byp, Voltage, (Current?), Firmware Version, SN, Mode (AmpPower), Temp, Fwd/Ref/SWR?
boolean KPA500(byte &Mode, boolean &Act_Byp, double AmpTemp, float Volts, byte &CurrentBand, int FwdPower, float Swr) {
  //Returns true (Failed) if:
  //  1. Received string is less than 3 characters
  //  2. Command is received but is not recognized:
  //Returns false (Passed) 
  //  1. NO incoming commands 
  //  2. If the command is recognized and a Return String is successfully passed back.
  String Recieved = "";
  String Return = "";

  if (Serial2.available()) {
    Recieved = Serial2.readString();
  }
  else return false;

  if (Recieved.length() < 3) return true;

  //Get the first 3 characters (^xx) for the command.
  String Command = Recieved.substring(0, 2);


  if (Command == "^SN") {  //Serial Number
    //^SN (Serial Number, GET only)
    //RSP format: ^SNnnnnn; where nnnnn - the KPA500 serial number
    Return = "^SN01011;";
  }

  else if (Command == "^RV") {  // ^RVM, Firmware Release
    //^RVM (Firmware Release identifier, GET only)
    //RSP format: ^RVMnn.nn; where nn.nn is the firmware version
    //const String sVersion = "5.6";
    //Return my Version?  May have to adapt.

    Return = "^RVM01.54;";  //Current version of the KPA500.
  }

  else if (Command == "^ON") {  //Power Status & Off
    //^ON (Power Status & Off, GET/SET)
    //SET format: ^ON0; turns the KPA500 off.
    //RSP format: ^ONn; where n = 1. No response if off.
    //Note: if a ON; quickly follows a ON0; you _may_ see a ON0; response.
    //See the ‘P’ command in the BootLoader section for a serial command to turn on the KPA500.

    //Returns the Mode ModePowerTurnedOn or ModeOff
    // ON: Power Status control  ^ON0; = OFF, ^ON1; (no response if Mode is OFF.)

    //Get:
    if (Command == "^ON;") {
      if (Mode >= ModePowerTurnedOn) {
        Return = "^ON1;";
      }
      else {
        Return = "^ON0;";
      }
    }

    else { //Set:
      if (Command == "^ON1;") {
        Mode = ModePowerTurnedOn;
        Return = "^ON1;";
      }
      else if (Command == "^ON0;") {
        Mode = ModePrepareOff;
        Return = "^ON0;";
      }
    }
  }

  else if (Command == "^WS") {  //Power/SWR
    //^WS (Power/SWR, GET only)
    //RSP format: ^WSppp sss; where ppp = output power in watts and sss = SWR. There is an implied decimal
    //point after the second s. ppp has a range of 0 - 999 watts, and sss has range of 1.0 - 99.0. sss will return as
    //000 when not transmitting.

    if (Mode != ModeTransmit) {
      Return = "^WS0000;";
    }
    else {  //ModeTransmit
      if (FwdPower < 10)   Return = "^WS00" + String(FwdPower);
      else if (FwdPower < 100)  Return = "^WS0" + String(FwdPower);
      else if (FwdPower < 1000)  Return = "^WS" + String(FwdPower);
      else  Return = "^WS999";  // Return 999, maximum Value allowed in comms.

      int iSWR = Swr * 10; //Multiply * 10 to drop the tenths and convert to int.
      if (iSWR < 10)   Return = Return + " 00" + String(iSWR);
      else if (FwdPower < 100)  Return = Return + " 0" + String(FwdPower);
      else if (FwdPower < 1000)  Return = Return + " " + String(FwdPower);
    }
  }

  else if (Command == "^VI") {  //PA Volts/Current
    //^VI (PA Volts/Current, GET only)
    //RSP format: ^VIvvv iii; where vvv = the PA voltage with range 00.0 - 99.9 volts, and iii = PA current with
    //range of 00.0 - 99.9 amps. Note the implied decimal point after the second digit for both values.

    int Current = map(FwdPower, 10, 1200, 0, 320); //Uses 0-300 because the decimal 10ths is impliied.
    Return = "^VI" + String(roundf(Volts * 10)) + " " + String(Current);
  }

  else if (Command == "^TM") {
    //^TM (PA Temperature, GET only)
    //RSP format: ^TMnnn; where nnn = temperature with range of 0 - 150 degrees C
    //Return Value in F, rather than C.
    if (AmpTemp < 10)   Return = "^TM00" + String(AmpTemp) + ";";
    if (AmpTemp < 100)  Return = "^TM0" + String(AmpTemp) + ";";
    else  Return = "^TM" + String(AmpTemp) + ";";
  }

  else if (Command == "^OS") {
    //^OS (OP/STBY, GET/SET)
    //SET/RSP format: ^OSn; where n = 0 for Standby, or n = 1 for Operate mode.
    if (Command == "^OS;")
      Return = "^OS" + String(Act_Byp);
    else if (Command == "^OS1;") {
      Act_Byp = 1;
      Bypass(Act_Byp);
      Return = "^OS1;";
    }
    else if (Command == "^OS0;") {
      Act_Byp = 0;
      Bypass(Act_Byp);
      Return = "^OS0;";
    }
  }

  else if (Command == "^FL") {
    //^FL (Current Fault, GET/CLEAR)
    //SET format: ^FLC; clears the current fault.
    //RSP format: ^FLnn; where nn = current fault identifier. nn = 00 indicates no faults are active
    switch (Mode) {
      case ModePowerTurnedOn:
      case ModeReceive:
      case ModeTransmit:    {
          Return = "^FL00;";
          break;
        }
      case ModeSwrError:
      case ModeSwrErrorReset: {
          Return = "^FL09;";
          break;
        }
      case ModeError: {
          Return = "^FL08;";
          break;
        }
      case ModeOverTemp: {
          Return = "^FL04;";
          break;
        }
    }
  }

  else if (Command == "^BN") {
    //SET/RSP format: ^BNnn; where nn = the band. nn can have the following values:
    //00 = 160m, 01 = 80m, 02 = 60m, 03 = 40m, 04 = 30m, 05 = 20m,
    //06 = 17m, 07 = 15m, 08 = 12m, 09 = 10m, 10 = 6m
    //All other values are ignored.
    switch (CurrentBand) {
      case i160m: {
          Return = "^BN00;";
          break;
        }
      case i80m: {
          Return = "^BN01;";
          break;
        }
      case i60m: {
          Return = "^BN02;";
          break;
        }
      case i40m: {
          Return = "^BN03;";
          break;
        }
      case i30m: {
          Return = "^BN04;";
          break;
        }
      case i20m: {
          Return = "^BN05;";
          break;
        }
      case i17m: {
          Return = "^BN06;";
          break;
        }
      case i15m: {
          Return = "^BN07;";
          break;
        }
      case i12m: {
          Return = "^BN08;";
          break;
        }
      case i10m: {
          Return = "^BN09;";
          break;
        }
      case i6m: {
          Return = "^BN10;";
          break;
        }
      default: {
          Return = "^BN99;";
        }
    }
  }

  else if (Command == "^XI") {
    //^XI (Radio Interface, GET/SET)
    //SET/RSP format: ^XIno; where n = the transceiver type, and o = a radio-dependent value. The values for
    //n are:
    //0 K3 - option byte ignored. Always returns o = 1 in V1.04 and later.
    //1 BCD - option byte ignored.
    //2 Analog (Icom voltage levels) - option byte ignored.
    //3 Elecraft / Kenwood serial I/O. o =1 enables polling of radio for frequency
    Return = "^XI0;";
  }

  else if (Command == "^SP") { //Band, Ignore.
    //^SP (Fault Speaker On/Off, GET/SET)
    //SET/RSP format: ^SPn; where n = 0 for speaker off, or n = 1 for speaker on.
    Return = "^SP1;";
  }
  else {
    //Unrecognized Command
    Serial.print(F(" AmpComs command NOT recognized: ")); Serial.println(Command);
    return true;
  }

  //Send the Return back for the command
  
  //Send out the Return string:
  for (unsigned int i = 0; i < Return.length(); i++)  {
    Serial2.write(Return[i]);   // Push each char 1 by 1 on each loop pass
  }
  Serial.print(F(" AmpComs command: ")); Serial.print(Command); Serial.print(F("  Return = ")); Serial.println(Return);
  
  //Command passed, return false.
  return false;
}
