rem arduino-cli compile --warnings more --fqbn arduino:avr:mega SmartUpsEmulator\SmartUpsEmulator.ino
arduino-cli version
set ARDUINO_DIRECTORIES_USER=.
arduino-cli compile --warnings more --fqbn arduino:avr:leonardoNoCDC SmartUpsEmulator\SmartUpsEmulator.ino


rem curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="C:\Program Files (x86)\Arduino\Arduino_CLI" sh
pause
