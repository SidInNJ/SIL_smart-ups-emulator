# smart-ups-emulator
"Smart UPS" Emulation so a micro controller can be connected to a battery and be seen by a host OS as a smart UPS (allowing safe shutdown)

Starting point: https://github.com/abratchik/HIDPowerDevice

## Selecting CDC_DISABLED or a Normal (Serial enabled) Compile ##
It is nice if we can select building either with CDC (Serial debug over USB) enabled or disabled, 
without changing files common to all Arduino projects on a development PC.
One way to do this is to add a new board type to "boards.txt" that includes a define to
disable "CDC_DISABLED". See "SmartUpsEmulator\Boards_AddOnForLeonardoNoCDC" for notes and the text to add.
