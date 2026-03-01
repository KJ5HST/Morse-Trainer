#pragma once

#include <Arduino.h>

// ===================================================================
// Board Selection
// ===================================================================
// Uncomment ONE board, or pass -DBOARD_xxx as a PlatformIO build flag.
// Default: BOARD_NODEMCU
//
// #define BOARD_NODEMCU           // NodeMCU v2 with external peripherals
// #define BOARD_ESP8266_OLED      // ESP8266 0.96" OLED module v2.1.0

#if !defined(BOARD_NODEMCU) && !defined(BOARD_ESP8266_OLED)
  #define BOARD_NODEMCU
#endif

// ===================================================================
// Board: NodeMCU v2
// ===================================================================
#if defined(BOARD_NODEMCU)

#define BUZZER_PIN      4   // GPIO4  / D2 — passive buzzer (PWM)
#define MORSE_LED_PIN   5   // GPIO5  / D1 — morse signal LED
#define STATUS_LED_PIN  2   // GPIO2  / D4 — on-board status LED
#define OLED_SDA_PIN    12  // GPIO12 / D6
#define OLED_SCL_PIN    14  // GPIO14 / D5
#define KEY_DIT_PIN     13  // GPIO13 / D7 — straight key / dit paddle
#define KEY_DAH_PIN     0   // GPIO0  / D3 — dah paddle (do NOT hold during boot)

// Uncomment ONE display type (or leave all commented for no display):
#define OLED_SSD1306_128X64
// #define OLED_SSD1306_128X32
// #define OLED_SH1106_128X64

// ===================================================================
// Board: ESP8266 0.96" OLED module v2.1.0
// ===================================================================
#elif defined(BOARD_ESP8266_OLED)

#define BUZZER_PIN      4   // GPIO4  / D2 — passive buzzer (PWM)
#define MORSE_LED_PIN   5   // GPIO5  / D1 — morse signal LED
#define STATUS_LED_PIN  2   // GPIO2  / D4 — on-board status LED
#define OLED_SDA_PIN    14  // GPIO14 / D5 — built-in OLED SDA
#define OLED_SCL_PIN    12  // GPIO12 / D6 — built-in OLED SCL
#define KEY_DIT_PIN     13  // GPIO13 / D7 — straight key / dit paddle
#define KEY_DAH_PIN     0   // GPIO0  / D3 — dah paddle (do NOT hold during boot)

// If your module uses GPIO4 (SDA) / GPIO5 (SCL) instead, swap the
// OLED and buzzer/LED pin assignments above.

// Display type fixed by hardware
#define OLED_SSD1306_128X64

#else
  #error "Unknown board. Define BOARD_NODEMCU or BOARD_ESP8266_OLED."
#endif

// Key mode: straight key or iambic paddle
#define KEY_MODE_STRAIGHT  0
#define KEY_MODE_IAMBIC    1
#define KEY_MODE           KEY_MODE_STRAIGHT

// --- Buzzer ---
#define TONE_FREQ       800 // Hz
#define BUZZER_ACTIVE_DEFAULT true

// --- Morse Timing ---
#define DASH_TICKS      2   // dash length in ticks (counted from 0)
#define END_TICKS       1   // inter-character gap ticks
#define SPACE_TICKS     4   // word-space ticks

// --- Training ---
#define QUEUE_LENGTH    10
#define GROUP_LENGTH    5
#define FIRST_CHAR      33  // ASCII '!'
#define LAST_CHAR       90  // ASCII 'Z'
#define CHAR_COUNT      (LAST_CHAR - FIRST_CHAR + 1)  // 58 characters
#define STAT_LENGTH     10  // letters before speed analysis
#define UP_PROP         20  // probability increase on wrong answer
#define DOWN_PROP       5   // probability decrease on correct answer
#define SPEED_INC       2   // WPM increase on zero errors
#define SPEED_DEC       4   // WPM decrease on errors
#define MIN_SPEED       20
#define MAX_SPEED       200
#define DEFAULT_SPEED   25
#define DEFAULT_PROFILE 1
#define CONTEXT_LOST_DIST 5 // queue distance triggering context-lost

// --- WiFi ---
#define AP_SSID         "MorseTrainer"
#define AP_PASS         "morsecode"
#define MDNS_HOST       "morse"

// --- Storage ---
#define PROBS_FILE      "/probs.dat"
#define CONFIG_FILE     "/config.json"

// --- Number of profiles (P0 = saved, P1-P9 = preset) ---
#define NUM_PROFILES    10
