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
}

byte GetRigModel(byte RigPortNumber) {

  String Model = RadioCommandResponse("OM;", RigPortNumber);
  if (Model.indexOf("X") > -1) {
    int Xval = Model.indexOf("X");
    return 1;
  }
  else if (Model.indexOf("B") > -1) {
    return 2;
  }
  else {
    return 0;
  }
}

bool K3AmpOnOff(byte RigPortNumber, bool OnOff) {
  //Turn the KPA3 K3 Amp Off to assure that we don't overdrive the LDMOS Amplifier:
  //  Return "true" for Failed, or "false" for Passed.
  String Response;
  bool Result = true;
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
  //Returns Frequency.
  String Frequency = "";

  //5 Retries:
  byte count = 5;
  do {
    Frequency = RadioCommandResponse("FA;", RigPortNumber);
    count -= 1;
    delay(25);
  } while ((Frequency == "") && (count > 0));

  if (Frequency == "") return 255;
  //Convert the Power to an Integer:
  unsigned int iFreq = Frequency.substring(5, 10).toInt();
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

//Currently Not Used...
unsigned int ReadThePower(byte RigPortNumber) {
  String Power = RadioCommandResponse("PC;", RigPortNumber);

  //Convert the Power Reading to an Integer:
  unsigned int iPower = Power.substring(3, 5).toInt();
  return iPower;
}

bool SetTunePower(byte TuneValue, byte RigPortNumber) {
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

bool SetPower(byte PowerValue, byte RigPortNumber) {
  //Returns true on failure.
  String CommandResponse;
  byte count = 5;
  String Command;
  int PowerResponse;

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

void RigPowerOff(byte RigPortNumber) {
  //Turn Off Rig by sending "PS0" command (No response expected)
  RadioCommandResponse("PS0;", RigPortNumber);
  
  //Just in case the command fails, Try again...
  delay(100);
  RadioCommandResponse("PS0;", RigPortNumber);
}
