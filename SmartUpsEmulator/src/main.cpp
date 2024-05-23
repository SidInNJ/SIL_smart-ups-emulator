/*
  main.cpp - Main loop for Arduino sketches
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

//volatile bool USBCDCNeeded = false;      // DBC.008b
bool USBCDCNeeded = false;      // DBC.008b				SLR - Match the other def's

#include "ProjectDefs.h"	// For our Smart UPS Emulator project. Defs SERIAL1_DEBUG
#if SERIAL1_IRQ_DEBUG
    char USBDebug[SIZE_USBDebug] = {0};       // DBC.009
#endif

void readSwitch() {                                // DBC.009
	noInterrupts();                                  // DBC.009
	pinMode(2,INPUT_PULLUP);                         // DBC.009  Switch, Press for CDC, Green LED
	if (digitalRead(2) == LOW) USBCDCNeeded = true;  // DBC.009
	interrupts();                                    // DBC.008e

	pinMode(7,OUTPUT);              // DBC.008  Green
	pinMode(6,OUTPUT);              // DBC.008  Blue
	pinMode(5,OUTPUT);              // DBC.008a Yellow Diagnostic if (... whatever ...) digitalWrite(5, HIGH); // Turn on yellow LED
	digitalWrite(7, LOW);           // DBC.008  Green on
	digitalWrite(6, LOW);           // DBC.008  Blue off
	digitalWrite(5, LOW);           // DBC.008a Yellow off
		
	if (USBCDCNeeded) {             // DBC.009 Switch is pressed
		digitalWrite(7, HIGH);        // DBC.008 Green on
		digitalWrite(6, LOW);         // DBC.008 Blue off
	}                               // DBC.008
	else {                          // DBC.008 Switch is open (not pressed)
		digitalWrite(7, LOW);         // DBC.008 Green off
		digitalWrite(6, HIGH);        // DBC.008 Blue on
	}                               // DBC.008
	return;                         // DBC.009
}                                 // DBC.008

int main(void)
{
#if SERIAL1_IRQ_DEBUG
    char prTxt[] = "main() called.\n\r";    // DEBUG DOYET
    if ((sizeof(USBDebug) - strlen(USBDebug)) > (strlen(prTxt)+1))  // Debug printout later
        memcpy(&USBDebug[strlen(USBDebug)], prTxt, strlen(prTxt)+1);
#endif

    readSwitch();                   // DBC.009

	init();

	initVariant();
	
#if defined(USBCON)
	USBDevice.attach();
#endif
	
	setup();
    
	for (;;) {
		loop();
		if (serialEventRun) serialEventRun();
	}
        
	return 0;
}

