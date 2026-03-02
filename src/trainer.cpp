#include "trainer.h"
#include "morse_engine.h"
#include "profiles.h"
#include "storage.h"

Trainer trainer;

void Trainer::begin() {
    memset(_charProb, 0, sizeof(_charProb));
    memset(_queue, ' ', sizeof(_queue));
    _running = false;
    _plainText = false;
    _queueIndexS = 0;
    _queueIndexR = 0;
    _lGroup = 0;
    _statErrors = 0;
    _statGroup = 0;
    _recoverySpaces = 0;
}

void Trainer::onEvent(TrainerEventCB cb) {
    _eventCB = cb;
}

void Trainer::start(int profile, int speed) {
    // Reset queue
    memset(_queue, ' ', sizeof(_queue));
    _queueIndexS = 0;
    _queueIndexR = 0;
    _lGroup = 0;
    _statErrors = 0;
    _statGroup = 0;
    _recoverySpaces = 0;
    _plainText = false;

    _speed = constrain(speed, MIN_SPEED, MAX_SPEED);
    _profile = profile;

    loadProfile(profile);

    MorseEngine::setSpeed(_speed);
    _running = true;

    TrainerEvent evt;
    evt.type = TrainerEvent::SESSION_STATE;
    evt.running = true;
    evt.speed = _speed;
    emitEvent(evt);
}

void Trainer::stop() {
    _running = false;

    // Save probabilities
    Storage::saveProbs(_charProb);

    // Save config (load first to preserve WiFi/buzzer settings)
    Storage::Config cfg;
    Storage::loadConfig(cfg);
    cfg.speed = _speed;
    cfg.profile = _profile;
    Storage::saveConfig(cfg);

    TrainerEvent evt;
    evt.type = TrainerEvent::SESSION_STATE;
    evt.running = false;
    evt.speed = _speed;
    emitEvent(evt);
}

bool Trainer::isRunning() const {
    return _running;
}

void Trainer::update() {
    if (!_running) return;
    if (MorseEngine::isSending()) return;

    // Drain recovery spaces non-blockingly (one per tick)
    if (_recoverySpaces > 0) {
        MorseEngine::sendLetter(' ');
        _recoverySpaces--;
        return;
    }

    sendNextChar();

    if (queueDist() >= CONTEXT_LOST_DIST) {
        contextLost();
    }

    if (_statGroup >= STAT_LENGTH) {
        analyzeSpeed();
        _statGroup = 0;
    }
}

void Trainer::sendNextChar() {
    char ch;
    if (_lGroup < GROUP_LENGTH) {
        ch = generateLetter();
        _lGroup++;
    } else {
        ch = ' ';
        _lGroup = 0;
    }

    _queue[_queueIndexS] = ch;
    MorseEngine::sendLetter(ch);

    TrainerEvent evt;
    evt.type = TrainerEvent::CHAR_SENT;
    evt.sentChar = ch;
    MorseEngine::getPattern(ch, evt.pattern);
    evt.queueDist = queueDist();
    emitEvent(evt);

    _queueIndexS = indexAdv(_queueIndexS);
    _statGroup++;
}

void Trainer::processInput(char ch) {
    if (!_running) return;

    // Make uppercase
    if (ch > 96) ch -= 32;

    // Space: synchronize with sender
    if (ch == ' ') {
        while (_queue[_queueIndexR] != ' ' && queueDist() > 0) {
            wrong(' ', _queue[_queueIndexR]);
            _queueIndexR = indexAdv(_queueIndexR);
        }
    }

    if (_queue[_queueIndexR] == ch) {
        correct(ch);
    } else {
        // Check if next character matches (one char lost during reception)
        int nextIdx = indexAdv(_queueIndexR);
        if (_queue[nextIdx] == ch) {
            wrong(_queue[_queueIndexR], _queue[_queueIndexR]);
            _queueIndexR = indexAdv(_queueIndexR);
            correct(ch);
        } else {
            wrong(ch, _queue[_queueIndexR]);
        }
    }

    // Prevent underrun
    if (queueDist() == 0) _queueIndexR = _queueIndexS;

    _queueIndexR = indexAdv(_queueIndexR);
}

int Trainer::getSpeed() const {
    return _speed;
}

int Trainer::getProfile() const {
    return _profile;
}

const uint8_t* Trainer::getProbs() const {
    return _charProb;
}

bool Trainer::isPlainText() const {
    return _plainText;
}

void Trainer::setSpeed(int wpm) {
    _speed = constrain(wpm, MIN_SPEED, MAX_SPEED);
    MorseEngine::setSpeed(_speed);

    TrainerEvent evt;
    evt.type = TrainerEvent::SPEED_CHANGE;
    evt.speed = _speed;
    evt.direction = "set";
    emitEvent(evt);
}

