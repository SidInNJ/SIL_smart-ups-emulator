/*
 * Filename:  SmartUpsEmulator.008
 * Date:      8 Feb 2024
 *            This code works both when CDC_ENABLED is defined and when CDC_DISABLED is defined
 *            When CDC_ENABLED is defined output is written to the USB serial port
 *            When CDC_DISABLED is defined Output is written to the Serial1 (pins 0 & 1) hardware UART
*/

/*
  In USBDesc.h
    Change CDC_ACM_INTERFACE	0 -> CDC_ACM_INTERFACE	1  // Leave interface 0 open for HID device
    Change CDC_DATA_INTERFACE	1 -> CDC_DATA_INTERFACE	2

  In HID.h
    Change HID_INTERFACE (CDC_ACM_INTERFACE + CDC_INTERFACE_COUNT) -> 0
                                  0         +         2
  This doesn't work. The Device Monitoring Studio does not see the device.
  The USB Device Tree Viewer lists the device with "Device has Problem Code 10 (failed start)."

*/

//#include <HIDPowerDevice.h>
#include "HIDPowerDevice.h"
#include "TimerHelpers.h"

//#include "HID.h"    // SLR: to define HID_INTERFACE

#include "HandyHelpers.h"
HandyHelpers MH; // My Handy Helper

#ifdef CDC_ENABLED
#warning "NOTE: Serial console via USB (CDC) is still enabled!"
#endif

#ifdef USBCON
#warning "NOTE: USBCON is defined!"
#else
#warning "NOTE: USBCON is NOT defined!"
#endif

#define MINUPDATEINTERVAL   26

int iIntTimer = 0;

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
bool doDebugPrints = true;  // Enable printing by default
#define DBPRINTLN(args...) if(doDebugPrints) { SERIALPORT.println(args);}
#define DBPRINT(args...)   if(doDebugPrints) { SERIALPORT.print(args);}
#define DBWRITE(args...)   if(doDebugPrints) { SERIALPORT.write(args);}

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

byte iRemaining = 100, iPrevRemaining = 0;

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

struct CalibPoint
{
    uint16_t a2dValue;
    uint16_t voltage;     // in hundredths of volts
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


    // Physical parameters
    uint16_t iAvgTimeToFull  ;  // in seconds
    uint16_t iAvgTimeToEmpty ;  // in seconds
    uint16_t iRemainTimeLimit;  // in seconds
    uint16_t batFullVoltage  ;  // in hundredths of volts
    uint16_t batEmptyVoltage ;  // in hundredths of volts

    // Parameters for ACPI compliancy
    byte iWarnCapacityLimit;    // warning at 10%
    byte iRemnCapacityLimit;    // low at 5%
    bool msgPcEnabled;

