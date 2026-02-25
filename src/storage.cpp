#include "storage.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool Storage::begin() {
    return LittleFS.begin();
}

bool Storage::saveProbs(const uint8_t probs[CHAR_COUNT]) {
    File f = LittleFS.open(PROBS_FILE, "w");
    if (!f) return false;
    f.write(probs, CHAR_COUNT);
    f.close();
    return true;
}

bool Storage::loadProbs(uint8_t probs[CHAR_COUNT]) {
    File f = LittleFS.open(PROBS_FILE, "r");
    if (!f) return false;
    if (f.size() != CHAR_COUNT) {
        f.close();
        return false;
    }
    f.read(probs, CHAR_COUNT);
    f.close();
    return true;
}

bool Storage::saveConfig(const Config& cfg) {
    JsonDocument doc;
    doc["speed"] = cfg.speed;
    doc["profile"] = cfg.profile;
    doc["wifiMode"] = cfg.wifiMode;
    doc["staSSID"] = cfg.staSSID;
    doc["staPass"] = cfg.staPass;

    File f = LittleFS.open(CONFIG_FILE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

bool Storage::loadConfig(Config& cfg) {
    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    cfg.speed = doc["speed"] | DEFAULT_SPEED;
    cfg.profile = doc["profile"] | DEFAULT_PROFILE;
    cfg.wifiMode = doc["wifiMode"] | "ap";
    cfg.staSSID = doc["staSSID"] | "";
    cfg.staPass = doc["staPass"] | "";
    return true;
}
