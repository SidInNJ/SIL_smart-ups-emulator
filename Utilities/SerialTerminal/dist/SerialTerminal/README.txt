SerialTerminal - By default will look for a COM port with an attached Arduino Leonardo board.
It will create an interactive serial terminal on that port at 115,200 baud. All communication 
on that port will be logged to SmartUPSLog.txt. See additional command line options below.
You may use the ESC key to abort the connection.

Note that the Leonardo has two modes, Normal Mode (emulate a UPS) and Configuration Mode. You cannot
use the built in menu system or do configuration in Normal mode.

Ncessary Files: 
The folder containing the SerialTerminal.exe program also has a folder named "_internal". 
If you copy SerialTerminal.exe to use on another computer, keep the _internal folder along with it.
The SerialTerminal.py is not needed for running the program. It is provided in case you'd like
to make changes or enhancements.

"SerialTerminal -h" Prints the below:

usage: SerialTerminal.exe [-h] [-t] [-n BOARDNAME] [-b BAUDRATE] [-f LOGFILE] [-c COMPORT]

Serial Terminal that auto-connects to a Smart-UPS and logs to the file SmartUPSLog.txt. Tap H and Enter for help from
the Smart-UPS.

options:
  -h, --help            show this help message and exit
  -t, --timestamps      put timestamps in log file
  -n BOARDNAME, --boardname BOARDNAME
                        Specify a board name to look for (use for other than a Leonardo)
  -b BAUDRATE, --baudrate BAUDRATE
                        baud rate in bps ie. 9600, 38400, (115200)
  -f LOGFILE, --logfile LOGFILE
                        Output Log file name (include extension: ie. .TXT)
  -c COMPORT, --comport COMPORT
                        Serial port name: ie. COM6



