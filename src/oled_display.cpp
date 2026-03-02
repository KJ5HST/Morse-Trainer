#include "oled_display.h"
#include "config.h"
#include <U8g2lib.h>
#include <Wire.h>

// --- Display constructor based on config.h selection ---
#if defined(OLED_SSD1306_128X64)
static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);
static const bool compact = false;
#elif defined(OLED_SSD1306_128X32)
static U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);
static const bool compact = true;
#elif defined(OLED_SH1106_128X64)
static U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);
static const bool compact = false;
#else
// No display defined — provide stub
static const bool compact = false;
#endif

// --- Display state ---
static bool displayFound = false;
static bool dirty = true;
static unsigned long lastRedraw = 0;
static const unsigned long REDRAW_INTERVAL = 80; // ms

// Cached display data
static int dSpeed = 0;
static int dProfile = 0;
static bool dRunning = false;
static bool dToneOn = false;
static char dChar = ' ';
static char dPattern[8] = "";
static bool dResultValid = false;
static bool dCorrect = false;
static int dQueueDist = 0;
static uint8_t dProb = 0;
static char dTypedChar = ' ';
static char dExpectedChar = ' ';

// --- Rendering ---

static void drawScreen64() {
    // Line 0: speed, profile, status (y=10, font 6x10)
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[24];
    snprintf(buf, sizeof(buf), "%d WPM", dSpeed);
    u8g2.drawStr(0, 10, buf);

    snprintf(buf, sizeof(buf), "P%d", dProfile);
    u8g2.drawStr(56, 10, buf);

    // Status indicator
    if (dToneOn) {
        u8g2.drawDisc(120, 5, 4); // filled circle = tone on
    } else if (dRunning) {
        u8g2.drawCircle(120, 5, 4); // hollow circle = ready
    }

    if (dRunning) {
        u8g2.drawStr(90, 10, "RUN");
    } else {
        u8g2.drawStr(86, 10, "IDLE");
    }

    // Line 1-2: current character (large font, centered)
    if (dChar != ' ') {
        u8g2.setFont(u8g2_font_logisoso22_tf);
        char chBuf[2] = { dChar, '\0' };
        int w = u8g2.getStrWidth(chBuf);
        u8g2.drawStr((128 - w) / 2, 40, chBuf);
    }

    // Line 3: morse pattern (centered)
    u8g2.setFont(u8g2_font_7x13_tf);
    if (dPattern[0]) {
        int w = u8g2.getStrWidth(dPattern);
        u8g2.drawStr((128 - w) / 2, 52, dPattern);
    }

    // Line 5: result, queue distance, probability
    u8g2.setFont(u8g2_font_6x10_tf);
    if (dResultValid) {
        if (dCorrect) {
            u8g2.drawStr(0, 63, "OK");
        } else {
            snprintf(buf, sizeof(buf), "%c!=%c", dTypedChar, dExpectedChar);
            u8g2.drawStr(0, 63, buf);
        }
    }

    snprintf(buf, sizeof(buf), "d:%d", dQueueDist);
    u8g2.drawStr(60, 63, buf);

    snprintf(buf, sizeof(buf), "p:%d", dProb);
    u8g2.drawStr(100, 63, buf);
}

static void drawScreen32() {
    // Compact: two lines only
    // Line 1: speed, profile, char, pattern
    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[24];
    snprintf(buf, sizeof(buf), "%dW P%d", dSpeed, dProfile);
    u8g2.drawStr(0, 10, buf);

    if (dChar != ' ') {
        u8g2.setFont(u8g2_font_7x13B_tf);
        char chBuf[2] = { dChar, '\0' };
        u8g2.drawStr(68, 12, chBuf);
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    if (dPattern[0]) {
        u8g2.drawStr(82, 10, dPattern);
    }

    // Line 2: result, distance, probability
    if (dResultValid) {
        if (dCorrect) {
            u8g2.drawStr(0, 24, "OK");
        } else {
            snprintf(buf, sizeof(buf), "%c!=%c", dTypedChar, dExpectedChar);
            u8g2.drawStr(0, 24, buf);
        }
    }

    snprintf(buf, sizeof(buf), "d:%d p:%d", dQueueDist, dProb);
    u8g2.drawStr(60, 24, buf);

    // Tone indicator
    if (dToneOn) {
        u8g2.drawDisc(122, 19, 3);
    }
}

// --- Public API ---

bool OledDisplay::begin() {
#if defined(OLED_SSD1306_128X64) || defined(OLED_SSD1306_128X32) || defined(OLED_SH1106_128X64)
    u8g2.begin();

    // Quick I2C probe — if begin() succeeded but nothing is wired,
    // the display will just show nothing. U8g2 doesn't provide a
    // connection check, so we assume connected if a display type
    // is compiled in.
    displayFound = true;

    // Splash screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tf);
    u8g2.drawStr(12, compact ? 20 : 30, "Morse Trainer");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(30, compact ? 30 : 46, "Starting...");
    u8g2.sendBuffer();

    dirty = true;
    return true;
#else
    displayFound = false;
    return false;
#endif
}

void OledDisplay::update() {
    if (!displayFound || !dirty) return;

    unsigned long now = millis();
    if (now - lastRedraw < REDRAW_INTERVAL) return;
    lastRedraw = now;
    dirty = false;

#if defined(OLED_SSD1306_128X64) || defined(OLED_SSD1306_128X32) || defined(OLED_SH1106_128X64)
    u8g2.clearBuffer();
    if (compact) {
        drawScreen32();
    } else {
        drawScreen64();
    }
    u8g2.sendBuffer();
#endif
}

void OledDisplay::onTrainerEvent(const TrainerEvent& evt) {
    if (!displayFound) return;

    switch (evt.type) {
        case TrainerEvent::CHAR_SENT:
            dChar = evt.sentChar;
            strncpy(dPattern, evt.pattern, sizeof(dPattern) - 1);
            dPattern[sizeof(dPattern) - 1] = '\0';
            dQueueDist = evt.queueDist;
            dResultValid = false;
            break;

        case TrainerEvent::RESULT:
            dResultValid = true;
            dCorrect = evt.correct;
            dTypedChar = evt.typedChar;
            dExpectedChar = evt.expectedChar;
            dProb = evt.prob;
            break;

        case TrainerEvent::SPEED_CHANGE:
            dSpeed = evt.speed;
            break;

        case TrainerEvent::SESSION_STATE:
            dRunning = evt.running;
            dSpeed = evt.speed;
            if (evt.running) {
                dResultValid = false;
                dChar = ' ';
                dPattern[0] = '\0';
            }
            break;

        case TrainerEvent::CONTEXT_LOST:
            dSpeed = evt.speed;
            dChar = ' ';
            dPattern[0] = '\0';
            dResultValid = false;
            break;
    }
    dirty = true;
}

void OledDisplay::onMorseElement(bool on) {
    if (!displayFound) return;
    dToneOn = on;
    dirty = true;
}
