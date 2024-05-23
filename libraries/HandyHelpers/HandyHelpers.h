// Helper Functions

#ifndef HANDY_HELPERS_H
#define HANDY_HELPERS_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include <Wire.h>

//#include <SoftwareSerial.h>   // Removed for PurpleV.ino (Landor)
#include <IPAddress.h>
#include <EEPROM.h>     // for dumpEEProm()

extern bool USBCDCNeeded;           // Set in main()
extern bool SerialIsInitialized;    // Set in SmartUpsEmulator.ino

#ifdef CDC_ENABLED
    #define SERIALPORT ((USBCDCNeeded && SerialIsInitialized) ? Serial : Serial1)   // Using USB Serial
    #define SERIALPORT_Addr ((USBCDCNeeded && SerialIsInitialized) ? (Stream *)&Serial : (Stream *)&Serial1)
    #define SERIALPORT_PRINTLN(args...) {if(USBCDCNeeded && SerialIsInitialized) Serial.println(args); else Serial1.println(args); }
    #define SERIALPORT_PRINT(args...) {if(USBCDCNeeded && SerialIsInitialized) Serial.print(args); else Serial1.print(args); }
    #define SERIALPORT_AVAILABLE() ((USBCDCNeeded && SerialIsInitialized) ? Serial.available() : Serial1.available())
    #define SERIALPORT_READ() ((USBCDCNeeded && SerialIsInitialized) ? Serial.read() : Serial1.read())
    #define SERIALPORT_WRITE(a) ((USBCDCNeeded && SerialIsInitialized) ? Serial.write(a) : Serial1.write(a))
#else
    #define SERIALPORT Serial1  // Using pins 0/1 for Serial1 Rx/Tx
    #define SERIALPORT_Addr             (Stream *)&Serial1
    #define SERIALPORT_PRINTLN(args...) Serial1.println(args)
    #define SERIALPORT_PRINT(args...)   Serial1.print(args) 
    #define SERIALPORT_AVAILABLE()      Serial1.available())
    #define SERIALPORT_READ()           Serial1.read())
    #define SERIALPORT_WRITE(a)         Serial1.write(a))
#endif

// Common ASCII Chars
#define ASCII_STX           0x02
#define ASCII_CR            0x0D
#define ASCII_LF            0x0A
#define ASCII_ESC           0x1B

// For the EEProm functions
#if defined(__AVR_ATmega2560__)
//#define EEPROM_SIZE 0x1000  // 4k  (Actual, but most released with it set to 2k)
#define EEPROM_SIZE 0x800   // 2k
#endif

#if defined(__AVR_ATmega328P__)
//#define EEPROM_SIZE 0x2000  // Prior setting, but wrong & won't compile on Uno
//#define EEPROM_SIZE 0x400   // 1k (actual size of EEPROM)
#define EEPROM_SIZE 0x200   // 512 - usable size, able to compile
#endif


// Below used by the dump function and EEPromRestore.ino
#if defined(__AVR_ATmega2560__) || defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
    #define EEPROM_END_ADDR 0x1000  // 4k (actual size of EEPROM) of Mega 2560, Teensy 3.5, Teensy 3.6
#elif defined(ARDUINO_TEENSY41)
    #define EEPROM_END_ADDR (4284)   // Teensy 4.1 4284 (actual size of emulated EEPROM)
#elif defined(ARDUINO_TEENSY32)
    #define EEPROM_END_ADDR 0x800   // Teensy 3.2 2k (actual size of EEPROM)
#elif defined(ARDUINO_TEENSY40)
    #define EEPROM_END_ADDR (1080)   // Teensy 4.0 1080 (actual size of emulated EEPROM)
#elif defined(__AVR_ATmega328P__)
    #define EEPROM_END_ADDR 0x400   // 1k (actual size of EEPROM)
#elif defined(__AVR_ATmega32U4__)
    #define EEPROM_END_ADDR 0x400   // 1k (actual size of EEPROM)
    //#define EEPROM_END_ADDR 0x200   // 512 bytes (Half of actual size of EEPROM), Helps with RAM resources.
#endif

