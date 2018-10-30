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

     NEED to Verify All Specifications.
*/

#define  VERSION_ID  F("LDMOS 8-2018 2.0" )

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
const int iFanCount = 38;

//Relay Name Constants
const int OFF = 1;
const int ON = 0;

//Reserve Pins 0 and 1 For Comms
// Reserver Pin 2 for Interrupt
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

//byte Mode;
// 0 is used to know we just went through Setup(), loop() resets to "ModeOff" on startup.
const byte ModeOff = 1;
const byte ModePowerTurnedOn = 2;  //Power was just turned on, need to initialize.
const byte ModeSwrError = 3;     //Bypass mode until corrected, Power Cycle to Reset.
const byte ModeSwrErrorReset = 4;  //User pressed button to reset Power from ModeSwrError.
const byte ModeReceive = 5;        //Amp is ready to go, but not currently transmitting.
const byte ModeTransmit = 6;  //Transmit Detected, Update FWD/REF SWR as well as voltage.
const byte ModeOverTemp = 7;  //OverTemp is active, need to wait for Amp to Cool Down. Dictated by Amp Control board.
const byte ModeSetupBandPower = 8; //We are in Setup Mode, Adjust Per-Band Power setting (to Eeprom)
const byte ModeSetupPwrTimeout = 9; //We are in Setup Mode, adjust the Power Off Timeout.
const byte ModeSetupBypOperMode = 10;  //Select if on Power Up if we start in Bypass or Amplify mode.

//Changes the speed of the Morse Code Error Indications.
int tempo = 75;

// SendMorse is used for Error indications
//Declared here to allow for an optional "bool Repeat = default" argument (in the MorseCode file).
void SendMorse(String CharsToSend, bool Repeat = 0);

