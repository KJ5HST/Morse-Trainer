#include "buzzer.h"
#include "config.h"

static bool _on = false;

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    analogWriteFreq(TONE_FREQ);
    analogWrite(BUZZER_PIN, 0);
    _on = false;
}

void Buzzer::toneOn() {
    if (!_on) {
        analogWriteFreq(TONE_FREQ);
        analogWrite(BUZZER_PIN, 512);  // 50% duty cycle
        _on = true;
    }
}

void Buzzer::toneOff() {
    if (_on) {
        analogWrite(BUZZER_PIN, 0);
        _on = false;
    }
}

bool Buzzer::isOn() {
    return _on;
}
