/*
   This program is intended to run on an Arduino Mega using an Adafruit display with 5 button.
     (Later on an Arduino Mega with a graphical display).
   Requirements are:
     1. Power Down mode which blanks the display of the Arduino, wait for Select Button to activate.
     2  Power Up which:
        A: Initialize the Arduino Display
        B: Attempt to read the Frequency from the Radio (K3 or KX3) through Comms.
           If no Communication, keep trying, display an error message.
        C: If the frequency can be read from the radio, then proceed with normal operation.
     3. Normal Operation
        A.  Turn on power to the amplifier.  This is done by a 5v signal from the Arduino PS to the SSR in the Power Supply.
        B.  Loop on the Comms to the Rig until Frequency is read.
        C.  Determine the band from the Frequency and Activate the correct band to the Amplifier, but leave the Amplifier in the Bypass mode (?).
        D.  Set the radio output power to the appropriate power level for the band.
        E.  Set the radio tune output power to the appropriate power level for the band (TBD).
        F.  Up Button to change from Bypass to Amplify mode
              or have a setting to automatically change from Bypass to Amplify.
     4. Monitoring Sequence:
        A. Continue to read Frequency from the radio to determine if the band has changed (Every 2 to 3 seconds?).
            If Band changes, change the amplifier band to match, and update the Rig Power Output to match the Band, as set in Eeprom.
        B. Display "Transmit" when transmit is detected.
        C. Monitor the Fault line to determine if there has been an SWR Fault.  Requires Power Cycle to clear.
            Add function to cycle power to amp to clear fault (Select button).
            Check for SWR fault vs Band switch fault (Software error???)
        D. Monitor Over Temperature fault.  Fans will run and eventually cool the amp off.  No Power Cycle required.
        E. Display FWD Power (as fast as possible)
        F. Display REF Power (as fast as possible)
        G. Display 50v Voltage (as fast as possible)
     5. System will support a communications output port that will simulate some of the KPA500 functions for remote operation.

    Questions:
      1. Should I add the capabilty to automatically change the antenna for the KX3, similar to the K3?

    Button Operation:
     Select
       1. Press and hold for 3 Seconds > Power Down
       2. Press and release:
            Switch Setup Modes:
              A: Band Output Power Mode.  Display "Output Pwr- (Band String)"
                   For Current Band, Show Current Power (from Eeprom).
                    Up button increases 1 watt (Up to Max Power - Hard Coded Per band)
                    Down button decreases by 1 Watt (Down to 0)
             B: Power Up Timeout (1 to 10 Hours)
             C: Power Up Bypass or Operate Mode

     Down Button:
       Receive Mode: Shortcut to set "Bypass Mode"
         Note: If already "Bypeass Mode", then short beep.
       Band Output Power Mode: Decrease Output Power by 1 Watt (down to 0)
     Up
       Receive Mode: Shortcut to set "Amplify Mode" (Clear Bypass)
         Note: If already "Amplify Mode", then short beep.
        Band Output Power Mode: Increase Output Power by 1 Watt (Up to Max Power - Hard Coded Per band)

     Left
     Right
*/

#define  VERSION_ID  F("LDMOS 8-2018 1.8" )

#define DEBUG
#include <EEPROM.h>
#include <Wire.h>
//#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
//#define WHITE 0x7



// Set a value for the different Bands, NOTE that this is ALSO the Eeprom read/write value for each band to set the Power Value.
const int i160m = 0;
const int i80m = 2;
const int i40m = 4;
const int i20m = 6;
const int i17m = 8;
const int i15m = 10;
const int i12m = 12;
const int i10m = 14;
const int i6m = 16;
const int i60m = 30;
const int i30m = 32;

const int iHoursEeprom = 34; //Eeprom storage for the Timeout time Hours.
const int iBypassModeEeprom = 36;

//Relay Name Constants
const int OFF = 1;
const int ON = 0;

//Reserve Pins 0 and 1 For Comms
// Reserver Pint 2 for Interrupt
const int iFanPwmPin = 3;
const int PowerSwitchPin = 4;  //Output
const int ActBypSwitchPin = 5; //Output
const int XmtLedPin = 6;  //Input
const int SwrFailLedPin = 7;  //Input
const int OverTempLedPin = 2;  //Input (changed from pin 8 to 2) Pin 13 doesn't work well as an Input either!!

