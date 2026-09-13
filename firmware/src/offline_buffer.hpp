#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

/**
 * Estructura de muestra para buffer offline
 */
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
    const size_t MAX_FILE_SIZE = 300 * 1024; // 300 KB max en LittleFS (~3500 muestras)
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

public:
    OfflineBufferManager() {}

    bool init() {
        if (!LittleFS.begin(true)) {
            Serial.println("[OFFLINE-FS] ERROR: No se pudo montar LittleFS.");
            isMounted = false;
            return false;
        }
        isMounted = true;
        updatePendingCount();
        Serial.printf("[OFFLINE-FS] LittleFS montado con éxito. Muestras pendientes en cola: %u\n", (unsigned int)pendingCount);
        return true;
    }

    bool storeRecord(const OfflineTelemetryRecord& rec) {
        if (!isMounted) return false;

        // Comprobación de cuota máxima de archivo
        if (LittleFS.exists(filePath)) {
            File checkFile = LittleFS.open(filePath, "r");
            if (checkFile && checkFile.size() > MAX_FILE_SIZE) {
                checkFile.close();
                Serial.println("[OFFLINE-FS] Buffer lleno. Rotando archivo para proteger la flash...");
                // Eliminamos para evitar saturar almacenamiento si estuvo desconectado semanas
                LittleFS.remove(filePath);
                pendingCount = 0;
            } else if (checkFile) {
                checkFile.close();
            }
        }

        File file = LittleFS.open(filePath, "a");
        if (!file) {
            Serial.println("[OFFLINE-FS] Error al abrir archivo para anexar muestra.");
            return false;
        }

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
        Serial.printf("[OFFLINE-FS] Muestra encolada localmente. Total acumulado: %u\n", (unsigned int)pendingCount);
        return true;
    }

    size_t getPendingCount() const {
        return pendingCount;
    }

    bool hasPendingData() const {
        return pendingCount > 0;
    }

    /**
     * Vaciado y sincronización masiva hacia el Backend al recuperar WiFi.
     * Envía lotes de 15 muestras vía HTTP POST a /api/telemetry/batch
     * o por MQTT si HTTP falla.
     */
    bool flushToBackend(const char* backendUrl, PubSubClient* mqttClient = nullptr) {
        if (!isMounted || !hasPendingData()) return true;

        Serial.printf("[OFFLINE-SYNC] Iniciando vaciado de buffer (%u muestras acumuladas)...\n", (unsigned int)pendingCount);

        File file = LittleFS.open(filePath, "r");
        if (!file) {
            pendingCount = 0;
            return false;
        }

        HTTPClient http;
        bool allFlushedSuccessfully = true;
        const size_t BATCH_SIZE = 15;

        JsonDocument batchDoc;
        JsonArray samplesArray = batchDoc["samples"].to<JsonArray>();
        size_t currentBatchCount = 0;
        size_t totalSent = 0;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() < 5) continue;

            JsonDocument itemDoc;
            DeserializationError err = deserializeJson(itemDoc, line);
            if (!err) {
                samplesArray.add(itemDoc);
                currentBatchCount++;
            }

            if (currentBatchCount >= BATCH_SIZE || (!file.available() && currentBatchCount > 0)) {
                String payload;
                serializeJson(batchDoc, payload);

                bool batchSuccess = false;

                // 1. Intentar HTTP POST primero (óptimo para lotes grandes)
                if (backendUrl && strlen(backendUrl) > 0) {
                    http.begin(backendUrl);
                    http.addHeader("Content-Type", "application/json");
                    http.setTimeout(1500); // Evitar congelar Core 0 si el backend no responde
                    int httpCode = http.POST(payload);
                    if (httpCode == 200 || httpCode == 201) {
                        batchSuccess = true;
                        totalSent += currentBatchCount;
                        Serial.printf("[OFFLINE-SYNC] Lote de %u enviado por HTTP (Código %d). Progreso: %u\n",
                                      (unsigned int)currentBatchCount, httpCode, (unsigned int)totalSent);
                    } else {
                        Serial.printf("[OFFLINE-SYNC] HTTP falló (código %d). Reintentando por MQTT...\n", httpCode);
                    }
                    http.end();
                }

                // 2. Fallback a MQTT si HTTP falló o no se configuró
                if (!batchSuccess && mqttClient && mqttClient->connected()) {
                    if (mqttClient->publish("airguardian/telemetry/batch", payload.c_str())) {
                        batchSuccess = true;
                        totalSent += currentBatchCount;
                        Serial.printf("[OFFLINE-SYNC] Lote de %u enviado vía MQTT.\n", (unsigned int)currentBatchCount);
                    }
                }

                if (!batchSuccess) {
                    Serial.println("[OFFLINE-SYNC] Error enviando lote. Se conservarán los datos restantes.");
                    allFlushedSuccessfully = false;
                    break;
                }

                // Reiniciar para el siguiente lote
                batchDoc.clear();
                samplesArray = batchDoc["samples"].to<JsonArray>();
                currentBatchCount = 0;
                vTaskDelay(pdMS_TO_TICKS(150)); // Pequeña pausa para descongestionar el socket
            }
        }

        file.close();

        // Si se vació exitosamente todo el histórico acumulado, limpiamos el archivo
        if (allFlushedSuccessfully) {
            LittleFS.remove(filePath);
            pendingCount = 0;
            Serial.printf("[OFFLINE-SYNC] ¡SINCRONIZACIÓN EXITOSA! %u registros volcados al Data Lake.\n", (unsigned int)totalSent);
            return true;
        } else {
            updatePendingCount();
            return false;
        }
    }
};