// Use SQRT_MAX_PWM_VALUE when making a squared function lighting curve (simulate an audio taper pot from a linear pot/TOF)
#ifndef SQRT_MAX_PWM_VALUE
    // Resolution of PWM (analogWrite)
    #if defined(ARDUINO_TEENSY41)   // Teensy 4.1
        //#define BITS_PWM  12  // PWM resolution in bits <<< CHANGE Resolution HERE <<<<
        #define BITS_PWM  14U  // PWM resolution in bits <<< CHANGE Resolution HERE <<<< Very Smooth! :-)
        //#define BITS_PWM    13  // PWM resolution in bits <<< CHANGE Resolution HERE <<<< Plenty smooth enough!
    #else
        #define BITS_PWM     8  // Arduino Uno/Mega,...
    #endif
    //
    #define MAX_PWM_VALUE  ((1U<<BITS_PWM)-1U)        // Max value for that many PWM bits
    #if MAX_PWM_VALUE == 255        // 8 bits
        #define SQRT_MAX_PWM_VALUE  (15.968719)     // Square root of max value for that many PWM bits
        #define PWM_IDEAL_FREQ      (36621.09)      // Lower than fastest frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 511      // 9 bits       
        #define SQRT_MAX_PWM_VALUE  (22.605309)     // Square root of 511
        #define PWM_IDEAL_FREQ      (36621.09)      // Lower than fastest frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 1023     // 10 bits      
        #define SQRT_MAX_PWM_VALUE  (31.984371)     // Square root of 1023
        #define PWM_IDEAL_FREQ      (36621.09)      // Lower than fastest frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 2047     // 11 bits      
        #define SQRT_MAX_PWM_VALUE  (45.243784)     // Square root of 2047
        #define PWM_IDEAL_FREQ      (36621.09)      // Lower than fastest frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 4095     // 12 bits      
        #define SQRT_MAX_PWM_VALUE  (63.992187)     // Square root of 4095
        #define PWM_IDEAL_FREQ      (36621.09)      // Lower than fastest frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 8191      // 13 bits     
        #define SQRT_MAX_PWM_VALUE  (90.5041435)    // Square root of 8191
        #define PWM_IDEAL_FREQ      (18310.55)      // Best frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #elif MAX_PWM_VALUE == 16383     // 14 bits     
        #define SQRT_MAX_PWM_VALUE  (127.99609)     // Square root of 16383
        #define PWM_IDEAL_FREQ      (9155.27)       // Best frequency for this many bits for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
    #else
        #pragma message "WARNING: DID NOT DEFINE SQRT_MAX_PWM_VALUE FOR THIS NUMBER OF BITS"
        #undef BITS_PWM
        #undef MAX_PWM_VALUE
        #define BITS_PWM         8  // Default to this
        #define PWM_IDEAL_FREQ      (4482)        // Default frequency for FlexPWM timers: https://www.pjrc.com/teensy/td_pulse.html
        #define MAX_PWM_VALUE  ((1<<BITS_PWM)-1)  // Max value for that many PWM bits
    #endif
#endif //SQRT_MAX_PWM_VALUE

//enum UsersActive
//{
//     UAct_NeitherActive = 1
//    ,UAct_OneActive     = 2
//    ,UAct_BothActive    = 3
//};

/*
W5500 Ethernet Shield by Seeed: http://wiki.seeedstudio.com/W5500_Ethernet_Shield_v1.0/
Sold by Adafruit https://www.adafruit.com/product/2971
and Digikey: 1597-1330-ND
    D4: SD card chip Selection (should'd make any difference if no SD card present.)
    D10: W5200 Chip Selection (Slave Select "SS") Used on Mega2560 by ethernet library too!
    D11: SPI MOSI
    D12: SPI MISO
    D13: SPI SCK

Rogue Robotics rMP3 Player (available at Robot Shop):
The default connections for the Arduino Connector are as follows:

    Serial
      Pin 6: Serial receive on Arduino, transmit on rMP3
      Pin 7: Serial transmit on Arduino, receive on rMP3
        Note: Above assumes using Software Serial on 6 and 7. Their docs say that doesn't work on the Mega and they suggest using
              Rx1 and Tx1 instead on the Mega. 
    SPI (Uno)
      Pin 8: SPI chip select (http://home.roguerobotics.com/CS - active low)
      Pin 10: SPI SS - Slave Select (Slave Select "SS") Used on Mega2560 by ethernet library too!
      Pin 11: SPI MOSI (Master Output, Slave Input)
      Pin 12: SPI MISO (Master Input, Slave Output)
      Pin 13: SPI SCK (SPI Clock)
*/
// Relay shield: Relays 1 & 2, driven by D7 and D6 have screw terminals on the vertical edge of the PCB. Relays 3 & 4's (D5, D4) terminals are on the LOWER horizontal edge. :-(

