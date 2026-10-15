#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

struct OfflineTelemetryRecord {
    const char* deviceId;
    uint32_t timestamp;
    float ppm;
    float co_ppm;
    float temperature;
    float humidity;
    float delta_ppm;
    int raw_adc;
};

class OfflineBufferManager {
private:
    const char* filePath = "/offline_queue.jsonl";
    const size_t MAX_FILE_SIZE = 300 * 1024;
    size_t pendingCount = 0;
    bool isMounted = false;

    void updatePendingCount() {
        if (!isMounted) return;
        File file = LittleFS.open(filePath, "r");
        if (!file) {
            pendingCount = 0;
            return;
        }
        size_t count = 0;
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 5) count++;
        }
        file.close();
        pendingCount = count;
    }

    bool sendBatchPayload(const String& payload, const char* backendUrl, PubSubClient* mqttClient) {
        if (backendUrl && strlen(backendUrl) > 0) {
            String urlStr(backendUrl);
            WiFiClientSecure secureClient;
            WiFiClient plainClient;
            HTTPClient http;
            if (urlStr.startsWith("https://")) {
                secureClient.setInsecure();
                http.begin(secureClient, urlStr);
            } else {
                http.begin(plainClient, urlStr);
            }
            http.addHeader("Content-Type", "application/json");
            http.setTimeout(3000);
            int httpCode = http.POST(payload);
            http.end();
            if (httpCode == 200 || httpCode == 201 || httpCode == 400) {
                return true;
            }
        }
        if (mqttClient && mqttClient->connected()) {
            if (mqttClient->publish("airguardian/telemetry/batch", payload.c_str())) {
                return true;
            }
        }
        return false;
    }

public:
    OfflineBufferManager() = default;

    bool init() {
        if (!LittleFS.begin(true)) {
            Serial.println("[OFFLINE-FS] ERROR: No se pudo montar LittleFS.");
            isMounted = false;
            return false;
        }
        isMounted = true;
        updatePendingCount();
        if (pendingCount > 50) {
            LittleFS.remove(filePath);
            pendingCount = 0;
        }
        Serial.printf("[OFFLINE-FS] LittleFS montado. Pendientes: %u\n", (unsigned int)pendingCount);
        return true;
    }

    bool storeRecord(const OfflineTelemetryRecord& rec) {
        if (!isMounted) return false;

        if (LittleFS.exists(filePath)) {
            File checkFile = LittleFS.open(filePath, "r");
            if (checkFile && checkFile.size() > MAX_FILE_SIZE) {
                checkFile.close();
                LittleFS.remove(filePath);
                pendingCount = 0;
            } else if (checkFile) {
                checkFile.close();
            }
        }

        File file = LittleFS.open(filePath, "a");
        if (!file) return false;

        JsonDocument doc;
        doc["device_id"] = rec.deviceId;
        doc["timestamp"] = rec.timestamp;
        doc["ppm"] = round(rec.ppm * 10) / 10.0;
        doc["co_ppm"] = round(rec.co_ppm * 10) / 10.0;
        doc["temperature"] = round(rec.temperature * 10) / 10.0;
        doc["humidity"] = round(rec.humidity * 10) / 10.0;
        doc["delta_ppm"] = round(rec.delta_ppm * 10) / 10.0;
        doc["raw_adc"] = rec.raw_adc;

        serializeJson(doc, file);
        file.print('\n');
        file.close();

        pendingCount++;
        return true;
    }

    size_t getPendingCount() const { return pendingCount; }
    bool hasPendingData() const { return pendingCount > 0; }

    bool flushToBackend(const char* backendUrl, PubSubClient* mqttClient = nullptr) {
        if (!isMounted || !hasPendingData()) return true;

        File file = LittleFS.open(filePath, "r");
        if (!file) {
            pendingCount = 0;
            return false;
        }

        bool allFlushedSuccessfully = true;
        const size_t BATCH_SIZE = 10;

        JsonDocument batchDoc;
        JsonArray samplesArray = batchDoc["samples"].to<JsonArray>();
        size_t currentBatchCount = 0;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() < 5) continue;

            JsonDocument itemDoc;
            if (!deserializeJson(itemDoc, line)) {
                if (itemDoc["timestamp"].as<uint32_t>() == 0) itemDoc["timestamp"] = 1;
                samplesArray.add(itemDoc);
                currentBatchCount++;
            }

            if (currentBatchCount >= BATCH_SIZE || (!file.available() && currentBatchCount > 0)) {
                String payload;
                serializeJson(batchDoc, payload);

                if (!sendBatchPayload(payload, backendUrl, mqttClient)) {
                    allFlushedSuccessfully = false;
                    break;
                }

                batchDoc.clear();
                samplesArray = batchDoc["samples"].to<JsonArray>();
                currentBatchCount = 0;
                vTaskDelay(pdMS_TO_TICKS(150));
            }
        }

        file.close();

        if (allFlushedSuccessfully) {
            LittleFS.remove(filePath);
            pendingCount = 0;
            return true;
        }
        updatePendingCount();
        return false;
    }
};
