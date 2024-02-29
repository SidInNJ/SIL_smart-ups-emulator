@echo off

Rem
Rem Double click the reset button and immediately run this batch file
Rem 
Rem Note: For Windows, the COM port may change after the double click of the reset button.
Rem  Watch device manager under "Ports (COM & LPT) to see what new COM port shows up.
Rem  Put that one in the "-PCOMxx" parameter below.
Rem 
Rem 
Rem 

Rem Davis: avrdude "-Cavrdude.conf" -v -V -patmega32u4 -cavr109 "-PCOM6" -b57600 -D "-Uflash:w:SmartUpsEmulator.007.ino.hex:i"
Rem Sid:
Rem "C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" "-Cavrdude.conf" -v -V -patmega32u4 -cavr109 "-PCOM16" -b57600 -D "-Uflash:w:SmartUpsEmulator.007.ino.hex:i"
avrdude.exe "-Cavrdude.conf" -v -V -patmega32u4 -cavr109 "-PCOM30" -b57600 -D "-Uflash:w:SmartUpsEmulator.007.ino.hex:i"
