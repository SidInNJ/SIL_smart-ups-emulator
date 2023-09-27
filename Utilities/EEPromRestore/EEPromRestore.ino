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
    delay(2000);

#if defined(TEENSYDUINO)
    Serial.println("Restore EEPROM");   // Baud rates not used by Teensy
    Serial.flush();
#else
    Serial.println("Restore EEPROM - Switching baud rate to 57600 (default of RealTerm).\nYou should switch now too!\r\n\r\n\r\n");

    Serial.flush();
    delay(200);

    Serial.begin(57600);
    Serial.flush();
    delay(200);
    Serial.println("\r\nRestore EEPROM - Now at 57600 baud");
    Serial.println("Compiled at: " __DATE__ ", " __TIME__);
    
    // flush old stuff
    while (Serial.available())
    {
        Serial.read();
    }
#endif

    Serial.println("Transmit file now.");

    for (uint16_t i=0; i<EEPROM_END_ADDR; i++)
    {
        EEImage[i] = 0xFF;  // Fill image to "factory fresh" state
    }
}

enum SerState {
    StIdle
    ,StProcessing
    ,StCompleted
};

#define MAXCHARS 70
char inLine[MAXCHARS];

void loop() 
{
    static SerState state     = StIdle;
    static uint8_t  chIndex   = 0;
    static uint16_t eeAddress = 0;
    char * ptr;

    if (chIndex >= (MAXCHARS-2))
    {
        chIndex = 0;
        state   = StIdle;
    }

    if (Serial.available())
    {
        char ch = Serial.read();

        if ((ch!=ASCII_LF) && (ch!=ASCII_CR))
        {
            inLine[chIndex++] = ch;
        }
        else
        {
            inLine[chIndex] = 0;

            // We have a complete line (or an extra line termination)
            if (chIndex > 1)
            {
                if (chIndex<6)
                {
                    Serial.print(F("Short line received, aborting. Text Received: "));
                    Serial.println(inLine);
                    state = StCompleted;
                }
                // Process line
                switch (state)
                {
                case StIdle:
                    {
                        // Expect: "$$-EEPROM CONTENTS:"
                        //          01234567890
                        if ((inLine[0]=='$') && (inLine[1]=='$') && (inLine[10]=='C'))
                        {
                            state = StProcessing;
                        }
                    }
                    break;

                case StProcessing:
                    {
                        ptr = 0;

                        if ((inLine[0]=='$') && (inLine[1]==' '))
                        {
                            // A line with data, format: "$ 0000 C3 F5 48 40 FF FF FF FF FF FF FF FF FF FF FF FF"
                            eeAddress = strtol(&inLine[2], &ptr, 16);    // Get starting address of this line; ptr set to char after address

                            while ((*ptr == ' ') && (eeAddress < EEPROM_END_ADDR))
                            {
                                EEImage[eeAddress] = strtol(ptr, &ptr, 16);
                                eeAddress++;
                            }
                        }
                        else if ((inLine[0]=='$') && (inLine[1]=='$') && (inLine[4]=='N')) // Expect "$$-END EEPROM"
                        {
                            state = StCompleted;
                            // Write to EEPROM here!
                            Serial.println(F("Writing EEPROM..."));
                            EEPROM.put(0, EEImage);     // We have a good image, write it!
                            Serial.println(F("EEPROM Written!"));
                            //EEPROM.get(0, EEImageFrEEProm);     // Fetch the structure just written to EEPROM for compare

                            //EEImageFrEEProm[0x101] = 0xEE;  // Induce error for DEBUG DOYET

                            bool fail=false;
                            for (eeAddress=0; eeAddress<EEPROM_END_ADDR; eeAddress++)
                            {
                                uint8_t eeByte;
                                EEPROM.get(eeAddress, eeByte);     // Fetch the next byte from EEPROM (just written to) for compare

                                if(eeByte != EEImage[eeAddress])
                                {
                                    fail = true;
                                    Serial.println("ERROR: Mismatch of written and read-back at EEPROM address 0x" + String(eeAddress, HEX) 
                                                   + ", \nExpected 0x" + String(EEImage[eeAddress], HEX) + ", Actual 0x" + String(eeByte, HEX));
                                    break;
                                }

                            }

                            if (fail==false)
                            {
                                Serial.println(F("\r\nEEProm verified against received data!"));
                            }
                            
                            // DEBUG: Show what was received:
                            dumpEEProm();   // Just show 1st line and any with non-0xFF bytes
                            //dumpEEImage();    // Showed all lines of image

                            if (fail==true)
                            {
                                Serial.println(F("\r\nERROR: Mismatch of written and read-back of EEPROM!\r"));
                            }
                        }
                        else
                        {
                            state = StCompleted;
                            Serial.print(F("Error in data! Aborting!\nText Received: "));
                            Serial.println(inLine);
                        }
                    }
                    break;

                case StCompleted:
                    {
                        Serial.println(F("Extra line after data, ignoring:"));
                        Serial.println(inLine);
                    }
                    break;

                default:
                    {
                        Serial.println(F("Illegal state!"));
                        state = StCompleted;
                    }
                    break;
                }

            }

            chIndex = 0;
        }
    }

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
