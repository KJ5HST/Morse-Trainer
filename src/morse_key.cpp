#include "morse_key.h"
#include "morse_table.h"
#include "config.h"
#include "buzzer.h"
#include "trainer.h"
#include "morse_engine.h"

// --- Timing helpers ---
static inline unsigned long ditTimeMs() {
    return 1200UL / (unsigned long)trainer.getSpeed();
}

// --- State ---
static char patternBuf[8]; // max 6 elements + null
static int patternLen = 0;
static unsigned long pressStart = 0;
static unsigned long releaseTime = 0;
static bool keyWasDown = false;
static bool spaceSent = false;

#if KEY_MODE == KEY_MODE_IAMBIC
// Iambic state
enum IambicState { IDLE, DIT_ON, DIT_OFF, DAH_ON, DAH_OFF };
static IambicState iState = IDLE;
static unsigned long elementStart = 0;
static bool lastWasDit = false;
static bool ditMemory = false;
static bool dahMemory = false;
#endif

// --- Straight key implementation ---
#if KEY_MODE == KEY_MODE_STRAIGHT

void MorseKey::begin() {
    pinMode(KEY_DIT_PIN, INPUT_PULLUP);
    patternLen = 0;
    patternBuf[0] = '\0';
    keyWasDown = false;
    spaceSent = false;
}

void MorseKey::update() {
    if (!trainer.isRunning()) return;

    bool down = (digitalRead(KEY_DIT_PIN) == LOW);
    unsigned long now = millis();
    unsigned long dit = ditTimeMs();

    if (down && !keyWasDown) {
        // Key just pressed
        pressStart = now;
        keyWasDown = true;
        spaceSent = false;
        Buzzer::toneOn();
    } else if (!down && keyWasDown) {
        // Key just released
        Buzzer::toneOff();
        keyWasDown = false;
        releaseTime = now;

        unsigned long duration = now - pressStart;
        if (patternLen < 6) {
            patternBuf[patternLen++] = (duration < 2 * dit) ? '.' : '-';
            patternBuf[patternLen] = '\0';
        }
    } else if (!down && !keyWasDown && patternLen > 0) {
        // Key is up and we have accumulated pattern
        unsigned long idle = now - releaseTime;

        if (idle > 3 * dit) {
            // Character gap — decode and submit
            char ch = morseDecode(patternBuf);
            if (ch) {
                trainer.processInput(ch);
            }
            patternLen = 0;
            patternBuf[0] = '\0';

            if (idle > 7 * dit && !spaceSent) {
                trainer.processInput(' ');
                spaceSent = true;
            }
        }
    }
}

#else // KEY_MODE == KEY_MODE_IAMBIC

// --- Iambic paddle implementation (Mode B) ---

void MorseKey::begin() {
    pinMode(KEY_DIT_PIN, INPUT_PULLUP);
    pinMode(KEY_DAH_PIN, INPUT_PULLUP);
    patternLen = 0;
    patternBuf[0] = '\0';
    iState = IDLE;
    spaceSent = false;
    ditMemory = false;
    dahMemory = false;
}

void MorseKey::update() {
    if (!trainer.isRunning()) return;

    bool ditDown = (digitalRead(KEY_DIT_PIN) == LOW);
    bool dahDown = (digitalRead(KEY_DAH_PIN) == LOW);
    unsigned long now = millis();
    unsigned long dit = ditTimeMs();
    unsigned long dah = 3 * dit;

    // Capture squeeze memories while elements are playing
    if (ditDown) ditMemory = true;
    if (dahDown) dahMemory = true;

    switch (iState) {
        case IDLE:
            if (ditDown || ditMemory) {
                // Start a dit
                iState = DIT_ON;
                elementStart = now;
                Buzzer::toneOn();
                ditMemory = false;
                dahMemory = dahDown; // capture dah during dit start
                spaceSent = false;
            } else if (dahDown || dahMemory) {
                // Start a dah
                iState = DAH_ON;
                elementStart = now;
                Buzzer::toneOn();
                dahMemory = false;
                ditMemory = ditDown; // capture dit during dah start
                spaceSent = false;
            } else if (patternLen > 0) {
                // No paddle pressed — check for character/word gap
                unsigned long idle = now - releaseTime;
                if (idle > 3 * dit) {
                    char ch = morseDecode(patternBuf);
                    if (ch) {
                        trainer.processInput(ch);
                    }
                    patternLen = 0;
                    patternBuf[0] = '\0';

                    if (idle > 7 * dit && !spaceSent) {
                        trainer.processInput(' ');
                        spaceSent = true;
                    }
                }
            }
            break;

        case DIT_ON:
            if (now - elementStart >= dit) {
                // Dit tone complete — turn off
                Buzzer::toneOff();
                if (patternLen < 6) {
                    patternBuf[patternLen++] = '.';
                    patternBuf[patternLen] = '\0';
                }
                lastWasDit = true;
                iState = DIT_OFF;
                elementStart = now;
                releaseTime = now;
            }
            break;

        case DIT_OFF:
            if (now - elementStart >= dit) {
                // Inter-element gap complete
                // Iambic Mode B: check memories for squeeze alternation
                if (dahMemory) {
                    iState = DAH_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    dahMemory = false;
                    ditMemory = ditDown;
                } else if (ditDown) {
                    iState = DIT_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    ditMemory = false;
                    dahMemory = dahDown;
                } else if (dahDown) {
                    iState = DAH_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    dahMemory = false;
                    ditMemory = ditDown;
                } else {
                    // Nothing pressed — go idle
                    iState = IDLE;
                    releaseTime = now;
                }
            }
            break;

        case DAH_ON:
            if (now - elementStart >= dah) {
                // Dah tone complete — turn off
                Buzzer::toneOff();
                if (patternLen < 6) {
                    patternBuf[patternLen++] = '-';
                    patternBuf[patternLen] = '\0';
                }
                lastWasDit = false;
                iState = DAH_OFF;
                elementStart = now;
                releaseTime = now;
            }
            break;

        case DAH_OFF:
            if (now - elementStart >= dit) {
                // Inter-element gap complete
                // Iambic Mode B: check memories for squeeze alternation
                if (ditMemory) {
                    iState = DIT_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    ditMemory = false;
                    dahMemory = dahDown;
                } else if (dahDown) {
                    iState = DAH_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    dahMemory = false;
                    ditMemory = ditDown;
                } else if (ditDown) {
                    iState = DIT_ON;
                    elementStart = now;
                    Buzzer::toneOn();
                    ditMemory = false;
                    dahMemory = dahDown;
                } else {
                    // Nothing pressed — go idle
                    iState = IDLE;
                    releaseTime = now;
                }
            }
            break;
    }
}

#endif
