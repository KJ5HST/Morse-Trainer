#include "profiles.h"

// Probability profiles from the original MTR_V2.
// 58 entries each: ASCII 33 ('!') through 90 ('Z').
// Index mapping: charProp[ch - 33]
//   Indices 0-31:  '!' through '@' (punctuation/numbers)
//   Indices 32-57: 'A' through 'Z' (letters)

// P1: All letters evenly distributed
const uint8_t P1[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

// P2: Letters with German text frequency distribution (plainText mode)
const uint8_t P2[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    30, 11, 17, 27, 90, 8, 16, 27, 43, 2, 7, 19, 14, 56, 12, 4,
    0, 37, 34, 31, 21, 5, 10, 1, 1, 7
};

// P3: Numbers only
const uint8_t P3[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// P4: Punctuation marks
const uint8_t P4[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 50, 50, 50, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 0, 50, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// P5: Letters and punctuation marks
const uint8_t P5[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 50, 50, 50, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 0, 50, 0,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

// P6: Beginner 1 (A, E, M, N, T)
const uint8_t P6[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    50, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 50, 50, 0, 0,
    0, 0, 0, 50, 0, 0, 0, 0, 0, 0
};

// P7: Beginner 2 (A, E, G, I, K, M, N, O, R, T)
const uint8_t P7[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    50, 0, 0, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 0,
    0, 50, 0, 50, 0, 0, 0, 0, 0, 0
};

// P8: Beginner 3 (A, D, E, G, I, K, M, N, O, P, R, T, U, W, Z)
const uint8_t P8[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    50, 0, 0, 50, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 50,
    0, 50, 0, 50, 50, 0, 50, 0, 0, 50
};

// P9: Beginner 4 (A, B, C, D, E, F, G, H, I, J, K, M, N, O, P, R, T, U, W, Z)
const uint8_t P9[CHAR_COUNT] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 50, 50, 50, 50,
    0, 50, 0, 50, 50, 0, 50, 0, 0, 50
};

static const uint8_t* const profileTable[] PROGMEM = {
    nullptr, P1, P2, P3, P4, P5, P6, P7, P8, P9
};

const uint8_t* getProfile(uint8_t index) {
    if (index < 1 || index > 9) return nullptr;
    return (const uint8_t*)pgm_read_ptr(&profileTable[index]);
}
