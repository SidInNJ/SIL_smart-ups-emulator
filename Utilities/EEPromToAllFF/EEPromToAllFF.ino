#include "HandyHelpers.h"

//// For the EEProm functions 
// 
// Below now defined in Maltbie_Helper.h
//  
//#if defined(__AVR_ATmega2560__)
//#define uP_EEPROM_SIZE 0x1000  // 4k  (Actual, but most released with it set to 2k)
//#endif
//
//#if defined(__AVR_ATmega328P__)
//#define uP_EEPROM_SIZE 0x400   // 1k (actual size of EEPROM)
////#define uP_EEPROM_SIZE 0x200   // 512 - usable size if compile problems
//#endif
//

// Below used by the dump function and EEPromRestore.ino
// In case not defined in this project's Maltbie_Helper.h
#ifndef EEPROM_END_ADDR
    #if defined(__AVR_ATmega2560__) || defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
        #define EEPROM_END_ADDR 0x1000  // 4k (actual size of EEPROM) of Mega 2560, Teensy 3.5, Teensy 3.6
    #elif defined(ARDUINO_TEENSY41)
        #define EEPROM_END_ADDR (4284)   // Teensy 4.1 4284 (actual size of emulated EEPROM)
    #elif defined(ARDUINO_TEENSY32)
        #define EEPROM_END_ADDR 0x800   // Teensy 3.2 2k (actual size of EEPROM)
    #elif defined(ARDUINO_TEENSY40)
        #define EEPROM_END_ADDR (1080)   // Teensy 4.0 1080 (actual size of emulated EEPROM)
    #elif defined(__AVR_ATmega328P__)
        //#define EEPROM_END_ADDR 0x400   // 1k (actual size of EEPROM)
        #define EEPROM_END_ADDR 0x200   // 512 bytes (Half of actual size of EEPROM), Helps with RAM resources.
    #endif
#endif



uint8_t EEImage[EEPROM_END_ADDR];
//uint8_t EEImageFrEEProm[EEPROM_SIZE];

void setup()
{
    // initialize the serial port:
    Serial.begin(115200);
    delay(100);
    Serial.println(F("Starting..., delay 1 second..."));
    Serial.print(F("Size of EEPROM: "));
    Serial.println(EEPROM_END_ADDR);

    Serial1.begin(115200);
    delay(100);
    Serial1.println(F("Starting..., delay 1 second..."));
    Serial1.print(F("Size of EEPROM: "));
    Serial1.println(EEPROM_END_ADDR);

    delay(1000);

    for (uint16_t i=0; i<EEPROM_END_ADDR; i++)
    {
        EEImage[i] = 0xFF;  // Fill image to "factory fresh" state
    }

    Serial.println(F("Writing EEPROM..."));
    EEPROM.put(0, EEImage);     // We have a good image, write it!
    Serial.println(F("EEPROM Written with all FF!"));
}

void loop() 
{
}

//Show EEPROM contents. If captured to a file, the file may be written to the EEPROM of a new Arduino using EEPromRestore.ino and RealTerm https://realterm.sourceforge.io/
//void dumpEEImage(void)
//{
//    uint8_t i, c;
//    uint16_t addr = 0;
//
//    Serial.print(F("\r\nEEPROM Image received:"));
//
//    do
//    { /* ------ Print the address padded with spaces ------ */
//        Serial.print(F("\r\n$ "));    // Start lines with "$ "
//        if (addr<0x1000)
//        {
//            Serial.print("0");
//        }
//        if (addr<0x100)
//        {
//            Serial.print("0");
//        }
//        if (addr<0x10)
//        {
//            Serial.print("0");
//        }
//        Serial.print (addr, HEX);
//        /*------- Print the Hex Values in groups of 16 ---------*/
//        for (i=0; i<16; ++i)
//        {
//            Serial.print (" ");
//            c=EEImage[addr];
//            if (c < 0x10)
//            {
//                Serial.print("0");
//            }
//            Serial.print (c, HEX);
//            ++addr;
//        }
//    } while (addr < uP_EEPROM_SIZE);   // E2END is the end of the EEPROM
//
//    Serial.print ("\r\n");
//}
