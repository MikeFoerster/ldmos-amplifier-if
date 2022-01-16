
void StatusChecks(float &Volts, double &AmpTemp, byte &Mode, String &ErrorString) {
  //Check for Mode Changes & update TransmitLED and Voltmeter...

  //OverTemp
  boolean OverTemp = digitalRead(OverTempLedPin);
  if ((OverTemp == false) && (Mode != ModeOverTemp)) Mode = ModeOverTemp;  //Have to wait for temp to come down.
  else if ((OverTemp == true) && (Mode == ModeOverTemp)) {
    //Temp has come down.
    if (gManualMode) Mode = ModeManual;
    else Mode = ModeReceive;
  }

  //Test the SWR Fail input
  if (!(digitalRead(SwrFailLedPin))) {
    Mode = ModeSwrError;  //Have to cycle power from Left button press, to recover.
    SendMorse("Swr Err ");
  }

  //Update between Receive and Transmit.
  boolean TransmitIndication = digitalRead(XmtLedPin);
  if ((TransmitIndication == false) && ((Mode == ModeReceive) || (Mode == ModeManual))) {
    Mode = ModeTransmit;
    gTxFlag = true;
  }
  else if ((TransmitIndication == true) && (Mode == ModeTransmit)) {
    if (gManualMode) Mode = ModeManual;
    else  {           Mode = ModeReceive; gTxFlag = false;}
  }

  //Update the reading for the Volt Meter
  Volts = ReadVoltage();
  //Check for Low Voltage (Perhaps one supply failed?)
  if (Volts < 37) {
    //Error
    ErrorString = String(Volts) +  "v  LOW VOLTS!";
    Mode = ModeError;
  }
  else if (Volts > 52) {
    //Error
    ErrorString = String(Volts) +  "v HIGH VOLTS!";
    Mode = ModeError;
  }

  //Read the amplifier temp.
  AmpTemp = ReadAmpTemp();
}

void ReadPower(int &FwdPower, int &RefPower, boolean FirstTransmit, float &Swr) {
  static int Fwd[5];
  static int Ref[5];
  static int Index;

  //First press of the transmit, clear the array (memset didn't seem to work!)
  if (FirstTransmit) {
    for (int i = 0; i < 5; i++) {
      Fwd[i] = 0;
      Ref[i] = 0;
    }
    //Restart the Index variable at 0:
    Index = 0;
  }

  FwdPower = analogRead(ForwardInputPin);
  RefPower = analogRead(ReflectedInputPin);
  //Store the latest Readings into the array:
  Fwd[Index] = FwdPower * 1.5;
  Ref[Index] = RefPower * 0.7;

  //Need to calculate the SWR on the current values, not after the averaging.
  Swr = CalculateSwr(Fwd[Index], Ref[Index]);

  //Increment where we store the NEXT value:
  if (Index < 5) Index += 1;
  else Index = 0;

  //Find the highest of the last 5 Readings:
  FwdPower = Fwd[0];
  RefPower = Ref[0];
  for (int i = 0; i < 5; i++) {
    if (Fwd[i] > FwdPower) FwdPower = Fwd[i];
    if (Ref[i] > RefPower) RefPower = Ref[i];
  }
}

float CalculateSwr(float FwdPower, float RefPower) {
  //We are in Transmit Mode,
  //Note: FwdPower & RefPower are passed in from an Int, cast to a float here.

  //Prevent division by Zero:
  if (RefPower == 0) RefPower = .0001;
  if (FwdPower == 0) FwdPower = 1;
  float SWR = sqrt(RefPower / FwdPower);
  return ((1 + SWR) / (1 - SWR));
}


float ReadVoltage() {
  //Returns Voltage
  static int LastCounts;

  //Take the Voltage Reading:
  int Counts = analogRead(A0);

  //Minimize the Display bobble by only updating if it changes by more than 3 digits:
  if (abs(Counts - LastCounts) > 3) {
    LastCounts = Counts;
    return (float(Counts) / 16.94);
  }
  else {
    return (float(LastCounts) / 16.94);
  }
}

double ReadAmpTemp() {
  int counts = analogRead(TempReadout);
  //Convert Counts to Temperature
  double temp = Thermistor(counts, false);
  //If the temp shows very high, it's disconnected, show 0!
  if (temp > 200.0) temp = 0;
  return temp;
}

