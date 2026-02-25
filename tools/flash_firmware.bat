@echo off
setlocal
title Morse Trainer - Firmware Flash Tool
echo =============================================
echo   Morse Trainer Firmware Flash Tool
echo =============================================
echo.

:: ---- Check Python ----
where python >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found.
    echo Install Python 3.8+ from https://www.python.org/downloads/
    echo IMPORTANT: Check "Add python.exe to PATH" during install.
    pause
    exit /b 1
)

:: ---- Check/install PlatformIO ----
python -m platformio --version >nul 2>&1
if errorlevel 1 (
    echo PlatformIO not found. Installing...
    python -m pip install platformio
    if errorlevel 1 (
        echo ERROR: Failed to install PlatformIO.
        echo Try: python -m pip install --user platformio
        pause
        exit /b 1
    )
)

echo PlatformIO version:
python -m platformio --version
echo.

:: ---- Detect serial port ----
echo Scanning for serial ports...
python -m platformio device list
echo.

:: ---- Choose board ----
echo Which board are you flashing?
echo   1) NodeMCU v2 (default - external buzzer/LED/OLED)
echo   2) ESP8266 OLED module v2.1.0 (built-in OLED)
echo.
set /p BOARD_CHOICE="Enter 1 or 2: "

if "%BOARD_CHOICE%"=="2" (
    set PIO_ENV=esp8266_oled
    set BOARD_NAME=ESP8266 OLED
) else (
    set PIO_ENV=nodemcuv2
    set BOARD_NAME=NodeMCU v2
)

echo.
echo Flashing firmware for: %BOARD_NAME% (env:%PIO_ENV%)
echo.

:: ---- Navigate to project root ----
cd /d "%~dp0"

:: ---- Build and upload firmware ----
echo [1/2] Building and uploading firmware...
python -m platformio run -e %PIO_ENV% -t upload
if errorlevel 1 (
    echo.
    echo ERROR: Firmware upload failed.
    echo - Is the board plugged in via USB?
    echo - Do you have the USB-serial driver installed?
    echo   CH340: https://www.wch-ic.com/downloads/CH341SER_EXE.html
    echo   CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
    pause
    exit /b 1
)

:: ---- Upload filesystem (web UI) ----
echo.
echo [2/2] Uploading web UI filesystem...
python -m platformio run -e %PIO_ENV% -t uploadfs
if errorlevel 1 (
    echo.
    echo WARNING: Filesystem upload failed. Web UI may not work.
    echo The serial client will still work fine.
)

echo.
echo =============================================
echo   Flash complete!
echo =============================================
echo.
echo IMPORTANT: Press the RESET button on the board now.
echo.
echo Next steps:
echo   1. Open MorseClient folder
echo   2. Double-click run.bat (requires Java 11+)
echo   3. Select the COM port and click Connect
echo.
pause
