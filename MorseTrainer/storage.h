#pragma once

#include <Arduino.h>
#include "config.h"

namespace Storage {
    // Initialize LittleFS. Returns true on success.
    bool begin();

    // Probability persistence
    bool saveProbs(const uint8_t probs[CHAR_COUNT]);
    bool loadProbs(uint8_t probs[CHAR_COUNT]);

    // Config persistence (speed, last profile, WiFi settings)
    struct Config {
        int speed = DEFAULT_SPEED;
        int profile = DEFAULT_PROFILE;
        String wifiMode = "ap";      // "ap" or "sta"
        String staSSID = "";
        String staPass = "";
    };

    bool saveConfig(const Config& cfg);
    bool loadConfig(Config& cfg);
}
