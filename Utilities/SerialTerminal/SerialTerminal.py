#!/usr/bin/python

# Serial Terminal Program for SIL Smart-UPS Configuration 
# and Battery profile loggins
# 
# To Do:
#   
#   Make .EXE file
# 

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
parser.add_argument("-n", "--boardname", action="store", default=LookForBoard,
                    help="Specify a board name to look for (in case you have multiple Arduino boards attached)")
parser.add_argument("-b", "--baudrate", action="store", default=115200, type=int,
                    help="baud rate in bps ie. 9600, 38400, (115200)")
parser.add_argument("-f", "--logfile", action="store", default='SmartUPSLog.txt', 
                    help="Output Log file name (include extension: ie. .TXT)")
parser.add_argument("-c", "--comport", action="store", default='NoneGiven', 
                    help="Serial port name: ie. COM6")
args = parser.parse_args()

if args.comport == 'NoneGiven':
    LookForBoard = args.boardname
    print('Board name to look for: ', end =" ")
    print(LookForBoard)

    # got these lines of code from: https://stackoverflow.com/questions/24214643/python-to-automatically-select-serial-ports-for-arduino
    arduino_ports = [
        p.device
        for p in serial.tools.list_ports.comports()
        if LookForBoard in p.description  # may need tweaking to match new arduinos
    ]

    # Derived and enhanced from: https://stackoverflow.com/questions/24214643/python-to-automatically-select-serial-ports-for-arduino
    ports = list(serial.tools.list_ports.comports())
    print('Board(s) Found: ')
    for aPort in ports:
        print('   ', end =" ")
        print(aPort[1])
        if LookForBoard in aPort[1]:
            ArduinoDescription = aPort[1]
        if 'USB Serial Device' in aPort[1]:
            TeensyDescription = aPort[1]    # Note: this isn't used!

    if not arduino_ports:
        print('No Arduino found. Aborting...')
        time.sleep(2)
        raise IOError("No Arduino found")

    if len(arduino_ports) == 1:
        print('One', end =" ")
        print(LookForBoard, end =" ")
        print('found. Good!')
        time.sleep(2)

    if len(arduino_ports) > 1:
        warnStr = 'WARNING: More than 1 ' + LookForBoard + ' found!'
        warnings.warn(warnStr)

    inPortName = arduino_ports[0]       # this works if only one Uno/Mega Arduino
else:
    inPortName = args.comport
    if LookForBoard != args.boardname:
        print ('WARNING: Supplied board name ignored since COM port name supplied.')


print ('Getting input from COM port', end =" ")
print (inPortName, flush=True)


try:
    arduinoPort = serial.Serial( inPortName, baudrate=args.baudrate )
    #print ('Description of Input Teensy: ', TeensyDescription)
    #sys.stdout.flush()
except Exception as inst:
    print(type(inst))
    print(inst.args)
    print('Something have the serial port to the Arduino open; like the Arduino IDE?')
    #sys.stdout.flush()
    time.sleep(3)
    sys.exit(0)

print ('Communicating on serial port', inPortName)



# Log Arduino output to a file
fLog = open(args.logfile, "a")
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

print ('Serial Terminal Starting...')
#sys.stdout.flush()

#print ('Serial Port open state:', arduinoPort.isOpen())

print ('ESC key - Exits Serial Terminal')

if args.timestamps:
    print ('Will put timestamps in the logfile and on screen.')

#sys.stdout.flush()


doPolling = True
inString = ''
initialPass = True
countOfTimeouts = 0

print ('Ready... ')

#sys.stdout.flush()

lastMsgTime = int(time.time_ns() / 1000)

#print("ver 1.1")

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

unFlushedChars = False
timeout_start = time.time()

while 1 :

    if arduinoPort.inWaiting() > 0:
        chr = arduinoPort.read(1)
        unFlushedChars = True
        timeout_start = time.time()
        #sys.stdout.write(chr)      # gives error: TypeError: write() argument must be str, not bytes
        if needTimeStamp and not ((ord(chr) == 0x0D)  or (ord(chr) == 0x0A)):
            now = datetime.now() # current date and time
            date_time = now.strftime("%H:%M:%S - ")
            fLog.write(date_time)
            #sys.stdout.flush()
            print(date_time, end ='', flush=True)
            #sys.stdout.flush()

        if (ord(chr) == 0x0A):
            print()         # cause flush
        else:
            sChar = chr.decode(encoding)
            print(sChar, end ='', flush=True)
            #sys.stdout.buffer.write(chr)

        #sys.stdout.flush()
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
        unFlushedChars = True
        timeout_start = time.time()

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

    if unFlushedChars and time.time() > timeout_start + 0.4:     # If it has been quiet for a while:
        fLog.close()                        # Close log file to flush for external editor access
        fLog = open(args.logfile, "a")      # Reopen so we can add more
        #print('Debug: Just closed and reopened the log file')
        unFlushedChars = False


