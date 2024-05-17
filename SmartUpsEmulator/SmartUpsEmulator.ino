/*
 * Filename:  SmartUpsEmulator.009
 * Date:      21 Mar 2024
 *            Leave CDC_ENABLED defined
 *            Boot without pressing switch on pin 2 to run as HID device only
 *            Blue LED will light up
 *            Press and hold switch on pin 2 at bootup to run as HID device and enable CDC serial input/output on USB serial port
 *            Release switch when green LED lights up
*/

/*
  In USBDesc.h
    Uncomment SERIAL1_DEBUG to enable Serial1 debugging messages (pins 0 & 1) hardware UART
*/

/*****************************************************************
Design Notes:
    Internally, will treat all systems as a 12v sytem. When set for a 24 or
    48v system, will multiply appropriately for voltage display. User entered
    voltages will be divided down to 12v system values.

Sid's To Do:

## Hysteresis for updating % and Time remaining too.
##

  Done: 
    Config: Cmd to turn en/dis telling PC to shut off while doing calibration,
    en/dis for normal UPS Emulation mode. With PC reporting disabled, also set the capacities
    to 100% so that the PC queries return good capacities and dont put PCs to sleep.
    Print calculated capacities, but do NOT update what gets sent to the PC/NAS
    while in config mode (let us verify calc's w/o PC shutdown).
    Default: Config mode: Disabled, Normal Run mode: Enabled.

    At factory: 1st load EEPROM with compiled values. Then may adjust and save
    A/D Voltage calibration and  tweeked curves to EEPROM with unpublished command ("ZSF")
    User request of Load Factory Settings gets values from this part of EEPROM.

    Voltage to Capacity calculations:
        4 voltage&Capacity points to better match voltage curves
        One default set for each battery type

  Not Yet:

    Determine Charge vs Discharge via voltage history

    Add commands for changing battery parameters.
    
    Config of UPS name/# (value is 2 or 3?) <-- Need this?

    Cmd for printing voltage in Serial Plotter mode? (properly formatted CSV),
    and suppress other printouts.
    
    Perhaps:
        In PC mode (CDC Enabled), allow config cmds over Serial1 also
            (handy for edge connector automation / Factory automated calibration)
        Cmd to switch between internal or external AREF
            (Put a 5k resistor in series with the pin to protect internal source)
            Could instead/also short another pin to ground to indicate external (PCB version).
        
    Open Questions:
        Are Charge and Discharge mutually exclusive, or can it be neither if no charging and no load?
        Should we have a specific voltage: Above this V, assume charging.
        How do we determine if we started charging, or a higher voltage is just
            "bounch up" from the load being disconnected (or server died &no load)

        Battery Questions:
            LFP batteries have an extreemly flat voltage-capacity curve (0.1v/10% capacity).

            Do we already have some good discharge curves by battery type and full-discharge time?

            How constant are the loads (HD active vs no recent read/writes)
            
            Do we need to do an at-site V-Capacity calibration?
                User enters how long until empty. Start recording voltages at full charge,
                record every 10% of discharge time until down to at least 50%,
                extrapolate the rest of the curve and store.
        
        Better ways to determine remaining capacity via voltage?

        Large/Small of battery types
        See table in half pint manual

        Per Paul:
            Generic shutdown voltage if we don't know battery chemistry: 12.3V
            Hopefully only need to asK: Chemestry, Nominal (system) voltage (12/24/48),
                backup time (from full to empty in minutes)

*****************************************************************/

//#include <HIDPowerDevice.h>
#include "HIDPowerDevice.h"
#include "TimerHelpers.h"

#include "HandyHelpers.h"
HandyHelpers MH; // My Handy Helper

#include "ProjectDefs.h"	// For our Smart UPS Emulator project. Defs SERIAL1_DEBUG

//#define MINUPDATEINTERVAL   26
#define MINUPDATEINTERVAL   10


int iIntTimer = 0;
bool SerialIsInitialized = false;

#define SEND_INITIAL_RPT false      // Should we volunteer the UPS/Battery status, vs. waiting for host query. False=Don't volunteer
//#define SEND_UPDATE_RPTS false
//#define SEND_INITIAL_RPT true      // Should we volunteer the UPS/Battery status, vs. waiting for host query. False=Don't volunteer
#define SEND_UPDATE_RPTS true

#define SHOW_ALL_PARAMS true      // Def to enable cmd 'P' to dump all parameters

/*
 For Sid, the project is located at: C:\Users\User\Documents\GitHub\smart-ups-emulator
 arduino-cli compile --warnings more --fqbn arduino:avr:mega SmartUpsEmulator\SmartUpsEmulator.ino
 */

/*
Todo: 
    Board voltage calibration. To EEPROM
    Via commands, Simulate battery voltage for easy PC comm testing
    Enable Watchdog
  Testing:
    See if need % left or just the shutdown cmd: Appeard to be % left.
    What kind of shut down does it do? (Sleep: Open app is open on start up)
    OS's tried so far: Win10 Home, Win10 Pro: Both do Sleep by default.
    Win10 Pro: I changed config and was able to set it to do a full shutdown.
    Appears the Shutdown requested/Immenent message is NOT what tells the PC to shut down, but instead the battery level(?)
    Try RPi Pico (but would need an external EEPROM for production version)
 
  Shutdown config on Win10 Pro: Settings, Power&Sleep, Additional Power Settings (green text in right side of window),
    For the power plan in use: click "Change plan settings", "Change advanced power settings", scroll down to Battery,
    Open "Critical battery action", "On battery:" use pulldown to select desired action.
*/

// Note: if CDC_ENABLED is defined, SERIALPORT is defined as Serial, otherwise as Serial1 (Pins 0/1)
#ifdef CDC_ENABLED
    bool doDebugPrints = true;  // Enable printing by default
    #define DBPRINTLN(args...) if(doDebugPrints) { SERIALPORT_PRINTLN(args);}
    #define DBPRINT(args...)   if(doDebugPrints) { SERIALPORT_PRINT(args);}
    #define DBWRITE(args...)   if(doDebugPrints) { SERIALPORT_WRITE(args);}
#else
    bool doDebugPrints = true;  // Enable printing by default
    #define DBPRINTLN(args...) if(doDebugPrints) { Serial1.println(args);} 
    #define DBPRINT(args...)   if(doDebugPrints) { Serial1.print(args);}   
    #define DBWRITE(args...)   if(doDebugPrints) { Serial1.write(args);}   
#endif

// String constants
const char STRING_DEVICECHEMISTRY[]PROGMEM = "PbAc";
const char STRING_OEMVENDOR[]PROGMEM = "MyCoolUPS";
const char STRING_SERIAL[]PROGMEM = "UPS10";

const byte bDeviceChemistry = IDEVICECHEMISTRY;
const byte bOEMVendor = IOEMVENDOR;

uint16_t iPresentStatus = 0, iPreviousStatus = 0;

bool bForceShutdownPrior = false;

byte bRechargable = 1;
byte bCapacityMode = 2;  // units are in %%

// Physical parameters
const uint16_t iConfigVoltage = 1380;
uint16_t iVoltage = 1300, iPrevVoltage = 0;
uint16_t iRunTimeToEmpty = 7200, iPrevRunTimeToEmpty = 0;
int16_t  iDelayBe4Reboot = -1;
int16_t  iDelayBe4ShutDown = -1;

byte iAudibleAlarmCtrl = 2; // 1 - Disabled, 2 - Enabled, 3 - Muted


// Parameters for ACPI compliancy
const byte iDesignCapacity = 100;
const byte bCapacityGranularity1 = 1;
const byte bCapacityGranularity2 = 1;
byte iFullChargeCapacity = 100;
bool bCharging    = true; 
bool bACPresent   = true; 
bool bDischarging = false; 

byte iRemaining = 100, iPrevRemaining = 0;
byte iRemainingInternal = 100;
uint16_t iRunTimeToEmptyInternal = 7200;
uint16_t iPresentStatusInternal = 0;

int iRes = 0;

// Watchdog DOYET
//#define WATCHDOG_RESET true   // DOYET
#ifndef WATCHDOG_RESET
    #define WATCHDOG_RESET
#endif 

#define PROG_NAME_VERSION "Smart UPS Emulator v0.1"

