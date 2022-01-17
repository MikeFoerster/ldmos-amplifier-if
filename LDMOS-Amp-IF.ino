/*  Updated 3-28-21
   This program is intended to run on an Arduino Mega using an Adafruit display with 5 button.
   Requirements are:
     1. Power Down mode which blanks the display of the Arduino, wait for Select Button to activate.
     2  Power Up which:
        A: Initialize the Arduino Display
        B: Attempt to read the Frequency from the Radio (K3 or KX3) through Comms.
           If no Communication, keep trying, display an error message, & beep.
        C: If the frequency can be read from the radio, then proceed with normal operation.
            Ver 4.5, Added Manual Run Mode (For IC-705, before any ICOM CI-V Comms was set up), change bands manually.
     3. Normal Operation
        A.  Turn on power to the amplifier.  This is done by a 5v signal from the Arduino PS to the SSR in the Power Supply.
        B.  Loop on the Comms to the Rig until Frequency is read, trying both Comm1 and Comm2.
        C.  Determine the band from the Frequency and Activate the correct band to the Amplifier, but leave the Amplifier in the Bypass mode (?).
        D.  Disable the internal K3 100w amp, and set the radio output power to the appropriate power level for the band.
        E.  Set the radio tune output power to 1 watt.
        F.  Up Button to change from Bypass to Amplify mode
              or have a setting to automatically change from Bypass to Amplify.
     4. Monitoring Sequence:
        A. Use AI2 Mode to follow any band changes.
            If Band changes, change the amplifier band to match, and update the Rig Power Output to match the Band, as set in Eeprom.
            Also, allow Query mode, read frequency every few seconds to verify Band, or change.
        B. Display "Transmit" when transmit is detected from input from Amp.
        C. Monitor the Fault line to determine if there has been an SWR Fault.  Requires Power Cycle to clear.
            Add function to cycle power to amp to clear fault (Select button).
            Check for SWR fault vs Band switch fault (Software error???)
        D. Monitor Over Temperature fault.  Fans will run and eventually cool the amp off.  No Power Cycle required.
        E. Display FWD Power (as fast as possible), Average power for last X samples.
        F. Display REF Power (as fast as possible)
        G. Display 50v Voltage (as fast as possible), Fault if below voltage
     5. System will support a communications output port that will simulate some of the KPA500 functions for remote operation.
     6. Monitor heat sink temperature and turn fans on proportionately higher as the temp increases, while in Receive mode (Transmit mode is always on full speed via Control Board.)

    Questions:
      1. Should I add the capabilty to automatically change the antenna switch for the KX3, similar to the K3?

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

//3.2: Add command to take K3 out of AI (Auto-Info) mode.
//     Stopped the Serial1-2.end cmd to keep the Arduino comms from messing up the P3
//        (Helps, still need to establish comms & then disable.)
//     Fixed Receive display problem where when Amp Temp > 100, the minutes showing would drop the LSB digit. (Temp > 100, dropped an extra space.)
//     Calculated SWR on the current readings, not the averaged Forward value.

//3.3:  (Moved both power supply and amp to the basement, truely remote.
//   1. Add Fan On indication to the display whenever temp is over 100 Deg F.
//   2. Beep to indicate Transmit Temp > 110
//   3. When changing bands, add some more time, add more retries.
//   4. On 160m the display voltage bobbles more than other bands.
//   5. If Set Power or Tune fails, Beep Continously.
//   6. Make sure that AI2 mode is back to AI0.
//   7. Shortened Delay at Power Off from 10 sec to 5 sec (RigPowerDownRoutine).
//   8. Set Tune Power to 1 Watt (400 watts out of amp?)

//3.7:  (Updated from 3.3, (3.4, 3.5 and 3.6 didn't work when I tried adding PID Control for the Fan when in Receive).
//      Added Fan Speed control to turn the fans on slow at 72F and to full speed by 90F.
//         This requires that Pin 44 of the Arduino MEGA be connected to the base of the Fan Enable transistor on the W6PQL Control board (through a diode & 2K resistor).
//      Added CoolDown mode to allow the Fans to stay on until the amp cools down to about 75 Deg F.
//         Had to add 2 more Modes, ModePrepareOff (decide to turn OFF or Cooldown) and ModeCoolDown
//           This took some fairly extensive re-oganization of the Rig and Amp Power Down routines.
//   2. Adding Fan Output Power to display, 2 chars 0-99 to the Top of the Recieve line.
//   3. Reset Tune Power on K3 to 5 watts.
//   4. Power Down, Reset K3 Power to 28 Watts or KX3 to 10 Watts.

//4.0: Attempt at using AI2 Mode and start on code to emulate KPA500
//  On Startup, Check AI mode, if not Set to AI2.
//  ALLOW AI2 Mode as an Option, use Eeprom
//    Identify Rig Model (Existing)
//    K3: Turn off Amp (Existing)
//    Set Tune Power to 1 Watt
//    Set Band Power (Existing)
//      Instead of "ReadTheFrequency" use "ReadComms" quite frequently (every cyclel?)
//4.1: Added Fan Control
//        Including Adding Power Down Cooling, ModeCoolDown.
//4.1  Added AI2 On/Off Menu Item & Cleanup.
//       Add Mode & Menu to SetupAi2Mode On/Off
//      I saw (once) where the amp could not determine the Band Frequency (AI2 mode), add code to retry appropriately.
//     Update Fan speed only every 10 seconds.
//     Change UpdateFan() to UpdateAmpFan() for clarity.
//     Simplify CheckBandUpdate()
//     Simplify main() case: ModeOff
//     Remove a bunch of Serial.print statements.
//     Added some delay(100); to UpdateBandPower to prevent errors that I was seeing.
//     Added Hysteresis to Amp Fan operation.

//   BUFFER OVERRUN
//     **Found that the AI2 Mode required MORE THAN the stock 64 bytes of Buffer in the Serial Comms.
//         My solution was to Modify the C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\HardwareSerial.h file to increase the size:
//             #define SERIAL_TX_BUFFER_SIZE 64   to #define SERIAL_TX_BUFFER_SIZE 256.
//               IMPORTANT:
//         Without this fix, when the Band is changed, it can sometimes NOT change to the correct setting!!!
//           https://www.galaxysofts.com/new/increasing-length-serial-buffer-arduino/   (MAY BE OLD!!!)
//         WARNING: This article talks about changing the HardwareSerial.CPP file, but I in fact changed the HardwareSerial.h file,
//              NOT the .cpp: “#define SERIAL_BUFFER_SIZE, but the .h: #define SERIAL_TX_BUFFER_SIZE 64 to 256.
//           This HAS been done on the W0IH_MAIN computer!!!!  (I did put it on the Lenovo (RCForb Server) but I haven't verified that it works yet.)
//  See also: https://forum.arduino.cc/t/increasing-arduino-serial-buffer-size/322151/5


//4.3  Just some Cleanup (so far)
//      Removed the Retest, leftover from Buffer Overrun, But I had increased the size of the buffer).
//     Change Amp Fans to slightly higher temperatures, to turn on.  was 72 to 90, changed to 76 to 96 Deg for fans.
//4.4  Adding ICOM IC-705 Code (NOTE: 4.3 has not been debugged yet!)
//     Adding File IcomComs
//4.5  Skipping over the Icom Comms and putting in a Manual Mode to run the IC-705 temporarily.
//       Turns out that USB may be somewhat difficult, after discussion with Glen Popiel.
//     Manual Mode allows operating with a rig that the Arduino can't communicate with.
//       Adds a Setting to allow manually changing the Band, but keeps all the other functionality (Fan support, Remote Display, etc.)
//5.1  Adding in CI_V Code for the IC=705.
//       Read Frequency (in Transceive Mode, similar to AI2 mode for Elecraft).
//       Write the Power to the IC-705.
//       Turn OFF the IC-705
//5.2    Added Antenna Switching (from 4.6)
//5.5  Moved CI-V comms over to ESP32.
//       ESP32 does all the comms, and Answers (through Serial3) for all requests.
//       Comms requests the CurrentBand, not the frequency.
//       MEGA pin 35 (Connected to ESP32 GPIO_Num_35) toggles HIGH when we go into Transmit mode, and ESP32 doesn't update Frequency or Band.
//       ESP32 is put into Deep Sleep when not in use (minimal current)
//         MEGA pin 33 is the WAKE pin for the ESP32
//    Added 'HamBand' flag to decoding Frequency to Band to be compatible with Bypass() function, to allow for SWLing, when HamBand == false, Set Amp Bypass.
//    Changed how the Act_Byp works, took it out of SubReceive and combined with HamBand variable.
//       Removed a lot of stuff from SubRecieve that I don't think was needed. (i.e.: if (CurrentBand == 255)
//    Cleaned up some of the code to combine functions for the different Rigs.
//      Changed CheckBandUpdate() and ReadBand() so the Elecraft stuff worked again.  Moved stuff around a lot.
//  Compatible with IC-705-BT-ESP-1.5
//5.6  Started adding the AmpComs
//     Adding the 'ModeReceiveOnly' mode.  When you turn off the Amp, and press the LEFT button to keep the rig on, that it maintains the Antenna Relay setting.  Top Display Line Only.
//     Setup ESP32 BT to return Version when connecting to IC-705 Changed Version number to a whole number.
//      Compatible with IC-705-BT-ESP-16.
//    Added ID Timer to the Menu.
//    Changed Bypass Mode from a boolean to a byte to allow forcing the Amp to Bypass, yet allowing !HamBand to request Bypass.

//Commands to recognize:
//    AI  (Change of AI mode)
//    FA  (A VFO Frequency)  Read from "FA" to ";"
//           Followed by: unsigned int iFreq = Frequency.substring(5, 10).toInt();
//    IF  Band Change indication of new frequency
//    FC  (Change of Power) Allow a limited range: 10 watts max


//Commands Used in AI2:
// *FA/FB (VFO A/B)         - Care about FA Only.
// FR  Receive VFO select   -  Look into later
// FT  TX VFO select
// IF  General information  *** Used for Band Change!
// IS  IF Shift
// *PO  Power Output Read   ***** - Look into later
// PA  RX preamp on/off
// RA  RX atteunuator on/off
// AN  Antenna selection
// GT  AGC speed and On/Off
// FW  Filter Bandwidth
// FC
// NB  Noise Blanker state
// FW  Filter bandwitch and #

//KPA500 code emulation. (Later!)
// Commands:
// RVM: Firmware Version  ^RVMnn.nn; (2.2 digits)
// SN: Serial Number      ^SNnnnnn; (5 digits)
// ON: Power Status control  ^ON0; = OFF, ^ON1; (no response if Mode is OFF.)
// FL: Fault Value   ^FLC; Clears the current fault.  response: ^FL; sends: ^FLnn where nn = current fault identifier.  nn=00 = No Faults active.
// WS  Power, SWR    ^WSppp sss; (ppp= power output in watts, sss= nn.n or 1.0 to 99.0)
// VI: PA Voltage    ^VIvvv iii;  PA voltage 00.0 -99.9  iii=PA current 00.0 to 99.9
// OS: Operate/Standby  ^OSn;  n=0 for Standby or n=1 for Operate Mode
// TM: PA Temp          ^TMnnn;  nnn PA temp 0 to 150 Deg C  (I'll use Deg F!)
// BN: Band Selection   ^BNnn; 00=160, 01=80, 02=60, 03=40, 04=30, 05=20, 06-17, 07=15, 08=12, 09=10, 10=6
// SP: Speaker On/Off   ^SPn;   n=0 OFF, n=1 ON
// FC: Fan Control      ^FCn; n=fan minimum speed, range of 0 (off) to 6 (high).
// PJ: Power Adjustment  ^PJnnn;  nnn=power adjustment setting range of 80 to 120  Poewr adjustment value is saved on a per-band basis for current band.  Response is for current band.

const String sVersion = "5.6";
#define  VERSION_ID "  LDMOS 1/22 " + sVersion


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

//Temporary ManualMode for IC-705 Operation
boolean gManualMode = 0;

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
//Other Eeprom addreses:
const int iHoursEeprom = 34; //Eeprom storage for the Timeout time Hours.
const int iBypassModeEeprom = 36;
const int iFanCount = 38;
const int iAI2Mode = 40;  // 0= Query Mode, 1 = AI2 Mode, Used as AI2Mode=true/false
const int iManBand = 42;  //Write the Manual Band Setting to Eeprom as well.

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

//Declare the Amplifier Band Pin Numbers for the Output
const int i80mBandPin = 9;
const int i40mBandPin = 10;
const int i20mBandPin = 11;
const int i10mBandPin = 12;
const int i6mBandPin = 8; //Was 13, but 13 oscillates on startup & download.
const int iBeeperPin = 46;
const int iFanOutputPin = 44; //Verfified as a PWM Pin.  Defaults to 490 Hz. Ref: https://www.etechnophiles.com/how-to-change-pwm-frequency-of-arduino-mega/

//  Antenna IO port definition for bands to turn on the external relays.
const int b160 = 53;
const int b80 = 51;
const int b60 = 49;
const int b40 = 47;
const int bBeam = 45;
const int b6m = 43;
const int bVHF = 41;

//Declare the Analog Input Pins
const int VoltageInputPin = A0;
const int ForwardInputPin = A1;
const int ReflectedInputPin = A2;
const int TempReadout = A3;
const int TempInternal = A4;
const int WakePin = 33;
const int TxInhibit = 35;

//byte Mode;
// 0 is used to know we just went through Setup(), loop() resets to "ModeOff" on startup.
const byte ModeOff = 1;
const byte ModePowerTurnedOn = 2;  //Power was just turned on, need to initialize.
const byte ModeSwrError = 3;       //Bypass mode until corrected, Power Cycle to Reset.
const byte ModePrepareOff = 4;      //Get ready to turn the system off (To Cool or Not to Cool)
const byte ModeCoolDown = 5;        //Allow the Amp to stay ON and cool the Heat Sink Down.
const byte ModeSwrErrorReset = 6;  //User pressed button to reset Power from ModeSwrError.
const byte ModeError = 7;          //Error Investigation Mode, Comms Fault, Volts Hi/Lo, etc.
const byte ModeReceiveOnly = 8;    //If the LEFT button is pressed during power down, we maintain Recieve Only with the Antenna Relays still active.  Top Display Line only.
const byte ModeReceive = 9;        //Amp is ready to go, but not currently transmitting.
const byte ModeTransmit = 10;       //Transmit Detected from Amp, Update FWD/REF SWR as well as voltage.
const byte ModeManual = 11;        //Manual Mode for temporarily running the IC-705 with no comms.
const byte ModeOverTemp = 12;       //OverTemp is active, need to wait for Amp to Cool Down. Dictated by Amp Control board.
const byte ModeSetupBandPower = 13; //We are in Setup Mode, Adjust Per-Band Power setting (to Eeprom)
const byte ModeSetupTimeout = 14;  //We are in Setup Mode, adjust the Power Off Timeout.
const byte ModeSetupBypOper = 15;  //Select if on Power Up if we start in Bypass or Amplify mode.                  DON'T KNOW IF I NEED THIS FUNCTION!!  Complicates things and not really needed.
const byte ModeSetupAi2Mode = 16;  //Allow turnig AI2 Mode On and Off (Off is Query mode every 2 seconds)
const byte ModeSetupManualBand = 17;  //Setup the Band when in Manual Mode
const byte ModeSetupAntenna = 18;     //Set Antenna to either AutoMode (AntennaSwitch==0) or cycle through the antenna settings
const byte ModeSetupIdTimer = 19;    //Turn the ID Timer On or Off.

int FanOutput = 0;
const int TurnOffTemp = 78;  //CoolDown Temperature

int gTxFlag;
unsigned long lID_Time = 0;
boolean bID_Timer = false;
byte bID_Min_Remaining;
boolean gbRecieveOnly;

/*
    ############### Setup Procedure ############################
*/
void setup() {
  // Monitor Comm Port Setup Baud Rate
  Serial.begin(115200);

  // milliseconds for Serial.readString (default is 1 sec!)
  Serial2.setTimeout(25);
  Serial3.setTimeout(25);

  //Make sure the ESP32 (DeepSleep) WakePin is off, before initializing it.
  digitalWrite(WakePin, 0);
  pinMode(WakePin, OUTPUT); //Wake on true, then reset to off.
  //Bluetooth TX Inhibit pin:  (Connected to ESP32 GPIO_NUM_33 to Stop all comms during Amp TX.
  digitalWrite(TxInhibit, 0);
  pinMode(TxInhibit, OUTPUT);

  //CLEAR ALL THE Antenna Relay OUTPUTS FIRST so that when we assign them they don't chatter.
  digitalWrite(b160, OFF);
  digitalWrite(b80, OFF);
  digitalWrite(b60, OFF);
  digitalWrite(b40, OFF);
  digitalWrite(bBeam, OFF);
  digitalWrite(b6m, OFF);
  digitalWrite(bVHF, OFF);

  //Setup the Antenna Band Output Pins:
  pinMode(b160, OUTPUT);
  pinMode(b80, OUTPUT);
  pinMode(b60, OUTPUT);
  pinMode(b40, OUTPUT);
  pinMode(bBeam, OUTPUT);
  pinMode(b6m, OUTPUT);
  pinMode(bVHF, OUTPUT);

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
  pinMode(iFanOutputPin, OUTPUT);

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
  //Serial.println(VERSION_ID);
  lcd.display();
  lcd.setBacklight(1);  //ON
  lcd.setCursor(0, 0);
  lcd.print(" W0IH LDMOS Amp ");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_ID);
  delay(1500);
  lcd.print("                ");

  //Display the FanCounter, this is here just to see if I really need the fan!  I don't think that I do.
  int FanCounter = EEPROMReadInt(iFanCount);
  lcd.setCursor(0, 1);
  lcd.print("Fan Count " + String(FanCounter) + "     ");
  delay(1000);  //Leave it up 1 second.

  lcd.setBacklight(0);  //Turn it off.  We will turn it back on when we power up for operation.
  lcd.clear();
  lcd.noDisplay();

  Serial.println(F(" Setup()... "));
  //Run the Amp Off Routine, but don't allow the CoolDown mode.
  byte TmpMode = 0;
  AmpOffRoutine(TmpMode);  //Send Mode 0, doesn't matter, must be a variable though!

  gTxFlag = false;
}


