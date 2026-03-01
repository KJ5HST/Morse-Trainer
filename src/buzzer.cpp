#include "buzzer.h"
#include "config.h"

static bool _on = false;
static int _freq = TONE_FREQ;
static bool _active = BUZZER_ACTIVE_DEFAULT;

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    _on = false;
}

void Buzzer::toneOn() {
    if (!_on) {
        if (_active) {
            digitalWrite(BUZZER_PIN, HIGH);
        } else {
            analogWriteFreq(_freq);
            analogWrite(BUZZER_PIN, 512);
        }
        _on = true;
    }
}

void Buzzer::toneOff() {
    if (_on) {
        if (_active) {
            digitalWrite(BUZZER_PIN, LOW);
        } else {
            analogWrite(BUZZER_PIN, 0);
        }
        _on = false;
    }
}

bool Buzzer::isOn() {
    return _on;
}

void Buzzer::setFrequency(int hz) {
    _freq = hz;
    if (_on && !_active) {
        analogWriteFreq(_freq);
    }
}

int Buzzer::getFrequency() {
    return _freq;
}

void Buzzer::setActive(bool active) {
    bool wasOn = _on;
    if (wasOn) toneOff();
    _active = active;
    if (wasOn) toneOn();
}

bool Buzzer::isActive() {
    return _active;
}
