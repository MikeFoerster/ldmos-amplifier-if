//Calculate the Timeout Time from the Hours and Minutes that are set (All Global Variables)
void CalculateTimeout(byte bHours, byte bMinutes, unsigned long &ulTimeout) {
  //Set the Timeout Time, set ONLY from OnRoutine or if the time is changed or reset.
  ulTimeout = (millis() / 1000) + ((((unsigned long)bHours * 3600) + ((unsigned long)bMinutes * 60)) + 15);
}


//Update the Hours and Minutes (Used each Loop) and call Beeper if less than 10 minutes.
boolean TimeUpdate(byte &bHours, byte &bMinutes, unsigned long &ulTimeout, byte &Mode) {
  //We should NOT get here if Mode == OFFMODE:
  unsigned long Seconds = ulTimeout - (millis() / 1000);

  //Calculate the Hours and Minutes:
  bHours =  Seconds / 3600;  //Ignore the remainder
  bMinutes = (Seconds - (bHours * 3600)) / 60;

  //Less than 10 Minutes, Sound the Beep: (Use Hours < 1 in case bHours goes negative.
  if ((bHours <= 0) && ((bMinutes <= 10) && bMinutes > 0)) {
    TimeoutBeeper(bMinutes, Mode);
  }
    //Check for Timeout: (TimeoutTime is stored in Seconds)
  if (Seconds <= 0) return true;  //Return True value so we shut down the Rig and Amp
  else return false;
}

void TimeoutBeeper(byte bMinutes, byte Mode) {
  //Beep once at 10 minutes, twice at 9 minutes, etc.
  // We ONLY call this if time is < 10 Minutes!!
  if (Mode <= ModeTransmit) { //Don't beep during Transmit or Config Modes
    static int LastBeep;
    int numberOfBeeps = 11 - bMinutes;

    if (LastBeep != bMinutes)  {
      //Cycle through the number of beeps:
      for (int i = 0; i < numberOfBeeps; i++) {
        SendMorse("e");
        delay(100);
      }
      //Store the Last Beep so we don't repeat each cycle:
      LastBeep = bMinutes;
    }
  }
}
