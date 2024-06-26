/*
   Copyright (c) 2011, Peter Barrett
   Copyright (c) 2015, Arduino LLC

   Permission to use, copy, modify, and/or distribute this software for
   any purpose with or without fee is hereby granted, provided that the
   above copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
   BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
   OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
   SOFTWARE.
 */

#include "ProjectDefs.h"	// For our Smart UPS Emulator project. Defs SERIAL1_DEBUG

#define PLUGGABLE_USB_ENABLED

#if defined(EPRST6)
#define USB_ENDPOINTS 7 // AtMegaxxU4
#else
#define USB_ENDPOINTS 5 // AtMegaxxU2
#endif

#define ISERIAL_MAX_LEN     20

// Uncomment the following line or pass -DCDC_DISABLED to the compiler
// to disable CDC (serial console via USB).
// That's useful if you want to create an USB device (like an USB Boot Keyboard)
// that works even with problematic devices (like KVM switches).
// Keep in mind that with this change you'll have to use the Arduino's
// reset button to be able to flash it.
//#define CDC_DISABLED

#ifndef CDC_DISABLED
#define CDC_ENABLED
#endif

#ifdef CDC_ENABLED
#define CDC_INTERFACE_COUNT	2
#define CDC_ENPOINT_COUNT	3
#else // CDC_DISABLED
#define CDC_INTERFACE_COUNT	0
#define CDC_ENPOINT_COUNT	0
#endif

#define CDC_ACM_INTERFACE	0	// CDC ACM
#define CDC_DATA_INTERFACE	1	// CDC Data
#define CDC_FIRST_ENDPOINT	1
#define CDC_ENDPOINT_ACM	(CDC_FIRST_ENDPOINT)							// CDC First
#define CDC_ENDPOINT_OUT	(CDC_FIRST_ENDPOINT+1)
#define CDC_ENDPOINT_IN		(CDC_FIRST_ENDPOINT+2)

#define INTERFACE_COUNT		(MSC_INTERFACE + MSC_INTERFACE_COUNT)

#define CDC_RX CDC_ENDPOINT_OUT
#define CDC_TX CDC_ENDPOINT_IN

#define IMANUFACTURER   1
#define IPRODUCT        2
#define ISERIAL         3

// Below replaced by define in "ProjectDefs.h"  // SLR
//#define SERIAL1_DEBUG true  // DBC.008f  // Uncomment this line to envoke Serial1 debugging

//#define USB_UPS_VID 0x0463  // Eaton
//#define USB_UPS_PID 0xffff  // Eaton UPS
#define USB_UPS_VID 0x051d  // APC
#define USB_UPS_PID 0x0003  // APC UPS  // 0x001 Not valid with Synology // 0x0002 Synology Recognized // 0x0003 Synology likes best
