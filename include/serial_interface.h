#pragma once

#include <Arduino.h>
#include "trainer.h"

namespace SerialInterface {
    void begin();
    void update();  // call from loop() to process serial input
    void onTrainerEvent(const TrainerEvent& evt);
}