//Declare the Band Pin Numbers for the Output
const int i80mBandPin = 9;
const int i40mBandPin = 10;
const int i20mBandPin = 11;
const int i10mBandPin = 12;
const int i6mBandPin = 8; //Was 13, but 13 oscillates on startup & download.
const int iBeeperPin = 46;

//Declare the Analog Input Pins
const int VoltageInputPin = A0;
const int ForwardInputPin = A1;
const int ReflectedInputPin = A2;
const int TempReadout = A3;
const int TempInternal = A4;


byte Mode;
const byte ModeOff = 0;
const byte ModePowerTurnedOn = 1;  //Power was just turned on, need to initialize.
const byte ModeSwrError = 2;     //Bypass mode until corrected
const byte ModeSwrErrorReset = 3;  //User pressed button to reset Power.
const byte ModeReceive = 4;        //Amp is ready to go, but not currently transmitting. Update Voltage.
const byte ModeTransmit = 5;  //Transmit Detected, Update FWD/REF SWR as well as voltage.
const byte ModeOverTemp = 6;  //OverTemp is active, need to wait for Amp to Cool Down.
const byte ModeSetupBandPower = 7; //We are in Setup Mode, Adjust Per-Band Power setting (to Eeprom)
const byte ModeSetupPwrTimeout = 8; //We are in Setup Mode, adjust the Power Off Timeout.
const byte ModeSetupBypOperMode = 9;  //Select if on Power Up if we start in Bypass or Amplify mode.

int tempo = 60;  //Changes the speed of the Morse Code Error Indications.

//Declared here to allow for an optional "bool Repeat = default" argument (in the MorseCode file).
void SendMorse(String CharsToSend, bool Repeat = 0);

void QuickBeep() {
  //Turn the Beeper On breifly:
  digitalWrite(iBeeperPin, OFF);
  delay(100);
  digitalWrite(iBeeperPin, ON);
}

void LongBeep() {
  //Turn the Beeper On breifly:
  digitalWrite(iBeeperPin, OFF);
  delay(300);
  digitalWrite(iBeeperPin, ON);
}
/*
    ############### Setup Procedure ############################
*/
void setup() {
  // Monitor Comm Port Setup
  Serial.begin(115200);

  // set the data rate for the SoftwareSerial port
  Serial1.begin(38400);

  Serial2.begin(38400);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  digitalWrite(iFanPwmPin, OFF);
  digitalWrite(PowerSwitchPin, OFF);
  digitalWrite(ActBypSwitchPin, OFF);
  digitalWrite(iBeeperPin, ON);

  // initialize the digital pin as an output.
  pinMode(iFanPwmPin, OUTPUT);
  pinMode(PowerSwitchPin, OUTPUT);
  pinMode(ActBypSwitchPin, OUTPUT);
  pinMode(iBeeperPin, OUTPUT);

  pinMode(XmtLedPin, INPUT_PULLUP);
  pinMode(OverTempLedPin, INPUT_PULLUP);
  pinMode(SwrFailLedPin, INPUT_PULLUP);

  //Keep the output pins from toggling;
  digitalWrite(i80mBandPin, OFF);
  digitalWrite(i40mBandPin, OFF);
  digitalWrite(i20mBandPin, OFF);
  digitalWrite(i10mBandPin, OFF);
  digitalWrite(i6mBandPin, OFF);

  pinMode(i80mBandPin, OUTPUT);
  pinMode(i40mBandPin, OUTPUT);
  pinMode(i20mBandPin, OUTPUT);
  pinMode(i10mBandPin, OUTPUT);
  pinMode(i6mBandPin, OUTPUT);
  //Start by setting the band to 160 (all relays off).
  SetupBandOutput(i160m);


  //Don't need to declare the Analog Input pins.

  Serial.println(VERSION_ID);
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print(" W0IH LDMOS Amp ");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_ID);
  delay(2000);
  lcd.setBacklight(0);

  Mode = ModeOff;
  int CurrentBand = i160m;
  Serial.println(F("OffRoutine from Setup"));

  //OffRoutine requires a Mode variable, create one:
  byte tmpMode = 0;

  //Use the OffRoutine to disable everything.
  OffRoutine(CurrentBand, tmpMode);

  //I had trouble initialzing the time, if it's greater than 9, put it back to 3 hours.
  if (EEPROMReadInt(iHoursEeprom) > 9) {
    EEPROMWriteInt(iHoursEeprom, 3);
  }
}


