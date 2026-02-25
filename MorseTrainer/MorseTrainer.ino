/*
  ESP8266 Morse Trainer
  Adaptive morse code trainer with serial + web UI.

  Board: NodeMCU 1.0 (ESP-12E Module)

  Required libraries (install via Sketch > Include Library > Manage Libraries):
    - ArduinoJson by Benoit Blanchon
    - ESPAsyncTCP by dvarrel
    - ESPAsyncWebServer by lacamera
    - U8g2 by oliver (OLED display)

  Upload the data/ folder to LittleFS using:
    Arduino IDE 2.x: https://github.com/earlephilhower/arduino-littlefs-upload
*/

// Library includes — must be in .ino for Arduino IDE to detect them
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "config.h"
#include "storage.h"
#include "morse_engine.h"
#include "buzzer.h"
#include "trainer.h"
#include "serial_interface.h"
#include "web_server.h"
#include "oled_display.h"
#include "morse_key.h"

// Unified event handler — broadcasts to serial, web, and OLED
static void onTrainerEvent(const TrainerEvent& evt) {
    SerialInterface::onTrainerEvent(evt);
    WebServer::onTrainerEvent(evt);
    OledDisplay::onTrainerEvent(evt);
}

// Morse element callback — broadcasts tone on/off to web and OLED
static void onMorseElement(bool on) {
    WebServer::broadcastMorseElement(on);
    OledDisplay::onMorseElement(on);
}

void setup() {
    // Status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW); // on (active low on ESP8266)

    // Serial
    SerialInterface::begin();

    // Storage
    if (!Storage::begin()) {
        Serial.println(F("LittleFS mount failed!"));
    }

    // Morse engine
    MorseEngine::begin();
    MorseEngine::onElement(onMorseElement);

    // Trainer
    trainer.begin();
    trainer.onEvent(onTrainerEvent);

    // Web server (WiFi + HTTP + WebSocket)
    WebServer::begin();

    // OLED display
    OledDisplay::begin();

    // Morse key input
    MorseKey::begin();

    // Random seed
    randomSeed(analogRead(A0) ^ micros());

    digitalWrite(STATUS_LED_PIN, HIGH); // off (ready)
    Serial.println(F("Ready. Type /help for commands."));
}

void loop() {
    SerialInterface::update();
    MorseKey::update();
    trainer.update();
    WebServer::update();
    OledDisplay::update();
}
