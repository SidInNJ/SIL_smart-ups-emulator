#ifndef PROJECTDEFS_H
#define PROJECTDEFS_H

#include <stdint.h>


//#define SERIAL1_DEBUG       true    // Prints some USB connection info
#define SERIAL1_IRQ_DEBUG   true    // IRQ code prints via a buffer which gets printed in loop()
//
#if SERIAL1_IRQ_DEBUG
    #define SIZE_USBDebug 256
    extern char USBDebug[SIZE_USBDebug];          // DBC.009  SLR: Reduced size as RAM getting tight
#endif

extern bool USBCDCNeeded;  // DBC.008b
extern uint8_t pcSetErrorCount;         // How many times the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT value
extern uint8_t pcSetValue;     // The value that the host (PC) tried to set HID_PD_REMNCAPACITYLIMIT to

#endif //PROJECTDEFS_H
