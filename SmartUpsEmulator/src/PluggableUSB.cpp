/*
  PluggableUSB.cpp
  Copyright (c) 2015 Arduino LLC

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

#include "USBAPI.h"
#include "PluggableUSB.h"

#if defined(USBCON)	
#ifdef PLUGGABLE_USB_ENABLED

extern uint8_t _initEndpoints[];

extern char USBDebug[512];          // DBC.009

int PluggableUSB_::getInterface(uint8_t* interfaceCount)
{
	int sent = 0;
	PluggableUSBModule* node;
	for (node = rootNode; node; node = node->next) {
		int res = node->getInterface(interfaceCount);
		if (res < 0)
			return -1;
		sent += res;
	}
	return sent;
}

int PluggableUSB_::getDescriptor(USBSetup& setup)
{
	PluggableUSBModule* node;
	for (node = rootNode; node; node = node->next) {
		int ret = node->getDescriptor(setup);
		// ret!=0 -> request has been processed
		if (ret)
			return ret;
	}
	return 0;
}

void PluggableUSB_::getShortName(char *iSerialNum)
{
	PluggableUSBModule* node;
	for (node = rootNode; node; node = node->next) {
		iSerialNum += node->getShortName(iSerialNum);
	}
	*iSerialNum = 0;
}

bool PluggableUSB_::setup(USBSetup& setup)
{
	PluggableUSBModule* node;
	for (node = rootNode; node; node = node->next) {
		if (node->setup(setup)) {
			return true;
		}
	}
	return false;
}

extern bool USBCDCNeeded;  // DBC.008b
extern char USBDebug[512];          // DBC.009

bool PluggableUSB_::plug(PluggableUSBModule *node)
{
	if ((lastEp + node->numEndpoints) > USB_ENDPOINTS) {
		return false;
	}

	if (!rootNode) {
		rootNode = node;
	} else {
		PluggableUSBModule *current = rootNode;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}

	pinMode(2,INPUT_PULLUP);                             // DBC.008d  Switch, Press for CDC, Green LED
  if (USBCDCNeeded || (digitalRead(2) == LOW)) {       // DBC.008d  Switch is pressed. Not sure why the switch needs to be read here. Errors without it

      if (!USBCDCNeeded)                                                        // SLR 2024-04-19   
      {
          Serial1.println("USBCDCNeeded NOT set before PluggableUSB_::plug");   // SLR 2024-04-19
          USBCDCNeeded = true;                                                  // SLR 2024-04-19   
      }

        node->pluggedInterface = CDC_ACM_INTERFACE + CDC_INTERFACE_COUNT;    // DBC.008b 0 + 2 = 2
		node->pluggedEndpoint = CDC_FIRST_ENDPOINT + CDC_ENPOINT_COUNT;      // DBC.008b 1 + 3 = 4
	}
	else                                                 // DBC.008b
	{                                                    // DBC.008b
		node->pluggedInterface = 0;                        // DBC.008b
		node->pluggedEndpoint = 0;                         // DBC.008b
	}                                                    // DBC.008b
	
	lastIf += node->numInterfaces;
	for (uint8_t i = 0; i < node->numEndpoints; i++) {
		_initEndpoints[lastEp] = node->endpointType[i];
		lastEp++;
	}
	//sprintf(&USBDebug[strlen(USBDebug)], "node->pluggedInterface %i\r\n", node->pluggedInterface);  // DBC.009
	//sprintf(&USBDebug[strlen(USBDebug)], "node->pluggedEndpoint %i\r\n", node->pluggedEndpoint);        // DBC.009
	//sprintf(&USBDebug[strlen(USBDebug)], "node->numInterfaces %i\r\n", node->numInterfaces);  // DBC.009
	//sprintf(&USBDebug[strlen(USBDebug)], "node->numEndpoints %i\r\n", node->numEndpoints);  // DBC.009
	//sprintf(&USBDebug[strlen(USBDebug)], "lastIf %i\r\n", lastIf);        // DBC.009
	//sprintf(&USBDebug[strlen(USBDebug)], "lastEp %i\r\n", lastEp);        // DBC.009
	return true;
	// restart USB layer???
}


PluggableUSB_& PluggableUSB()
{
	static PluggableUSB_ obj;
	return obj;
}

PluggableUSB_::PluggableUSB_() : lastIf(CDC_ACM_INTERFACE + CDC_INTERFACE_COUNT),  // DBC.008b
                                 lastEp(CDC_FIRST_ENDPOINT + CDC_ENPOINT_COUNT),   // DBC.008b
//PluggableUSB_::PluggableUSB_() : lastIf(0),                                          // DBC.008b
//                                 lastEp(0),                                          // DBC.008b
                                 rootNode(NULL)
{
	// Empty
}

#endif

#endif /* if defined(USBCON) */