// Shields, if present use pins:    (* by number means used, - by number means bent out for jumper (above boards will see what's jumped to)
// 0 1 2 3 4 5 6 7 8 9 10 11 12 13 16 17 18 20 21 50 51 52 53 
//                                                             24,25 26-37 <-- Used so far
//         4 5 6 7                                         Relay Shield See note above. Relay1-D7, R2-D6, R3-D5, R4-D4. 
//       3                                                 Servo: Probably needs to be on a PWM output (Uno: 3 5 6 9 10 11)
//       3   5 6     9 10 11                               MOSFET Driver: 3 5 6 9 10 11 (PWM capable pins on Uno)
//             6 7                                         rMP3 D6 D7 (expects Software Serial: Wired over to Mega Tx3/Rx3 on D14/D15)
//                                                         SDA/SCL Relay Driver PCBs (2x)
//         4           10 11 12 13        -or- 50 51 52 53 W5500_Ethernet: 4 10 11 12 13 (D4 for SD card, rest appear on SPI interface for Uno, 50/1/2/3 for Mega)
//                     10                                  W5500_Ethernet: Hardware CONFLICT: This pin is driven when Ethernet is enabled! Pattern changes if cord unplugged.
//                                                           The SeedStudio Ethernet shield expects SS to be on pin 10, not on the SPI specific pins (so pin 10 even on Mega2560)
//         4     7 8                Bluefruit BLE shield: 4:RST, 7:IRQ, 8:CS
//                 8 9 10 11 12 13                         Stepper controller
// 0 1 2                                                   DFRobot RS485 Shield 0/1: Serial (cut pins!) 2: Direction Control for RS485 for Rx
//                                                              Typically jumper: 0to10:Rx from DMX; (not used), 1to11:Tx to DMX
// 0 1 2 3                                                 RS232/485 Shield by Tinysine: HW Rx/Tx D0/D1, SW Rx/Tx D2/D3
//                                       20 21             Mega SDA, SCL
//                                             50 51 52 53 Mega Only: SPI (Bluefruit BLE shield)
//                                                  (shield does Not connect SPI signals to pins 10-13)
//                     10 11 12 13                         UNO Only: SPI (Bluefruit BLE shield)
//     2 3                               18 19 20 21       IRQ Pins for Mega (Uno: only 2, 3)  


// Prototypes:
uint32_t disolveDot(uint32_t colorOld, uint32_t colorNew, uint16_t ratioNew256);
#define DUR_DONE_DEFAULT 2000
uint32_t disolveColor(uint32_t colorOld, uint32_t colorNew, unsigned long startTime, unsigned long dissolveDuration = DUR_DONE_DEFAULT);
uint16_t disolve16BitNum(uint32_t numOld, uint32_t numTarget, unsigned long startTime, unsigned long dissolveDuration); // Disolve (change the value) a 16 bit number over time (convert to 32bit on the way in)
int check_mem(bool doPrint = true) ;     // Display how much memory is available (call after dynamically allocating memory)
void dumpEEProm(Stream *serialPtr=SERIALPORT_Addr);      // Show EEPROM contents. If captured to a file, the file may be written to the EEPROM of a new Arduino using EEPromRestore.ino and RealTerm https://realterm.sourceforge.io/
int uint16Compare(const void *arg1, const void *arg2); // Use for qsort() comparisons of uint16_t's. ie. qsort(arOfEdges, numZones, sizeof(arOfEdges[0]), uint16Compare);
bool isConfigEEPromMismatch(uint16_t addr, uint8_t *configPtr, uint16_t len);
// Convert a linear lighting "curve" to a squared lighting curve
// Take the square root of the linear: 0 stays 0, max value stays max value, curve in between
uint16_t squaredCurve(uint16_t newValLinear);
uint32_t Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = 0);
void toString(char str[], uint16_t num);
void toHexString(char str[], uint16_t num);


