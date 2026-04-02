/**
 * Minimal Arduino.h shim for native (desktop) unit testing.
 * Provides PROGMEM, pgm_read_byte, and other macros needed
 * to compile Arduino source files on a native platform.
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <algorithm>

// PROGMEM is a no-op on native — data is already in RAM
#define PROGMEM

// Flash-read macros just dereference the pointer
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_ptr(addr)  (*(const void* const*)(addr))

// Arduino utility macros
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

using std::min;
using std::max;

// random() compatible with Arduino API
inline long random(long lo, long hi) {
    if (lo >= hi) return lo;
    return lo + (std::rand() % (hi - lo));
}
