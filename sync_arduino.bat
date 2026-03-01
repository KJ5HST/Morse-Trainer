@echo off
echo Syncing Arduino IDE files...
echo.

cd /d "%~dp0"

where python >nul 2>&1 && (python sync_arduino.py & goto :done)
where python3 >nul 2>&1 && (python3 sync_arduino.py & goto :done)
where py >nul 2>&1 && (py sync_arduino.py & goto :done)

echo Python not found. Install Python 3.8+ from https://www.python.org/downloads/
echo Make sure "Add python.exe to PATH" is checked during install.
pause
exit /b 1

:done
echo.
pause
