Morse Trainer - Quick Start
===========================

WHAT'S IN THE BOX
-----------------
  MorseClient/          Desktop app (connects to trainer via USB serial)
    MorseClient.jar     The app (double-click or use run.bat)
    run.bat             Windows launcher
    run.sh              Mac/Linux launcher
  flash_firmware.bat    Windows script to flash firmware onto the ESP8266


STEP 1: FLASH THE FIRMWARE (one time)
--------------------------------------
You have three options:

OPTION A: Windows quick flash (easiest)
  Prerequisites:
    - Python 3.8+  https://www.python.org/downloads/
      IMPORTANT: Check "Add python.exe to PATH" during install
    - USB cable connected to the ESP8266 board
    - USB-serial driver (if Windows doesn't recognize the board):
      CH340:  https://www.wch-ic.com/downloads/CH341SER_EXE.html
      CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  Steps:
    1. Plug in the ESP8266 via USB
    2. Double-click flash_firmware.bat
    3. Choose your board type (NodeMCU v2 or ESP8266 OLED)
    4. Wait for flash to complete
    5. Press the RESET button on the board

OPTION B: Arduino IDE
  The MorseTrainer/ folder is a complete Arduino sketch you can open
  and compile directly.

  1. Install Arduino IDE 2.x
       https://www.arduino.cc/en/software

  2. Add ESP8266 board support
       File > Preferences > Additional Board Manager URLs, add:
       https://arduino.esp8266.com/stable/package_esp8266com_index.json
       Then: Tools > Board > Boards Manager > search "esp8266" > Install

  3. Install these libraries (Sketch > Include Library > Manage Libraries):
       - ArduinoJson         by Benoit Blanchon
       - ESPAsyncTCP         by dvarrel
       - ESPAsyncWebServer   by lacamera
       - U8g2                by oliver

  4. Open MorseTrainer/MorseTrainer.ino

  5. Tools menu settings:
       Board:        NodeMCU 1.0 (ESP-12E Module)
       Flash Size:   4MB (FS:2MB OTA:~1019KB)
       Upload Speed: 115200
       Port:         (your COM port)

  6. Click Upload (arrow button)

  7. Upload the web UI filesystem:
       Press Ctrl+Shift+P, type "Upload LittleFS", select it
       (requires the LittleFS plugin:
        https://github.com/earlephilhower/arduino-littlefs-upload)

  8. Press the RESET button on the board

  Board selection: The default builds for NodeMCU v2. For the ESP8266
  OLED module, edit MorseTrainer/config.h and uncomment:
    #define BOARD_ESP8266_OLED

OPTION C: PlatformIO CLI
  python -m pip install platformio
  python -m platformio run -e nodemcuv2 -t upload
  python -m platformio run -e nodemcuv2 -t uploadfs


STEP 2: RUN THE DESKTOP CLIENT
-------------------------------
Prerequisites:
  - Java 11+  https://adoptium.net
    Pick "Latest LTS" and select the correct architecture:
      Windows x64 .msi   - for Intel or AMD processors (most PCs)
      Windows aarch64     - for ARM processors (Surface Pro X, etc.)
    If you get an error about "arm" or wrong architecture, you
    downloaded the wrong one. Uninstall and get x64 instead.

Steps:
  1. Open the MorseClient folder
  2. Double-click run.bat  (or: java -jar MorseClient.jar)
  3. Select the serial port (COM3, COM4, etc.) from the dropdown
  4. Click Connect - the dot turns green
  5. Click Start - morse tones play through your speakers
  6. Type the letter you hear - results show in the log
  7. Click Stop when done - see your accuracy and weakest characters


CONTROLS
--------
  Port dropdown    Select the USB serial port (click Refresh to rescan)
  Connect          Opens/closes the serial connection
  Start/Stop       Starts or stops a training session
  Show Answers     When unchecked, hides what character was sent
                   (forces you to decode by ear only)
  Pitch slider     Tone frequency (300-2400 Hz)
  Spacing slider   Gap between dits/dahs (25-300%, 100% = standard)


TIPS
----
  - The whole window captures keystrokes. Just type - no need to
    click a text field.
  - When you close the app, a CSV log file is saved to the current
    directory with your session stats.
  - If the serial port doesn't appear, check that the USB-serial
    driver is installed and the board is plugged in.
  - On Mac/Linux: run.sh or just "java -jar MorseClient.jar"


TROUBLESHOOTING
---------------
  "Java not found"
    Install Java 11+ from https://adoptium.net

  Error about "arm" or wrong architecture
    You installed the wrong Java version. Most Windows PCs need x64.
    Uninstall the ARM version, then download the x64 .msi from:
    https://adoptium.net

  Serial port not listed
    Install the USB-serial driver (CH340 or CP2102, see above)

  "Failed to open COMx"
    Close any other program using that port (Arduino IDE, PuTTY, etc.)

  No sound
    Check your system volume. The tone plays through your default
    audio output.
