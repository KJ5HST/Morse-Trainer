#pragma once

#include <Arduino.h>

// Shared morse binary tree table (ITU with punctuation)
extern const char morseTable[] PROGMEM;
extern const int morseTreetop;
extern const int morseTableLength;
extern const int morseTreeLevels;

// Encode a character to its dot/dash pattern.
// Writes pattern to buf (must be >= 7 bytes).
// Returns number of elements for normal chars (1-6),
// 1 for space (buf = " "), or 0 if character not found.
int morseEncode(char ch, char* buf);

// Decode a dot/dash pattern string to a character.
// Returns the decoded character, or '\0' if not found.
char morseDecode(const char* pattern);