double ReadInternalTemp() {
  int counts = analogRead(TempInternal);
  //Convert Counts to Temperature (Need to tweek)
  double temp = Thermistor(counts, true);   //Runs the fancy math on the raw analog value
  //Turn on the Fan if necessary
  SetArduinoFanSpeed(temp);
  return temp;
}


void SetArduinoFanSpeed(double InternalTemp) {
  // analogWrite(Pin, [0 to 255])
  if (InternalTemp >= 105) {
    analogWrite(iFanPwmPin, 255); //Fast
    SendMorse("Fan 1 ");
  }
  else if (InternalTemp >= 100) {
    analogWrite(iFanPwmPin, 200); //Medium
    SendMorse("Fan 2 ");
  }
  else if (InternalTemp >= 95) {
    analogWrite(iFanPwmPin, 150);  //Slow
    SendMorse("Fan 3 ");
    //Does the fan ever come on?  Keep track of it for a while:
    int FanCounter = EEPROMReadInt(iFanCount);
    EEPROMWriteInt(iFanCount, FanCounter + 1);
  }
  //Allow for 5 degrees of hysteresis.
  else if (InternalTemp < 90) {
    analogWrite(iFanPwmPin, 0);  //Off
    //SendMorse("Fan Off ");
  }
}

double Thermistor(int RawADC, boolean Internal) {
  /*
      I DON"T UNDERSTAND WHY THIS WORKS!!!
        The RawADC variable is NOT used, but the ADC variable is NOT declared anywhere!
      Wihout the RawADC variable, this function returns 74 or 75 degrees, and doesn't change.
      The RasADC seems to

  */

  //Serial.print(F("RawADC value = ")); Serial.println(RawADC);

  // From: https://playground.arduino.cc/ComponentLib/Thermistor2
  // Inputs ADC Value from Thermistor and outputs Temperature in Fahrenheit
  //  requires: include <math.h>
  // Utilizes the Steinhart-Hart Thermistor Equation:
  //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
  //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
  long Resistance;
  double Temp;  // Dual-Purpose variable to save space.
  //Resistance=1000.0*((1024.0/RawADC) - 1);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 /ADC -1) * BalanceResistor
  // For a GND-Thermistor-PullUp--Vref circuit it would be
  if (Internal == true) {
    Resistance = 10000 / (1024.0 / RawADC - 1);
    //Serial.print(F("Internal ADC value = ")); Serial.println(ADC);
  }
  else {
    Resistance = 7500 / (2048.0 / RawADC - 1);  //5000=97  6000=90, 7000=83, 7500=80.7 seems to be OK
    //Serial.print(F("External ADC value = ")); Serial.println(ADC);
  }

  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
  //Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
  Temp = 1 / (0.001305 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
  //              ^130 worked close.  Down in value takes temp up.  1.305 seems to hit it!
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius
  Temp = (Temp * 9.0) / 5.0 + 32.0; // Convert to Fahrenheit
  return Temp;  // Return the Temperature
}

void Bypass(boolean State) {
  //Toggle the Bypass mode Off or On;
  if (State == false) digitalWrite(ActBypSwitchPin, OFF);
  else digitalWrite(ActBypSwitchPin, ON);
}

void UpdateAmpFan(double AmpTemp) {
  static boolean LastValue0;
  static unsigned long LastRun;

  //Update the Fan Speed every 10 seconds
  if ((millis() - LastRun) > 10000) {

    //Calculate the Fan Speed, starting at 78 Deg F, and run at 100% at 96 Deg F (was 72 to 90)
    // NOTE: See also TurnOffTemp!
    FanOutput = map(AmpTemp, 76, 96, 50, 255);
    //Limit output between 50 & 255 as the PWM value.
    if (FanOutput < 50) FanOutput = 0;
    if (FanOutput > 255) FanOutput = 255;

    //Prevent the fan from turning on and off, adding in some Hysteresis:  Don't turn ON until 77, but turn OFF at 76.
    if ((LastValue0 == true) && (AmpTemp <= 77)) {
      //Make sure that the FanOutput is Zero.  This will be skipped the first time the Value goes above 77 F.
      FanOutput = 0;
    }
    else if (LastValue0 == false ) {
      //Update the Output to the PID Fan pin:
      analogWrite(iFanOutputPin, FanOutput);
    }

    //If the FanOutput was 0, set the LastValue0 to true.
    if (FanOutput == 0) {
      LastValue0 = true;
    }
    else {
      LastValue0 = false;
    }

    //Reset for the next 20 seconds:
    LastRun = millis();
  }
}