// Useful Defines
#define ABS(a) (((a)>=0) ? a : 0-(a))

#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
    #define MAX_MENU_CHARS 90   // Need enough room for "//" comments
#else
#define MAX_MENU_CHARS 30   // Need enough room for IP addresses!
#endif
//#define MAX_MENU_CHARS 16

class HandyHelpers {
public: 
    HandyHelpers(void) { m_serialPtr = SERIALPORT_Addr; }

    uint16_t anaFilter_Mid(uint8_t AnaInChan, uint8_t numSamples = 29); // 29 is good value for numSamples as it takes the time of a full-wave of AC power.
    uint16_t anaFilter_ms(uint8_t AnaInChan, uint16_t ms = 100); // 100ms: 6 full-waves of AC power at 60Hz, 5 at 50Hz.

    uint8_t valueToTableIndex(int32_t val, int16_t table[], uint8_t tblCount);  // Return  index of table member closest to supplied value
    //
    // User input for storing into EEPROM,...
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint16_t &varLocn_16, const __FlashStringHelper *varNameF, bool isHex=false, uint16_t minVal=0)
    {
        String tStr = varNameF;
        return updateFromUserInput(userIn, indexUserIn, inByte, maxAllowableValue, varLocn_16, tStr.c_str(), isHex, minVal);
    }
    // Signed 16-bit int
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, int16_t maxAllowableValue, int16_t &varLocn_i16, const char *varName, bool isHex=false, int16_t minVal=0);
    // Unsigned 16-bit int
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint16_t &varLocn_16, const char *varName, bool isHex=false, uint16_t minVal=0);
    // Unsigned 8-bit int
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint8_t &varLocn_8, const __FlashStringHelper *varNameF, bool isHex=false, uint8_t minVal=0)
    {   
        String tStr = varNameF;
        return updateFromUserInput(userIn, indexUserIn, inByte, maxAllowableValue, varLocn_8, tStr.c_str(), isHex, minVal);
    }
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, uint8_t &varLocn_8, const char *varName, bool isHex=false, uint8_t minVal=0);
    bool updateFromUserInput_8or16(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, bool is8bit, uint8_t &varLocn_8, uint16_t &varLocn_16, const char *varName, bool isHex=false, uint16_t minVal=0);
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, bool &varLocn_bool, const __FlashStringHelper *varNameF)
    {
        String tStr = varNameF;
        return updateFromUserInput(userIn, indexUserIn, inByte, maxAllowableValue, varLocn_bool, tStr.c_str());
    }
    bool updateFromUserInput(char *userIn, uint8_t &indexUserIn, int &inByte, uint16_t maxAllowableValue, bool &varLocn_bool, const char *varName);
    bool updateIndex_16bit(char *userIn, uint8_t &indexUserIn, int &inByte, uint8_t maxIndex, uint16_t maxAllowableValue, uint8_t &memNum_8, uint16_t &varLocn_16, const char *varName);
    void UserIPAddressEntry(char userIn[MAX_MENU_CHARS], uint8_t &indexUserIn, char cmdChar, IPAddress &destIpAddr, uint32_t &eeAddressStorage, uint8_t valStrOffset=1);
    // Translate string of comma/dot/space delimited numbers into a uint32_t (4 byte word)
    // Return true if OK. Supply char string, destination of number, number of delimited fields
    bool updateFromUserInputWFields(char userIn[MAX_MENU_CHARS], uint8_t &indexUserIn, char inByte, uint32_t &eeAddressStorage, const __FlashStringHelper * varNameF, uint8_t numFields=3, char delim=',') // Call with F("name")
    {
        String tStr = varNameF;
        return updateFromUserInputWFields(userIn, indexUserIn, inByte, eeAddressStorage, tStr.c_str(), numFields, delim);
    }
    bool updateFromUserInputWFields(char userIn[MAX_MENU_CHARS], uint8_t &indexUserIn, char inByte, uint32_t &eeAddressStorage, const char *varName, uint8_t numFields=3, char delim=',');                  // Call with "name"
    bool uint32FromStringOfFields(const char *numString,uint32_t &destNum, uint8_t numFields, bool showErrors = true);
    #define MAX_NUMS2CONVERT 7
    uint8_t uint32sFromStringOfFields(const char *numString, uint32_t destNum[MAX_NUMS2CONVERT], bool showErrors = true);
    void printParsedNumber(uint32_t num, uint8_t numFields=3, char delim=',');
    void printParsedNumberEndian(uint32_t num, uint8_t numFields=3, char delim=',');
    void printDivBy10(uint16_t num);    // Divide by 10 and print with tenths: 123 prints as "12.3". Good for storing # accurate to 10ths by multiplying it by 10.
    void printNumPadBlanks(uint16_t num, uint8_t numDigits);
    void printParsedBytes(uint8_t *byteArray, uint8_t numFields, char delim=',', uint8_t base=HEX);
    uint32_t reduceToMaxIntensity(uint32_t newColor, uint16_t maxIntensity);    // Reduce color values so sum does not exceed maxIntensity
    void setSerialOutputStream(Stream *serialPtr = SERIALPORT_Addr) {m_serialPtr = serialPtr;}
    void resetSerialOutputStream(void) { m_serialPtr = SERIALPORT_Addr; }
    Stream* serPtr(void) { return m_serialPtr; }    // use as: MH.serPtr()->print(F("Hi"));

