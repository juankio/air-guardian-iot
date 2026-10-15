#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "network/serial_cli_handler.hpp"

static constexpr const char* DEFAULT_SERVER = "https://servidor-taller.tailf19567.ts.net:10000";

class WiFiProvisioner {
public:
    using TelemetryContext = TelemetryCliContext;

private:
    Preferences prefs;
    const char* PREFS_NAMESPACE = "air-config";
    String ssid, password, server, pendingSsid, pendingEmotionCmd;
    static const size_t SERIAL_BUF_SIZE = 512;
    char serialBuffer[SERIAL_BUF_SIZE];
    size_t serialBufIndex = 0;
    bool isReconnecting = false;
    uint32_t reconnectStartMs = 0;
    const uint32_t RECONNECT_TIMEOUT_MS = 15000;
    bool newConfigSaved = false, pendingCalibrateMpu = false, hasPendingEmotion = false;

public:
    WiFiProvisioner() = default;

    void init(
        const char* defaultSsid = "TU_RED_WIFI",
        const char* defaultPass = "TU_PASSWORD",
        const char* defaultServer = DEFAULT_SERVER
    ) {
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            static uint32_t lastDiscLog = 0;
            if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                if (millis() - lastDiscLog > 30000) {
                    lastDiscLog = millis();
                    Serial.printf("[WIFI-DEBUG] Desconexion. Reason: %d\n", info.wifi_sta_disconnected.reason);
                }
            } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                Serial.printf("[WIFI-DEBUG] ¡CONECTADO CON IP!: %s\n", WiFi.localIP().toString().c_str());
            }
        });

        if (!prefs.begin(PREFS_NAMESPACE, false)) {
            ssid = defaultSsid; password = defaultPass; server = defaultServer;
            return;
        }
        ssid = prefs.getString("ssid", defaultSsid);
        password = prefs.getString("pass", defaultPass);
        server = prefs.getString("server", defaultServer);
        if (server.length() == 0 || server == "192.168.137.1" || server == "192.168.1.100") {
            server = defaultServer;
            prefs.putString("server", server);
        }
        prefs.end();
    }

    const String& getSSID() const { return ssid; }
    const String& getPassword() const { return password; }
    const String& getServer() const { return server; }

    String getMqttHost() const {
        String s = server; s.trim();
        if (s.startsWith("https://")) s = s.substring(8);
        else if (s.startsWith("http://")) s = s.substring(7);
        int slashIdx = s.indexOf('/');
        if (slashIdx >= 0) s = s.substring(0, slashIdx);
        int colonIdx = s.indexOf(':');
        if (colonIdx >= 0) s = s.substring(0, colonIdx);
        return s;
    }

    String buildBaseUrl(uint16_t lanPort = 3002) const {
        String s = server; s.trim();
        if (s.length() == 0) s = DEFAULT_SERVER;
        while (s.endsWith("/")) s.remove(s.length() - 1);
        if (s.startsWith("http://") || s.startsWith("https://")) return s;
        if (s.indexOf(".ts.net") >= 0) {
            return (s.indexOf(':') >= 0) ? (String("https://") + s) : (String("https://") + s + ":10000");
        }
        return (s.indexOf(':') >= 0) ? (String("http://") + s) : (String("http://") + s + ":" + String(lanPort));
    }

    String getIngestUrl() const { return buildBaseUrl(3002) + "/api/telemetry/ingest"; }
    String getBackendUrl(uint16_t port = 3002) const { return buildBaseUrl(port) + "/api/telemetry/batch"; }

    void startConnection() {
        WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(20);
        WiFi.begin(ssid.c_str(), password.c_str());
        isReconnecting = true; reconnectStartMs = millis(); pendingSsid = ssid;
    }

    bool saveCredentials(const String& newSsid, const String& newPass, const String& newServer) {
        if (!prefs.begin(PREFS_NAMESPACE, false)) return false;
        prefs.putString("ssid", newSsid);
        prefs.putString("pass", newPass);
        prefs.putString("server", newServer);
        prefs.end();
        ssid = newSsid; password = newPass; server = newServer;
        newConfigSaved = true;
        return true;
    }

    bool consumeNewConfigFlag() { bool val = newConfigSaved; newConfigSaved = false; return val; }
    bool consumeCalibrateMpuFlag() { bool val = pendingCalibrateMpu; pendingCalibrateMpu = false; return val; }
    bool consumeEmotionCommand(String& outEmotion) {
        if (!hasPendingEmotion) return false;
        outEmotion = pendingEmotionCmd; hasPendingEmotion = false;
        return true;
    }

    void processSerial(const TelemetryContext& ctx) {
        int linesProcessed = 0;
        while (Serial.available() > 0 && linesProcessed < 4) {
            char c = (char)Serial.read();
            if (c == '\n' || c == '\r') {
                if (serialBufIndex > 0) {
                    serialBuffer[serialBufIndex] = '\0';
                    String line(serialBuffer); serialBufIndex = 0; line.trim();
                    if (line.length() == 0 || !line.startsWith("{")) continue;
                    linesProcessed++;
                    handleJsonCommandLine(line.c_str(), ctx);
                }
            } else if (serialBufIndex < SERIAL_BUF_SIZE - 1) {
                serialBuffer[serialBufIndex++] = c;
            } else {
                serialBufIndex = 0;
            }
        }
    }

    void updateConnectionStatus() {
        if (!isReconnecting) return;
        if (WiFi.status() == WL_CONNECTED) {
            isReconnecting = false;
            JsonDocument res;
            res["status"] = "CONNECTED"; res["ssid"] = pendingSsid;
            res["ip"] = WiFi.localIP().toString(); res["rssi"] = WiFi.RSSI();
            serializeJson(res, Serial); Serial.println();
        } else if (millis() - reconnectStartMs >= RECONNECT_TIMEOUT_MS) {
            isReconnecting = false;
            SerialCliHandler::sendQuickJsonAck(nullptr, "FAILED", "error", "TIMEOUT_CONNECTING");
        }
    }

