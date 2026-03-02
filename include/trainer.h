#pragma once

#include <Arduino.h>
#include "config.h"

// Callback types for trainer events
struct TrainerEvent {
    enum Type {
        CHAR_SENT,      // a character was sent via morse
        RESULT,         // correct/wrong result for a character
        SPEED_CHANGE,   // speed was adjusted
        SESSION_STATE,  // started/stopped
        CONTEXT_LOST    // trainee fell too far behind
    };

    Type type;

    // CHAR_SENT
    char sentChar;
    char pattern[8];
    int queueDist;

    // RESULT
    bool correct;
    char typedChar;
    char expectedChar;
    uint8_t prob;

    // SPEED_CHANGE
    int speed;
    const char* direction;

    // SESSION_STATE
    bool running;
};

using TrainerEventCB = void (*)(const TrainerEvent& evt);

class Trainer {
public:
    void begin();

    // Set event callback
    void onEvent(TrainerEventCB cb);

    // Start/stop training session
    void start(int profile, int speed);
    void stop();
    bool isRunning() const;

    // Process a typed input character (from serial or websocket)
    void processInput(char ch);

    // Called by main loop to drive the sending state machine
    void update();

    // Getters
    int getSpeed() const;
    int getProfile() const;
    const uint8_t* getProbs() const;
    bool isPlainText() const;

    // Manual overrides
    void setSpeed(int wpm);
    void setProfile(int p);

private:
    TrainerEventCB _eventCB = nullptr;

    uint8_t _charProb[CHAR_COUNT];
    bool _plainText = false;
    bool _running = false;
    int _speed = DEFAULT_SPEED;
    int _profile = DEFAULT_PROFILE;

    // Circular queue
    char _queue[QUEUE_LENGTH + 1];
    int _queueIndexS = 0;  // send index
    int _queueIndexR = 0;  // receive index
    int _lGroup = 0;       // letters in current group
    int _statErrors = 0;
    int _statGroup = 0;
    int _recoverySpaces = 0; // pending spaces after context lost (drained non-blockingly)

    int queueDist() const;
    int indexAdv(int index) const;
    char generateLetter();
    void correct(char letter);
    void wrong(char typed, char expected);
    void analyzeSpeed();
    void contextLost();
    void loadProfile(int profile);
    void emitEvent(const TrainerEvent& evt);
    void sendNextChar();
};

extern Trainer trainer;