// I/O Pin Usage
#define PIN_LED_STATUS          13
#define PIN_BATTERY_VOLTAGE     A5
#define PIN_INPUT_PWR_FAIL       4  // Short to ground to indicate loss of AC power / running on battery

#define PIN_USBCDCNEEDED    2  // Used in main.cpp
#define PIN_LED_YELLOW      5  // Used in main.cpp
#define PIN_LED_BLUE        6  // Used in main.cpp
#define PIN_LED_GREEN       7  // Used in main.cpp
#define PIN_LOGIC_TRIG      8  // Trigger logic analyzer when battery info sent to Host

struct CalibPoint
{
    uint16_t a2dValue;
    uint16_t voltage;     // in hundredths of volts
};


struct VtoCapacity
{
    uint16_t voltage;     // in hundredths of volts
    uint16_t capacity;
};

#define NUM_CAPACITY_POINTS 4   // Points on discharge curve for interpolating capacity
#define NUM_SYS_VOLTAGES    4   // 12, 24, 48V, and a spare/special (must be divisible by the lowest V, ie. 36V)
#define NUM_BATT_TYPES      5   // Lead Acid, AGM, Lith Iron Phospate (LFP?)

// For all bat chemistries: Capacity curve, Definetely charging V, Discharging V
struct BatteryParams
{
    VtoCapacity VtoCapacities[NUM_CAPACITY_POINTS];     // Points on discharge curve for interpolating capacity
    uint8_t     numCapacityPointsUsed;                  // number of points in capacity graph (VtoCapacities[])
    uint16_t    batFullVoltage;                         // in hundredths of volts
    uint16_t    batEmptyVoltage;                        // in hundredths of volts
    uint16_t    isChargingVolts;                        // in hundredths of volts: Above this v, must be charging
    uint16_t    isDisChargingVolts;                     // " " ": Below this v, must be discharging
    uint16_t    iCalcdTimeToEmpty;                      // Runtime calculated time to empty from full
    uint8_t     timeToEmptyCalcState;                   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
};

enum BatteryChemistryType 
{
    BC_LeadAcid = 0,
    BC_LI_ION = 1,
    BC_LFP = 2,
    BC_AGM = 3,
    BC_Other = 4,
    BC_Last = BC_Other
};




/////////////
// General EEPROM
//
#define EEPROM_VALID_PAT1  0xAA
#define EEPROM_VALID_PAT2  0x55
#define EEPROM_END_VER_SIG 0xA501
struct EEPROM_Struct
{
    uint8_t     eeValid_1;      // EE is Valid_1: 0xAA
    uint8_t     eeValid_2;      // EE is Valid_2  0x55

    // Physical parameters - User System Specific
    uint16_t              iAvgTimeToFull  ;     // in seconds
    uint16_t              iAvgTimeToEmpty ;     // in seconds
    uint16_t              iRemainTimeLimit;     // in seconds
    BatteryChemistryType  BatChem;              // Lead Acid, LFP, AGM, Other
    uint8_t               battSysVMultiplier;   // 1/2/3/4 for 12, 24, 36, 48 (V must be divisible by 12v)

    // Parameters for ACPI compliancy
    byte iWarnCapacityLimit;    // warning at 10%
    byte reserved8_5;           // Offset the byte below
    byte iRemnCapacityLimit;    // low at 5%

    bool msgPcEnabledCfgMode;   // Host (PC/NAS) to be updated with real battery status in Config Mode
    bool msgPcEnabledRunMode;   // ... in Normal Run mode (CDC Disabled)

    // Board/Resistor Divider Calibration
    CalibPoint  calibPointLow;  // Note A2D reading and associated voltage at two points. "map" from there
    CalibPoint  calibPointHigh;

    // Battery characteristics by chemistry.
    BatteryParams   BattParams[NUM_BATT_TYPES];     // For all bat chemistries: Capacity curve, Definetely charging V, Discharging V

    uint16_t  reserved16_1;
    uint16_t  reserved16_2;
    uint16_t  reserved16_3;
    uint16_t  reserved16_4;
    uint8_t   reserved8_1;
    uint8_t   reserved8_2;
    uint8_t   reserved8_3;
    uint8_t   reserved8_4;


    uint8_t     debugFlags;  // Debug flags 0x01=Skip Early Audio
    uint16_t    eeVersion;  // Change this if the eeprom layout changes
};

EEPROM_Struct   StoreEE;        // User EEPROM & unchanging calibs. May restore from user or factory inmages in EEPROM.

#define EEP_OFFSET_USER     0   // Starting address in EEPROM of normal user saved parameters
#define EEP_OFFSET_FACTORY  256   // Starting address in EEPROM of "Factory Default" parameters

////////////////////
// P R O T O S
////////////////////
void handleLaptopInput(void);
void printHelp(void);
void watchDogReset(void);
void printHelp(Stream *serialPtr = SERIALPORT_Addr, bool handleUsbOnlyOptions = true);
void processUserInput(char userIn[MAX_MENU_CHARS], uint8_t& indexUserIn, int inByte, Stream *serialPtr = SERIALPORT_Addr, bool handleUsbOnlyOptions = true);
void showPersistentSettings(Stream *serialPtr = SERIALPORT_Addr, bool handleUsbOnlyOptions = true);
void showParameters(Stream *serialPtr);

void enableDebugPrints(uint8_t debugPrBits);
#define DBG_PRINT_MAIN      0    // 0x01

enum Status_HID_Comm
{
    stat_Disabled,
    stat_Good,
    stat_Error
};
Status_HID_Comm status_HID = stat_Disabled;

#define BLINKON_DIS    500  // Blink patterns for the status LED
#define BLINKOFF_DIS   500
#define BLINKON_GOOD   900
#define BLINKOFF_GOOD  100
#define BLINKON_ERROR  100
#define BLINKOFF_ERROR 900

//Stream *serPtr = SERIALPORT_Addr;  // DBC.007
Stream *serPtr = SERIALPORT_Addr;
int batVoltage = 1300;
int batVoltageInternal = 1300;

Timer_ms statusLedTimerOn;
Timer_ms statusLedTimerOff;

extern bool USBCDCNeeded;  // DBC.008b
extern long USBSwitchTime[6];  // DBC.008c
extern long USBSwitchCount[6]; // DBC.008d

uint8_t pcSetErrorCount = 0;    // How many times the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT value
uint8_t pcSetValue      = 0;    // The value that the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT to

int memStillFree = 32000;   // Print every time this decreases
byte priorRemnCapacityLimit = 0;    // Will notifiy when StoreEE.iRemnCapacityLimit changes

