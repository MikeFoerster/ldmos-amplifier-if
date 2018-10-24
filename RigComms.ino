/*
    COMMUNICATIONS WITH RIG AND AMP
*/

String RadioCommandResponse(String InputCommand, byte PortNumber) {
  //Send a Command, expecting a Response:
  char character;
  String Response = "";

  //Serial.print(F("1  Port = ")); Serial.println(PortNumber);
  //Serial.print(F("2  Sending = ")); Serial.println(InputCommand);

  //  static unsigned long ExecuteTime;
  //  Serial.print(F("  Rig Cmd: ")); Serial.print(InputCommand);
  //  Serial.print(F("  Time from last Command = ")); Serial.println(millis() - ExecuteTime);
  //  ExecuteTime = millis();

  if (PortNumber == 1) {
    Serial1.print(InputCommand);
    //Serial1.flush();  //wait for the command to go out before trying to read

    delay(25); //changed from 75 to 200 to 50, and to 25.  Manual says data should be ready in 10ms.

    while (Serial1.available())  {
      character = Serial1.read();
      Response.concat(character);
      delay(5);  //changed from 5 to 10
      //Serial.print(Response);
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
    //Serial.print(F("4  RadioCommandResponse = ")); Serial.println(Response);
    return Response;
  }
}

byte GetRigModel(byte RigPortNumber) {

  String Model = RadioCommandResponse("OM;", RigPortNumber);
  //Serial.print(F("  Model Read as: ")); Serial.println(Model);
  if (Model.indexOf("X") > -1) {
    int Xval = Model.indexOf("X");
    //Serial.print(F("X position = ")); Serial.println(Xval);
    //Serial.println(F("Identified Model as K3 with KXV3 Transverter Module"));
    return 1;
  }
  else if (Model.indexOf("B") > -1) {
    //Serial.println(F("Identified Model as KX3 with KXBC3 Battery Module"));
    return 2;
  }
  else {
    //Serial.println(F("Model Not Identified"));
    return 0;
  }
}

bool K3AmpOnOff(byte RigPortNumber, bool OnOff) {
  //Turn the KPA3 K3 Amp Off to assure that we don't overdrive the LDMOS Amplifier:
  //  Return "true" for Failed, or "false" for Passed.
  String Response;
  bool Result;

  if (OnOff == 0) {
    Response = RadioCommandResponse("MN055;MP007;", RigPortNumber);
  }
  else {
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

  //Take the rig back out of the Menu mode:
  Response = RadioCommandResponse("MN255;", RigPortNumber);
  //Return the result:
  return Result;
}

unsigned int ReadTheFrequency(byte RigPortNumber) {
  String Frequency = RadioCommandResponse("FA;", RigPortNumber);
  //Serial.print(F("  RawFrequency Read as: ")); Serial.println(Frequency);
  if (Frequency == "") return 255;
  //Convert the Power to an Integer:
  unsigned int iFreq = Frequency.substring(5, 10).toInt();
  //Serial.print(F("  iFreq Read as: ")); Serial.println(iFreq);
  return iFreq;
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

unsigned int ReadThePower(byte RigPortNumber) {
  String Power = RadioCommandResponse("PC;", RigPortNumber);

  Serial.print(F("  String Power Read as: ")); Serial.println(Power);
  //Convert the Power Reading to an Integer:
  unsigned int iPower = Power.substring(3, 5).toInt();

  return iPower;
}

bool SetTunePower(byte TuneValue, byte RigPortNumber) {
  //Returns true on failure
  //Set Menu Command for Tune Power
  String Menu = RadioCommandResponse("MN058;", RigPortNumber);
  //Serial.print(F("  Menu Command Read as: ")); Serial.println(Menu);
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
  String Power = RadioCommandResponse("Mp;", RigPortNumber);
  //Serial.print(F("  Menu Command Verified as: ")); Serial.println(Power);

  delay(10);
  //Exit the Menu mode:
  RadioCommandResponse("MN255;", RigPortNumber);

  //Get the Power setting from the return string:
  int PwrSetting = Power.substring(2, 4).toInt();
  if (TuneValue != (PwrSetting * 10)) {
    return true;
  }
  else return false;
}

bool SetThePower(byte PowerValue, byte RigPortNumber) {
  //Returns true on failure.
  String CommandResponse;
  //Serial.print(F("  Setting Power to: ")); Serial.println(PowerValue);

  if (PowerValue < 10) {
    RadioCommandResponse("PC00" + String(PowerValue), RigPortNumber);
  }
  else if (PowerValue >= 10) {
    RadioCommandResponse("PC0" + String(PowerValue), RigPortNumber);
  }

  delay(50);
  CommandResponse = RadioCommandResponse("PC;", RigPortNumber);
  if (CommandResponse == "") {
    //I have no idea what happened!!! This used to work, but now, it takes a second try!!!
    delay(50);
    //Serial.println(F("Second Try:"));
    CommandResponse = RadioCommandResponse("PC;", RigPortNumber);
  }
  //Serial.print(F("  CommandResponse : ")); Serial.println(CommandResponse);
  int iPowerResponse = CommandResponse.substring(2, 14).toInt();
  //Serial.print(F("  Power Set To: ")); Serial.println(CommandResponse);
  //Serial.print(F("  Power VALUE Set To: ")); Serial.println(iPowerResponse);
  if (iPowerResponse != PowerValue) {
    //Command Failed, NOW WHAT???)
    //   Retry and Beep???
    return true;
  }
  return false;
}

void RigPowerOff(byte RigPortNumber) {
  //Returns true on failure
  //Set Menu Command for Tune Power
  String Response = RadioCommandResponse("PS0;", RigPortNumber);
  if (Response == "") {
    //Just in case the command fails, Try again...
    delay(100);
    Response = RadioCommandResponse("PC0;", RigPortNumber);
  }
  Serial.print(F("Power Off returned: ")); Serial.println(Response);
  //Close the external Serial Ports...
  Serial1.end();
  Serial2.end();
}


/*
   NOT USING AI2 mode.  Experiment with it later...
     It worked at first, then seemed to quit.  ANY comms with the rig can grab the Freq Change stuff and it's lost.
  //Set AI2 Mode
  bool SetRigAi2Mode(byte RigPortNumber) {
  String RigMode = RadioCommandResponse("AI2;", RigPortNumber);
  //Serial.print(F("  AI2 Read as: ")); Serial.println(RigMode);
  delay(50);
  RigMode = RadioCommandResponse("AI;", RigPortNumber);
  if (RigMode.indexOf("AI2") == -1) {
    Serial.print(F("AI2 Command not Read as 'AI2'"));
    Serial.print(F("  AI2 Read as: ")); Serial.println(RigMode);
    //AI2 Mode not set,
    Serial.println(F("       Set to BYPASS MODE since we can't verify AI2 mode."));
    return true;
  }
  else {
    Serial.println(F(" Set AI2 Mode."));
    return false;
  }
  }

  bool VerifyRigAI2Set(byte RigPortNumber) {
  String RigMode = RadioCommandResponse("AI;", RigPortNumber);
  Serial.print(F("  AI2 Read as: ")); Serial.println(RigMode);
  if (RigMode.indexOf("AI2") == -1) {
    Serial.print(F("AI2 Command not Verified as 'AI2'"));
    Serial.print(F("  AI2 Read as: ")); Serial.println(RigMode);
    //AI2 Mode not set,
    Serial.println(F("       Set to BYPASS MODE since we can't verify AI2 mode."));
    return true;
  }
  else {
    Serial.println(F(" Verified AI2 Mode."));
    return false;
  }
  }

  bool CheckRigAI2Mode(byte RigPortNumber, int &CurrentBand, int &PowerSetting) {
  char character;
  String Response = "";
  String Frequency = "";
  //Serial.println(F("CheckRigAI2Mode..."));
  while (Serial1.available())  {
    character = Serial1.read();
    Response.concat(character);
    delay(5);  //changed from 5 to 10
    Serial.print(Response);
  }

  if (Response.length() > 0) {
    Serial.print(F("  RadioCommandResponse = ")); Serial.println(Response);
    //Serial.print(F("  String Length is = ")); Serial.println(Response.length());
    //We only need the LAST Frequency

    int FrequencyIndex = Response.indexOf("FA");
  Serial.print(F("  Index is = ")); Serial.println(FrequencyIndex);
    if (FrequencyIndex > -1) {
      //Valid Frequency detected in the respone:
      int ColonIndex = Response.indexOf(";", FrequencyIndex);
      Frequency = Response.substring(FrequencyIndex, ColonIndex);
      Serial.print(F("  Frequency Read As = ")); Serial.println(Frequency);
      unsigned int iFreq = Frequency.substring(5, 10).toInt();
      Serial.print(F("  Frequency Int Value Read As = ")); Serial.println(iFreq);
      byte ThisBand = FreqToBand(iFreq);

      Serial.print(F("  Band Value Read As = ")); Serial.println(ThisBand);
      if (ThisBand != CurrentBand) {
        CurrentBand = ThisBand;
        SetupBandOutput(CurrentBand);

        Serial.println(F("Calling UpdateBandPower!"));
        //Set the Power for the Rig to the Power Value stored in Eeprom
        UpdateBandPower(CurrentBand, RigPortNumber);
      }
    }
    return true;
  }
  else return false;
  }
*/
