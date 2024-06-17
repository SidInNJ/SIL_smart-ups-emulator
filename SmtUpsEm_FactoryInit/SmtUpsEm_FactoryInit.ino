/*
 * Filename:  SmtUpsEm_FactoryInit.ino
 * 
 *   Use for initial programming of the EEPROM with factory defaults.
 *      Load this program, execute, then load the normal runtime program: SmartUpsEmulator.ino.
 *      We don't have enough flash space to have this functionallity in SmartUpsEmulator.ino
 *      so it has been moved to here.
 *      
 * 
 */

#include "ProjectDefs.h"	// For our Smart UPS Emulator project. Defs SERIAL1_DEBUG
#include "HandyHelpers.h"

EEPROM_Struct   StoreEE;        // User EEPROM & unchanging calibs. May restore from user or factory inmages in EEPROM.


// Below used by the dump function and EEPromRestore.ino
// In case not defined in this project's HandyHelpers.h
#ifndef EEPROM_END_ADDR
    #if defined(__AVR_ATmega2560__) || defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
        #define EEPROM_END_ADDR 0x1000  // 4k (actual size of EEPROM) of Mega 2560, Teensy 3.5, Teensy 3.6
    #elif defined(ARDUINO_TEENSY41)
        #define EEPROM_END_ADDR (4284)   // Teensy 4.1 4284 (actual size of emulated EEPROM)
    #elif defined(ARDUINO_TEENSY32)
        #define EEPROM_END_ADDR 0x800   // Teensy 3.2 2k (actual size of EEPROM)
    #elif defined(ARDUINO_TEENSY40)
        #define EEPROM_END_ADDR (1080)   // Teensy 4.0 1080 (actual size of emulated EEPROM)
    #elif defined(__AVR_ATmega328P__)    // Leonardo
        #define EEPROM_END_ADDR 0x400   // 1k (actual size of EEPROM)
        //#define EEPROM_END_ADDR 0x200   // 512 bytes (Half of actual size of EEPROM), Helps with RAM resources.
    #endif
#endif



uint8_t EEImage[EEPROM_END_ADDR];
//uint8_t EEImageFrEEProm[EEPROM_SIZE];

bool doDebugPrints = true;  // Enable printing by default
bool doVoltageGraphOutput = false;  // true for printing voltage CSV style for graphing

void setup(void)
{
    Serial1.begin(115200);  // Always enable Serial1 in case we enable Serial1 debugging  SLR
    while (!Serial1)
        delay(10);
    Serial.begin(115200);

    delay(2000);

    Serial.println("This will fill the EEPROM with FF's, then");
    Serial.println("initialize it with factory default settings.\n\r");
    Serial.println("Filling the EEPROM with FF's...");

    Serial1.println("This will fill the EEPROM with FF's, then");
    Serial1.println("initialize it with factory default settings.\n\r");
    Serial1.println("Filling the EEPROM with FF's...");

    Serial1.print(F("Size of EEPROM: "));
    Serial1.println(EEPROM_END_ADDR);
    Serial.print(F("Size of EEPROM: "));
    Serial.println(EEPROM_END_ADDR);

    delay(1000);

    for (uint16_t i=0; i<EEPROM_END_ADDR; i++)
    {
        EEImage[i] = 0xFF;  // Fill image to "factory fresh" state
    }

    Serial.println(F("Writing EEPROM..."));
    Serial1.println(F("Writing EEPROM..."));
    EEPROM.put(0, EEImage);     // We have a good image, write it!
    Serial.println(F("EEPROM Written with all FF!"));
    Serial1.println(F("EEPROM Written with all FF!"));


    Serial.println(F("Now writing with Compiled Factory Defaults."));
    Serial1.println(F("Now writing with Compiled Factory Defaults."));
    FactoryCompiledDefault();
    EEPROM.put(EEP_OFFSET_FACTORY, StoreEE);    // Write to "Factory Default" EEPROM 
    EEPROM.put(EEP_OFFSET_USER, StoreEE);       // Write to "User" EEPROM 
    FactoryDefault();

}

void loop(void)
{
    Serial.println(F("Finished! Ready for you to load SmartUpsEmulator.ino!"));
    Serial1.println(F("Finished! Ready for you to load SmartUpsEmulator.ino!"));
    delay(2000);
}

