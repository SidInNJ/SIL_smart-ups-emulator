echo off
rmdir /S /Q build
rem del /S /F /Q dist\SerialTerminal\_internal
rmdir /S /Q dist
dir
pyinstaller SerialTerminal.py
echo Your bundled application should now be available in the dist\SerialTerminal folder:
dir dist\SerialTerminal
echo You will need to distribute both the program "SerialTerminal.exe" and the folder "_internal".
pause