void setup(void)
{
    char prTxt[] = ".ino setup() started.\n\r";
    if ((sizeof(USBDebug) - strlen(USBDebug)) > (strlen(prTxt)+1))  // Debug printout later
        memcpy(&USBDebug[strlen(USBDebug)], prTxt, strlen(prTxt)+1);

    Serial1.begin(115200);  // Always enable Serial1 in case we enable Serial1 debugging  SLR
    while (!Serial1)
        delay(10);

    // New for GTIS
    EEPROM.get(EEP_OFFSET_USER, StoreEE); // Fetch our structure of non-volitale vars from EEPROM

    if ((StoreEE.eeValid_1 == EEPROM_VALID_PAT1) && (StoreEE.eeValid_2 == EEPROM_VALID_PAT2) && (StoreEE.eeVersion == EEPROM_END_VER_SIG)) // Signature Valid?
    {
        if(!USBCDCNeeded)
            Serial1.println(F("Good: EEPROM is initialized."));
    }
    else
    {
        if(!USBCDCNeeded)
            Serial1.println(F("EEPROM not init'd. Using Compiled Defaults."));
        FactoryCompiledDefault();
        EEPROM.put(EEP_OFFSET_FACTORY, StoreEE);    // Write to "Factory Default" EEPROM 
        EEPROM.put(EEP_OFFSET_USER, StoreEE);       // Write to "User" EEPROM 
        FactoryDefault();
    }
    enableDebugPrints(StoreEE.debugFlags);

    priorRemnCapacityLimit = StoreEE.iRemnCapacityLimit;    // Will notifiy when StoreEE.iRemnCapacityLimit changes

    // Init Watchdog DOYET

    // Serial Startup was here, but will do after we have some USB setup done. Otherwise the USB may get started
    //  and run with needed things uninitialized.
    PowerDevice.begin();

    // Serial No is set in a special way as it forms Arduino port name
    PowerDevice.setSerial(STRING_SERIAL);

//    // Used for debugging purposes.
#ifdef CDC_ENABLED
    if(USBCDCNeeded)
        PowerDevice.setOutput(Serial);      // Don't think this get used, but sounds like it should be for debug prints, maybe. SLR DOYET
#endif

    pinMode(PIN_INPUT_PWR_FAIL, INPUT_PULLUP); // ground this pin to simulate power failure.
    pinMode(PIN_BATTERY_VOLTAGE, INPUT); // Battery input (needs a voltage divider)
    digitalWrite(PIN_LED_STATUS, HIGH); // Flash with different patterns to show HID comm status

    pinMode(PIN_LOGIC_TRIG, OUTPUT);    // Trigger logic analyzer
    digitalWrite(PIN_LOGIC_TRIG, HIGH); //  Trig on low going edge, high for now

    //pinMode(6, INPUT_PULLUP); // ground this pin to send only the shutdown command without changing bat charge // Sid added for temperary test
    //pinMode(5, OUTPUT);  // output flushing 1 sec indicating that the arduino cycle is running.
    //pinMode(10, OUTPUT); // output is on once commuication is lost with the host, otherwise off.


    // Set the first stuff set up to be looking good, so we don't cause a PC shutdown before we get things config'd and working.
    bitSet(iPresentStatus, PRESENTSTATUS_CHARGING);
    bitSet(iPresentStatus, PRESENTSTATUS_ACPRESENT);
    bitSet(iPresentStatus, PRESENTSTATUS_FULLCHARGE);

    bCharging = true;
    //digitalRead(PIN_INPUT_PWR_FAIL);
    bACPresent = bCharging;    // TODO - replace with sensor
    bDischarging = !bCharging; // TODO - replace with sensor     PowerDevice.setFeature(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));

    // Read battery, calc % charge, time remaining, status bits to set/clear
    UpdateBatteryStatus(bCharging, bACPresent, bDischarging);

    PowerDevice.setFeature(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));
    PowerDevice.setFeature(HID_PD_RUNTIMETOEMPTY,    &iRunTimeToEmpty           , sizeof(iRunTimeToEmpty            ));
    PowerDevice.setFeature(HID_PD_AVERAGETIME2FULL,  &StoreEE.iAvgTimeToFull    , sizeof(StoreEE.iAvgTimeToFull     ));
    PowerDevice.setFeature(HID_PD_AVERAGETIME2EMPTY, &StoreEE.iAvgTimeToEmpty   , sizeof(StoreEE.iAvgTimeToEmpty    ));
    PowerDevice.setFeature(HID_PD_REMAINTIMELIMIT,   &StoreEE.iRemainTimeLimit  , sizeof(StoreEE.iRemainTimeLimit   ));
    PowerDevice.setFeature(HID_PD_DELAYBE4REBOOT,    &iDelayBe4Reboot           , sizeof(iDelayBe4Reboot            ));
    PowerDevice.setFeature(HID_PD_DELAYBE4SHUTDOWN,  &iDelayBe4ShutDown         , sizeof(iDelayBe4ShutDown          ));
    PowerDevice.setFeature(HID_PD_RECHARGEABLE,      &bRechargable              , sizeof(bRechargable               ));
    PowerDevice.setFeature(HID_PD_CAPACITYMODE,      &bCapacityMode             , sizeof(bCapacityMode              ));
    PowerDevice.setFeature(HID_PD_CONFIGVOLTAGE,     &iConfigVoltage            , sizeof(iConfigVoltage             ));
    PowerDevice.setFeature(HID_PD_VOLTAGE,           &iVoltage                  , sizeof(iVoltage                   ));
    PowerDevice.setFeature(HID_PD_AUDIBLEALARMCTRL,  &iAudibleAlarmCtrl         , sizeof(iAudibleAlarmCtrl          ));
    PowerDevice.setFeature(HID_PD_DESIGNCAPACITY,    &iDesignCapacity           , sizeof(iDesignCapacity            ));
    PowerDevice.setFeature(HID_PD_FULLCHRGECAPACITY, &iFullChargeCapacity       , sizeof(iFullChargeCapacity        ));
    PowerDevice.setFeature(HID_PD_REMAININGCAPACITY, &iRemaining                , sizeof(iRemaining                 ));
    PowerDevice.setFeature(HID_PD_WARNCAPACITYLIMIT, &StoreEE.iWarnCapacityLimit, sizeof(StoreEE.iWarnCapacityLimit ));
    PowerDevice.setFeature(HID_PD_REMNCAPACITYLIMIT, &StoreEE.iRemnCapacityLimit, sizeof(StoreEE.iRemnCapacityLimit ));
    PowerDevice.setFeature(HID_PD_CPCTYGRANULARITY1, &bCapacityGranularity1     , sizeof(bCapacityGranularity1      ));
    PowerDevice.setFeature(HID_PD_CPCTYGRANULARITY2, &bCapacityGranularity2     , sizeof(bCapacityGranularity2      ));

    PowerDevice.setStringFeature(HID_PD_IDEVICECHEMISTRY, &bDeviceChemistry, STRING_DEVICECHEMISTRY );
    PowerDevice.setStringFeature(HID_PD_IOEMINFORMATION, &bOEMVendor,        STRING_OEMVENDOR       );

    batVoltage = StoreEE.BattParams[StoreEE.BatChem].batFullVoltage;

#ifdef CDC_ENABLED
    if (USBCDCNeeded)
    {
        Serial.begin(57600);  // Always enable Serial1 in case we enable Serial1 debugging  SLR
        while (!Serial)
            delay(10);
        SerialIsInitialized = true;
        Serial.print(F("USBCDCNeeded: "));  // DBC.008b
        Serial.println(USBCDCNeeded);    // DBC.008b
    }
#endif

    Serial1.print(F("USBCDCNeeded: "));  // DBC.008b
    Serial1.println(USBCDCNeeded);    // DBC.008b

    Serial1.flush();

    // Initially say all is well and avoid PC shutdown on first plug-in.
    bCharging = true;
    bDischarging = !bCharging; // TODO - replace with sensor
    //
    // Send initial report with everything looking good, update in main loop4
