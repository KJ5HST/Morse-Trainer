#pragma once

#include <Arduino.h>

namespace Buzzer {
    void begin();
    void toneOn();
    void toneOff();
    bool isOn();
}
