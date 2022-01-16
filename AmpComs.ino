// For the LDMOS Amplifier, Simulate Communications (Uses RadioCommandResponse(String InputCommand, byte PortNumber)) from RemoteHams.com

//KPA500 code emulation.
// Commands:
// FL: Fault Value   ^FLC; Clears the current fault.  response: ^FL; sends: ^FLnn where nn = current fault identifier.  nn=00 = No Faults active.
// BN: Band Selection   ^BNnn; 00=160, 01=80, 02=60, 03=40, 04=30, 05=20, 06-17, 07=15, 08=12, 09=10, 10=6
// SP: Speaker On/Off   ^SPn;   n=0 OFF, n=1 ON
// FC: Fan Control      ^FCn; n=fan minimum speed, range of 0 (off) to 6 (high).
// PJ: Power Adjustment  ^PJnnn;  nnn=power adjustment setting range of 80 to 120  Poewr adjustment value is saved on a per-band basis for current band.  Response is for current band.


String AmpFirmwareVersion() {
  // Asking for the Firmware Version:
  // RVM: Firmware Version  ^RVMnn.nn; (2.2 digits)
  return "RVM0" + sVersion + "0;";
}

String AmpSerialNumber() {
  //Asking for the Serial Number, return "^SN01234"
  // SN: Serial Number      ^SNnnnnn; (5 digits)
  return "^SN01234;";
}

byte AmpPowerControl(boolean OffOn) {
  //Returns the Mode ModePowerTurnedOn or ModeOff
  // ON: Power Status control  ^ON0; = OFF, ^ON1; (no response if Mode is OFF.)
  if (OffOn == true) {
    return ModePowerTurnedOn;
  }
  else {
    return ModeOff;
  }
}

String AmpPowerSWR(int FWD, float SWR) {
  // Asking for the Power and SWR:
  // WS  Power, SWR    ^WSppp sss; (ppp= power output in watts, sss= nn.n or 1.0 to 99.0)
  String Response;

  if (FWD < 10)   Response = "^WS00" + String(FWD);
  else if (FWD < 100)  Response = "^WS0" + String(FWD);
  else if (FWD < 1000)  Response = "^WS" + String(FWD);
  else  Response = "^WS999";  // Return 999, maximum Value allowed in comms.

  int iSWR = int(SWR * 10);
  Response = Response + " " + String(iSWR);
  //Serial.print(F("AmpPowerSWR returned: ")); Serial.println(Response);
  return Response;
}

String AmpVoltsCurrent() {
  // VI: PA Voltage    ^VIvvv iii;  PA voltage 00.0 -99.9  iii=PA current 00.0 to 99.9
  //Firmware doesn't have the current available, leave as 00.0.
  String OutputString = "";
  float Volts = ReadVoltage();
  Volts = roundf(Volts * 10) / 10;
  return "^VI" + String(Volts) + " 00.0";
}

String AmpPaTemp() {
  // TM: PA Temp          ^TMnnn;  nnn PA temp 0 to 150 Deg C  (I'll use Deg F!)
  double TempF = ReadAmpTemp();
  int TempInt = (int) TempF;
  if (TempInt < 10)   return "^TM00" + String(TempInt) + ";";
  if (TempInt < 100)   return "^TM0" + String(TempInt) + ";";
  else  return "^TM0" + String(TempInt) + ";";
}

void AmpOpStby(boolean OpStby) {
  // OS: Operate/Standby  ^OSn;  n=0 for Standby or n=1 for Operate Mode
  //  NOT sure if the logic is correct here!
  Bypass(OpStby);
}
