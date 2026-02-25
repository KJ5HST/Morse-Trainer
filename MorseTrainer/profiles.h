#pragma once

#include <Arduino.h>
#include "config.h"

// P1-P9 probability profiles stored in PROGMEM.
// Each array has CHAR_COUNT (58) entries, indexed by (ASCII - FIRST_CHAR).
// P0 is "load from saved" — not stored here.

extern const uint8_t P1[CHAR_COUNT] PROGMEM;
extern const uint8_t P2[CHAR_COUNT] PROGMEM;
extern const uint8_t P3[CHAR_COUNT] PROGMEM;
extern const uint8_t P4[CHAR_COUNT] PROGMEM;
extern const uint8_t P5[CHAR_COUNT] PROGMEM;
extern const uint8_t P6[CHAR_COUNT] PROGMEM;
extern const uint8_t P7[CHAR_COUNT] PROGMEM;
extern const uint8_t P8[CHAR_COUNT] PROGMEM;
extern const uint8_t P9[CHAR_COUNT] PROGMEM;

// Get a pointer to a profile array by index (1-9). Returns nullptr for invalid.
const uint8_t* getProfile(uint8_t index);