/*
     ############### Main LOOP ############################
*/
void loop() {

  //Declare Vars
  int FwdPower = 0;
  int RefPower = 0;
  float Swr = 0;
  float Volts = 0;
  boolean Act_Byp;
  static byte RigPortNumber;
  static unsigned long TemperatureReadTime;
  double AmpTemp = 0;              //Removed static declaration
  static int CurrentBand;  //CurrentBand Must be an int.
  static byte bHours;
  static byte bMinutes;
  static unsigned long ulTimeout;
  static unsigned long ulCommTime;
  static byte RigModel;  //0 = None, 1 = K3, 2 = KX3, 3 = IC-705
  static boolean FirstTransmit;  //Keep track of first transmit cycle so we clear the Fwd/Ref Power on first Transmit.
  static byte Mode;
  static String ErrorString;
  static boolean AmpGT110;    //Amp Temp GreaterThan 110 Deg F for CW Alarm.
  static boolean AI2Mode;  // false = Query Mode (Original), true = AI2 Mode.
  static boolean Cleared;  //Used to clear the variables in the ModeOff case.
  static byte AntennaSwitch; //AntennaSwitch==0, Auto Mode, or use CurrentBand values for Manual
  static boolean PwrOff;  //Used for SubCoolDown and ModeReceiveOnly to know when to stop calling it.

  //  unsigned long looptime = millis();
  //  Serial.print(F(" LoopTime = ")); Serial.println(millis() - looptime);

  //Used for Startup Only:
  if (Mode == 0) Mode = ModeOff;


  //Read Internal Temp every 20 seconds and turn on the fan if necessary (ALL Modes):
  //  NOTE: This has NEVER come on, other than early testing.  Arduino runs cool.
  if ((millis() - TemperatureReadTime) > 20000) {  //(Runtime 1 ms.)
    ReadInternalTemp();  //Sets internal Fan speed (rarely needed)
    //Reset for the next 20 seconds:
    TemperatureReadTime = millis();
  }

  //Only used for Debug:
  PrintTheMode(Mode, "");

  //Check the buttons:  (Runtime 2-3 ms with no buttons.)
  HandleButtons(Mode, RigPortNumber, CurrentBand, bHours, bMinutes, ulTimeout, Act_Byp, AI2Mode, AntennaSwitch);



  //Run a few checks...  (Runtime 50-70ms for IC-705 in Recieve mode)
  if (Mode >= ModeReceive) {  //Running Amp
    //Status Checks
    //  Transmit: Change Mode to ModeTransmit.
    //  OverTemp: Change Mode to ModeOverTemp, or back to Receive when it clears.
    //  Change Mode to ModeSwrError if SwrFail is set (Press Left Button to Power Cycle to clear)
    //  Volts - Check for over/under voltage
    //  Read the Amp Temperature
    StatusChecks(Volts, AmpTemp, Mode, ErrorString);
  }
  if (Mode >= ModeReceiveOnly) {  //Running either With Amp, or ReceiveOnly.
    //Update Time each cycle, When Time Expires, will change to OFF mode.
    if (TimeUpdate(bHours, bMinutes, ulTimeout, Mode) == true) {
      //Run the RigPowerDownRoutine to as to turn rig off (in Timeout tab).
      //2 Second SELECT button turns the unit OFF.
      lcd.setCursor(0, 0);
      lcd.print("Power OFF       ");
      lcd.setCursor(0, 1);
      lcd.print("Check Cool Down ");
      Mode = ModePrepareOff;
    }

    //Receive Mode, AI2Mode active:
    if ((Mode == ModeReceive) || (Mode == ModeReceiveOnly)) {
      if (AI2Mode) {
        //Check Band every loop in AI2Mode:
        CheckBandUpdate(CurrentBand, RigPortNumber, Mode, AI2Mode, AntennaSwitch, Act_Byp);
        //      unsigned long looptime = millis();
        //      Serial.print(F(" LoopTime = ")); Serial.println(millis() - looptime);
      }
      else  {
        //Receive Mode, AI2Mode IN-Active (Polling):
        //Check Band using Polling Mode, every 5 seconds:
        if ((millis() - ulCommTime) > 5000) {
          CheckBandUpdate(CurrentBand, RigPortNumber, Mode, AI2Mode, AntennaSwitch, Act_Byp);
          //Reset for the next cycle update: (Note: This is ALSO set during Transmit cycle so we don't update during TX)
          ulCommTime = millis();
        }
      }
      //Check the ID Timer:
      if (bID_Timer) {
        Check_ID_Timer();
      }
    }
    //ModeManual, nothing changes...    We don't check the Current Band when ModeManual!
    //Manual Mode (IC-705 Temporary)
  }


  //Update as required depending on the Mode
  switch (Mode) {
    case ModeOff: {
        //Clear the Variables, ONCE!
        if (Cleared == false) {
          //Turn off some of the variables
          //Note: Not added to SubOff just because I didn't want to pass all the variables...
          bHours = 0;
          bMinutes = 0;
          Act_Byp = 0;
          CurrentBand = 255;
          RigPortNumber = 0;
          //Clear the RigModel variable so we no longer write to the K3:
          RigModel = 0;
          ErrorString = "";
          //Turn off the ID Timer so we always start up with it in the OFF mode.
          ToggleIdTimer(false);
          AntennaSwitch = 0; //Reset Antenna to AutoMode.
          digitalWrite(TxInhibit, 0);
          Cleared = true;
          PwrOff = false;
        }
        delay(100);  //ModeOff main delay...
        break;
      }
    case ModePrepareOff: {
        SubPrepareOff(Mode, RigPortNumber, RigModel);
        break;
      }
    case ModeCoolDown: {
        boolean PwrOff;  //Not needed here, the flag is needed for ModeReceiveOnly.
        SubCoolDown(Mode, PwrOff);
        break;
      }
    case ModePowerTurnedOn: {
        Cleared = false;
        SubPowerTurnedOn(AI2Mode, CurrentBand, RigPortNumber, RigModel, Mode, bHours, bMinutes, ulTimeout, Act_Byp, ErrorString);
        Volts = ReadVoltage();  //Update so that we don't get a false beep the first time through the Display loop.
        break;
      }
    case ModeSwrError: {
        SubSwrError(Act_Byp);
        break;
      }
    case ModeSwrErrorReset: {
        SubSwrErrorReset(Mode);
        break;
      }
    case ModeError: {
        SubError();
        //Consider turning off AC Power for Voltage Fail, High or Low.
        break;
      }
    case ModeReceive: {
        SubReceive(FirstTransmit, AmpTemp, RigPortNumber);
        break;
      }
    case ModeReceiveOnly: {
        //We may need to call SubCoolDown...
        if (!PwrOff) SubCoolDown(Mode, PwrOff);
        else {
          float Volts = ReadVoltage();
          if (Volts > 10) {
            //Amp Not Off!!!
            lcd.setCursor(0, 0);
            lcd.print("AMP NOT OFF     ");
            lcd.setCursor(0, 1);
            lcd.print("                ");
            delay(1000);
          }
        }
        break;
      }
    case ModeManual: {
        SubManual(FirstTransmit, AmpTemp);
        break;
      }
    case ModeTransmit: {
        SubTransmit(FwdPower, RefPower, FirstTransmit, Swr, ulCommTime, RigPortNumber);
        break;
      }
    case ModeOverTemp: {
        //OverTemp was detected in the top of the Loop.
        //  Only thing to do is to wait for Overtemp to clear.  It will clear when the Heat Sink cools down.
        //Cleared back to Receive in StatusChecks() routine.
        SendMorse("T"); delay(1000);
        break;
      }
    case ModeSetupBandPower: {
        //Note: PowerSetting var is filled in from Buttons function, and passed back to here and into Display function.
        //  Each individual band power is stored in Eeprom.
        break;
      }
    case ModeSetupTimeout: {
        //User is in Setup Mode: Changing the Power Off Timeout value (up to 9 Hours, stored in Eeprom)
        break;
      }
    case ModeSetupBypOper: {
        //Select if we will Power Up in Bypass or Operate Mode (stored in Eeprom)
        break;
      }
    case ModeSetupManualBand: {
        //No Action
        break;
      }
    case ModeSetupAntenna: {
        //No Action
        break;
      }
      //No Default Used

  }  //End of switch(Mode)


  //UpdateDisplay takes ~200ms (Most is in the Display Update)
  UpdateDisplay(Mode, CurrentBand, FwdPower, RefPower, Swr, Volts, Act_Byp, AmpTemp, bHours, bMinutes, AntennaSwitch, ErrorString);

  //With the Amp & Power Supply Remote, Send out "TEMP" when over 110 Deg F
  if ((AmpTemp > 110) && (AmpGT110 == false)) {
    //2 Second SELECT button turns the unit OFF.
    lcd.setCursor(0, 0);
    lcd.print("Temp Greater 110");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    SendMorse("TEMP");
    delay(3000);
    AmpGT110 = true;  //Set a flag so that we only notify ONCE
  }
  if ((AmpTemp < 100) && (AmpGT110 == true)) {
    AmpGT110 = false;  //Clear the flag.
  }


}  //End of Main Loop()
