echo off
rmdir /S /Q build
rmdir /S /Q dist
pyinstaller SerialTerminal.py
copy SerialTerminal.py dist\SerialTerminal\SerialTerminal.py
copy README.txt dist\SerialTerminal\README.txt
echo Your bundled application should now be available in the dist\SerialTerminal folder:
dir dist\SerialTerminal
echo You will need to distribute both the program "SerialTerminal.exe" and the folder "_internal".
pause
