#!/usr/bin/python
import time
import serial
import sys
import msvcrt
import warnings
import serial
import serial.tools.list_ports
import argparse
from datetime import datetime

countReadings = 0
ArduinoDescription = 'No Arduino found'
TeensyDescription  = 'No Teensy found'
LookForBoard = 'Leonardo'
#LookForBoard = 'Mega 2560'
#LookForBoard = 'Arduino'

#logFileName = 'SensorReadLog.txt'
#logSerialNumFileName = 'SN_Log.txt'

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--timestamps", action="store_true",
                    help="put timestamps in log file")
parser.add_argument("-b", "--boardname", action="store", default=LookForBoard,
                    help="Specify a board name to look for (in case you have multiple Arduino boards attached)")
args = parser.parse_args()

LookForBoard = args.boardname
print('Board name to look for: ')
print(LookForBoard)
print('\n')

# got these lines of code from: https://stackoverflow.com/questions/24214643/python-to-automatically-select-serial-ports-for-arduino
arduino_ports = [
    p.device
    for p in serial.tools.list_ports.comports()
    if LookForBoard in p.description  # may need tweaking to match new arduinos
]

# Derived and enhanced from: https://stackoverflow.com/questions/24214643/python-to-automatically-select-serial-ports-for-arduino
ports = list(serial.tools.list_ports.comports())
for aPort in ports:
    print(aPort[1])
    #if 'Arduino' in aPort[1]:
    #if 'Mega 2560' in aPort[1]:
    if LookForBoard in aPort[1]:
        #ArduinoDescription = aPort[2]
        ArduinoDescription = aPort[1]
        #break
    if 'USB Serial Device' in aPort[1]:
        #TeensyDescription = aPort[2]
        TeensyDescription = aPort[1]
        #break

if not arduino_ports:
    print('No Arduino found. Aborting...')
    time.sleep(2)
    raise IOError("No Arduino found")

if len(arduino_ports) == 1:
    print('1 Arduino found. Good!')
    time.sleep(2)
    #raise IOError("Only 1 Arduino found - Need 2")

if len(arduino_ports) > 1:
    warnings.warn('WARNING: More than 1 Arduino found')

inPortName = arduino_ports[0]       # this works if only one Uno/Mega Arduino
#outPortName = arduino_ports[1]

#inPortName = 'COM87' #      # i7   # Don't need if "inPortName = arduino_ports[0]" above works
#inPortName = 'COM7' #      # i5

#outPortName = 'COM15'
#outPortName = 'COM77'  #Metro Express Grand Central (Couldn't get to work)

print ('Getting input from COM port')
sys.stdout.flush()

print (inPortName)
sys.stdout.flush()

try:
    arduinoPort = serial.Serial( inPortName, baudrate=115200 )
    #print ('Description of Input Teensy: ', TeensyDescription)
    sys.stdout.flush()
except Exception as inst:
    print(type(inst))
    print(inst.args)
    print('Something have the serial port to the Arduino open; like the Arduino IDE?')
    sys.stdout.flush()
    time.sleep(3)
    sys.exit(0)

print ('Sending output to COM port', inPortName)
#arduinoPort = serial.Serial( outPortName, baudrate=57600 )
print ('Description of In/Output Arduino: ', ArduinoDescription)
sys.stdout.flush()

# Log Arduino output to a file
fLog = open("ArduinoLog.txt", "a")
fLog.write("\n------- Starting Log at: ")
now = datetime.now() # current date and time
date_time = now.strftime("%m/%d/%Y, %H:%M:%S")
fLog.write(date_time)
fLog.write("\nThis Arduino Board: ")
fLog.write(ArduinoDescription)
fLog.write(" on Port: ")
fLog.write(inPortName)
fLog.write("\n")


# configur
# e the serial connections (the parameters differs on the device you are connecting to)
#arduinoPort = serial.Serial(
#    port='COM9',
#    baudrate=38400,
#    timeout = 0.001
#)

#print ('Will Log everything to ', logFileName    #)
#print ('Will Log Serial Numbers and Results to ', logSerialNumFileName    #)

print ('Starting...')
sys.stdout.flush()

