/*
    COMMUNICATIONS WITH RIG AND AMP
*/

String RadioCommandResponse(String InputCommand, byte PortNumber) {
  //Send a Command, expecting a Response:
  char character;
  String Response = "";

  if (PortNumber == 1) {
    Serial1.print(InputCommand);
    //Serial1.flush();  //wait for the command to go out before trying to read

    delay(25); //changed from 75 to 200 to 50, and to 25.  Manual says data should be ready in 10ms.

    while (Serial1.available())  {
      character = Serial1.read();
      Response.concat(character);
      delay(5);  //changed from 5 to 10
    }
    return Response;
  }
  else if (PortNumber == 2) {
    Serial2.print(InputCommand);

    delay(25);
    while (Serial2.available())  {
      character = Serial2.read();
      Response.concat(character);
      delay(5);
    }
    return Response;
  }
  return "";
}


String RadioAiResponse(byte PortNumber) {
  //Send a Command, expecting a Response:
  char character;
  String Response = "";

  if (PortNumber == 1) {
    while (Serial1.available())  {
      character = Serial1.read();
      Response.concat(character);
      delay(2);  //changed from 5 to 10
    }
    return Response;
  }
  else if (PortNumber == 2) {
    while (Serial2.available())  {
      character = Serial2.read();
      Response.concat(character);
      delay(2);
    }
    return Response;
  }
  return "";
}

unsigned int ReadFrequencyFromFA(String CommsString) {
  String Frequency = "";
  int Index = 0;
  int CmdIndex = -1;
  unsigned int iFreq = 0;


  //Search through the string for the LAST FA or IF command, use the LAST of either, that was in the string
  do {
    Index = CommsString.indexOf("FA", Index);
    //Serial.print(F(" FA Index = ")); Serial.println(Index);
    if (Index >= 0) {
      CmdIndex = Index;
      Index++;
    }
  } while (Index > -1);

  Index = 0;
  do {
    Index = CommsString.indexOf("IF", Index); \
    //Serial.print(F(" IF Index = ")); Serial.println(Index);
    if (Index >= 0) {
      CmdIndex = Index;
      Index++;
    }
  } while (Index > -1);

  //Serial.print(F("  LAST FA/IF Found at: ")); Serial.println(CmdIndex);


  //Get the Value from the String index FaCmd to EndSemi:
  Frequency = CommsString.substring(CmdIndex, (CmdIndex + 13));
  //Serial.print(F("Frequency Substring: ")); Serial.println(Frequency);
  //Convert the Frequency to an int.
  iFreq = Frequency.substring(5, 10).toInt();
  //Serial.print(F(" Found Both FA and ; for Freq Value: ")); Serial.println(iFreq);
  return iFreq;
}


byte GetRigModel(byte RigPortNumber) {

  if (RigPortNumber == 3) {  //Test for IC-705:
    return ICOM_Rig_Model(); //Returns 3 if comms is established
  }

  String Model = RadioCommandResponse("OM;", RigPortNumber);
  if (Model.indexOf("X") > -1) {
    //int Xval = Model.indexOf("X");
    return 1;
  }
  else if (Model.indexOf("B") > -1) {
    return 2;
  }
  else {
    return 0;
  }
}

boolean K3AmpOnOff(byte RigPortNumber, boolean OnOff) {
  //Turn the KPA3 K3 Amp Off to assure that we don't overdrive the LDMOS Amplifier:
  //  Return "true" for Failed, or "false" for Passed.
  String Response;
  boolean Result = true;
  byte count = 5;

  do {
    if (OnOff == 0) {
      //Set K3 Internal Amp to Bypass:
      Response = RadioCommandResponse("MN055;MP007;", RigPortNumber);
    }
    else {
      //Re-Enable the K3 Internal Amp.
      Response = RadioCommandResponse("MN055;MP008;", RigPortNumber);
    }

    //Allow time for response from the K3 Rig:
    delay(50);

    //Verify:
    //Send the MP; command, expect response "7" for K3 Amp Bypass mode, or "8" for K3 Amp Enabled.
    Response = RadioCommandResponse("MP;", RigPortNumber);
    if (OnOff == 0) {
      if (Response.indexOf("7") == -1) Result = true; //Failed
      else Result = false; //Passed OK
    }
    else {
      if (Response.indexOf("8") == -1) Result = true; //Failed
      else Result = false; //Passed OK
    }

    count -= 1;
    //Retry 5 times:
  } while ((Result == true) && (count > 0));

  //Take the rig back out of the Menu mode:
  Response = RadioCommandResponse("MN255;", RigPortNumber);

  //Return the result:
  return Result;
}

unsigned int ReadTheFrequency(byte RigPortNumber) {
  //Returns Frequency, or 0 if it fails
  String Frequency = "";
  byte count = 5;
  unsigned int iFreq;

  if (RigPortNumber == 3) {
    //Once:
    iFreq = CIV_Read_Frequency();

    //Retries:
    //    do {
    //      iFreq = CIV_Read_Frequency();
    //      count -= 1;
    //      delay(25);
    //    } while ((Frequency == "") && (count > 0));
    //
    //    if (iFreq == 0) return 255;

    //Return the frequency as an int:
    return iFreq;
  }
  else {  //Elecraft K3 or KX3:
    //5 Retries:

    do {
      Frequency = RadioCommandResponse("FA;", RigPortNumber);
      count -= 1;
      delay(25);
    } while ((Frequency == "") && (count > 0));

    if (Frequency == "") return 0;
    //Convert the Frequency to an Integer:
    unsigned int iFreq = Frequency.substring(5, 10).toInt();
    return iFreq;
  }
}