private:
    Stream *m_serialPtr; // Pointer to the Serial output device being used (Serial, Serial3, NeoSWSerial)

};

extern HandyHelpers MH; // My Handy Helper

class AverageRecent
{
public:
    AverageRecent(uint8_t listLen);         // Pass # of readings to average
    uint16_t aveRecent(uint16_t lastRead);  // Pass most recent reading, returns ave of last few readings

private: 
    uint8_t     m_listLen;
    int8_t      m_listLenSoFar;
    uint16_t   *m_list;         // pntr to the list of recent readings; aquire space via malloc()
};


#ifdef HELPER_SOFTWARE_SERIAL_LCD	// #Define in .INO file before include

#include "SoftwareSerial.h"   // Removed for PurpleV.ino (Landor)

class SoftSerial_LCD : public SoftwareSerial      
{
public:
    //SoftSerial_LCD(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic = false) : SoftwareSerial(receivePin, transmitPin, inverse_logic){;}   // Removed for PurpleV.ino (Landor)
    void clearLCD(void) {
//        SoftwareSerial::write(0xFE);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(0x58);  // Removed for PurpleV.ino (Landor)
        delay(10);   // Adafruit suggest putting delays after each command 
        homeLCD();
    }
    void homeLCD(void) {
//        SoftwareSerial::write(0xFE);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(0x48);  // Removed for PurpleV.ino (Landor)
        delay(10);       
    }
    void row2ndLCD(void) {
//        SoftwareSerial::write(0xFE);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(0x47);  // Removed for PurpleV.ino (Landor)
        ////SoftwareSerial::write(0x01);    // Removed for PurpleV.ino 
        ///(L /SoftwareSerial::write(0x01);    // Removed for     andor)
        ///Landor) PurpleV.ino ( 
//        SoftwareSerial::write(0x01);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(0x40);  // Removed for PurpleV.ino (Landor)
        delay(10);       
    }
    void bkLiteLCD(uint8_t r, uint8_t g, uint8_t b) {
//        SoftwareSerial::write(0xFE);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(0xD0);  // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(r); // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(g); // Removed for PurpleV.ino (Landor)
//        SoftwareSerial::write(b); // Removed for PurpleV.ino (Landor)
        delay(10);       
    }
    void print_maxLenPlusBlanks(const char *s, uint8_t maxLen)
    {
        uint8_t lenRFID = strlen(s);
        if (lenRFID > maxLen)
        {
            s += lenRFID-maxLen;    // Too long, cut off the beginning.
        }

        this->print(s);

        if (lenRFID < maxLen)
        {
            for (uint8_t i = maxLen-lenRFID; i; i--)
            {
                this->print(F(" "));
            }
        }
    }

};

#endif //#ifdef HELPER_SOFTWARE_SERIAL_LCD


#endif //end HANDY_HELPERS_H



