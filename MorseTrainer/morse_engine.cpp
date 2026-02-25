#include "morse_engine.h"
#include "config.h"
#include "buzzer.h"

// --- Morse binary tree table (from original MTR_V2) ---
// ITU with punctuation (without non-english characters)
static const int morseTreetop = 63;
static const char morseTable[] PROGMEM =
    "*5*H*4*S***V*3*I***F***U?*_**2*E*&*L\"**R*+.****A***P@**W***J'1* *6-B*=*D*/"
    "*X***N***C;(!K***Y***T*7*Z**,G***Q***M:8*!***O*9***0*";

static const int morseTableLength = (morseTreetop * 2) + 1;
static const int morseTreeLevels = 6; // log2(64) = 6

// --- State ---
static volatile char morseSignalString[7];
static volatile bool sendingMorse = false;
static volatile int tick;
static volatile bool ex = false;
static volatile int stat;
static volatile int morseSignals;
static volatile int morseSignalPos;
static volatile int currentSpeed = DEFAULT_SPEED;

static MorseElementCB elementCB = nullptr;
static MorseCharDoneCB charDoneCB = nullptr;
static volatile char currentChar = 0;

static Ticker morseTicker;

// Forward declaration
static void transmitMorse();

void MorseEngine::begin() {
    pinMode(MORSE_LED_PIN, OUTPUT);
    digitalWrite(MORSE_LED_PIN, LOW);
    Buzzer::begin();
    sendingMorse = false;
    stat = 1;
}

void MorseEngine::onElement(MorseElementCB cb) {
    elementCB = cb;
}

void MorseEngine::onCharDone(MorseCharDoneCB cb) {
    charDoneCB = cb;
}

bool MorseEngine::isSending() {
    return sendingMorse;
}

void MorseEngine::setSpeed(int wpm) {
    if (wpm < MIN_SPEED) wpm = MIN_SPEED;
    if (wpm > MAX_SPEED) wpm = MAX_SPEED;
    currentSpeed = wpm;
    // Tick interval: 6000/wpm ms (matching original: TIMER_MULT * 6 / value microseconds -> 6000/wpm ms)
    unsigned long interval = 6000UL / wpm;
    if (interval < 1) interval = 1;
    morseTicker.detach();
    morseTicker.attach_ms(interval, transmitMorse);
}

int MorseEngine::getSpeed() {
    return currentSpeed;
}

String MorseEngine::getPattern(char ch) {
    // Convert to uppercase
    if (ch > 96) ch -= 32;

    // Find character in morse table
    int i;
    for (i = 0; i < morseTableLength; i++) {
        if (pgm_read_byte(morseTable + i) == ch) break;
    }
    if (i >= morseTableLength) return "";

    int morseTablePos = i + 1; // 1-based

    // Find level in binary tree
    int test;
    int startLevel;
    for (startLevel = 0; startLevel < morseTreeLevels; startLevel++) {
        test = (morseTablePos + (1 << startLevel)) % (2 << startLevel);
        if (test == 0) break;
    }
    int numSignals = morseTreeLevels - startLevel;

    if (numSignals <= 0) return " "; // space character

    char buf[7];
    int pos = 0;
    int tPos = morseTablePos;

    for (int j = startLevel; j < morseTreeLevels; j++) {
        int add = (1 << j);
        test = (tPos + add) / (2 << j);
        if (test & 1) {
            tPos += add;
            buf[numSignals - 1 - pos++] = '.';
        } else {
            tPos -= add;
            buf[numSignals - 1 - pos++] = '-';
        }
    }
    buf[pos] = '\0';
    return String(buf);
}

