@echo off
setlocal enabledelayedexpansion
title Morse Trainer - Firmware Flash Tool

echo =============================================
echo   Morse Trainer Firmware Flash Tool
echo =============================================
echo.

:: ---- Navigate to project root (one level up from tools/) ----
cd /d "%~dp0\.."
echo Working directory: %CD%
echo.

:: =============================================
:: STEP 1: Find Python
:: =============================================
set PYTHON=
echo Looking for Python...

:: Try "python" first
where python >nul 2>&1
if not errorlevel 1 (
    :: Make sure it's real Python, not the Windows Store stub
    python --version >nul 2>&1
    if not errorlevel 1 (
        set PYTHON=python
        goto :found_python
    )
)

:: Try "python3"
where python3 >nul 2>&1
if not errorlevel 1 (
    python3 --version >nul 2>&1
    if not errorlevel 1 (
        set PYTHON=python3
        goto :found_python
    )
)

:: Try "py" (Python Launcher for Windows)
where py >nul 2>&1
if not errorlevel 1 (
    py --version >nul 2>&1
    if not errorlevel 1 (
        set PYTHON=py
        goto :found_python
    )
)

:: Try common install locations
for %%P in (
    "%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python311\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python310\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python39\python.exe"
    "%LOCALAPPDATA%\Programs\Python\Python38\python.exe"
    "C:\Python312\python.exe"
    "C:\Python311\python.exe"
    "C:\Python310\python.exe"
    "C:\Python39\python.exe"
    "C:\Python38\python.exe"
) do (
    if exist %%P (
        set PYTHON=%%P
        goto :found_python
    )
)

echo.
echo =============================================
echo   Python not found!
echo =============================================
echo.
echo Please install Python 3.8 or later:
echo.
echo   https://www.python.org/downloads/
echo.
echo IMPORTANT during install:
echo   [x] Check "Add python.exe to PATH"
echo   [x] Click "Disable path length limit" if offered
echo.
echo After installing Python, close this window and run
echo this script again.
echo.
pause
exit /b 1

:found_python
echo Found: %PYTHON%
%PYTHON% --version
echo.

:: =============================================
:: STEP 2: Make sure pip works
:: =============================================
echo Checking pip...
%PYTHON% -m pip --version >nul 2>&1
if errorlevel 1 (
    echo pip not found. Installing pip...
    %PYTHON% -m ensurepip --upgrade 2>nul
    if errorlevel 1 (
        echo.
        echo ERROR: pip is not available.
        echo Try reinstalling Python and check "pip" in the installer options.
        echo.
        pause
        exit /b 1
    )
)
echo pip OK.
echo.

:: =============================================
:: STEP 3: Install/check PlatformIO
:: =============================================
echo Checking for PlatformIO...
%PYTHON% -m platformio --version >nul 2>&1
if errorlevel 1 (
    echo PlatformIO not found. Installing...
    echo This may take a few minutes on first run.
    echo.
    %PYTHON% -m pip install platformio
    if errorlevel 1 (
        echo.
        echo First attempt failed. Trying with --user flag...
        %PYTHON% -m pip install --user platformio
        if errorlevel 1 (
            echo.
            echo =============================================
            echo   PlatformIO installation failed
            echo =============================================
            echo.
            echo Try running this command manually in a terminal:
            echo   %PYTHON% -m pip install platformio
            echo.
            echo If you get a permissions error, try:
            echo   %PYTHON% -m pip install --user platformio
            echo.
            echo If pip itself has problems, try upgrading it:
            echo   %PYTHON% -m pip install --upgrade pip
            echo.
            pause
            exit /b 1
        )
    )
    echo.
    :: Verify it actually installed
    %PYTHON% -m platformio --version >nul 2>&1
    if errorlevel 1 (
        echo PlatformIO installed but can't be found.
        echo You may need to close this window and reopen it,
        echo then run this script again.
        echo.
        pause
        exit /b 1
    )
)

echo PlatformIO version:
%PYTHON% -m platformio --version
echo.

:: =============================================
:: STEP 4: Check for USB serial port
:: =============================================
echo Scanning for serial ports...
echo.
%PYTHON% -m platformio device list 2>nul
echo.

echo Do you see your board's serial port listed above?
echo (It will show as COMx with a description like CH340 or CP2102)
echo.
echo If NO ports are listed:
echo   - Is the board plugged in via USB?
echo   - Is the USB cable a data cable (not charge-only)?
echo   - You may need a USB-serial driver:
echo     CH340:  https://www.wch-ic.com/downloads/CH341SER_EXE.html
echo     CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
echo.
set /p CONTINUE="Press Enter to continue (or Ctrl+C to quit and fix): "
echo.

:: =============================================
:: STEP 5: Choose board
:: =============================================
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

:: =============================================
:: STEP 6: Build and upload firmware
:: =============================================
echo [1/2] Building and uploading firmware...
echo (First build downloads toolchains - this can take several minutes)
echo.
%PYTHON% -m platformio run -e %PIO_ENV% -t upload
if errorlevel 1 (
    echo.
    echo =============================================
    echo   Firmware upload failed
    echo =============================================
    echo.
    echo Common causes:
    echo   1. Board not plugged in or wrong USB cable
    echo   2. Another program has the COM port open
    echo      (close Arduino IDE, PuTTY, serial monitors, etc.)
    echo   3. Missing USB-serial driver (see links above)
    echo   4. Wrong board selected - try the other option
    echo.
    echo You can safely run this script again after fixing the issue.
    echo.
    pause
    exit /b 1
)

:: =============================================
:: STEP 7: Upload filesystem (web UI)
:: =============================================
echo.
echo [2/2] Uploading web UI filesystem...
%PYTHON% -m platformio run -e %PIO_ENV% -t uploadfs
if errorlevel 1 (
    echo.
    echo WARNING: Filesystem upload failed. The web UI may not work,
    echo but the serial desktop client will still work fine.
    echo.
)

echo.
echo =============================================
echo   Flash complete!
echo =============================================
echo.
echo IMPORTANT: Press the RESET button on the board now.
echo.
echo Next steps:
echo   1. Open the MorseClient folder (inside tools/)
echo   2. Double-click run.bat (requires Java 11+)
echo      Get Java from: https://adoptium.net
echo   3. Select the COM port and click Connect
echo   4. Click Start and begin training!
echo.
pause