#arduinoPort.open()
sys.stdout.flush()

print ('In Port open state:', arduinoPort.isOpen())

#arduinoPort.open()
sys.stdout.flush()

print ('Out Port open state:', arduinoPort.isOpen())
sys.stdout.flush()

print ('\nCommands accepted by TestPoller:')
#print ('G - Sends GRON and ALWAYS to the Arduino, turns off polling and lets the Arduino feed')
#print ('    readings and graphical output to the PC screen.')
#print ('F - Turns off graphing by the Arduino and turns polling back on.')
#print ('I - put a message in the log files: "------- User Input: About to Intentionally Inject a Fault...".')
print ('ESC key - Exits TestPoller')
##print ('T, WDR and ? commands are passed through to the Arduino\n')
#print ('? commands are passed through to the Arduino\n')
if args.timestamps:
    print ('Will put timestamps in the logfile')

sys.stdout.flush()

print ('Sleeping to give time for boot up...')
print ('Will Send chars from Arduion to the screen...')
sys.stdout.flush()

timeout_start = time.time()

doPolling = True
inString = ''
initialPass = True
countOfTimeouts = 0

print ('Ready... ')

sys.stdout.flush()

lastMsgTime = int(time.time_ns() / 1000)

print("ver 1.1")

b1Val = 11
b2Val = 127 + 11
sentMsgCount = 0
sentMsgCount1Sec = 0
inc1 = 1
inc2 = 1
encoding = 'utf-8'
timeBetweenMsgs = 2200
lastSecondTimer = int(time.time_ns() / 1000000)
needTimeStamp = args.timestamps

while 1 :

    if arduinoPort.inWaiting() > 0:
        chr = arduinoPort.read(1)
        #sys.stdout.write(chr)      # gives error: TypeError: write() argument must be str, not bytes
        if needTimeStamp and not ((ord(chr) == 0x0D)  or (ord(chr) == 0x0A)):
            now = datetime.now() # current date and time
            date_time = now.strftime("%H:%M:%S - ")
            fLog.write(date_time)
            sys.stdout.flush()
            print(date_time, end ='')
            sys.stdout.flush()

        sys.stdout.buffer.write(chr)
        sys.stdout.flush()
        needTimeStamp = False
        try:
            sChar = chr.decode(encoding)
        except:
            sChar = '?' #.encode(encoding)

        fLog.write(sChar)
        if ((ord(chr) == 0x0D)  or (ord(chr) == 0x0A)): # CR or LF?
            needTimeStamp = args.timestamps



    if msvcrt.kbhit():
        #sys.stdin.getch()
        key = msvcrt.getch()
        #if ((key == ord(27)) or (key == 'q'):  # ESC?
        if ((ord(key) == 27)  or (key == b'q')): # ESC?
            print('Aborted by ESC')
            fLog.close()    # Close log file
            time.sleep(1)
            sys.exit(0)

        #if key == b'h':  # ESC?
        #    print('h entered!')


        try:
            inString = inString + key.decode(encoding)
        except:
            inString = inString + '?'

        #if inString == 'G' or inString == 'g':
        #    doPolling = False
        #    inString = ''
        #    arduinoPort.write('GRON\r')
        #    time.sleep(.05)
        #    arduinoPort.write('ALWAYS\r')
        #    key = ''
        #
        #if inString == 'F' or inString == 'f':
        #    doPolling = True
        #    inString = ''
        #    arduinoPort.write('PING\r')
        #    key = ''
        #
        #if inString == 'I' or inString == 'i':
        #    #file1.write("------- User Input: About to Intentionally Inject a Fault...\r\n")
        #    #fileSN.write("------- User Input: About to Intentionally Inject a Fault...\r\n")
        #    print "------- User Input: About to Intentionally Inject a Fault..."
        #    key = ''
        #

        if key == b'\r':
            print(inString)   # just to show the result
            #arduinoPort.write(inString)
            arduinoPort.write(inString.encode('ascii'))
            #write(arduinoPort, inString.c_str(), inString.size())
            fLog.write("PC: ")
            fLog.write(inString)
            inString = ''
            needTimeStamp = args.timestamps
            #arduinoPort.write('\r')


