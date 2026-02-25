#pragma once

#include <Arduino.h>
#include "trainer.h"

namespace OledDisplay {
    // Initialize display. Returns true if display found on I2C.
    bool begin();

    // Refresh display (call from loop, throttled internally)
    void update();

    // Trainer event handler — update display state
    void onTrainerEvent(const TrainerEvent& evt);

    // Morse element callback — show keying indicator
    void onMorseElement(bool on);
}