//int ReadTheBand(byte RigPortNumber) {
//  String Band = RadioCommandResponse("BN;", RigPortNumber);
//  //Serial.print(F("  Band Read as: ")); Serial.println(Band);
//  if (Band == "") return 255;
//  //Convert the Band string to an Integer:
//  int BandNumber = Band.substring(2).toInt();
//  //Serial.print(F("  Band Number Read as: ")); Serial.println(BandNumber);
//  return BandNumber;
//}

//Currently Not Used...
unsigned int ReadThePower(byte RigPortNumber) {
  String Power = RadioCommandResponse("PC;", RigPortNumber);

  //Convert the Power Reading to an Integer:
  unsigned int iPower = Power.substring(3, 5).toInt();
  return iPower;
}

boolean SetTunePower(byte TuneValue, byte RigPortNumber) {
  //Returns true on failure
  //Set Menu Command for Tune Power
  int PwrSetting = TuneValue * 10;
  byte count = 5;

  do {
    String Menu = RadioCommandResponse("MN058;", RigPortNumber);
    delay(50);
    //Limit TuneValue to 10 watts.
    if (TuneValue > 10) TuneValue = 10;
    //TuneValue is in the form of: 5 watts = "mp050;", 10 watts = "mp100;"
    TuneValue = TuneValue * 10;

    //TuneValue written must be 3 characters:
    if (TuneValue < 100) {
      String PowerResponse = RadioCommandResponse(("Mp0" + String(TuneValue) + ";"), RigPortNumber);
    }
    else {
      String Power = RadioCommandResponse(("Mp" + String(TuneValue) + ";"), RigPortNumber);
    }
    delay(25);
    String Power = RadioCommandResponse("Mp;", RigPortNumber);

    delay(25);
    //Exit the Menu mode:
    RadioCommandResponse("MN255;", RigPortNumber);

    //Get the Power setting from the return string:
    PwrSetting = Power.substring(2, 5).toInt();
    count -= 1; //Decrement the count.

  } while  ((TuneValue != PwrSetting) && (count > 0));


  if (TuneValue != PwrSetting) {
    return true; //Failed
  }
  else return false;
}

boolean SetPower(byte PowerValue, byte RigPortNumber) {
  //Sets Power 0 to 10 Watts
  //Returns true on failure.
  String CommandResponse;
  byte count = 5;
  String Command;
  int PowerResponse;

  //Serial.println(F(" SetPower ##### "));
  if (RigPortNumber == 3)
  {
    Serial.println(F(" SetPower"));
    if (CIV_SetPower(PowerValue) == false)
    {
      return false;
    }
    else return true;
  }
  else 
  {  //Elecraft:
    if (PowerValue < 10) {
      Command = "PC00" + String(PowerValue) + ";";
    }
    else if (PowerValue >= 10) {
      Command = "PC0" + String(PowerValue) + ";";
    }

    do {
      RadioCommandResponse(Command, RigPortNumber);
      delay (50);

      CommandResponse = RadioCommandResponse("PC;", RigPortNumber);
      PowerResponse = CommandResponse.substring(2, 14).toInt();

      count -= 1;
    } while ((PowerResponse != PowerValue) && (count > 0));

    if (PowerResponse != PowerValue) {
      //Command Failed to set the correct value
      return true;
    }
    return false;
  }
}

boolean SetAiOnOff(byte ZeroOr2, byte RigPortNumber) {
  //Returns true on failure.
  String CommandResponse;
  byte count = 5;
  String Command;
  int CmdResponse;

  //For the IC-705
  if (RigPortNumber == 3) {
    if (ZeroOr2 == 0) CIV_Transceive_On_Off(0);
    else CIV_Transceive_On_Off(1);
    return false;
  }
  else {  //Elecraft:
    if (ZeroOr2 == 2) {
      Command = "ai2;";
    }
    else {
      //Set the command to disable the AI command, set to 0.
      Command = "ai0;";
    }

    do {
      CommandResponse = RadioCommandResponse(Command, RigPortNumber);

      delay (50);

      CommandResponse = RadioCommandResponse("ai;", RigPortNumber);
      CmdResponse = CommandResponse.substring(2, 14).toInt();
      count -= 1;
    } while ((CmdResponse != ZeroOr2) && (count > 0));

    if (CmdResponse != ZeroOr2) {
      //Command Failed to set the correct value
      return true;
    }
  }
  return false;
}

void RigPowerOffCmd(byte RigPortNumber) {

  if (RigPortNumber == 3) {
    //IC-705:
    CIV_Power_Off();
    //Just in case the command fails, Try again...
    delay(100);
    CIV_Power_Off();
  }
  else {
    //Turn Off Rig by sending "PS0" command (No response expected)
    RadioCommandResponse("PS0;", RigPortNumber);

    //Just in case the command fails, Try again...
    delay(100);
    RadioCommandResponse("PS0;", RigPortNumber);
  }
}