    // Board/Resistor Divider Calibration
    CalibPoint  calibPointLow;  // Note A2D reading and associated voltage at two points. "map" from there
    CalibPoint  calibPointHigh;

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

EEPROM_Struct StoreEE;

////////////////////
// P R O T O S
////////////////////
void handleLaptopInput(void);
void printHelp(void);
void watchDogReset(void);
//void printHelp(Stream *serialPtr = &SERIALPORT, bool handleUsbOnlyOptions = true);   // DBC.007
//void printHelp(Stream *serialPtr = &Serial1, bool handleUsbOnlyOptions = true);  // DBC.007
//void processUserInput(char userIn[MAX_MENU_CHARS], uint8_t& indexUserIn, int inByte, Stream *serialPtr = &SERIALPORT, bool handleUsbOnlyOptions = true);  // DBC.007
void printHelp(Stream *serialPtr = &SERIALPORT, bool handleUsbOnlyOptions = true);
void processUserInput(char userIn[MAX_MENU_CHARS], uint8_t& indexUserIn, int inByte, Stream *serialPtr = &SERIALPORT, bool handleUsbOnlyOptions = true);
void showPersistentSettings(Stream *serialPtr = &SERIALPORT, bool handleUsbOnlyOptions = true);
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

//Stream *serPtr = &SERIALPORT;  // DBC.007
Stream *serPtr = &SERIALPORT;
int batVoltage = 1300;

Timer_ms statusLedTimerOn;
Timer_ms statusLedTimerOff;

extern volatile bool USBCDCNeeded;  // DBC.008b
extern bool AskedForCDC;   // DBC.008b
extern long USBSwitchTime[6];  // DBC.008c
extern long USBSwitchCount[6];

void setup(void)
{

#ifdef CDC_ENABLED
    Serial.begin(115200);
    delay(5000);
    Serial.print(F("USB Serial baud, bits, parity, stop bits: "));
    Serial.print(Serial.baud());
    Serial.print(F(", "));
    Serial.print(Serial.numbits());
    Serial.print(F(", "));
    Serial.print(Serial.paritytype());
    Serial.print(F(", "));
    Serial.println(Serial.stopbits());
#endif


// #ifdef CDC_DISABLED
    Serial1.begin(115200);  // Always enable Serial1 in case we enable Serial1 debugging  SLR
    while (!Serial1);
    delay(1000);
// #endif


    DBPRINTLN();
    DBPRINTLN(F(PROG_NAME_VERSION));
    DBPRINTLN(F(__FILE__));               // Print name of the source file
    DBPRINTLN("\nCompiled at: " __DATE__ ", " __TIME__);
    DBPRINTLN("Starting...\n\r");

    //Serial1.begin(115200);
    //while (!Serial1);
    //delay(1000);
    //Serial1.print("Starting...\n\r");

    // Davis, do we want to always send this to Serial1, CDC_ENABLED or not?
    Serial1.print("USBCDCNeeded: ");  // DBC.008b
    Serial1.println(USBCDCNeeded);    // DBC.008b

    // New for GTIS
    EEPROM.get(0, StoreEE); // Fetch our structure of non-volitale vars from EEPROM

    if ((StoreEE.eeValid_1 == EEPROM_VALID_PAT1) && (StoreEE.eeValid_2 == EEPROM_VALID_PAT2) && (StoreEE.eeVersion == EEPROM_END_VER_SIG)) // Signature Valid?
    {
        enableDebugPrints(StoreEE.debugFlags);
        DBPRINTLN(F("Good: EEPROM is initialized."));
    }
    else
    {
        DBPRINTLN(F("ERROR: Need to do configuration and write to EEPROM. Using Defaults."));
        FactoryDefault();
    }


    // Init Watchdog DOYET

    PowerDevice.begin();

    // Serial No is set in a special way as it forms Arduino port name
    PowerDevice.setSerial(STRING_SERIAL);

    // Used for debugging purposes.
#ifdef CDC_ENABLED
    PowerDevice.setOutput(SERIALPORT);
#endif

    pinMode(PIN_INPUT_PWR_FAIL, INPUT_PULLUP); // ground this pin to simulate power failure.
    pinMode(PIN_BATTERY_VOLTAGE, INPUT); // Battery input (needs a voltage divider)
    digitalWrite(PIN_LED_STATUS, HIGH); // Flash with different patterns to show HID comm status

    //pinMode(6, INPUT_PULLUP); // ground this pin to send only the shutdown command without changing bat charge // Sid added for temperary test
    //pinMode(5, OUTPUT);  // output flushing 1 sec indicating that the arduino cycle is running.
    //pinMode(10, OUTPUT); // output is on once commuication is lost with the host, otherwise off.


    // Set the first stuff set up to be looking good, so we don't cause a PC shutdown before we get things config'd and working.
    bitSet(iPresentStatus, PRESENTSTATUS_CHARGING);
    bitSet(iPresentStatus, PRESENTSTATUS_ACPRESENT);
    bitSet(iPresentStatus, PRESENTSTATUS_FULLCHARGE);

    PowerDevice.setFeature(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));

