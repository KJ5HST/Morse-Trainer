#include "morse_engine.h"
#include "morse_table.h"
#include "config.h"
#include "buzzer.h"

// --- FSM states ---
enum MorseFsmState {
    FSM_CHECK_OUTPUT = 1,
    FSM_OUTPUT_LOW,
    FSM_OUTPUT_HIGH,
    FSM_FIRST_ON,
    FSM_CHAR_GAP,
    FSM_WORD_GAP,
    FSM_NEXT_ON,
    FSM_DASH_TIMING,
    FSM_DOT_DONE,
    FSM_CHAR_GAP_WAIT,
    FSM_CHAR_DONE,
    FSM_WORD_GAP_WAIT,
    FSM_WORD_DONE,
    FSM_DASH_DONE,
    FSM_DASH_TICK,
};

// --- State ---
static volatile char morseSignalString[7];
static volatile bool sendingMorse = false;
static volatile int tick;
static volatile bool stepped = false;
static volatile MorseFsmState fsmState;
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
    fsmState = FSM_CHECK_OUTPUT;
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
    // Tick interval: 6000/wpm ms (rounded to nearest)
    unsigned long interval = (6000UL + wpm / 2) / wpm;
    if (interval < 1) interval = 1;
    morseTicker.detach();
    morseTicker.attach_ms(interval, transmitMorse);
}

int MorseEngine::getSpeed() {
    return currentSpeed;
}

int MorseEngine::getPattern(char ch, char* buf) {
    return morseEncode(ch, buf);
}

void MorseEngine::sendLetter(char encodeMorseChar) {
    // Convert to uppercase
    if (encodeMorseChar > 96) encodeMorseChar -= 32;
    currentChar = encodeMorseChar;

    char buf[7];
    int n = morseEncode(encodeMorseChar, buf);

    if (n == 0) {
        // Character not found — treat as space
        buf[0] = ' ';
        buf[1] = '\0';
        n = 1;
    }

    // Copy to volatile signal string
    for (int i = 0; i <= n; i++) {
        morseSignalString[i] = buf[i];
    }
    morseSignals = n;
    morseSignalPos = 0;

    stepped = false;
    fsmState = FSM_CHECK_OUTPUT;
    sendingMorse = true;
}

// --- 15-state FSM (faithful port of original transmitMorse ISR) ---
static void transmitMorse() {
    stepped = false;
    while (!stepped && sendingMorse) {
        switch (fsmState) {
            case FSM_CHECK_OUTPUT:
                if (digitalRead(MORSE_LED_PIN) == LOW) fsmState = FSM_OUTPUT_LOW;
                else fsmState = FSM_OUTPUT_HIGH;
                break;

            case FSM_OUTPUT_LOW:
                fsmState = FSM_NEXT_ON; // default: turn on next element
                if (morseSignalPos == 0) fsmState = FSM_FIRST_ON;
                if (morseSignalString[morseSignalPos] == '\0') fsmState = FSM_CHAR_GAP;
                if (morseSignalString[0] == ' ') fsmState = FSM_WORD_GAP;
                break;

            case FSM_OUTPUT_HIGH:
                if (morseSignalString[morseSignalPos] == '.') fsmState = FSM_DOT_DONE;
                else fsmState = FSM_DASH_TIMING;
                break;

            case FSM_FIRST_ON:
                digitalWrite(MORSE_LED_PIN, HIGH);
                Buzzer::toneOn();
                if (elementCB) elementCB(true);
                fsmState = FSM_OUTPUT_HIGH;
                stepped = true;
                tick = 0;
                break;

            case FSM_CHAR_GAP:
                if (tick < END_TICKS) fsmState = FSM_CHAR_GAP_WAIT;
                else fsmState = FSM_CHAR_DONE;
                break;

            case FSM_WORD_GAP:
                if (tick < SPACE_TICKS) fsmState = FSM_WORD_GAP_WAIT;
                else fsmState = FSM_WORD_DONE;
                break;

            case FSM_NEXT_ON:
                digitalWrite(MORSE_LED_PIN, HIGH);
                Buzzer::toneOn();
                if (elementCB) elementCB(true);
                stepped = true;
                fsmState = FSM_OUTPUT_HIGH;
                break;

            case FSM_DASH_TIMING:
                if (tick < DASH_TICKS) fsmState = FSM_DASH_TICK;
                else fsmState = FSM_DASH_DONE;
                break;

            case FSM_DOT_DONE:
                digitalWrite(MORSE_LED_PIN, LOW);
                Buzzer::toneOff();
                if (elementCB) elementCB(false);
                morseSignalPos++;
                fsmState = FSM_OUTPUT_LOW;
                stepped = true;
                break;

            case FSM_CHAR_GAP_WAIT:
                tick++;
                fsmState = FSM_CHAR_GAP;
                stepped = true;
                break;

            case FSM_CHAR_DONE:
                tick = 0;
                stepped = true;
                sendingMorse = false;
                if (charDoneCB) charDoneCB(currentChar);
                break;

            case FSM_WORD_GAP_WAIT:
                tick++;
                fsmState = FSM_WORD_GAP;
                stepped = true;
                break;

            case FSM_WORD_DONE:
                tick = 0;
                sendingMorse = false;
                stepped = true;
                if (charDoneCB) charDoneCB(currentChar);
                break;

            case FSM_DASH_DONE:
                digitalWrite(MORSE_LED_PIN, LOW);
                Buzzer::toneOff();
                if (elementCB) elementCB(false);
                tick = 0;
                morseSignalPos++;
                fsmState = FSM_CHECK_OUTPUT;
                stepped = true;
                break;

            case FSM_DASH_TICK:
                tick++;
                fsmState = FSM_DASH_TIMING;
                stepped = true;
                break;

            default:
                fsmState = FSM_CHECK_OUTPUT;
                break;
        }
    }
    stepped = false;
}