#if SEND_INITIAL_RPT
    int iRes0 = PowerDevice.sendReport(HID_PD_REMAININGCAPACITY, &iRemaining, sizeof(iRemaining));
    delay(100);
    if (bDischarging)
    {
        PowerDevice.sendReport(HID_PD_RUNTIMETOEMPTY, &iRunTimeToEmpty, sizeof(iRunTimeToEmpty));
        delay(1000);
    }
    iRes = PowerDevice.sendReport(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));
    SERIALPORT_PRINT(F("Initial capacity to PC, Result = "));
    SERIALPORT_PRINT(iRes0);
    SERIALPORT_PRINT(F(", status to PC, Result = "));
    SERIALPORT_PRINTLN(iRes);
    SERIALPORT_PRINT(F("Sending initial all-good report. iRemaining: "));
    SERIALPORT_PRINT(F(iRemaining));
    SERIALPORT_PRINT(F(", Status Bits: 0x"
    SERIALPORT_PRINTLN(String(iPresentStatus, HEX));

    SERIALPORT_PRINTLN(F(PROG_NAME_VERSION));
    SERIALPORT_PRINTLN(F(__FILE__));               // Print name of the source file
    SERIALPORT_PRINTLN("\nCompiled at: " __DATE__ ", " __TIME__);
    SERIALPORT_PRINTLN(F("Starting....\n\r"));
#endif // SEND_INITIAL_RPT

    delay(200);

    statusLedTimerOn.Start(10);
}


Timer_ms timeToUpdate;

void loop(void)
{
    static bool firstLoop = true;
    if (firstLoop)
    {
        char prTxt[] = "First call of loop()\n\r";
        if ((sizeof(USBDebug) - strlen(USBDebug)) > (strlen(prTxt)+1))  // Debug printout later
            memcpy(&USBDebug[strlen(USBDebug)], prTxt, strlen(prTxt)+1);
        firstLoop = false;
    }

    WATCHDOG_RESET; // Reset watchdog frequently

    handleLaptopInput();

    if (timeToUpdate.StartIfStopped(1000))
    {

        iIntTimer += 1;     // Send report to PC every few seconds, change or not.


        // If not enabled: Disable all updates. It seems like just suppressing the sending of reports to the PC isn't enough to 
        // keep the PC from shutting down when a low voltage is sensed. Interrupt driven something somewhere??
        // Default: Disabled in config (CDC Needed) mode to allow for the board to be callibrated
        // Enabled in normal run mode (CDC not needed) for comm with Synology NAS/PC host
        //
        //if (USBCDCNeeded ? StoreEE.msgPcEnabledCfgMode : StoreEE.msgPcEnabledRunMode)   // DOYET Try always sending, but change values being sent.
        {
            // Read battery, calc % charge, time remaining, status bits to set/clear
            UpdateBatteryStatus(bCharging, bACPresent, bDischarging);

            if (USBCDCNeeded ? StoreEE.msgPcEnabledCfgMode : StoreEE.msgPcEnabledRunMode)
            {
                // We do want to report to the PC, use actual values
                iRemaining      = iRemainingInternal;
                iPresentStatus  = iPresentStatusInternal;
                iRunTimeToEmpty = iRunTimeToEmptyInternal;
            }
            else
            {
                iRemaining = 100;   // Fake 100% remaining so we don't put PC to sleep
                iPresentStatus = 0;
                iRunTimeToEmpty = 120 * 60;
                bitSet(iPresentStatus, PRESENTSTATUS_ACPRESENT);
                bitSet(iPresentStatus, PRESENTSTATUS_FULLCHARGE);
                iVoltage = StoreEE.BattParams[BC_LeadAcid].batFullVoltage * StoreEE.battSysVMultiplier;

                static byte     prevRemainingInternal       = 100;
                static uint16_t prevRunTimeToEmptyInternal  = 7200;
                static uint16_t prevPresentStatusInternal   = 0;

                if ((iPresentStatusInternal != prevPresentStatusInternal) || (iRemainingInternal != prevRemainingInternal) || (iRunTimeToEmptyInternal != prevRunTimeToEmptyInternal) || (iIntTimer > MINUPDATEINTERVAL))
                {
                    Stream *serialPtr = SERIALPORT_Addr;

                    SERIALPORT_PRINT(F("# Sensed: "));

                    printValues(serialPtr, iRemainingInternal, iRunTimeToEmptyInternal, batVoltage, bDischarging, iPresentStatusInternal);     // Print Remaining minutes,

                    prevPresentStatusInternal  = iPresentStatusInternal ;
                    prevRunTimeToEmptyInternal = iRunTimeToEmptyInternal;
                    prevRemainingInternal      = iRemainingInternal     ;
                }
            }
            ////************ Delay ****************************************
            //delay(1000);
            //iIntTimer++;
            //digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level);
            //delay(1000);
            //iIntTimer++;
            //digitalWrite(5, LOW);   // turn the LED off;
            //
            //************ Check if we are still online ******************



            //************ Bulk send or interrupt ***********************

            if ((iPresentStatus != iPreviousStatus) || (iRemaining != iPrevRemaining) || (iRunTimeToEmpty != iPrevRunTimeToEmpty) || (iIntTimer > MINUPDATEINTERVAL))
            {

#if SEND_UPDATE_RPTS
                digitalWrite(PIN_LOGIC_TRIG, LOW); //  Trig on low going edge

                //if (false)
                //if (StoreEE.msgPcEnabledCfgMode)
                //{

                //Stream *serialPtr = &Serial1;
                Stream *serialPtr = SERIALPORT_Addr;

                SERIALPORT_PRINT(F("Sending: "));

                printValues(serialPtr, iRemaining, iRunTimeToEmpty, batVoltage, bDischarging, iPresentStatus);     // Print Remaining minutes,

                SERIALPORT_PRINT(F("Comms with PC (neg=bad): "));

                iRes = PowerDevice.sendReport(HID_PD_REMAININGCAPACITY, &iRemaining, sizeof(iRemaining));
                SERIALPORT_PRINT(iRes);
                SERIALPORT_PRINT(",");

                if (bDischarging)
                {
                    iRes = PowerDevice.sendReport(HID_PD_RUNTIMETOEMPTY, &iRunTimeToEmpty, sizeof(iRunTimeToEmpty));
                    SERIALPORT_PRINT(iRes);
                    SERIALPORT_PRINT(",");
                }
                iRes = PowerDevice.sendReport(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));
                SERIALPORT_PRINTLN(iRes);
                //DBPRINTLN(F("Sent bat status to PC"));
                //}
                digitalWrite(PIN_LOGIC_TRIG, HIGH); //  Trig'd on low going edge, back to high 
#endif // SEND_UPDATE_RPTS
                // DOYET: attache LED to pint 10
                //if (iRes < 0)
                //{
                //    digitalWrite(10, HIGH);
                //}
                //else digitalWrite(10, LOW);

                iIntTimer = 0;
                iPreviousStatus = iPresentStatus;
                iPrevRemaining = iRemaining;
                iPrevRunTimeToEmpty = iRunTimeToEmpty;
                //bForceShutdownPrior = bForceShutdown;

                if (iRes>=0)
                    status_HID = stat_Good;
                else
                    status_HID = stat_Error;

            }//end of: did anything change?
        }//end if (StoreEE.msgPcEnabledCfgMode)
        //else
        //{
        //    status_HID = stat_Disabled;
        //}


        
        DBPRINT(F("BatV*100: "));
        DBPRINTLN(batVoltage);
        //SERIALPORT_PRINT("BatV*100:: ");  // DBC.008
        //SERIALPORT_PRINTLN(batVoltage);  // DBC.008

#if SERIAL1_IRQ_DEBUG
        if (USBDebug[0])
        {
            SERIALPORT_PRINTLN(USBDebug);                   // DBC.009
            if (USBCDCNeeded)
                Serial1.print(USBDebug);    // Print to Serial1 if not already

            USBDebug[0] = 0;
        }
#endif

        if (pcSetErrorCount)
        {
            SERIALPORT_PRINT(F("## PC Setting REMNCAPACITYLIMIT to 0x"))    ;
            SERIALPORT_PRINT(pcSetValue, HEX);
            SERIALPORT_PRINTLN(F(" (Ignored)"));
            
            if (USBCDCNeeded)
            {
                Serial1.print(F("## PC Setting REMNCAPACITYLIMIT to 0x"));  // Could do this better memory wise! DOYET
                Serial1.print(pcSetValue, HEX);
                Serial1.println(F(" (Ignored)"));
            }
            pcSetErrorCount = 0;
            pcSetValue      = 0;
        }

        if(USBCDCNeeded) 
            Serial.flush(); 
        else 
            Serial1.flush();

    }//end if (timeToUpdate.StartIfStopped(1000))

    uint16_t DurOn;
    uint16_t DurOff;
    switch (status_HID)
    {
    case stat_Disabled: DurOn = 500; DurOff = 500; break;
    case stat_Good:     DurOn = 900; DurOff = 100; break;
    case stat_Error:    DurOn = 100; DurOff = 900; break;
    }
    if (statusLedTimerOn.isComplete())
    {
        statusLedTimerOff.Start(DurOff);
        digitalWrite(PIN_LED_STATUS, LOW);
    }
    if (statusLedTimerOff.isComplete())
    {
        statusLedTimerOn.Start(DurOn);
        digitalWrite(PIN_LED_STATUS, HIGH);
    }

    if (memStillFree > freeMemory())
    {
        memStillFree = freeMemory();
        SERIALPORT_PRINT(F("Available Memory: "));
        SERIALPORT_PRINTLN(memStillFree);
    }

    if (priorRemnCapacityLimit != StoreEE.iRemnCapacityLimit)
    {
        priorRemnCapacityLimit = StoreEE.iRemnCapacityLimit;
        SERIALPORT_PRINT(F("iRemnCapacityLimit CHANGED, now: "));
        SERIALPORT_PRINTLN(priorRemnCapacityLimit);
    }

}

