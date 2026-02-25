#pragma once

#include <Arduino.h>

namespace MorseKey {
    // Initialize key pins with internal pullups
    void begin();

    // Poll key state, decode timing, feed characters to trainer
    // Call from loop()
    void update();
}
