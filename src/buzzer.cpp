#include "buzzer.h"
#include "config.h"

static bool _on = false;
static int _freq = TONE_FREQ;

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    analogWriteFreq(_freq);
    analogWrite(BUZZER_PIN, 0);
    _on = false;
}

void Buzzer::toneOn() {
    if (!_on) {
        analogWriteFreq(_freq);
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

void Buzzer::setFrequency(int hz) {
    _freq = hz;
    if (_on) {
        analogWriteFreq(_freq);
    }
}

int Buzzer::getFrequency() {
    return _freq;
}