#define TREND_HYSTERYSIS_V 8
void UpdateBatteryStatus(bool &bCharging, bool &bACPresent, bool &bDischarging)
{
    static int16_t recentVoltage = 0;
    static bool firstPass = true;
    static bool chargingTrend  = false;



    // DOYET Replace:
    // Do noise resistant reading of battery voltage
    int anaValue = analogRead(PIN_BATTERY_VOLTAGE);       // TODO - this is for debug only. Replace with charge estimation

    //if (true)
    //{
    batVoltageInternal = map(anaValue, StoreEE.calibPointLow.a2dValue, StoreEE.calibPointHigh.a2dValue, StoreEE.calibPointLow.voltage, StoreEE.calibPointHigh.voltage);
    if (batVoltageInternal < 0) batVoltageInternal = 0;

    // Determine if charging or discharging:
    // Save a "Recent" voltage. Whenever a reading is more than the hysterysis value different 
    // from the Recent voltage, update the Recent voltage and note if the latest is lower (discharging)
    // or higher (charging).
    // If the latest reading is higher the the isChargingVolts value, say we're charging no matter what.
    // If the latest reading is lower the the isDisChargingVolts value, say we're discharging no matter what.
    // 
    if (firstPass)
    {
        recentVoltage = batVoltageInternal;
        batVoltage = batVoltageInternal;
        firstPass = false;
    }

    bool chargingTrendTemp = batVoltageInternal > recentVoltage ;

    if (ABS(recentVoltage - batVoltageInternal) > TREND_HYSTERYSIS_V)
    {
        chargingTrend = chargingTrendTemp;
        recentVoltage = batVoltageInternal;
        batVoltage = batVoltageInternal;    // batVoltage gets reported externally
    }

    if (batVoltageInternal > (int16_t)StoreEE.BattParams[StoreEE.BatChem].isChargingVolts)
        chargingTrend = true;
    else if ((batVoltageInternal < (int16_t)StoreEE.BattParams[StoreEE.BatChem].isDisChargingVolts))
        chargingTrend = false;

    bACPresent = bCharging = chargingTrend;
    bDischarging = !bCharging;

    // For determining remaining capacity, we may need different charging and discharging curves.
    // The voltages read will probably be pushed down more if discharging faster.
    // 
    // 
    int iRemainingInt = map(batVoltageInternal, StoreEE.BattParams[StoreEE.BatChem].batEmptyVoltage, StoreEE.BattParams[StoreEE.BatChem].batFullVoltage, 0, 100);
    iRemainingInternal = constrain(iRemainingInt, 0, 100);

    iRunTimeToEmptyInternal = (uint16_t)((uint32_t)StoreEE.iAvgTimeToEmpty * (uint32_t)iRemainingInternal / (uint32_t)100);

    // Charging
    if (bCharging) bitSet(iPresentStatusInternal, PRESENTSTATUS_CHARGING);
    else bitClear(iPresentStatusInternal, PRESENTSTATUS_CHARGING);
    if (bACPresent) bitSet(iPresentStatusInternal, PRESENTSTATUS_ACPRESENT);
    else bitClear(iPresentStatusInternal, PRESENTSTATUS_ACPRESENT);
    if (iRemainingInternal == iFullChargeCapacity) bitSet(iPresentStatusInternal, PRESENTSTATUS_FULLCHARGE);
    else bitClear(iPresentStatusInternal, PRESENTSTATUS_FULLCHARGE);

    // Discharging
    if (bDischarging)
    {
        bitSet(iPresentStatusInternal, PRESENTSTATUS_DISCHARGING);
        // if(iRemainingInternal < iRemnCapacityLimit) bitSet(iPresentStatusInternal,PRESENTSTATUS_BELOWRCL);

        if (iRunTimeToEmptyInternal < StoreEE.iRemainTimeLimit)
        {
            SERIALPORT_PRINT(F("Shutdown now! Rtte="));
            SERIALPORT_PRINTLN(iRunTimeToEmptyInternal);
            SERIALPORT_Addr->flush();
            //delay(500);

            bitSet(iPresentStatusInternal, PRESENTSTATUS_RTLEXPIRED);
        }
        else bitClear(iPresentStatusInternal, PRESENTSTATUS_RTLEXPIRED);

    }
    else
    {
        bitClear(iPresentStatusInternal, PRESENTSTATUS_DISCHARGING);
        bitClear(iPresentStatusInternal, PRESENTSTATUS_RTLEXPIRED);
    }

    // Shutdown requested
    if (iDelayBe4ShutDown > 0)
    {
        bitSet(iPresentStatusInternal, PRESENTSTATUS_SHUTDOWNREQ);
        DBPRINTLN(F("shutdown requested"));
    }
    else bitClear(iPresentStatusInternal, PRESENTSTATUS_SHUTDOWNREQ);

    //if (bForceShutdown)
    //{
    //    bitSet(iPresentStatusInternal, PRESENTSTATUS_SHUTDOWNREQ);
    //}

    // Shutdown imminent
    if ((iPresentStatusInternal & (1 << PRESENTSTATUS_SHUTDOWNREQ)) ||
        (iPresentStatusInternal & (1 << PRESENTSTATUS_RTLEXPIRED)))
    {
        bitSet(iPresentStatusInternal, PRESENTSTATUS_SHUTDOWNIMNT);
        DBPRINTLN(F("shutdown imminent"));
    }
    else bitClear(iPresentStatusInternal, PRESENTSTATUS_SHUTDOWNIMNT);



    bitSet(iPresentStatusInternal, PRESENTSTATUS_BATTPRESENT);

}


// Print Remaining %, minutes:Seconds, Discharging Y/N, Status bits
void printValues(Stream *serialPtr, byte iRemaining, uint16_t iRunTimeToEmpty, uint16_t batV, bool bDischarging, uint16_t iPresentStatus)
{
    serialPtr->print(F("Batt Remaining = "));
    serialPtr->print(iRemaining);
    serialPtr->print(F("%, "));
    serialPtr->print(iRunTimeToEmpty / 60);
    serialPtr->print(F(":"));
    serialPtr->print(iRunTimeToEmpty % 60);
    serialPtr->print(F(", BatV Internal: "));    // DEBUG Remove these 2 lines eventually
    serialPtr->print(batVoltageInternal);       // DEBUG Remove these 2 lines 
    serialPtr->print(F(", BatV: "));
    serialPtr->print(batV);
    //serialPtr->print(F(", Discharging: "));
    //serialPtr->print(bDischarging ? "Y" : "N");
    serialPtr->print(F(", Status = 0x"));
    serialPtr->print(iPresentStatus, HEX);
    const char *sptr = "";
    for (uint8_t i = 0; i < 15; i++)
    {
        uint16_t b = 1 << i;
        if (b & iPresentStatus)
        {
            switch (i)
            {
            case  PRESENTSTATUS_CHARGING    : sptr = " CHRG";  break;
            case  PRESENTSTATUS_DISCHARGING : sptr = " DISCH";  break;
            case  PRESENTSTATUS_ACPRESENT   : sptr = " AC";  break;
            case  PRESENTSTATUS_RTLEXPIRED  : sptr = " RTLEXPIR";  break;
            case  PRESENTSTATUS_FULLCHARGE  : sptr = " FULL";  break;
            case  PRESENTSTATUS_SHUTDOWNREQ : sptr = " DwnSoon";  break;
            case  PRESENTSTATUS_SHUTDOWNIMNT: sptr = " DwnNow";  break;
            case  PRESENTSTATUS_BATTPRESENT : sptr = " BatPres";  break;
            default  : sptr = " ??";  break;
            }
            serialPtr->print(sptr);
        }
    }
    serialPtr->println();
    serialPtr->flush();

}

