#pragma once

#include <Arduino.h>
#include <Ticker.h>

// Callback types for morse engine events
using MorseElementCB = void (*)(bool on);   // tone on/off
using MorseCharDoneCB = void (*)(char ch);  // character finished sending

namespace MorseEngine {
    void begin();

    // Set callbacks for tone events and character-done events
    void onElement(MorseElementCB cb);
    void onCharDone(MorseCharDoneCB cb);

    // Queue a character for morse transmission
    void sendLetter(char ch);

    // Is the engine currently sending?
    bool isSending();

    // Set speed in WPM — adjusts ticker interval
    void setSpeed(int wpm);
    int  getSpeed();

    // Look up the morse pattern string (dots/dashes) for a character
    // Returns empty string if not found.
    String getPattern(char ch);
}