void Bypass(bool State) {
  //Toggle the Bypass mode Off or On;
  if (State == false) digitalWrite(ActBypSwitchPin, OFF);
  else digitalWrite(ActBypSwitchPin, ON);
}

/*
     ############### Main LOOP ############################
*/
void loop() {

  //Declare Vars
  int FwdPower;
  int RefPower;
  float Swr;
  float Volts;
  bool Act_Byp;
  bool TransmitIndication;
  bool OverTemp;
  bool SwrFail;
  bool SwrFailMessage;
  String Error;
  bool PowerSwitch;
  static byte RigPortNumber;
  static unsigned long TemperatureReadTime;
  static double InternalTemp;
  static double AmpTemp;
  static int PowerSetting;
  static int CurrentBand;
  static byte bHours;
  static byte bMinutes;
  static unsigned long ulTimeout;
  static unsigned long ulCommTime;
  static unsigned long ulCommWait;
  static byte RigModel;  //0 = None, 1 = K3, 2 = KX3
  static bool FirstTransmit;

  // Try just a 50 ms sleep...
  delay(50);

  if (Mode == ModeOff) {
    //Longer Delay
    delay(100);
  }
  else if (Mode >= ModeTransmit) {
    // Very Short Delay for FWD, REF, and system reponse
    delay(5);
  }
  else {
    //If Not Off, Update the time.  (When Time Expires, will change to OFF mode.)
    Mode = TimeUpdate(Mode, CurrentBand, bHours, bMinutes, ulTimeout);
    delay(10);
  }

  //No matter what the mode is, Read Internal Temp every 10 seconds
  //  and turn on the fan if necessary:
  if ((millis() - TemperatureReadTime) > 10000) {
    //Reads internal temp and turns on the fan if necessary:
    InternalTemp = ReadInternalTemp(); //Serial.print(F("Internal Temp: ")); Serial.println(InternalTemp);

    //Reset for the next 10 seconds:
    TemperatureReadTime = millis();
  }

#ifdef DEBUG
  PrintTheMode(Mode);
#endif

  //Check the buttons:
  Mode = HandleButtons(Mode, RigPortNumber, CurrentBand, PowerSetting, bHours, bMinutes, ulTimeout, Act_Byp);

  //Run a few checks...
  if (Mode >= ModeReceive ) {  //Running
    //Status Checks
    //  OverTemp will change Mode to ModeOverTemp
    //  SwrFail will change Mode to ModeSwrError
    //  TransmitIndication will change Mode to ModeTransmit or ModeReceive
    StatusChecks(OverTemp, SwrFail, TransmitIndication, Volts);

    AmpTemp = ReadAmpTemp();  //Read the amplifier temp.

    //Check the Band every so many seconds...
    // NOTE: We reset the ulCommTime during transmit so we wait 3 seconds AFTER transmit before checking!
    if ((millis() - ulCommTime) > 2000) {
      //Check and update the Band.
      if (CheckBandUpdate(CurrentBand, RigPortNumber)) {
        //if (CheckRigAI2Mode(RigPortNumber, CurrentBand, PowerSetting))
        //Serial.println(F("           CheckBandUpdate returned true."));

        //Read the data in Eeprom, should we start in Bypass-(0) or Operate-(1) mode?
        Act_Byp = bool(EEPROMReadInt(iBypassModeEeprom));
        Bypass(Act_Byp);
        Serial.println(F("           Set to Active."));
      }
      //Serial.print(F("    BandUpdate Returned: ")); Serial.println(CurrentBand);
      //Reset for the next 10 seconds:
      //  (Note: This is ALSO set during Transmit so that we don't attempt to change bands right after a transmit cycle.)
      ulCommTime = millis();
    }
  }


  switch (Mode) {
    case ModeOff: {
        //System is off.
        //Power to the Amp is Off.
        //Display is blank.
        //Auto-Detect Mode: Monitor for Power Up Switch indication using Voltage.


        //For the K3, Re-enable the KPA3 internal amplifier.
        if (RigModel == 1) {
          if (K3AmpOnOff(RigPortNumber, true)) {
            lcd.setCursor(0, 1);
            lcd.print("K3 Amp NOT ON!  ");  //Display the Error
            SendMorse("No Rig ");
            //This is not a critical fault, but Setting the K3 KPA3 amp to BYPASS assures us that we can't overdrive the LDMOS amp.
            // In this case, it was not Re-Enabled.
          }
        }
        //Clear the RigModel variable:
        RigModel = 0;
        //Turn the time OFF.
        bHours = 0;
        bMinutes = 0;
        Act_Byp = 0;

        //Not sure if I want this...
        //        Volts = ReadVoltage(1);
        //        Serial.print(F("Volts = ")); Serial.println(Volts);
        //        if (Volts > 30) {
        //          //Manual Power Up Detected!
        //          //Change Mode to ModePowerTurnedOn.
        //          Mode = ModePowerTurnedOn;
        //          //Problem is, it may cycle Display On and Off!!!
        //
        //        }
        //        Serial.print(F("Mode was Off, Mode = ")); Serial.println(Mode);  //Did we change modes?

        break;
      }

    case ModePowerTurnedOn: {
        //Power On was just detected on the push buttons.
        //Attempt to establish communications with the radio (either K3 or KX3)
        if (PowerUpRoutine(CurrentBand, RigPortNumber, RigModel)) {
          if (CurrentBand == 255) {
            //FAILED, Didn't establish Comms:
            lcd.setCursor(0, 1);
            lcd.print("No Comm with Rig");
            delay(2000);
            //Turn the Mode back off...
            Mode = ModeOff;
          }
        }
        else {
          //Established Comms with rig OK:

          //For the K3, DISABLE the internal 100w Amp:
          if (RigModel == 1) {
            //DISABLE the internal 100w Amp:
            if (K3AmpOnOff(RigPortNumber, false)) {
              //Failed to Disable the K3 KPA3 Amp
              Act_Byp = 0;
              Bypass(Act_Byp);  //Set to Bypass
              lcd.setCursor(0, 1);
              lcd.print("K3 Amp NOT OFF! ");  //Display the Error
              SendMorse("K3 Amp err ");

              //Display a "Continuing anyway" message for 1 second.
              lcd.setCursor(0, 1);
              lcd.print("Continuing (Byp)");
              delay(1000);
              //This is not a critical fault, but Setting the K3 KPA3 amp to BYPASS assures us that we can't overdrive the LDMOS amp.
            }
          }

          //Turn on the Power to the Amp
          digitalWrite(PowerSwitchPin, ON);
          delay(1000);

          //Check the reading for the Volt Meter
          Volts = ReadVoltage(1);
          if (Volts < 30) {
            //Power Up FAILED
            lcd.setCursor(0, 1);  //Second Line
            lcd.print("Pwr Fail:NoVolts");
            delay(2000);
            //Change back to Off Mode?
            //Mode = ModeOff;
            Mode = ModeReceive;
            //Make sure we are in Bypass mode (though it probably won't matter.
            Act_Byp = false;
            Bypass(Act_Byp);
            Serial.print(F("Failed Power Up Voltage Test")); Serial.println(Volts);
          }
          else {
            //Power Up PASSED:

            //Change the Mode to normal Receive mode.
            Mode = ModeReceive;
            //Serial.print(F("Using PortNumber: ")); Serial.println(RigPortNumber);
            //Initialize the Time:
            bHours = EEPROMReadInt(iHoursEeprom);
            //Serial.print(F("ModePowerTurnedOn Read time as: ")); Serial.print(bHours);
            bMinutes = 0;
            //Calculate the Timeout Time with the current Hours and Minutes:
            CalculateTimeout(bHours, bMinutes, ulTimeout);

            //Setup Bypass/Operate Mode per stored value:
            //Read the data in Eeprom, should we start in Bypass(0) or Operate(1) mode?
            Act_Byp = bool(EEPROMReadInt(iBypassModeEeprom));
            Bypass(Act_Byp);

            if (CurrentBand > i6m) {
              //Invalid Band:
              lcd.setCursor(0, 1);
              lcd.print("Invalid Band    ");
              delay(2000);
            }
            //Turn the Mode back off...
            Mode = ModeReceive;
            //Do I want to try to get the AI2 working???
            //Set Rig to Mode "AI2" to indicate Freq & Power Changes:
            //if (SetRigAi2Mode(RigPortNumber))
          }
        }
        break;
      }

    case ModeSwrError: {
        // SWR Error was detected at the top of the Main Loop.
        //Make sure the Transmit indication is off:
        TransmitIndication = false; //(Not sure if this is necessary!)

        //The only thing to clear this is to cycle Power.
        //  Set the Error Message:
        SwrFailMessage = true;

        //Set to Bypass:
        Act_Byp = 0;
        Bypass(Act_Byp);

        // Wait for Power Reset button command.
        //   Power Down, Wait for Voltage to drop to near Zero (about 5 seconds), then Power up again.

        //Indicate the error.  This should beep pretty much continously, each time through the loop.
        // ERR
        SendMorse("Swr Err ");
        break;
      }

    case ModeSwrErrorReset: {
        //User pressed the Select button
        // Cycle the Power
        Serial.println(F("OffRoutine from Loop-ModeSwrErrorReset"));
        OffRoutine(CurrentBand, Mode);
        Act_Byp = 0;
        Bypass(Act_Byp);  //Make sure we are in Bypass mode.
        delay(7000);  //Wait 7 seconds
        PowerUpRoutine(CurrentBand, RigPortNumber, RigModel);
        //Finally, change the mode to Receive.
        Mode = ModeReceive;  //?? Is there something else that should change the Mode to Receive?????
      }

    case ModeReceive: {
        //Read Internal Temp, even in the Off Mode:
        //        int InternalTemp = ReadInternalTemp();
        //        if (InternalTemp > 120) {
        //          //Alarm???

        //If the band is invalid, Set to Bypass
        if (CurrentBand > i6m) {
          //INVALID Band, set amp to Bypass Mode
          Act_Byp = 0;
          Bypass(Act_Byp);
          //Serial.print(F("  Set to Bypass for Band: ")); Serial.println(CurrentBand);
          //Serial.println(F("           Set to Active."));
        }

        FirstTransmit = true;
        break;
      }

    case ModeTransmit: {
        //Make sure that the Transimit Indication is ON.
        // Update the Display for Fwd, Ref
        ReadPower(FwdPower, RefPower, FirstTransmit);

        //Clear the 'FirstTransmit' variable, will be reset in Receive mode:
        FirstTransmit = false;

        Swr = CalculateSwr(FwdPower, RefPower);
        //double AmpTemp = ReadAmpTemp();  //Read the amplifier temp.
        //Reset the start time so we are SURE that we don't change bands.
        ulCommTime = millis();  //Serial.println(F("Hit ulCommWait in ModeTransmit."));
        break;
      }


    case ModeOverTemp: {
        //OverTemp was detected in the top of the Loop.
        //  Only thing to do is to wait for Overtemp to clear.  It will clear when the Heat Sink cools down.
        if (OverTemp == 1) Mode = ModeReceive;
        break;
      }


    case ModeSetupBandPower: {
        //User is in Setup Mode: Changing the Per Band Power Output.
        // Current Setting stored in Eeprom.  Max Power allowed per band is hard coded.

        //THIS MAY NOT BE NEEDED!!!  RECHECK...
        //This function just READs the current setting and displays ONLY the Error Messages such as "Already Max Power"
        //  on the second line while in the ModeSetupBandPower mode.
        byte Read = 1;
        String ReturnString = BumpPowerSetting(Read, CurrentBand, PowerSetting);
        if (ReturnString != "") {
          lcd.setCursor(0, 1);
          lcd.print(ReturnString);
          delay(1000);
        }
        break;
      }

    case ModeSetupPwrTimeout: {
        //User is in Setup Mode: Changing the Power Off Timeout value (up to 10 Hours)

        //Clear the PowerSetting var:
        PowerSetting = 0;

        break;
      }

    case ModeSetupBypOperMode: {
        //Select if we will Power Up in Bypass or Operate Mode (stored in Eeprom)
        break;
      }
      //default:

  }  //End of switch(Mode)

  //UpdateDisplay takes ~200ms
  UpdateDisplay(Mode, CurrentBand, FwdPower, RefPower, Swr, Volts, PowerSetting, Act_Byp, AmpTemp, bHours, bMinutes);

}  //End of Main Loop()

