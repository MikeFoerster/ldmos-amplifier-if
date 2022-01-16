// USED FOR ANTENNA AUTO & MANUAL MODE
boolean SetAutoAntenna(int AntennaBand) {
  //SetAutoAntenna uses CurrentBand Values
  //  To change Antennas for each band, change HERE!!!
  //   Returns false if it passes
  boolean Valid = false;
  String Antenna;
  Clear_Outputs();

  switch (AntennaBand) {
    case i160m: {
        //160 Meters
        Antenna = "160m";
        digitalWrite(b160, ON);
        break;
      }
    case i60m:  //For now, use 80 Meter Antenna for 60 meters. (NOT CONNECTED AT THE TOWER.)
    case i6m: //6 Meters, switch in the 80M antenna, I get pretty good SWRs with that antenna.
    case i80m: {
        //80 Meters
        Antenna = "80m";
        digitalWrite(b80, ON);
        break;
      }
    case i30m:
    case i40m: {
        //40 Meters
        Antenna = "40m";
        digitalWrite(b40, ON);
        break;
      }
    case i20m:
    case i17m:
    case i15m:
    case i12m:
    case i10m: {
        //10 Meters
        Antenna = "Beam";
        digitalWrite(bBeam, ON);
        break;
      }
    default: {
        // if nothing else matches, do the default
        Valid = true;
        break;
      }
  }
   Serial.print(F("  Set Antenna to: ")); Serial.println(Antenna);
  //Return if there is a valid Antanna
  return Valid;
}

byte ManualBandIncDec(boolean Increment) {
  //Arg: Increment, if True, we Increment the value by 1, if False, We Decrement by 1
  //Valid values 0=Auto to 7.
  static int Antenna;

  if (Increment) {
    //Increment the Antenna
    if (Antenna < 7) Antenna += 1;
    else Antenna = 0;
  }
  else {
    //Decrement the antenna
    if (Antenna > 0) Antenna -= 1;
    else Antenna = 7;
  }

  //Change the antenna
  SetManualAntenna(Antenna);
  return Antenna;
}

void SetManualAntenna(int ManualBand) {
  //ManualBand uses = 0 Auto, 1=160, 2=80, 3=40, 4=Beam, 5=6m, 6=VHF
  //Switch in all antenna, used or not...
  //Returns true on failure

  Clear_Outputs();

  switch (ManualBand) {
    case 0: {
        //No antenna to set, let the Auto - CurrentBand take care of setting the band.
        break;
      }
    case 1: {
        //160 Meters
        digitalWrite(b160, ON);
        break;
      }
    case 2: {
        //80 Meters
        digitalWrite(b80, ON);
        break;
      }
    case 3: {
        //60 Meters
        digitalWrite(b60, ON);
        break;
      }
    case 4: {
        //40 Meters
        digitalWrite(b40, ON);
        break;
      }
    case 5: {
        //Beam
        digitalWrite(bBeam, ON);
        break;
      }
    case 6: {
        //VHF Vert
        digitalWrite(b6m, ON);
        break;
      }
    case 7: {
        //VHF
        digitalWrite(bVHF, ON);
        break;
      }
    default: {
        // if nothing else matches, do the default
        break;
      }
  }
}

String ManualAntennaString(int ManualBand) {
  //ManualBand uses = 0 Auto, 1=160, 2=80, 3=40, 4=Beam, 5=6m, 6=VHF
  //60M is currently not supported.

  switch (ManualBand) {
    case 0: {
        return "Auto";
        break;
      }
    case 1: {
        return "160M";
        break;
      }

    case 2: {
        return "80M";
        break;
      }
    case 3: {
        return "60M";
        break;
      }
    case 4: {
        return "40M";
        break;
      }
    case 5: {
        return "Beam";
        break;
      }
    case 6: {
        return "6M";
        break;
      }
    case 7: {
        return "VHF";
        break;
      }
    default: {
        // if nothing else matches, do the default
        return "Invalid";
        break;
      }
  }
}

void Clear_Outputs() {
  digitalWrite(b160, OFF);
  digitalWrite(b80, OFF);
  digitalWrite(b60, OFF);
  digitalWrite(b40, OFF);
  digitalWrite(bBeam, OFF);
  digitalWrite(b6m, OFF);
  digitalWrite(bVHF, OFF);
}