private:
    void handleJsonCommandLine(const char* line, const TelemetryContext& ctx) {
        JsonDocument doc;
        if (deserializeJson(doc, line)) return;
        const char* cmd = doc["cmd"];
        if (!cmd || doc["status"].is<const char*>() || doc["network"].is<JsonObject>()) return;

        if (strcmp(cmd, "SET_WIFI") == 0) {
            const char* newSsid = doc["ssid"] | "";
            const char* newPass = doc["pass"] | "";
            const char* newServer = doc["server"] | server.c_str();
            if (strlen(newSsid) == 0) {
                SerialCliHandler::sendQuickJsonAck(nullptr, "ERROR", "error", "MISSING_SSID");
                return;
            }
            saveCredentials(newSsid, newPass, newServer);
            SerialCliHandler::sendQuickJsonAck(nullptr, "CONNECTING", "ssid", newSsid);
            WiFi.disconnect(); delay(20); WiFi.begin(newSsid, newPass);
            isReconnecting = true; reconnectStartMs = millis(); pendingSsid = newSsid;
        } else if (strcmp(cmd, "GET_STATUS") == 0) {
            SerialCliHandler::sendStatusReport(ctx, ssid, server, getIngestUrl());
        } else if (strcmp(cmd, "SCAN_I2C") == 0) {
            SerialCliHandler::handleScanI2CCommand();
        } else if (strcmp(cmd, "CALIBRATE_MPU") == 0) {
            pendingCalibrateMpu = true;
            SerialCliHandler::sendQuickJsonAck("CALIBRATE_MPU", "OK");
        } else if (strcmp(cmd, "SET_EMOTION") == 0) {
            const char* emo = doc["emotion"] | "OPTIMAL";
            pendingEmotionCmd = String(emo); hasPendingEmotion = true;
            SerialCliHandler::sendQuickJsonAck("SET_EMOTION", "OK", "emotion", emo);
        }
    }
};
