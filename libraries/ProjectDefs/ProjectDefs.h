#ifndef PROJECTDEFS_H
#define PROJECTDEFS_H

#include <stdint.h>
#include <Arduino.h>
#include <EEPROM.h>     

//#define SERIAL1_DEBUG       true    // Prints some USB connection info
//#define SERIAL1_IRQ_DEBUG   true    // IRQ code prints via a buffer which gets printed in loop()
//
#if SERIAL1_IRQ_DEBUG
    #define SIZE_USBDebug 400
    extern char USBDebug[SIZE_USBDebug];          // DBC.009  SLR: Reduced size as RAM getting tight
#endif

extern bool USBCDCNeeded;  // DBC.008b
extern uint8_t pcSetErrorCount;         // How many times the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT value
extern uint8_t pcSetValue;     // The value that the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT to

/////////////////////////////////////
//  E E P R O M   S t r u c t u r e 
/////////////////////////////////////

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
    VtoCapacity VtoCapacitiesHiDisch[NUM_CAPACITY_POINTS];     // Points on discharge curve for interpolating capacity
    VtoCapacity VtoCapacitiesLoDisch[NUM_CAPACITY_POINTS];     // Points on discharge curve for interpolating capacity
    VtoCapacity VtoCapacitiesChrg[NUM_CAPACITY_POINTS];     // Points on discharge curve for interpolating capacity
    uint16_t    batFullVoltage;                         // in hundredths of volts
    uint16_t    warningVoltage;                        // in hundredths of volts    (Never used)
    uint16_t    shutdownVoltage;                        // in hundredths of volts
    uint16_t    isChargingVolts;                        // in hundredths of volts: Above this v, must be charging
    uint16_t    isDisChargingVolts;                     // " " ": Below this v, must be discharging
    uint16_t    iCalcdTimeToEmpty;                      // Runtime calculated time to empty from full
    uint16_t    hiRateDichargeMins;                     // Minutes to discharge at High rate
    uint16_t    loRateDichargeMins;                     // Minutes to discharge at Low rate
    uint8_t     timeToEmptyCalcState;                   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
};

enum BatteryChemistryType 
{
    BC_LeadAcid = 0,
    BC_LI_ION   = 1,
    BC_LFP      = 2,
    BC_AGM      = 3,
    BC_Other    = 4,
    BC_Last     = BC_Other
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
    uint16_t              reportIntervalSecs;   // in seconds
    BatteryChemistryType  BatChem;              // Lead Acid, LFP, AGM, Other
    uint8_t               battSysVMultiplier;   // 1/2/3/4 for 12, 24, 36, 48 (V must be divisible by 12v)
    uint8_t               reportingHysteresis;  // mV diff before reporting/printing a change

    // Parameters for ACPI compliancy
    byte iWarnCapacityLimit;    // warning at 10%
    byte reserved8_5;           // Offset the byte below
    byte iRemnCapacityLimit;    // low at 5%: Shutdown

    byte msgPcEnabledCfgMode;   // Host (PC/NAS) to be updated with real battery status in Config Mode
    byte msgPcEnabledRunMode;   // ... in Normal Run mode (CDC Disabled)
    //bool msgPcEnabledCfgMode;   // Host (PC/NAS) to be updated with real battery status in Config Mode
    //bool msgPcEnabledRunMode;   // ... in Normal Run mode (CDC Disabled)

    // Board/Resistor Divider Calibration
    CalibPoint  calibPointLow;  // Note A2D reading and associated voltage at two points. "map" from there
    CalibPoint  calibPointHigh;

    // Battery characteristics by chemistry.
    BatteryParams   BattParams[NUM_BATT_TYPES];     // For all bat chemistries: Capacity curve, Definetely charging V, Discharging V
    VtoCapacity     VtoCapacitiesUser[NUM_CAPACITY_POINTS];     // Points on discharge curve for interpolating capacity
    uint16_t        dischRateMinutes;                           // Sys design(actual) capacity in minutes

    uint8_t     debugFlags;  // Debug flags 0x01=Skip Early Audio
    uint16_t    eeVersion;  // Change this if the eeprom layout changes
};

extern EEPROM_Struct   StoreEE;        // User EEPROM & unchanging calibs. May restore from user or factory inmages in EEPROM.


#define EEP_OFFSET_USER     0   // Starting address in EEPROM of normal user saved parameters
#define EEP_OFFSET_FACTORY  400   // Starting address in EEPROM of "Factory Default" parameters

// Prototypes
void FactoryDefault(void);
void FactoryCompiledDefault(void);
void calcUserCapacityCurve(void);
void enableDebugPrints(uint8_t debugPrBits);

void enableDebugPrints(uint8_t debugPrBits);
#define DBG_PRINT_MAIN      0    // 0x01
#define DBG_PRINT_CSV_VOLTS 1    // 0x02

extern bool doDebugPrints;
extern bool doVoltageGraphOutput;  // true for printing voltage CSV style for graphing


#endif //PROJECTDEFS_H
