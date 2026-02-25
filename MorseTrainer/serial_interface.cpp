#include "serial_interface.h"
#include "morse_engine.h"
#include "storage.h"
#include "config.h"

static String inputBuffer;

static void printHelp() {
    Serial.println(F("=== Morse Trainer Commands ==="));
    Serial.println(F("/start [profile] [speed]  - Start training (profile 0-9, speed 20-200)"));
    Serial.println(F("/stop                     - Stop training"));
    Serial.println(F("/speed N                  - Set speed to N WPM"));
    Serial.println(F("/profile N                - Set profile (0-9)"));
    Serial.println(F("/status                   - Show current status"));
    Serial.println(F("/probs                    - Show character probabilities"));
    Serial.println(F("/help                     - Show this help"));
    Serial.println(F("/wifi [ap|sta] [ssid] [pass] - Configure WiFi"));
    Serial.println(F("Any other character       - Training input"));
}

static void printStatus() {
    Serial.print(F("Running: ")); Serial.println(trainer.isRunning() ? "yes" : "no");
    Serial.print(F("Speed: ")); Serial.print(trainer.getSpeed()); Serial.println(F(" WPM"));
    Serial.print(F("Profile: ")); Serial.println(trainer.getProfile());
    Serial.print(F("PlainText: ")); Serial.println(trainer.isPlainText() ? "yes" : "no");
}

static void printProbs() {
    const uint8_t* probs = trainer.getProbs();
    Serial.println(F("Character probabilities:"));
    for (int i = 0; i < CHAR_COUNT; i++) {
        if (probs[i] > 0) {
            Serial.print((char)(FIRST_CHAR + i));
            Serial.print(F(": "));
            Serial.println(probs[i]);
        }
    }
}

static void processCommand(const String& cmd) {
    if (cmd.startsWith("/start")) {
        int profile = DEFAULT_PROFILE;
        int speed = DEFAULT_SPEED;

        // Parse optional arguments
        int firstSpace = cmd.indexOf(' ', 1);
        if (firstSpace > 0) {
            String args = cmd.substring(firstSpace + 1);
            args.trim();
            int secondSpace = args.indexOf(' ');
            if (secondSpace > 0) {
                profile = args.substring(0, secondSpace).toInt();
                speed = args.substring(secondSpace + 1).toInt();
            } else {
                profile = args.toInt();
            }
        }

        profile = constrain(profile, 0, 9);
        speed = constrain(speed, MIN_SPEED, MAX_SPEED);
        Serial.print(F("Starting: profile=")); Serial.print(profile);
        Serial.print(F(" speed=")); Serial.println(speed);
        trainer.start(profile, speed);
    }
    else if (cmd.startsWith("/stop")) {
        trainer.stop();
        Serial.println(F("Training stopped."));
    }
    else if (cmd.startsWith("/speed")) {
        int sp = cmd.substring(7).toInt();
        if (sp >= MIN_SPEED && sp <= MAX_SPEED) {
            trainer.setSpeed(sp);
            Serial.print(F("Speed set to ")); Serial.println(sp);
        } else {
            Serial.println(F("Speed must be 20-200"));
        }
    }
    else if (cmd.startsWith("/profile")) {
        int p = cmd.substring(9).toInt();
        if (p >= 0 && p <= 9) {
            trainer.setProfile(p);
            Serial.print(F("Profile set to ")); Serial.println(p);
        } else {
            Serial.println(F("Profile must be 0-9"));
        }
    }
    else if (cmd.startsWith("/status")) {
        printStatus();
    }
    else if (cmd.startsWith("/probs")) {
        printProbs();
    }
    else if (cmd.startsWith("/wifi")) {
        String args = cmd.substring(6);
        args.trim();
        Storage::Config cfg;
        Storage::loadConfig(cfg);

        int sp1 = args.indexOf(' ');
        if (sp1 < 0) {
            Serial.print(F("WiFi mode: ")); Serial.println(cfg.wifiMode);
            Serial.print(F("STA SSID: ")); Serial.println(cfg.staSSID);
            Serial.println(F("Reboot to apply changes."));
        } else {
            cfg.wifiMode = args.substring(0, sp1);
            String rest = args.substring(sp1 + 1);
            int sp2 = rest.indexOf(' ');
            if (sp2 > 0) {
                cfg.staSSID = rest.substring(0, sp2);
                cfg.staPass = rest.substring(sp2 + 1);
            } else {
                cfg.staSSID = rest;
                cfg.staPass = "";
            }
            Storage::saveConfig(cfg);
            Serial.print(F("WiFi configured: ")); Serial.print(cfg.wifiMode);
            Serial.print(F(" SSID=")); Serial.println(cfg.staSSID);
            Serial.println(F("Reboot to apply."));
        }
    }
    else if (cmd.startsWith("/help")) {
        printHelp();
    }
    else {
        Serial.print(F("Unknown command: ")); Serial.println(cmd);
        printHelp();
    }
}

void SerialInterface::begin() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("=== ESP8266 Morse Trainer ==="));
    Serial.println(F("Type /help for commands"));
    inputBuffer = "";
}

void SerialInterface::update() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                if (inputBuffer.startsWith("/")) {
                    processCommand(inputBuffer);
                } else {
                    // Send each character as training input
                    for (unsigned int i = 0; i < inputBuffer.length(); i++) {
                        trainer.processInput(inputBuffer[i]);
                    }
                }
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
            // If not in command mode and training is running, process single chars immediately
            if (trainer.isRunning() && c != '/' && inputBuffer.length() == 1 && !inputBuffer.startsWith("/")) {
                trainer.processInput(c);
                inputBuffer = "";
            }
        }
    }
}

void SerialInterface::onTrainerEvent(const TrainerEvent& evt) {
    switch (evt.type) {
        case TrainerEvent::CHAR_SENT:
            if (evt.sentChar != ' ') {
                Serial.print(F("[TX] ")); Serial.print(evt.sentChar);
                Serial.print(F(" (")); Serial.print(evt.pattern);
                Serial.print(F(") dist=")); Serial.println(evt.queueDist);
            }
            break;

        case TrainerEvent::RESULT:
            if (evt.correct) {
                Serial.print(F("[OK] ")); Serial.print(evt.typedChar);
                Serial.print(F(" prob=")); Serial.println(evt.prob);
            } else {
                Serial.print(F("[ERR] typed=")); Serial.print(evt.typedChar);
                Serial.print(F(" expected=")); Serial.print(evt.expectedChar);
                Serial.print(F(" prob=")); Serial.println(evt.prob);
            }
            break;

        case TrainerEvent::SPEED_CHANGE:
            Serial.print(F("[SPEED] ")); Serial.print(evt.speed);
            Serial.print(F(" WPM (")); Serial.print(evt.direction);
            Serial.println(F(")"));
            break;

        case TrainerEvent::SESSION_STATE:
            Serial.print(F("[SESSION] "));
            Serial.println(evt.running ? F("started") : F("stopped"));
            break;

        case TrainerEvent::CONTEXT_LOST:
            Serial.println(F("[CONTEXT LOST] Resynchronizing..."));
            Serial.print(F("[SPEED] ")); Serial.print(evt.speed);
            Serial.println(F(" WPM (down)"));
            break;
    }
}