//#ifdef CDC_ENABLED  // DBC.007
void handleLaptopInput(void)
{
#define ASCII_CR            0x0D
#define ASCII_LF            0x0A
#define ASCII_ESC           0x1B
    
    static char userIn[MAX_MENU_CHARS];     // MAX_MENU_CHARS Def'd in Maltbie_Helper.h
    static uint8_t indexUserIn = 0;
    int inByte;

    while (SERIALPORT_AVAILABLE())     // DBC.007
    {
        inByte = SERIALPORT_READ();    // DBC.007
        if (doDebugPrints)
        {
            SERIALPORT_WRITE(inByte);  // DBC.007
        }
        //if (inByte == ASCII_CR)
        //{
        //    DBPRINTLN(F("<CR>"));
        //}

        if (indexUserIn < MAX_MENU_CHARS)
        {
            if (!((inByte == ASCII_CR) || (inByte == ASCII_LF) || (inByte == 0xEF) || (inByte == 0xBB) || (inByte == 0xBF)))    // 0xBF sent by RealTerm before sending a file (why???)
            {
                userIn[indexUserIn++] = inByte; // Append the latest character, unless it is a CR or LF
            }

            userIn[indexUserIn] = 0;
            DBPRINT(F("["));    // DBC.007
            DBPRINT(userIn);    // DBC.007
            DBPRINTLN(F("]"));  // DBC.007

            if (indexUserIn)    // Skip only CR or LF without a real string
            {
                //processUserInput(userIn, indexUserIn, inByte);                // DBC.007
                processUserInput(userIn, indexUserIn, inByte, SERIALPORT_Addr, true);  // DBC.007
            }
        } //end if (indexUserIn<MAX_MENU_CHARS)
        else
        {
            // Attempted to overflow the input buffer, reset it.
            indexUserIn = 0;
            userIn[0] = 0;
        }

        if (indexUserIn == 0)
        {
            DBPRINT(F("\nCmd: "));
        }
    } // end while (SERIALPORT_AVAILABLE())

}


void processUserInput(char userIn[MAX_MENU_CHARS], uint8_t& indexUserIn, int inByte, Stream *serialPtr, bool handleUsbOnlyOptions)
{
    //char delim[] = ",. ";   // delimiters between fields in serial commands

    MH.setSerialOutputStream(serialPtr);    // Send serial output to Serial or to BlueTooth

    if (((inByte == ASCII_CR) || (inByte == ASCII_LF)) && indexUserIn)
    {
        char cmdChar = (char)toupper(userIn[0]);
        switch (cmdChar)
        {
        case 'E':   // Battery Config
            {
                bool goodCmd = false;
                if (toupper(userIn[1]) == 'N')
                {
                    if (toupper(userIn[2]) == 'C')
                    {
                        MH.updateFromUserInput(userIn+2, indexUserIn, inByte, 1, StoreEE.msgPcEnabledCfgMode , F("Run (Normal) Mode: Enable PC Reporting/Shutdown")); 
                        goodCmd = true;
                    }
                    else if (toupper(userIn[2]) == 'R')
                    {
                        MH.updateFromUserInput(userIn+2, indexUserIn, inByte, 1, StoreEE.msgPcEnabledRunMode , F("Config Mode: Enable PC Reporting/Shutdown")); 
                        goodCmd = true;
                    }
                }
                if (!goodCmd)
                {
                    serialPtr->print(F("ERROR: Bad ENable command command: "));
                    serialPtr->println(userIn);
                }
            }
            break;
        
        case 'B':   // Battery Config
            {
                switch ((char)toupper(userIn[1]))
                {
                case 'F': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 7000, StoreEE.BattParams[StoreEE.BatChem].batFullVoltage , F("Battery Full Charge V")); break;
                case 'E': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 7000, StoreEE.BattParams[StoreEE.BatChem].batEmptyVoltage, F("Battery Empty V"      )); break;
                case 'C': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, (uint8_t)BC_Last, (uint8_t&)StoreEE.BatChem, F("Bat Chemistry: 0=PbAc 1=Li-Ion 2=LFP 3=AGM")); break;
                case 'V': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 4, (uint8_t&)StoreEE.battSysVMultiplier, F("1=12V 2=24V 4=48V"), false, 1); break;
                default:
                    serialPtr->print(F("ERROR: Bad B (Battery) command: "));
                    serialPtr->println(userIn);
                    break;
                }
            }
            break;

        case 'T':   // Expected Durations
            {
                switch ((char)toupper(userIn[1]))
                {
                case 'C': 
                    {
                        uint16_t minsTemp = StoreEE.iAvgTimeToFull / 60;
                        MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, minsTemp, F("Minutes to fully charge from dead"   )); 
                        StoreEE.iAvgTimeToFull = minsTemp * 60;
                    }
                    break;

                case 'D': 
                    {
                        uint16_t minsTemp = StoreEE.iAvgTimeToEmpty / 60;
                        MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, minsTemp, F("Minutes to fully discharge from full")); 
                        StoreEE.iAvgTimeToEmpty = minsTemp * 60;
                    }
                    break;

                default:
                    serialPtr->print(F("ERROR: Bad Time command: "));
                    serialPtr->println(userIn);
                    break;
                }
            }
            break;

        case 'C':   // Board Calibration
            {
                switch ((char)toupper(userIn[1]))
                {
                case 'L': 
                    {
                        uint16_t valVin100s = StoreEE.calibPointLow.voltage;
                        if (MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, valVin100s , F("V*100 of low voltage")   ))
                        {
                            StoreEE.calibPointLow.voltage = valVin100s;
                            StoreEE.calibPointLow.a2dValue = MH.anaFilter_Mid(PIN_BATTERY_VOLTAGE);
                        }
                    }
                    break;
                case 'H':
                    {
                        uint16_t valVin100s = StoreEE.calibPointHigh.voltage;
                        if (MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, valVin100s , F("V*100 of higher voltage")))
                        {
                            StoreEE.calibPointHigh.voltage = valVin100s;
                            StoreEE.calibPointHigh.a2dValue = MH.anaFilter_Mid(PIN_BATTERY_VOLTAGE);
                        }
                    }
                    break;


                default:
                    serialPtr->print(F("ERROR: Bad B (Battery) command: "));
                    serialPtr->println(userIn);
                    break;
                }
            }
            break;

        
        case 'H':
        case '?':
            {
                switch ((char)toupper(userIn[1]))
                {
                case 'E':
                    break;

#if SHOW_ALL_PARAMS
                case 'P':
                    showParameters(serialPtr);
                    break;
#endif

                default:
                      printHelp(serialPtr, handleUsbOnlyOptions);  // DBC.007
                    break;                                         // DBC.007
                }
            }
            break;

        case 'S':
            {
                EEPROM.put(EEP_OFFSET_USER, StoreEE);     // Write to EEPROM at users command
                serialPtr->println(F("\nSaved new value(s) to EEPROM.\n"));
            }
            break;

        case 'Z':
            {
                if (toupper(userIn[1]) == 'C')
                {
                    if (toupper(userIn[2]) == 'D')
                    {
                        serialPtr->println(F("\nRestored Compiled Factory Defaults!"));
                        FactoryCompiledDefault();
                    }
                }
                else if (toupper(userIn[1]) == 'S')
                {
                    if (toupper(userIn[2]) == 'F')
                    {
                        EEPROM.put(EEP_OFFSET_FACTORY, StoreEE);     // Write to "Factory Default" EEPROM 
                        serialPtr->println(F("\nSaved new value(s) to Factory Default\n"));
                    }
                }
                else
                {

                    FactoryDefault();       // Reset color and air pump settings
                    serialPtr->println(F("\nRestored Factory Defaults...\r\n Must do 'S' to save to EEPROM (non-volatile memory)."));
                }
                enableDebugPrints(StoreEE.debugFlags);  // Update the debug printing
            }
            break;


        //case '#':
        //    {
        //        watchDogReset();
        //    }
        //    break;

        case '~':
            dumpEEProm(serialPtr);
            break;

        case 'D':
            {
                MH.updateFromUserInput(userIn, indexUserIn, inByte, 0xFF, StoreEE.debugFlags, F("Debug Flags (1:Main)"), true);
                enableDebugPrints(StoreEE.debugFlags);  // Update the debug printing
            }
            break;  // Expects Hex string

        // Clear EEPROM to FFs (uncomment to use)
        //case '`':
        //    clearEEPromToFFs();
        //    serialPtr->println(F("Set all of EEPROM to FF"));
        //    break;

        case 'N':
            {
                switch ((char)toupper(userIn[1]))
                {
                default:
                    serialPtr->println(F("Bad command. Should be NL or NH"));
                    break;
                }
            }
            break;

        case 'Q':  // DBC.008
            {                                              // DBC.008
              serialPtr->print(F("CDC_ACM_INTERFACE: "));  // DBC.008
              serialPtr->println(CDC_ACM_INTERFACE);       // DBC.008
              serialPtr->print(F("HID_INTERFACE: "));      // DBC.008
              serialPtr->println(HID_INTERFACE);           // DBC.008 SLR HID_INTERFACE wasn't defined
            }                                              // DBC.008
            break;                                         // DBC.008

        default:
            serialPtr->print(F("\nUnknown command ["));
            serialPtr->print(userIn);
            serialPtr->println(F("]."));
            serialPtr->println(F("H for Help\n"));
            indexUserIn = 0;
            userIn[0] = 0;
            break;
        }
        
        indexUserIn = 0;
    } // end if ((inByte == ASCII_CR) || (inByte == ASCII_LF))

}