    PowerDevice.setFeature(HID_PD_RUNTIMETOEMPTY,    &iRunTimeToEmpty, sizeof(iRunTimeToEmpty));
    PowerDevice.setFeature(HID_PD_AVERAGETIME2FULL,  &StoreEE.iAvgTimeToFull, sizeof(StoreEE.iAvgTimeToFull));
    PowerDevice.setFeature(HID_PD_AVERAGETIME2EMPTY, &StoreEE.iAvgTimeToEmpty, sizeof(StoreEE.iAvgTimeToEmpty));
    PowerDevice.setFeature(HID_PD_REMAINTIMELIMIT,   &StoreEE.iRemainTimeLimit, sizeof(StoreEE.iRemainTimeLimit));
    PowerDevice.setFeature(HID_PD_DELAYBE4REBOOT,    &iDelayBe4Reboot, sizeof(iDelayBe4Reboot));
    PowerDevice.setFeature(HID_PD_DELAYBE4SHUTDOWN,  &iDelayBe4ShutDown, sizeof(iDelayBe4ShutDown));

    PowerDevice.setFeature(HID_PD_RECHARGEABLE, &bRechargable, sizeof(bRechargable));
    PowerDevice.setFeature(HID_PD_CAPACITYMODE, &bCapacityMode, sizeof(bCapacityMode));
    PowerDevice.setFeature(HID_PD_CONFIGVOLTAGE, &iConfigVoltage, sizeof(iConfigVoltage));
    PowerDevice.setFeature(HID_PD_VOLTAGE, &iVoltage, sizeof(iVoltage));

    PowerDevice.setStringFeature(HID_PD_IDEVICECHEMISTRY, &bDeviceChemistry, STRING_DEVICECHEMISTRY);
    PowerDevice.setStringFeature(HID_PD_IOEMINFORMATION, &bOEMVendor, STRING_OEMVENDOR);

    PowerDevice.setFeature(HID_PD_AUDIBLEALARMCTRL, &iAudibleAlarmCtrl, sizeof(iAudibleAlarmCtrl));

    PowerDevice.setFeature(HID_PD_DESIGNCAPACITY, &iDesignCapacity, sizeof(iDesignCapacity));
    PowerDevice.setFeature(HID_PD_FULLCHRGECAPACITY, &iFullChargeCapacity, sizeof(iFullChargeCapacity));
    PowerDevice.setFeature(HID_PD_REMAININGCAPACITY, &iRemaining, sizeof(iRemaining));
    PowerDevice.setFeature(HID_PD_WARNCAPACITYLIMIT, &StoreEE.iWarnCapacityLimit, sizeof(StoreEE.iWarnCapacityLimit));
    PowerDevice.setFeature(HID_PD_REMNCAPACITYLIMIT, &StoreEE.iRemnCapacityLimit, sizeof(StoreEE.iRemnCapacityLimit));
    PowerDevice.setFeature(HID_PD_CPCTYGRANULARITY1, &bCapacityGranularity1, sizeof(bCapacityGranularity1));
    PowerDevice.setFeature(HID_PD_CPCTYGRANULARITY2, &bCapacityGranularity2, sizeof(bCapacityGranularity2));

    batVoltage = StoreEE.batFullVoltage;

    if (true)
    {
        bool bCharging = true;
        bool bDischarging = !bCharging; // TODO - replace with sensor

        // Send initial report with everything looking good, update in main loop
        PowerDevice.sendReport(HID_PD_REMAININGCAPACITY, &iRemaining, sizeof(iRemaining));
        if (bDischarging) PowerDevice.sendReport(HID_PD_RUNTIMETOEMPTY, &iRunTimeToEmpty, sizeof(iRunTimeToEmpty));
        iRes = PowerDevice.sendReport(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));
        DBPRINTLN("Sending initial all-good report. iRemaining: " + String(iRemaining) + ", Status Bits: 0x" + String(iPresentStatus, HEX));
    }

    statusLedTimerOn.Start(10);
}


