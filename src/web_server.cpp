#include "web_server.h"
#include "config.h"
#include "storage.h"
#include "morse_engine.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static void handleWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    // Parse JSON message
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) return;

    const char* type = doc["type"];
    if (!type) return;

    if (strcmp(type, "key") == 0) {
        const char* ch = doc["char"];
        if (ch && ch[0]) {
            char c = ch[0];
            if (c > 96) c -= 32; // uppercase
            trainer.processInput(c);
        }
    }
    else if (strcmp(type, "command") == 0) {
        const char* cmd = doc["cmd"];
        if (!cmd) return;

        if (strcmp(cmd, "start") == 0) {
            int profile = doc["profile"] | DEFAULT_PROFILE;
            int speed = doc["speed"] | DEFAULT_SPEED;
            trainer.start(profile, speed);
        }
        else if (strcmp(cmd, "stop") == 0) {
            trainer.stop();
        }
        else if (strcmp(cmd, "speed") == 0) {
            int speed = doc["speed"] | trainer.getSpeed();
            trainer.setSpeed(speed);
        }
        else if (strcmp(cmd, "status") == 0) {
            // Send status response
            JsonDocument resp;
            resp["type"] = "status";
            resp["running"] = trainer.isRunning();
            resp["speed"] = trainer.getSpeed();
            resp["profile"] = trainer.getProfile();

            String out;
            serializeJson(resp, out);
            client->text(out);
        }
        else if (strcmp(cmd, "probs") == 0) {
            // Send probabilities
            JsonDocument resp;
            resp["type"] = "probs";
            JsonArray arr = resp["data"].to<JsonArray>();
            const uint8_t* probs = trainer.getProbs();
            for (int i = 0; i < CHAR_COUNT; i++) {
                if (probs[i] > 0) {
                    JsonObject entry = arr.add<JsonObject>();
                    char ch = (char)(FIRST_CHAR + i);
                    entry["char"] = String(ch);
                    entry["prob"] = probs[i];
                }
            }
            String out;
            serializeJson(resp, out);
            client->text(out);
        }
    }
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            Serial.printf("[WS] Client #%u connected from %s (heap: %u, clients: %u)\n",
                          client->id(), client->remoteIP().toString().c_str(),
                          ESP.getFreeHeap(), ws.count());
            // Close any older connections from the same IP (stale after refresh)
            IPAddress newIP = client->remoteIP();
            for (auto& c : ws.getClients()) {
                if (c->id() != client->id() && c->remoteIP() == newIP) {
                    c->close();
                }
            }
            break;
        }
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected (heap: %u)\n",
                          client->id(), ESP.getFreeHeap());
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                handleWebSocketMessage(client, data, len);
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

static void broadcastJson(JsonDocument& doc) {
    String out;
    serializeJson(doc, out);
    ws.textAll(out);
}

void WebServer::begin() {
    // Load WiFi config
    Storage::Config cfg;
    Storage::loadConfig(cfg);

    if (cfg.wifiMode == "sta" && cfg.staSSID.length() > 0) {
        // Station mode
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.staSSID.c_str(), cfg.staPass.c_str());
        Serial.print(F("Connecting to WiFi: ")); Serial.println(cfg.staSSID);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.print(F("Connected! IP: ")); Serial.println(WiFi.localIP());
        } else {
            Serial.println(F("\nSTA failed, falling back to AP mode"));
            WiFi.mode(WIFI_AP);
            WiFi.softAP(AP_SSID, AP_PASS);
            Serial.print(F("AP IP: ")); Serial.println(WiFi.softAPIP());
        }
    } else {
        // AP mode (default)
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        Serial.print(F("AP Mode - SSID: ")); Serial.println(AP_SSID);
        Serial.print(F("AP IP: ")); Serial.println(WiFi.softAPIP());
    }

    // mDNS
    if (MDNS.begin(MDNS_HOST)) {
        Serial.print(F("mDNS: http://")); Serial.print(MDNS_HOST); Serial.println(F(".local"));
    }

    // WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.begin();
    Serial.println(F("HTTP server started"));
}

void WebServer::update() {
    ws.cleanupClients();
    MDNS.update();
}

void WebServer::broadcastMorseElement(bool on) {
    (void)on; // No longer sent to web clients
}

void WebServer::onTrainerEvent(const TrainerEvent& evt) {
    JsonDocument doc;

    switch (evt.type) {
        case TrainerEvent::CHAR_SENT:
            doc["type"] = "char_sent";
            doc["char"] = String(evt.sentChar);
            doc["pattern"] = evt.pattern;
            doc["queue_dist"] = evt.queueDist;
            break;

        case TrainerEvent::RESULT:
            doc["type"] = "result";
            doc["correct"] = evt.correct;
            doc["typed"] = String(evt.typedChar);
            doc["expected"] = String(evt.expectedChar);
            doc["prob"] = evt.prob;
            break;

        case TrainerEvent::SPEED_CHANGE:
            doc["type"] = "speed_change";
            doc["speed"] = evt.speed;
            doc["direction"] = evt.direction;
            break;

        case TrainerEvent::SESSION_STATE:
            doc["type"] = "session";
            doc["state"] = evt.running ? "started" : "stopped";
            doc["speed"] = evt.speed;
            break;

        case TrainerEvent::CONTEXT_LOST:
            doc["type"] = "context_lost";
            doc["speed"] = evt.speed;
            break;
    }

    broadcastJson(doc);
}
