#pragma once

#include <Arduino.h>
#include "trainer.h"

namespace WebServer {
    void begin();
    void update();  // call from loop() for cleanup
    void onTrainerEvent(const TrainerEvent& evt);

    // Broadcast a morse element on/off event to all WS clients
    void broadcastMorseElement(bool on);
}
