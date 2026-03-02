#pragma once

#include <Arduino.h>
#include "trainer.h"

namespace WebServer {
    void begin();
    void update();  // call from loop() for cleanup
    void onTrainerEvent(const TrainerEvent& evt);
}