Timer_ms timeToUpdate;

void loop(void)
{
    WATCHDOG_RESET; // Reset watchdog frequently

    handleLaptopInput();

    if (timeToUpdate.StartIfStopped(1000))
    {
        //*********** Measurements Unit ****************************
        bool bCharging = digitalRead(PIN_INPUT_PWR_FAIL);
        //bool bForceShutdown = digitalRead(6) ? false : true;  // Sid added for temperary test
        bool bACPresent = bCharging;    // TODO - replace with sensor
        bool bDischarging = !bCharging; // TODO - replace with sensor
        int iA7 = analogRead(PIN_BATTERY_VOLTAGE);       // TODO - this is for debug only. Replace with charge estimation

        //if (true)
        //{
        batVoltage = map(iA7, StoreEE.calibPointLow.a2dValue, StoreEE.calibPointHigh.a2dValue, StoreEE.calibPointLow.voltage, StoreEE.calibPointHigh.voltage);
        if (batVoltage < 0)
            batVoltage = 0;

        // If not enabled: Disable all updates. It seems like just suppressing the sending of reports to the PC isn't enough to 
        // keep the PC from shutting down when a low voltage is sensed. Interrupt driven something somewhere??
        // Initially set to disabled to allow for the board to be callibrated, including a zero or low voltage.
        if (StoreEE.msgPcEnabled)   
        {

            int iRemainingInt = map(batVoltage, StoreEE.batEmptyVoltage, StoreEE.batFullVoltage, 0, 100);
            iRemaining = constrain(iRemainingInt, 0, 100);
            //}
            //else
            //{
            //    iRemaining = (byte)(round((float)100 * iA7 / 1024));
            //}

            iRunTimeToEmpty = (uint16_t)round((float)StoreEE.iAvgTimeToEmpty * iRemaining / 100);

            // Charging
            if (bCharging) bitSet(iPresentStatus, PRESENTSTATUS_CHARGING);
            else bitClear(iPresentStatus, PRESENTSTATUS_CHARGING);
            if (bACPresent) bitSet(iPresentStatus, PRESENTSTATUS_ACPRESENT);
            else bitClear(iPresentStatus, PRESENTSTATUS_ACPRESENT);
            if (iRemaining == iFullChargeCapacity) bitSet(iPresentStatus, PRESENTSTATUS_FULLCHARGE);
            else bitClear(iPresentStatus, PRESENTSTATUS_FULLCHARGE);

            // Discharging
            if (bDischarging)
            {
                bitSet(iPresentStatus, PRESENTSTATUS_DISCHARGING);
                // if(iRemaining < iRemnCapacityLimit) bitSet(iPresentStatus,PRESENTSTATUS_BELOWRCL);

                if (iRunTimeToEmpty < StoreEE.iRemainTimeLimit) bitSet(iPresentStatus, PRESENTSTATUS_RTLEXPIRED);
                else bitClear(iPresentStatus, PRESENTSTATUS_RTLEXPIRED);

            }
            else
            {
                bitClear(iPresentStatus, PRESENTSTATUS_DISCHARGING);
                bitClear(iPresentStatus, PRESENTSTATUS_RTLEXPIRED);
            }

            // Shutdown requested
            if (iDelayBe4ShutDown > 0)
            {
                bitSet(iPresentStatus, PRESENTSTATUS_SHUTDOWNREQ);
                DBPRINTLN("shutdown requested");
            }
            else bitClear(iPresentStatus, PRESENTSTATUS_SHUTDOWNREQ);

            //if (bForceShutdown)
            //{
            //    bitSet(iPresentStatus, PRESENTSTATUS_SHUTDOWNREQ);
            //}

            // Shutdown imminent
            if ((iPresentStatus & (1 << PRESENTSTATUS_SHUTDOWNREQ)) ||
                (iPresentStatus & (1 << PRESENTSTATUS_RTLEXPIRED)))
            {
                bitSet(iPresentStatus, PRESENTSTATUS_SHUTDOWNIMNT);
                DBPRINTLN("shutdown imminent");
            }
            else bitClear(iPresentStatus, PRESENTSTATUS_SHUTDOWNIMNT);



            bitSet(iPresentStatus, PRESENTSTATUS_BATTPRESENT);




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

            if ((iPresentStatus != iPreviousStatus) || (iRemaining != iPrevRemaining) || (iRunTimeToEmpty != iPrevRunTimeToEmpty) || (iIntTimer > MINUPDATEINTERVAL)
               /* || (bForceShutdownPrior != bForceShutdown) */
               )
            {

                //if (false)
                //if (StoreEE.msgPcEnabled)
                //{
                    PowerDevice.sendReport(HID_PD_REMAININGCAPACITY, &iRemaining, sizeof(iRemaining));
                    if (bDischarging) PowerDevice.sendReport(HID_PD_RUNTIMETOEMPTY, &iRunTimeToEmpty, sizeof(iRunTimeToEmpty));
                    iRes = PowerDevice.sendReport(HID_PD_PRESENTSTATUS, &iPresentStatus, sizeof(iPresentStatus));
                //}

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

            }
        }//end if (StoreEE.msgPcEnabled)
        else
        {
            status_HID = stat_Disabled;
        }

        if (StoreEE.msgPcEnabled)
        {
            DBPRINT("Remaining: ");
            DBPRINT(iRemaining);
            DBPRINT(", To Empty mm:ss ");
            DBPRINT(iRunTimeToEmpty / 60);
            DBPRINT(":");
            DBPRINT(iRunTimeToEmpty % 60);
            DBPRINT(", Comm with PC (negative=bad): ");
            DBPRINT(iRes);
            DBPRINT(", ");
        }
        doDebugPrints = true;
        DBPRINT("BatV*100: ");
        DBPRINTLN(batVoltage);
        SERIALPORT.print("BatV*100: ");  // DBC.008
        SERIALPORT.println(batVoltage);  // DBC.008
        #ifdef SERIAL1_DEBUG                         // DBC.008f   
        Serial1.print("USBCDCNeeded: ");  // DBC.008b
        Serial1.print(USBCDCNeeded);      // DBC.008b
        Serial1.print("  AskedForCDC: "); // DBC.008b
        Serial1.println(AskedForCDC);     // DBC.008b
        for (int u=0; u<6; u++) {                    // DBC.008c
          Serial1.print("USBSwitchTime[");           // DBC.008c
          Serial1.print(u);                          // DBC.008c
          Serial1.print("]: ");                      // DBC.008c
          Serial1.print(USBSwitchTime[u]);           // DBC.008c
          Serial1.print("\t\tUSBSwitchCount[");        // DBC.008d
          Serial1.print(u);                          // DBC.008d
          Serial1.print("]: ");                      // DBC.008d
          Serial1.println(USBSwitchCount[u]);        // DBC.008d
        }                                            // DBC.008c
        
        Serial1.flush();
        #endif                                       // DBC.008f
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

    while (SERIALPORT.available())     // DBC.007
    {
        inByte = SERIALPORT.read();    // DBC.007
        if (doDebugPrints)
        {
            SERIALPORT.write(inByte);  // DBC.007
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
                processUserInput(userIn, indexUserIn, inByte, &SERIALPORT, true);  // DBC.007
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
            DBPRINT("\nCmd: ");
        }
    } // end if (SERIALPORT.available())

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
                switch ((char)toupper(userIn[1]))
                {
                case 'N': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 1, StoreEE.msgPcEnabled , "Enable PC Reporting/Shutdown"); break;
                default:
                    serialPtr->print("ERROR: Bad B (Battery) command: ");
                    serialPtr->println(userIn);
                    break;
                }
            }
            break;
        
        case 'B':   // Battery Config
            {
                switch ((char)toupper(userIn[1]))
                {
                case 'F': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 7000, StoreEE.batFullVoltage , "Battery Full Charge V"); break;
                case 'E': MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 7000, StoreEE.batEmptyVoltage, "Battery Empty V"      ); break;
                default:
                    serialPtr->print("ERROR: Bad B (Battery) command: ");
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
                        MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, minsTemp, "Minutes to fully charge from dead"   ); 
                        StoreEE.iAvgTimeToFull = minsTemp * 60;
                    }
                    break;

                case 'D': 
                    {
                        uint16_t minsTemp = StoreEE.iAvgTimeToEmpty / 60;
                        MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, minsTemp, "Minutes to fully discharge from full"); 
                        StoreEE.iAvgTimeToEmpty = minsTemp * 60;
                    }
                    break;

                default:
                    serialPtr->print("ERROR: Bad Time command: ");
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
                        if (MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, valVin100s , "V*100 of low voltage"   ))
                        {
                            StoreEE.calibPointLow.voltage = valVin100s;
                            StoreEE.calibPointLow.a2dValue = MH.anaFilter_Mid(PIN_BATTERY_VOLTAGE);
                        }
                    }
                    break;
                case 'H':
                    {
                        uint16_t valVin100s = StoreEE.calibPointHigh.voltage;
                        if (MH.updateFromUserInput(userIn+1, indexUserIn, inByte, 8000, valVin100s , "V*100 of higher voltage"))
                        {
                            StoreEE.calibPointHigh.voltage = valVin100s;
                            StoreEE.calibPointHigh.a2dValue = MH.anaFilter_Mid(PIN_BATTERY_VOLTAGE);
                        }
                    }
                    break;


                default:
                    serialPtr->print("ERROR: Bad B (Battery) command: ");
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

                case 'Q':
                    break;

                default:
                      printHelp(serialPtr, handleUsbOnlyOptions);  // DBC.007
                    break;                                         // DBC.007
                }
            }
            break;

        case 'S':
            {
                EEPROM.put(0, StoreEE);     // Write to EEPROM at users command
                serialPtr->println(F("\nSaved new value(s) to EEPROM.\n"));
            }
            break;

        case 'Z':
            {
                FactoryDefault();       // Reset color and air pump settings
                serialPtr->println(F("\nRestored Factory Defaults...\r\n Must do 'S' to save to EEPROM (non-volatile memory)."));
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
                MH.updateFromUserInput(userIn, indexUserIn, inByte, 0xFF, StoreEE.debugFlags, "Debug Flags (1:Main)", true);
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
              //serialPtr->print(F("HID_INTERFACE: "));      // DBC.008
              //serialPtr->println(HID_INTERFACE);           // DBC.008 SLR HID_INTERFACE wasn't defined
            }                                              // DBC.008
            break;                                         // DBC.008

        default:
            serialPtr->print(F("\nUnknown command ["));
            serialPtr->print(userIn);
            serialPtr->println("].");
            serialPtr->println("H for Help\n");
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
        serialPtr->println("\nCompiled at: " __DATE__ ", " __TIME__);
        serialPtr->println();

        serialPtr->println(F("Present Settings:"));
        serialPtr->print(F("ENn    - ENable reporting/Shutdown to to the PC: ")); serialPtr->println(StoreEE.msgPcEnabled );
        serialPtr->print(F("         (0=No PC reporting/shutdown. 1=Enabled")); serialPtr->println( );
        serialPtr->print(F("BFnnnn - Battery Full Charge voltage    (V*100): ")); serialPtr->println(StoreEE.batFullVoltage );
        serialPtr->print(F("BEnnnn - Battery Empty voltage          (V*100): ")); serialPtr->println(StoreEE.batEmptyVoltage);
        serialPtr->print(F("TCnnnn - Average Time to fully Charge (minutes): ")); serialPtr->println(StoreEE.iAvgTimeToFull/60  );
        serialPtr->print(F("TDnnnn - Ave. Time for full Discharge (minutes): ")); serialPtr->println(StoreEE.iAvgTimeToEmpty/60 );
        serialPtr->print(F("CLnnnn - Calibrate low voltage point (V*100)   : ")); serialPtr->print(StoreEE.calibPointLow.voltage); serialPtr->println(F(" V*100"));
        serialPtr->print(F("         (Low V A2D value: "));                       serialPtr->print(StoreEE.calibPointLow.a2dValue); serialPtr->println(F(")"));
        serialPtr->print(F("CHnnnn - Calibrate high voltage point (V*100)  : ")); serialPtr->print(StoreEE.calibPointHigh.voltage); serialPtr->println(F(" V*100"));
        serialPtr->print(F("         (High V A2D value: "));                      serialPtr->print(StoreEE.calibPointHigh.a2dValue); serialPtr->println(F(")"));
        serialPtr->println(F(" To calibrate voltage sensing for this board:"));
        serialPtr->println(F("  Disconnect the battery voltage sense leads from the battery.  "));
        serialPtr->println(F("  Short the together. Enter the command CL0 (Zero, not Oh)."));
        serialPtr->println(F("  Hook the leads back up to the battery. Measure the voltage"));
        serialPtr->println(F("  with a good digital volt meter. Enter the command CHvvvv."));
        serialPtr->println(F("  where vvvv is the measured voltage in hundreths of a volt."));
        serialPtr->println(F("  (If you measured 12.45 volts, enter CH1245)"));
        serialPtr->println(F("  Voltages will always be entered in hundredths. "));
        serialPtr->println(F("  (V*100)"));

        serialPtr->print(F("Dx   - Debug flags              : 0x")); serialPtr->println(StoreEE.debugFlags, HEX);
        serialPtr->print(F("Battery Voltage*100: ")); serialPtr->println(batVoltage);


        serialPtr->println(F("   1:dbg "));

    }


