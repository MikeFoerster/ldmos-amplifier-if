
void StatusChecks(bool &OverTemp, bool &SwrFail, bool &TransmitIndication, float &Volts, double &AmpTemp, byte &Mode, String ErrorString) {
  //Check for Mode Changes & update TransmitLED and Voltmeter...

  //OverTemp
  OverTemp = digitalRead(OverTempLedPin);
  if ((OverTemp == false) && (Mode != ModeOverTemp)) Mode = ModeOverTemp;  //Have to wait for temp to come down.
  else if ((OverTemp == true) && (Mode == ModeOverTemp)) Mode = ModeReceive;  //Temp has come down.

  //Test the SWR Fail input
  if (!(digitalRead(SwrFailLedPin))) {
    //Serial.println(F("SWR Fail!!!"));
    Mode = ModeSwrError;  //Have to cycle power from Left button press, to recover.
  }

  //Update between Receive and Transmit.
  TransmitIndication = digitalRead(XmtLedPin);
  if ((TransmitIndication == false) && (Mode == ModeReceive)) Mode = ModeTransmit;
  else if ((TransmitIndication == true) && (Mode == ModeTransmit)) Mode = ModeReceive;

  //Update the reading for the Volt Meter
  Volts = ReadVoltage();
  //Check for Low Voltage (Perhaps one supply failed?)
  if (Volts < 37) {
    //Error only after the 5th time, otherwise may get false errors on startup.
    ErrorString = String(Volts) +  "v  LOW VOLTS!";
    Mode = ModeError;
  }
  else if (Volts > 52) {
    //Error only after the 5th time, otherwise may get false errors on startup.
    ErrorString = String(Volts) +  "v HIGH VOLTS!";
    Mode = ModeError;
  }

  //Read the amplifier temp.
  AmpTemp = ReadAmpTemp();
}

void ReadPower(int &FwdPower, int &RefPower, bool FirstTransmit) {
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
  //Serial.print(F("Raw Fwd = ")); Serial.print(FwdPower); Serial.print(F("  Raw Ref = ")); Serial.println(RefPower);
  //Store the latest Readings into the array:
  Fwd[Index] = FwdPower * 1.5;
  Ref[Index] = RefPower * 0.7;

  //Increment where we store the NEXT value:
  if (Index < 5) Index += 1;
  else Index = 0;

  //Find the highest Readings:
  FwdPower = Fwd[0];
  RefPower = Ref[0];
  for (int i = 0; i < 5; i++) {
    if (Fwd[i] > FwdPower) FwdPower = Fwd[i];
    if (Ref[i] > RefPower) RefPower = Ref[i];
  }

  //Serial.print(F("Final Fwd = ")); Serial.print(FwdPower); Serial.print(F("  Final Ref = ")); Serial.println(RefPower);
  //Serial.print(F("   Index = ")); Serial.println(Index);
}

float CalculateSwr(float FwdPower, float RefPower) {
  //We are in Transmit Mode,
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

  //Minimize the Display bobble:
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
  //Serial.print(F("Amp Temp Counts = ")); Serial.println(counts);
  //Convert Counts to Temperature (Need to tweek)
  double temp = Thermistor(counts, false);
  //Serial.print(F("Amp Temp = ")); Serial.println(temp);
  //If the temp shows very high, it's disconnected, show 0!
  if (temp > 200.0) temp = 0;
  return temp;
}

double ReadInternalTemp() {
  int counts = analogRead(TempInternal);
  //Serial.print(F("Internal Temp Counts: ")); Serial.println(counts);

  //Convert Counts to Temperature (Need to tweek)
  double temp = Thermistor(counts, true);   //Runs the fancy math on the raw analog value
  //Turn on the Fan if necessary
  SetFanSpeed(temp);
  return temp;
}


void SetFanSpeed(double InternalTemp) {
  // analogWrite(Pin, [0 to 255])
  if (InternalTemp >= 105) {
    analogWrite(iFanPwmPin, 255); //Fast
    Serial.println(F("Set Fan to 255"));
    SendMorse("Fan 1 ");
  }
  else if (InternalTemp >= 100) {
    analogWrite(iFanPwmPin, 200); //Medium
    Serial.println(F("Set Fan to 200"));
    SendMorse("Fan 2 ");
  }
  else if (InternalTemp >= 95) {
    analogWrite(iFanPwmPin, 150);  //Slow
    Serial.println(F("Set Fan to 150"));
    SendMorse("Fan3 ");
    //Does the fan ever come on?  Keep track of it for a while:
    int FanCounter = EEPROMReadInt(iFanCount);
    EEPROMWriteInt(iFanCount, FanCounter + 1);
  }
  //Allow for 5 degrees of hysteresis.
  else if (InternalTemp < 90) {
    analogWrite(iFanPwmPin, 0);  //Off
    //Serial.println(F("Set Fan to 0"));
    //SendMorse("Fan Off ");
  }
}




double Thermistor(int RawADC, bool Internal) {
  // From: https://playground.arduino.cc/ComponentLib/Thermistor2
  // Inputs ADC Value from Thermistor and outputs Temperature in Fahrenheit
  //  requires: include <math.h>
  // Utilizes the Steinhart-Hart Thermistor Equation:
  //    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
  //    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
  long Resistance;
  double Temp;  // Dual-Purpose variable to save space.
  //Resistance=1000.0*((1024.0/RawADC) - 1);  // Assuming a 10k Thermistor.  Calculation is actually: Resistance = (1024 /ADC -1) * BalanceResistor
  // For a GND-Thermistor-PullUp--Varef circuit it would be
  if (Internal == true) {
    Resistance = 10000 / (1024.0 / ADC - 1);
  }
  else {
    Resistance = 7500 / (2048.0 / ADC - 1);  //5000=97  6000=90, 7000=83, 7500=80.7 seems to be OK
  }

  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate it 4 times later. // "Temp" means "Temporary" on this line.
  //Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
  Temp = 1 / (0.001305 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));   // Now it means both "Temporary" and "Temperature"
  //              ^130 worked close.  Down in value takes temp up.  1.305 seems to hit it!
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius
  //Serial.print(Temp); Serial.println(" *C");
  Temp = (Temp * 9.0) / 5.0 + 32.0; // Convert to Fahrenheit
  return Temp;  // Return the Temperature
}

void Bypass(bool State) {
  //Toggle the Bypass mode Off or On;
  if (State == false) digitalWrite(ActBypSwitchPin, OFF);
  else digitalWrite(ActBypSwitchPin, ON);
}