// Used menu characters:
//            ABCDEFGHIJKLMNOPQRSTUVWXYZ
// #&()+--/?@^ABCDEFGHIJKLMN PQRSTUVWXYZ~  ('!' reserved for BLE commands (color/Up/DwnArrow), P for Password)
//
    void showPersistentSettings(Stream * serialPtr, bool handleUsbOnlyOptions)
    {
        serialPtr->println();
        serialPtr->println(F(PROG_NAME_VERSION));
        serialPtr->print(F("Compiled at: "));
        serialPtr->println(__DATE__ ", " __TIME__);
        serialPtr->println();

        serialPtr->println(F("Present Settings:"));
        serialPtr->print(F("Enable reporting/Shutdown to to the PC: ")); serialPtr->println(StoreEE.msgPcEnabledCfgMode );
        serialPtr->print(F("ENCn   - While in Config mode                  : ")); serialPtr->println(StoreEE.msgPcEnabledCfgMode );
        serialPtr->print(F("ENRn   - While in Normal run mode              : ")); serialPtr->println(StoreEE.msgPcEnabledRunMode );
        serialPtr->println(F("         (0=No PC reporting/shutdown. 1=Enabled)"));
        serialPtr->println(F("BCn    - Battery Chemistry"));
        serialPtr->print(F("             0=PbAc 1=Li-Ion 2=LFP 3=AGM       : ")); serialPtr->println(StoreEE.BatChem);
        serialPtr->print(F("BVn    - Bat System Volts 1=12V 2=24V 4=48V    : ")); serialPtr->println(StoreEE.battSysVMultiplier);
        serialPtr->print(F("BFnnnn - Battery Full Charge voltage    (V*100): ")); serialPtr->println(StoreEE.BattParams[StoreEE.BatChem].batFullVoltage );
        serialPtr->print(F("BEnnnn - Battery Empty voltage          (V*100): ")); serialPtr->println(StoreEE.BattParams[StoreEE.BatChem].batEmptyVoltage);
        serialPtr->print(F("TCnnnn - Average Time to fully Charge (minutes): ")); serialPtr->println(StoreEE.iAvgTimeToFull/60  );
        serialPtr->print(F("TDnnnn - Ave. Time for full Discharge (minutes): ")); serialPtr->println(StoreEE.iAvgTimeToEmpty/60 );
        serialPtr->print(F("CLnnnn - Calibrate low voltage point (V*100)   : ")); serialPtr->print(StoreEE.calibPointLow.voltage); serialPtr->println(F(" V*100"));
        serialPtr->print(F("         (Low V A2D value: "));                       serialPtr->print(StoreEE.calibPointLow.a2dValue); serialPtr->println(F(")"));
        serialPtr->print(F("CHnnnn - Calibrate high voltage point (V*100)  : ")); serialPtr->print(StoreEE.calibPointHigh.voltage); serialPtr->println(F(" V*100"));
        serialPtr->print(F("         (High V A2D value: "));                      serialPtr->print(StoreEE.calibPointHigh.a2dValue); serialPtr->println(F(")"));
        serialPtr->print(F("                            iRemnCapacityLimit : ")); serialPtr->print(StoreEE.iRemnCapacityLimit); serialPtr->println(F(")"));
        //serialPtr->println(F(" To calibrate voltage sensing for this board:"));
        //serialPtr->println(F("  Disconnect the battery voltage sense leads from the battery.  "));
        //serialPtr->println(F("  Short them together. Enter the command CL0 (Zero, not Oh)."));
        //serialPtr->println(F("  Hook the leads back up to the battery. Measure the voltage"));
        //serialPtr->println(F("  with a good digital volt meter. Enter the command CHvvvv."));
        //serialPtr->println(F("  where vvvv is the measured voltage in hundreths of a volt."));
        //serialPtr->println(F("  (If you measured 12.45 volts, enter CH1245)"));
        //serialPtr->println(F("  Voltages will always be entered in hundredths. "));
        //serialPtr->println(F("  (V*100)"));
        serialPtr->println(F("Z   - Restore factory defaults"));
        serialPtr->println(F("ZCD - Restore compiled factory defaults"));
        //serialPtr->println(F("ZSF - Save Factory defaults (Factory use only)"));

        serialPtr->print(F("Dx   - Debug flags              : 0x")); serialPtr->println(StoreEE.debugFlags, HEX);
        serialPtr->print(F("Battery Voltage*100: ")); serialPtr->println(batVoltage);
        serialPtr->print(F(", Batt Remaining = "));
        serialPtr->print(iRemaining);
        serialPtr->print(F("%, "));
        serialPtr->print(iRunTimeToEmpty / 60);
        serialPtr->print(F(":"));
        serialPtr->println(iRunTimeToEmpty % 60);
        serialPtr->print(F("Discharging: "));
        serialPtr->print(bDischarging ? "Y" : "N");
        serialPtr->print(F(", Status = 0x"));
        serialPtr->println(iPresentStatus, HEX);
        serialPtr->print(F("Available Memory: "));
        serialPtr->println(freeMemory());
    }

#if SHOW_ALL_PARAMS
    void showParameters(Stream * serialPtr)
    {
        serialPtr->println();
        serialPtr->print(F("RUNTIMETOEMPTY   : ")); serialPtr->println(iRunTimeToEmpty            );
        //serialPtr->print(F("AVERAGETIME2FULL : ")); serialPtr->println(StoreEE.iAvgTimeToFull     );
        //serialPtr->print(F("AVERAGETIME2EMPTY: ")); serialPtr->println(StoreEE.iAvgTimeToEmpty    );
        //serialPtr->print(F("REMAINTIMELIMIT  : ")); serialPtr->println(StoreEE.iRemainTimeLimit   );
        //serialPtr->print(F("DELAYBE4REBOOT   : ")); serialPtr->println(iDelayBe4Reboot            );
        //serialPtr->print(F("DELAYBE4SHUTDOWN : ")); serialPtr->println(iDelayBe4ShutDown          );
        //serialPtr->print(F("RECHARGEABLE     : ")); serialPtr->println(bRechargable               );
        //serialPtr->print(F("CAPACITYMODE     : ")); serialPtr->println(bCapacityMode              );
        //serialPtr->print(F("CONFIGVOLTAGE    : ")); serialPtr->println(iConfigVoltage             );
        //serialPtr->print(F("VOLTAGE          : ")); serialPtr->println(iVoltage                   );
        //serialPtr->print(F("AUDIBLEALARMCTRL : ")); serialPtr->println(iAudibleAlarmCtrl          );
        //serialPtr->print(F("DESIGNCAPACITY   : ")); serialPtr->println(iDesignCapacity            );
        //serialPtr->print(F("FULLCHRGECAPACITY: ")); serialPtr->println(iFullChargeCapacity        );
        serialPtr->print(F("REMAININGCAPACITY: ")); serialPtr->println(iRemaining                 );
        //serialPtr->print(F("WARNCAPACITYLIMIT: ")); serialPtr->println(StoreEE.iWarnCapacityLimit );
        serialPtr->print(F("REMNCAPACITYLIMIT: ")); serialPtr->println(StoreEE.iRemnCapacityLimit );
        //serialPtr->print(F("CPCTYGRANULARITY1: ")); serialPtr->println(bCapacityGranularity1      );
        //serialPtr->print(F("CPCTYGRANULARITY2: ")); serialPtr->println(bCapacityGranularity2      );
        //serialPtr->print(F("IDEVICECHEMISTRY : ")); serialPtr->println(STRING_DEVICECHEMISTRY );  // These are in flash
        //serialPtr->print(F("IOEMINFORMATION  : ")); serialPtr->println(STRING_OEMVENDOR       );
        serialPtr->print(F("batVoltage       : ")); serialPtr->println(batVoltage             );
        serialPtr->print(F("iPresentStatus   : 0x")); serialPtr->println(iPresentStatus, HEX  );
    }
