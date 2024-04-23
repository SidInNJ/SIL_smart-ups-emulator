# smart-ups-emulator
"Smart UPS" Emulation so a micro controller can be connected to a battery and be seen by a host OS as a smart UPS (allowing safe shutdown).


Starting point: https://github.com/abratchik/HIDPowerDevice using an Arduino Leonardo

## Selecting CDC_DISABLED (Default) or a Normal (Serial enabled) at Run Time ##
Off the shelf an Arduino Leonardo will communicate  with a host PC using
its serial port over the USB cable. If HID emulation is enabled (mouse/keyboard/UPS),
that communication is also over the same USB cable. Unfortunately, Synology NAS aren't 
compatible with the emulated UPS in this configuration.

So, to allow the emulated UPS to communicate with a Synology NAS, 
the Leonardo's normal serial over USB port must be disabled. This code does that
by default. But we also need to use that serial port for board configuration
and callibration. If IO pin 2 is shorted to ground at boot-up, the serial port 
will be enabled and configuration and calibration may be accomplished.