void MorseEngine::sendLetter(char encodeMorseChar) {
    // Convert to uppercase
    if (encodeMorseChar > 96) encodeMorseChar -= 32;
    currentChar = encodeMorseChar;

    // Find character in morse table
    int i;
    for (i = 0; i < morseTableLength; i++) {
        if (pgm_read_byte(morseTable + i) == encodeMorseChar) break;
    }
    int morseTablePos = i + 1; // 1-based

    // Find level in binary tree
    int test;
    int startLevel;
    for (startLevel = 0; startLevel < morseTreeLevels; startLevel++) {
        test = (morseTablePos + (1 << startLevel)) % (2 << startLevel);
        if (test == 0) break;
    }
    morseSignals = morseTreeLevels - startLevel;
    morseSignalPos = 0;

    if (morseSignals > 0) {
        // Build morse signal string (backwards from last to first)
        int pos = 0;
        int tPos = morseTablePos;
        for (int j = startLevel; j < morseTreeLevels; j++) {
            int add = (1 << j);
            test = (tPos + add) / (2 << j);
            if (test & 1) {
                tPos += add;
                morseSignalString[morseSignals - 1 - pos++] = '.';
            } else {
                tPos -= add;
                morseSignalString[morseSignals - 1 - pos++] = '-';
            }
        }
        morseSignalString[pos] = '\0';
    } else {
        // Space character
        morseSignalString[0] = ' ';
        morseSignalString[1] = '\0';
        morseSignalPos = 0;
        morseSignals = 1;
    }

    ex = false;
    stat = 1;
    morseSignalPos = 0;
    sendingMorse = true;
}

// --- 15-state FSM (faithful port of original transmitMorse ISR) ---
static void IRAM_ATTR transmitMorse() {
    // Run one step of the FSM per tick (ex flag controls single-step)
    ex = false;
    while (!ex && sendingMorse) {
        switch (stat) {
            case 1: // Check if output is LOW or HIGH
                if (digitalRead(MORSE_LED_PIN) == LOW) stat = 2;
                else stat = 3;
                break;

            case 2: // Output is LOW — decide what to do next
                stat = 7; // default: turn on
                if (morseSignalPos == 0) stat = 4;  // first element
                if (morseSignalString[morseSignalPos] == '\0') stat = 5; // end of char
                if (morseSignalString[0] == ' ') stat = 6; // space
                break;

            case 3: // Output is HIGH — check dot or dash
                if (morseSignalString[morseSignalPos] == '.') stat = 9;
                else stat = 8;
                break;

            case 4: // First element — turn ON
                digitalWrite(MORSE_LED_PIN, HIGH);
                Buzzer::toneOn();
                if (elementCB) elementCB(true);
                stat = 3;
                ex = true;
                tick = 0;
                break;

            case 5: // End of character — wait inter-char gap
                if (tick < END_TICKS) stat = 10;
                else stat = 11;
                break;

            case 6: // Space — wait word gap
                if (tick < SPACE_TICKS) stat = 12;
                else stat = 13;
                break;

            case 7: // Subsequent element — turn ON
                digitalWrite(MORSE_LED_PIN, HIGH);
                Buzzer::toneOn();
                if (elementCB) elementCB(true);
                ex = true;
                stat = 3;
                break;

            case 8: // Dash timing
                if (tick < DASH_TICKS) stat = 15;
                else stat = 14;
                break;

            case 9: // Dot done — turn OFF, advance
                digitalWrite(MORSE_LED_PIN, LOW);
                Buzzer::toneOff();
                if (elementCB) elementCB(false);
                morseSignalPos++;
                stat = 2;
                ex = true;
                break;

            case 10: // Waiting inter-char gap
                tick++;
                stat = 5;
                ex = true;
                break;

            case 11: // Inter-char gap done — character finished
                tick = 0;
                ex = true;
                sendingMorse = false;
                if (charDoneCB) charDoneCB(currentChar);
                break;

            case 12: // Waiting word gap
                tick++;
                stat = 6;
                ex = true;
                break;

            case 13: // Word gap done
                tick = 0;
                sendingMorse = false;
                ex = true;
                if (charDoneCB) charDoneCB(currentChar);
                break;

            case 14: // Dash done — turn OFF, advance
                digitalWrite(MORSE_LED_PIN, LOW);
                Buzzer::toneOff();
                if (elementCB) elementCB(false);
                tick = 0;
                morseSignalPos++;
                stat = 1;
                ex = true;
                break;

            case 15: // Dash timing tick
                tick++;
                stat = 8;
                ex = true;
                break;

            default:
                stat = 1;
                break;
        }
    }
    ex = false;
}