void printHelp(Stream * serialPtr, bool handleUsbOnlyOptions)
{
    serialPtr->println(F("\n----Menu----"));
    showPersistentSettings(serialPtr, handleUsbOnlyOptions); // DBC.007
    serialPtr->println(F(""));
}


void FactoryDefault(void)
{
    //StoreEE.debugFlags        =  0;  // No debug prints by default  // DBC.007
    StoreEE.debugFlags        =  1;  // Debug prints by default       // DBC.007
    StoreEE.eeValid_1         = EEPROM_VALID_PAT1;      // Set sig in case user stores config to EEPROM.

    StoreEE.eeValid_2         = EEPROM_VALID_PAT2;
    StoreEE.eeVersion         = EEPROM_END_VER_SIG;


    // Physical parameters
    StoreEE.iAvgTimeToFull   = 120*60;
    StoreEE.iAvgTimeToEmpty  = 120*60;
    StoreEE.iRemainTimeLimit =  10*60;
    StoreEE.batFullVoltage   = 1380;   // Voltage in hundredths, Bat Full
    StoreEE.batEmptyVoltage  = 1150;   // Voltage in hundredths, Bat Empty

    StoreEE.calibPointLow.voltage   = 0;
    StoreEE.calibPointLow.a2dValue  = 0;
    StoreEE.calibPointHigh.voltage  = 1400;
    StoreEE.calibPointHigh.a2dValue = 100;  // This will definetely need to be calibrated!

    // Parameters for ACPI compliancy
    StoreEE.iWarnCapacityLimit = 10; // warning at 10%
    StoreEE.iRemnCapacityLimit = 5; // low at 5%
    StoreEE.msgPcEnabled = false;

    DBPRINTLN(F("Restored factory defaults."));
}


void enableDebugPrints(uint8_t debugPrBits)
{
    doDebugPrints      = bitRead(debugPrBits, DBG_PRINT_MAIN     ) ? true : false;   // 0    // 0x01 Turn off debug printing for this file
}
