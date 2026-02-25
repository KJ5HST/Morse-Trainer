# Original MTR_V2 Morse Trainer — Comprehensive Analysis

**Source**: [SensorsIot/Morse-Trainer](https://github.com/SensorsIot/Morse-Trainer) — `MTR_V2/MTR_V2.ino`
**Author**: Andreas Spiess (Copyright 2014), `sendLetter()` based on work by raron (2010, 2012)
**License**: GNU GPLv3

---

## 1. Architecture Overview

MTR_V2 is a single-file Arduino sketch (~400 lines) implementing an adaptive Morse code training system. The trainer sends random characters as audible Morse code, accepts keyboard input from the trainee, evaluates correctness, and dynamically adjusts both character selection probabilities and transmission speed.

### Data Flow

```
  [Initialize]
       │
       v
  ┌─────────────────────────────────────────┐
  │                MAIN LOOP                 │
  │                                          │
  │  ┌─── SENDING ────┐  ┌── RECEIVING ──┐  │
  │  │ generateLetter()│  │ keyboard.read()│  │
  │  │ sendLetter()    │  │ correct()/     │  │
  │  │ queue[S] = ch   │  │ wrong()        │  │
  │  │ advance S       │  │ advance R      │  │
  │  └────────┬────────┘  └───────┬────────┘  │
  │           │                   │            │
  │           v                   v            │
  │     Timer1 ISR          analyzeSpeed()     │
  │   transmitMorse()       (every 10 chars)   │
  └─────────────────────────────────────────┘
```

### Hardware

| Component | Pin | Purpose |
|---|---|---|
| Morse output | D12 | Digital HIGH/LOW for morse signal |
| Tone output | D3 | PWM 1000 Hz tone via `tone()`/`noTone()` |
| Red LED | D9 | Wrong answer indicator |
| Green LED | D10 | Correct answer indicator |
| PS2 Keyboard | D7 (data), D2 (IRQ) | Trainee input |
| I2C LCD 20x4 | SDA/SCL | Display (address 0x27) |

### Libraries

- `TimerOne.h` — Hardware Timer1 for morse timing ISR
- `Wire.h` + `LiquidCrystal_I2C.h` — I2C LCD
- `EEPROM.h` — Persistence
- `PS2Keyboard.h` — PS/2 keyboard input

---

## 2. Morse Binary Tree Encoding

### The Table

```c
const char morseTable[] PROGMEM =
    "*5*H*4*S***V*3*I***F***U?*_**2*E*&*L\"**R*+.****A***P@**W***J'1* *6-B*=*D*/"
    "*X***N***C;(!K***Y***T*7*Z**,G***Q***M:8*!***O*9***0*";
```

This is a **dichotomic search table** (binary tree flattened into a 127-character array) where:
- The tree root is at index 63 (space character: ` `)
- Left child (dot): `parent - 2^level`
- Right child (dash): `parent + 2^level`
- `*` marks unused positions

### Tree constants

```c
const int morseTreetop = 63;      // root index
const int morseTableLength = 127; // (63 * 2) + 1
const int morseTreeLevels = 6;    // log2(64) = 6
```

### `sendLetter()` — Character to Morse Conversion

The algorithm works by **reverse tree traversal**:

1. **Find character** in the table by linear scan
2. **Determine tree level** by testing divisibility: `(pos + 2^i) % 2^(i+1) == 0`
3. **Number of signals** = `morseTreeLevels - startLevel`
4. **Build signal string backwards**: at each level, test if navigating left (dot) or right (dash) leads toward the root, building the pattern from last signal to first

The resulting string is stored in `morseSignalString[]` (e.g., `".-"` for 'A', `"-..."` for 'B').

---

## 3. Timer-Driven FSM (15 States)

`transmitMorse()` is attached to Timer1 as an ISR. It implements a 15-state finite state machine that controls the morse output pin and tone pin. The `ex` flag ensures only one state transition per timer tick.

### State Diagram

```
   ┌──1──┐
   │Check│──LOW──>──2── Decision
   │ pin │──HIGH─>──3── Check dot/dash
   └─────┘

State 2 (Decision when LOW):
   ├── morseSignalPos==0 ──> 4 (First element ON)
   ├── signal=='\0'      ──> 5 (End of character)
   ├── signal==' '       ──> 6 (Space/word gap)
   └── else              ──> 7 (Subsequent element ON)

State 3 (Check when HIGH):
   ├── signal=='.' ──> 9  (Dot done, OFF)
   └── signal=='-' ──> 8  (Check dash timing)

Timing states:
   4:  Turn ON, set stat=3               (first element)
   5:  tick < endTick? → 10 : 11         (inter-char gap)
   6:  tick < spaceTick? → 12 : 13       (word gap)
   7:  Turn ON, set stat=3               (subsequent element)
   8:  tick < dashTick? → 15 : 14        (dash duration)
   9:  Turn OFF, advance pos, stat=2     (dot complete)
   10: tick++, stat=5                    (waiting inter-char)
   11: sendingMorse=false                (character done)
   12: tick++, stat=6                    (waiting word gap)
   13: sendingMorse=false                (word gap done)
   14: Turn OFF, advance pos, stat=1     (dash complete)
   15: tick++, stat=8                    (dash timing)
```

### Timing Model

The timer period is `6,000,000 / speed` microseconds (where speed is in characters/minute, not standard WPM). At speed=25: period = 240ms per tick.

- **Dot** = 1 tick ON
- **Dash** = 3 ticks ON (dashTick=2, counted from 0)
- **Inter-element gap** = 1 tick OFF (implicit between ON states)
- **Inter-character gap** = 2 ticks OFF (endTick=1, counted from 0)
- **Word space** = 5 ticks OFF (spaceTick=4, counted from 0)

### Key Variables

| Variable | Type | Purpose |
|---|---|---|
| `sendingMorse` | volatile bool | True while a character is being transmitted |
| `stat` | volatile int | Current FSM state (1-15) |
| `tick` | volatile int | Counter for timing gaps and dashes |
| `ex` | volatile bool | Exit flag — ensures one state transition per ISR call |
| `morseSignalString[]` | volatile char[7] | Current character's dot/dash pattern |
| `morseSignalPos` | volatile int | Current position in the signal string |

---

## 4. Adaptive Training Algorithm

### Letter Generation: `generateLetter()`

Uses **weighted random selection** based on character probabilities:

```
1. Sum all probabilities: totProbabilities = Σ charProp[i]
2. Pick random number: pointer = random(0, totProbabilities)
3. Walk array accumulating sum until sum > pointer
4. Return character at that index
```

Characters with higher probability values are selected more frequently. This creates adaptive difficulty — characters the trainee struggles with appear more often.

### Scoring: `correct()` and `wrong()`

**On correct answer:**
- Decrease character probability by `downProp` (5)
- Minimum probability is 1 (never reaches 0 — character stays in rotation)
- Green LED ON, Red LED OFF

**On wrong answer:**
- Increase probability of **both** the typed character and the expected character by `upProp` (20)
- Maximum probability is 100
- Red LED ON, Green LED OFF
- Increment `statErrors`

**Skip detection**: If the next character in the queue matches the typed character, the system assumes the trainee missed one character. It marks the current queue position as wrong and advances.

### Speed Adaptation: `analyzeSpeed()`

Called every `statLength` (10) characters:

| Condition | Action |
|---|---|
| `statErrors == 0` | Speed += `speedInc` (2), max 200 |
| `statErrors > 1` | Speed -= `speedDec` (4), min 20 |
| `statErrors == 1` | No change |

### Context Lost: `contextLost()`

When `queueDist() >= 5`, the trainee has fallen too far behind:
- Speed -= `speedDec`
- Reset queue indices and counters
- Send 10 spaces (silence) to give time to recover
- Flush keyboard buffer

---

## 5. Probability Profiles (P1-P9)

Each profile is a 58-byte PROGMEM array mapping ASCII 33-90 to initial probabilities.

| Profile | Description | Active Characters |
|---|---|---|
| P0 | Saved from EEPROM | Last session's adapted probabilities |
| P1 | All letters even | A-Z, all at 50 |
| P2 | German frequency | A-Z weighted by German text frequency (plainText mode — probs don't change) |
| P3 | Numbers only | 0-9, all at 50 |
| P4 | Punctuation marks | Select punctuation at 50 |
| P5 | Letters + punctuation | A-Z + punctuation, all at 50 |
| P6 | Beginner 1 | A, E, M, N, T (5 chars) |
| P7 | Beginner 2 | A, E, G, I, K, M, N, O, R, T (10 chars) |
| P8 | Beginner 3 | A, D, E, G, I, K, M, N, O, P, R, T, U, W, Z (15 chars) |
| P9 | Beginner 4 | A-K, M, N, O, P, R, T, U, W, Z (20 chars) |

### Array Index Mapping

```
Index  0-14:  ! " # $ % & ' ( ) * + , - . /   (ASCII 33-47)
Index 15-24:  0 1 2 3 4 5 6 7 8 9              (ASCII 48-57)
Index 25-31:  : ; < = > ? @                     (ASCII 58-64)
Index 32-57:  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z  (ASCII 65-90)
```

---

## 6. Circular Queue

The queue bridges the asynchronous sender and receiver:

```c
char queue[queueLength + 1];  // 11 slots
int queueIndexS = 0;           // sender writes here
int queueIndexR = 0;           // receiver reads here
```

### `queueDist()`

```c
int queueDist() {
    int hi = queueIndexS - queueIndexR;
    if (hi < 0) hi += queueLength + 1;
    return hi;
}
```

Returns how many characters the sender is ahead of the receiver.

### `indexAdv()`

```c
int indexAdv(int index) {
    index++;
    if (index == queueLength) index = 0;
    return index;
}
```

Wraps the index at `queueLength` (10), so valid indices are 0-9.

### Group Spacing

Characters are sent in groups of `groupLength` (5), separated by spaces:
```
ABCDE FGHIJ KLMNO ...
```

---

## 7. EEPROM Persistence

### Layout

58 bytes at addresses 0-57, one byte per character probability:

```
Address 0:  charProp['!' - 33]  (probability for '!')
Address 1:  charProp['"' - 33]  (probability for '"')
...
Address 32: charProp['A' - 33]  (probability for 'A')
...
Address 57: charProp['Z' - 33]  (probability for 'Z')
```

### Write (on training end — `*` key)

```c
for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
    EEPROM.write(ii, charProp[ii]);
```

### Read (on profile 0 selection)

```c
for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
    charProp[ii] = EEPROM.read(ii);
```

---

## 8. Initialization Menu

The `initialize()` function runs at startup and after training ends (via `*` key):

1. **Flush keyboard buffer**
2. **Prompt for speed** (20-200, entered as up to 3 digits + Enter)
3. **Prompt for menu option** (0-9, single keypress)
4. **Load profile** based on menu selection
5. **Print active characters** to Serial for debugging

Speed is set via `setSpeed()`:
```c
void setSpeed(float value) {
    double M_tim = TIMER_MULT * 6 / value;  // microseconds
    Timer1.initialize(M_tim);
}
```

---

## 9. Constants Reference

| Constant | Value | Purpose |
|---|---|---|
| `morseOutPin` | 12 | Morse signal digital output |
| `tonePin` | 3 | PWM tone output |
| `TIMER_MULT` | 1000000 | Timer base (1 second in microseconds) |
| `dashTick` | 2 | Dash duration (3 ticks total, 0-indexed) |
| `endTick` | 1 | Inter-character gap (2 ticks) |
| `spaceTick` | 4 | Word space (5 ticks) |
| `queueLength` | 10 | Circular queue size |
| `groupLength` | 5 | Characters per group |
| `firstLetter` | 33 | ASCII start ('!') |
| `lastLetter` | 90 | ASCII end ('Z') |
| `statLength` | 10 | Chars between speed analysis |
| `upProp` | 20 | Probability increase on wrong |
| `downProp` | 5 | Probability decrease on correct |
| `speedInc` | 2 | WPM increase on zero errors |
| `speedDec` | 4 | WPM decrease on errors |

---

## 10. Global State Variables

| Variable | Type | Purpose |
|---|---|---|
| `charProp[58]` | byte[] | Per-character probability array |
| `speed` | int | Current transmission speed |
| `queue[11]` | char[] | Circular queue of sent characters |
| `queueIndexS` | int | Queue write pointer (sender) |
| `queueIndexR` | int | Queue read pointer (receiver) |
| `hKey` | char | Last keyboard input |
| `lGroup` | int | Characters sent in current group |
| `statErrors` | int | Error count in current analysis window |
| `statGroup` | int | Characters sent in current analysis window |
| `plainText` | boolean | If true, probabilities are not modified (P2 mode) |
| `sendingMorse` | volatile bool | True during morse transmission |
| `morseSignalString[7]` | volatile char[] | Current morse pattern |
| `morseSignalPos` | volatile int | Position in pattern |
| `morseSignals` | int | Number of dots/dashes in current character |
| `stat` | volatile int | FSM state |
| `tick` | volatile int | Timing counter |
| `ex` | volatile bool | ISR single-step exit flag |