void Trainer::setProfile(int p) {
    if (p < 0 || p > 9) return;
    _profile = p;
    loadProfile(p);
}

// --- Private ---

int Trainer::queueDist() const {
    int d = _queueIndexS - _queueIndexR;
    if (d < 0) d += QUEUE_LENGTH + 1;
    return d;
}

int Trainer::indexAdv(int index) const {
    index++;
    if (index == QUEUE_LENGTH) index = 0;
    return index;
}

char Trainer::generateLetter() {
    int totProb = 0;
    for (int i = 0; i < CHAR_COUNT; i++) {
        totProb += _charProb[i];
    }
    if (totProb == 0) return 'E'; // fallback

    int pointer = random(0, totProb);
    int sum = 0;
    int index = 0;
    while (sum <= pointer && index < CHAR_COUNT) {
        sum += _charProb[index];
        index++;
    }
    return (char)(FIRST_CHAR + index - 1);
}

void Trainer::correct(char letter) {
    int idx = letter - FIRST_CHAR;
    if (idx >= 0 && idx < CHAR_COUNT && _charProb[idx] != 0 && !_plainText) {
        if (_charProb[idx] <= 1 + DOWN_PROP) _charProb[idx] = 1;
        else _charProb[idx] -= DOWN_PROP;
    }

    TrainerEvent evt;
    evt.type = TrainerEvent::RESULT;
    evt.correct = true;
    evt.typedChar = letter;
    evt.expectedChar = letter;
    evt.prob = (idx >= 0 && idx < CHAR_COUNT) ? _charProb[idx] : 0;
    emitEvent(evt);
}

void Trainer::wrong(char typed, char expected) {
    int h1 = typed - FIRST_CHAR;
    int h2 = expected - FIRST_CHAR;

    if (h1 >= 0 && h1 < CHAR_COUNT && _charProb[h1] != 0 && !_plainText) {
        _charProb[h1] = min(100, _charProb[h1] + UP_PROP);
    }
    if (h2 >= 0 && h2 < CHAR_COUNT && _charProb[h2] != 0 && !_plainText) {
        _charProb[h2] = min(100, _charProb[h2] + UP_PROP);
    }

    _statErrors++;

    TrainerEvent evt;
    evt.type = TrainerEvent::RESULT;
    evt.correct = false;
    evt.typedChar = typed;
    evt.expectedChar = expected;
    evt.prob = (h1 >= 0 && h1 < CHAR_COUNT) ? _charProb[h1] : 0;
    emitEvent(evt);
}

void Trainer::analyzeSpeed() {
    if (_statErrors > 1) {
        _speed -= SPEED_DEC;
        if (_speed < MIN_SPEED) _speed = MIN_SPEED;
        MorseEngine::setSpeed(_speed);

        TrainerEvent evt;
        evt.type = TrainerEvent::SPEED_CHANGE;
        evt.speed = _speed;
        evt.direction = "down";
        emitEvent(evt);
    } else if (_statErrors == 0) {
        if (_speed < MAX_SPEED) _speed += SPEED_INC;
        MorseEngine::setSpeed(_speed);

        TrainerEvent evt;
        evt.type = TrainerEvent::SPEED_CHANGE;
        evt.speed = _speed;
        evt.direction = "up";
        emitEvent(evt);
    }
    _statErrors = 0;
}

void Trainer::contextLost() {
    _speed -= SPEED_DEC;
    if (_speed < MIN_SPEED) _speed = MIN_SPEED;
    MorseEngine::setSpeed(_speed);

    TrainerEvent evt;
    evt.type = TrainerEvent::CONTEXT_LOST;
    evt.speed = _speed;
    evt.direction = "down";
    emitEvent(evt);

    _statErrors = 0;
    _lGroup = 0;
    _queueIndexR = 0;
    _queueIndexS = 0;
    _statGroup = 0;

    // Queue recovery spaces (drained non-blockingly by update())
    _recoverySpaces = 5;
}

void Trainer::loadProfile(int profile) {
    if (profile == 0) {
        // Load saved probabilities
        if (!Storage::loadProbs(_charProb)) {
            // Fallback to P1 if no saved data
            const uint8_t* p = ::getProfile(1);
            for (int i = 0; i < CHAR_COUNT; i++) {
                _charProb[i] = pgm_read_byte(p + i);
            }
        }
    } else {
        const uint8_t* p = ::getProfile(profile);
        if (p) {
            for (int i = 0; i < CHAR_COUNT; i++) {
                _charProb[i] = pgm_read_byte(p + i);
            }
        }
        // P2 enables plainText mode (probabilities stay constant)
        _plainText = (profile == 2);
    }
}

void Trainer::emitEvent(const TrainerEvent& evt) {
    if (_eventCB) _eventCB(evt);
}