#endif

void printHelp(Stream * serialPtr, bool handleUsbOnlyOptions)
{
    serialPtr->println(F("\n----Menu----"));
    showPersistentSettings(serialPtr, handleUsbOnlyOptions); // DBC.007
    serialPtr->println(F("HP - Show Parameters"));
    serialPtr->println(F("Z  - Restore Factory Defaults"));
    serialPtr->println("Size of StoreEE: " + String(sizeof(StoreEE)));
}


void FactoryDefault(void)
{
    EEPROM.get(EEP_OFFSET_FACTORY, StoreEE); // Fetch our structure of non-volitale vars from "Factory Default" EEPROM
}

void FactoryCompiledDefault(void)
{
    //StoreEE.debugFlags        =  0;  // No debug prints by default  // DBC.007
    StoreEE.debugFlags        =  0;  // Debug prints by default       // DBC.007
    StoreEE.eeValid_1         = EEPROM_VALID_PAT1;      // Set sig in case user stores config to EEPROM.

    StoreEE.eeValid_2         = EEPROM_VALID_PAT2;
    StoreEE.eeVersion         = EEPROM_END_VER_SIG;


    // Physical parameters
    StoreEE.iAvgTimeToFull     = 120*60;
    StoreEE.iAvgTimeToEmpty    = 120*60;
    StoreEE.iRemainTimeLimit   =  10*60;

    StoreEE.BatChem            = BC_AGM;
    StoreEE.battSysVMultiplier = 1;     // 1:12V

    // Lead-Acid
    StoreEE.BattParams[BC_LeadAcid].batFullVoltage     = 1288;  // Voltage in hundredths (V*100), Bat Full
    StoreEE.BattParams[BC_LeadAcid].batEmptyVoltage    = 1162;  // V*100, Bat Empty       
    StoreEE.BattParams[BC_LeadAcid].isChargingVolts    = 1288;  // V*100, Above this, must be charging
    StoreEE.BattParams[BC_LeadAcid].isDisChargingVolts = 1170;  // V*100, Below this, must be discharging
    StoreEE.BattParams[BC_LeadAcid].numCapacityPointsUsed = NUM_CAPACITY_POINTS;    // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LeadAcid].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LeadAcid].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: AGM
    StoreEE.BattParams[BC_LeadAcid].VtoCapacities[0].voltage = 1162;  StoreEE.BattParams[BC_AGM].VtoCapacities[0].capacity =  0;    // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacities[1].voltage = 1170;  StoreEE.BattParams[BC_AGM].VtoCapacities[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacities[2].voltage = 1278;  StoreEE.BattParams[BC_AGM].VtoCapacities[2].capacity =  90;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacities[3].voltage = 1288;  StoreEE.BattParams[BC_AGM].VtoCapacities[3].capacity = 100;   // 

    // AGM
    StoreEE.BattParams[BC_AGM].batFullVoltage     = 1380;   // Voltage in hundredths (V*100), Bat Full
    StoreEE.BattParams[BC_AGM].batEmptyVoltage    = 1050;   // V*100, Bat Empty       
    StoreEE.BattParams[BC_AGM].isChargingVolts    = 1280;   // V*100, Above this, must be charging
    StoreEE.BattParams[BC_AGM].isDisChargingVolts = 1150;   // V*100, Below this, must be discharging
    StoreEE.BattParams[BC_AGM].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_AGM].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_AGM].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: AGM
    StoreEE.BattParams[BC_AGM].VtoCapacities[0].voltage = 1050;  StoreEE.BattParams[BC_AGM].VtoCapacities[0].capacity =  0;    // 
    StoreEE.BattParams[BC_AGM].VtoCapacities[1].voltage = 1151;  StoreEE.BattParams[BC_AGM].VtoCapacities[1].capacity =  10;   // 
    StoreEE.BattParams[BC_AGM].VtoCapacities[2].voltage = 1275;  StoreEE.BattParams[BC_AGM].VtoCapacities[2].capacity =  90;   // 
    StoreEE.BattParams[BC_AGM].VtoCapacities[3].voltage = 1282;  StoreEE.BattParams[BC_AGM].VtoCapacities[3].capacity = 100;   // 

    // LI_ION
    StoreEE.BattParams[BC_LI_ION].batFullVoltage     = 1360;   // Voltage in hundredths (V*100), Bat Full 
    StoreEE.BattParams[BC_LI_ION].batEmptyVoltage    = 1000;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LI_ION].isChargingVolts    = 1365;   // V*100, Above this, must be charging     
    StoreEE.BattParams[BC_LI_ION].isDisChargingVolts = 1200;   // V*100, Below this, must be discharging  
    StoreEE.BattParams[BC_LI_ION].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LI_ION].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LI_ION].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: LFP
    StoreEE.BattParams[BC_LI_ION].VtoCapacities[0].voltage = 1000;  StoreEE.BattParams[BC_AGM].VtoCapacities[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LI_ION].VtoCapacities[1].voltage = 1200;  StoreEE.BattParams[BC_AGM].VtoCapacities[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacities[2].voltage = 1280;  StoreEE.BattParams[BC_AGM].VtoCapacities[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacities[3].voltage = 1360;  StoreEE.BattParams[BC_AGM].VtoCapacities[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    // LFP
    StoreEE.BattParams[BC_LFP].batFullVoltage     = 1360;   // Voltage in hundredths (V*100), Bat Full 
    StoreEE.BattParams[BC_LFP].batEmptyVoltage    = 1000;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LFP].isChargingVolts    = 1365;   // V*100, Above this, must be charging     
    StoreEE.BattParams[BC_LFP].isDisChargingVolts = 1200;   // V*100, Below this, must be discharging  
    StoreEE.BattParams[BC_LFP].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LFP].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LFP].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: LFP
    StoreEE.BattParams[BC_LFP].VtoCapacities[0].voltage = 1000;  StoreEE.BattParams[BC_AGM].VtoCapacities[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LFP].VtoCapacities[1].voltage = 1200;  StoreEE.BattParams[BC_AGM].VtoCapacities[1].capacity =   9;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacities[2].voltage = 1290;  StoreEE.BattParams[BC_AGM].VtoCapacities[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacities[3].voltage = 1350;  StoreEE.BattParams[BC_AGM].VtoCapacities[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    memcpy(&StoreEE.BattParams[BC_Other], &StoreEE.BattParams[BC_AGM], sizeof(StoreEE.BattParams[BC_AGM]));   // Init "Other" too


    StoreEE.calibPointLow.voltage   = 0;
    StoreEE.calibPointLow.a2dValue  = 0;
    StoreEE.calibPointHigh.voltage  = 1400;
    StoreEE.calibPointHigh.a2dValue = 1023;  // This will definetely need to be calibrated!

    // Parameters for ACPI compliancy
    StoreEE.iWarnCapacityLimit = 10; // warning at 10%
    StoreEE.iRemnCapacityLimit = 5; // low at 5%
    StoreEE.msgPcEnabledCfgMode = false;
    StoreEE.msgPcEnabledRunMode = true;

    enableDebugPrints(StoreEE.debugFlags);


    Serial1.println(F("Restored factory defaults."));       // DOYET PUT BACK

    if(USBCDCNeeded && Serial)
        Serial.println(F("Restored factory defaults."));       // DOYET PUT BACK
}


void enableDebugPrints(uint8_t debugPrBits)
{
    doDebugPrints      = bitRead(debugPrBits, DBG_PRINT_MAIN     ) ? true : false;   // 0    // 0x01 Turn off debug printing for this file
}

//
// Used in calculating free memory.
//
extern unsigned int __bss_end;
extern void *__brkval;
//
// Returns the current amount of free memory in bytes.
//
int freeMemory() 
{
	int free_memory;
	if ((int) __brkval)
		return ((int) &free_memory) - ((int) __brkval);
	return ((int) &free_memory) - ((int) &__bss_end);
}