/*
    ############### Setup Procedure ############################
*/
void setup() {
  // Monitor Comm Port Setup
  Serial.begin(115200);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //Write the Output values to keep them from toggling during Initialization.
  digitalWrite(iFanPwmPin, OFF);
  digitalWrite(PowerSwitchPin, OFF);
  digitalWrite(ActBypSwitchPin, OFF);
  digitalWrite(iBeeperPin, ON);

  // Initialize the digital pin as an output.
  pinMode(iFanPwmPin, OUTPUT);
  pinMode(PowerSwitchPin, OUTPUT);
  pinMode(ActBypSwitchPin, OUTPUT);
  pinMode(iBeeperPin, OUTPUT);

  pinMode(XmtLedPin, INPUT_PULLUP);
  pinMode(OverTempLedPin, INPUT_PULLUP);
  pinMode(SwrFailLedPin, INPUT_PULLUP);

  //Keep the output pins from toggling during Initialization
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

  //Don't need to declare the Analog Input pins.

  //Display spash on startup.
  Serial.println(VERSION_ID);
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print(" W0IH LDMOS Amp ");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_ID);
  delay(1500);

  //Display the FanCounter
  int FanCounter = EEPROMReadInt(iFanCount);
  lcd.setCursor(0, 1);
  lcd.print("Fan Count " + String(FanCounter) + "     ");
  delay(1000);  //Leave it up 2 seconds.

  lcd.setBacklight(0);  //Turn it off.

  //Use the OffRoutine to disable everything.
  byte tmpMode = 0; //Var required for OffRoutine
  OffRoutine(tmpMode);

  //I had trouble initialzing the time, if it's greater than 9, put it back to 3 hours.
  if (EEPROMReadInt(iHoursEeprom) > 9) {
    EEPROMWriteInt(iHoursEeprom, 3);
  }
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
  bool PowerSwitch;
  static byte RigPortNumber;
  static unsigned long TemperatureReadTime;
  static double InternalTemp;
  static double AmpTemp;
  static int CurrentBand;
  static byte bHours;
  static byte bMinutes;
  static unsigned long ulTimeout;
  static unsigned long ulCommTime;
  static unsigned long ulCommWait;
  static byte RigModel;  //0 = None, 1 = K3, 2 = KX3
  static bool FirstTransmit;
  static byte Mode;

  //Used for Startup Only:
  if (Mode == 0) Mode = ModeOff;

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
    TimeUpdate(bHours, bMinutes, ulTimeout, Mode, RigPortNumber);
  }

  //No matter what the mode is, Read Internal Temp every 10 seconds
  //  and turn on the fan if necessary:
  if ((millis() - TemperatureReadTime) > 10000) {
    //Reads internal temp and turns on the fan if necessary:
    InternalTemp = ReadInternalTemp(); //Serial.print(F("Internal Temp: ")); Serial.println(InternalTemp);

    //Reset for the next 10 seconds:
    TemperatureReadTime = millis();
  }

  //#ifdef DEBUG
  //  PrintTheMode(Mode);
  //#endif

  //Check the buttons:
  HandleButtons(Mode, RigPortNumber, CurrentBand, bHours, bMinutes, ulTimeout, Act_Byp);

  //Run a few checks...
  if (Mode >= ModeReceive ) {  //Running
    //Status Checks
    //  OverTemp will change Mode to ModeOverTemp
    //  SwrFail will change Mode to ModeSwrError
    //  TransmitIndication will change Mode to ModeTransmit or ModeReceive
    StatusChecks(OverTemp, SwrFail, TransmitIndication, Volts, Mode);

    AmpTemp = ReadAmpTemp();  //Read the amplifier temp.

    //Check the Band every few seconds...
    // NOTE: We reset the ulCommTime during transmit so we wait the full time AFTER transmit before checking!
    if ((millis() - ulCommTime) > 2000) {
      //Check and update the Band.
      //if (CheckRigAI2Mode(RigPortNumber, CurrentBand, PowerSetting))

      if (Mode != ModeSetupBandPower) { //Don't Update for ModeSetupBandPower, it can change bands using the Left/Right buttons. 
        if (CheckBandUpdate(CurrentBand, RigPortNumber)) {
          //Band change detected

          //Read the data in Eeprom, should we start in Bypass-(0) or Operate-(1) mode?
          Act_Byp = bool(EEPROMReadInt(iBypassModeEeprom));
          Bypass(Act_Byp);
        }
      }
      //Reset for the next cycle update: (Note: This is ALSO set during Transmit cycle)
      ulCommTime = millis();
    }
  }


  //Update as required depending on the Mode
  switch (Mode) {
    case ModeOff: {
        SubOff(RigModel, RigPortNumber, bHours, bMinutes, Act_Byp, CurrentBand);
        break;
      }
    case ModePowerTurnedOn: {
        SubPowerTurnedOn(CurrentBand, RigPortNumber, RigModel, Mode, bHours, bMinutes, ulTimeout, Act_Byp);
        Volts = ReadVoltage();  //Update so that we don't get a false beep the first time through the Display loop.
        break;
      }
    case ModeSwrError: {
        SubSwrError(SwrFailMessage, Act_Byp);
        break;
      }
    case ModeSwrErrorReset: {
        SubSwrErrorReset(CurrentBand, Mode, Act_Byp, RigPortNumber, RigModel);
        break;
      }
    case ModeReceive: {
        SubReceive(CurrentBand, Act_Byp, FirstTransmit);
        break;
      }
    case ModeTransmit: {
        SubTransmit(FwdPower, RefPower, FirstTransmit, Swr, ulCommTime);
        break;
      }
    case ModeOverTemp: {
        //OverTemp was detected in the top of the Loop.
        //SubOverTemp(OverTemp, Mode);
        //  Only thing to do is to wait for Overtemp to clear.  It will clear when the Heat Sink cools down.
        if (OverTemp == 1) Mode = ModeReceive;
        break;
      }
    case ModeSetupBandPower: {
        //Note: PowerSetting var is filled in from Buttons function, and passed back to here and into Display function.
        //No Functionality, do not call.
        //SubSetupBandPower()
        break;
      }
    case ModeSetupPwrTimeout: {
        //User is in Setup Mode: Changing the Power Off Timeout value (up to 10 Hours)
        break;
      }
    case ModeSetupBypOperMode: {
        //Select if we will Power Up in Bypass or Operate Mode (stored in Eeprom)
        break;
      }
      //default:

  }  //End of switch(Mode)

  //UpdateDisplay takes ~200ms
  UpdateDisplay(Mode, CurrentBand, FwdPower, RefPower, Swr, Volts, Act_Byp, AmpTemp, bHours, bMinutes);

}  //End of Main Loop()

