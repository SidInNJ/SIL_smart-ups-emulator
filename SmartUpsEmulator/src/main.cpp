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

#warning "TEMP COMPILE CONFIRM: desired main.cpp is being compiled."

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

volatile bool USBCDCNeeded = false;      // DBC.008b 
volatile long USBSwitchTime[6] = {0};    // DBC.008c
volatile long USBSwitchCount[6] = {0};   // DBC.008d

/*
bool readSwitch() {               // DBC.008
	pinMode(2,INPUT_PULLUP);        // DBC.008  Switch, Press for CDC, Green LED
	pinMode(7,OUTPUT);              // DBC.008  Green
	pinMode(6,OUTPUT);              // DBC.008  Blue
	pinMode(5,OUTPUT);              // DBC.008a Yellow Diagnostic if (... whatever ...) digitalWrite(5, HIGH); // Turn on yellow LED
	digitalWrite(7, LOW);           // DBC.008  Green on
	digitalWrite(6, LOW);           // DBC.008  Blue off
	digitalWrite(5, LOW);           // DBC.008a Yellow off
	
	USBCDCNeeded = false;
	bool result = false;                          // DBC.008 Default, switch not pressed, no CDC
	unsigned long finalMillis = millis() + 3000;  // DBC.008 Loop for 3 seconds
	while(millis() < finalMillis) {               // DBC.008
		if (digitalRead(2) == LOW) {                // DBC.008 Switch is pressed
			result = true;                            // DBC.008
			USBCDCNeeded = true;                      // DBC.008b
			break;                                    // DBC.008
		}                                           // DBC.008
	}		                                          // DBC.008
	
	if (result) {                   // DBC.008 Switch is pressed
		digitalWrite(7, HIGH);        // DBC.008 Green on
		digitalWrite(6, LOW);         // DBC.008 Blue off
	}                               // DBC.008
	else {                          // DBC.008 Switch is open (not pressed)
		digitalWrite(7, LOW);         // DBC.008 Green off
		digitalWrite(6, HIGH);        // DBC.008 Blue on
	}                               // DBC.008
	return result;
}                                 // DBC.008
*/
#warning "TEMP COMPILE CONFIRM: desired main.cpp is being compiled."

int main(void)
{
	noInterrupts();                 // DBC.008e
	pinMode(2,INPUT_PULLUP);        // DBC.008  Switch, Press for CDC, Green LED
	if (digitalRead(2) == LOW) USBCDCNeeded = true;  // DBC.008e  Davis - try stopping interrupts before reading the switch
	USBSwitchTime[5] = millis();
	USBSwitchCount[5]++;
	interrupts();                   // DBC.008e

	init();

	initVariant();

	//USBCDCNeeded = false;                         // DBC.008b
	//USBDevice.setCDCNeeded(false);                // DBC.008b
	//while(true) {digitalWrite(6, HIGH);  delay(1000); digitalWrite(6, LOW); delay(1000);}  // DBC.008a Blue 1 second blink

	//USBCDCNeeded = true;                         // DBC.008b
	//USBDevice.setCDCNeeded(true);                // DBC.008b
	//while(true);                                 // DBC.008b
  //USBDevice.setCDCNeeded(readSwitch());                                            // DBC.008b
  //USBDevice.setCDCNeeded(readSwitch());   // DBC.008
	//USBDevice.getCDCNeeded();               // DBC.008a
	//USBDevice.setCDCNeeded(true);           // DBC.008b
	//readSwitch();                             // DBC.008b 
	//USBDevice.setCDCNeeded(true);             // DBC.008b
	//digitalWrite(5, HIGH);                    // DBC.008b Yellow on
	//delay(1000);                              // DBC.008b
	//digitalWrite(5, LOW);                     // DBC.008b

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

