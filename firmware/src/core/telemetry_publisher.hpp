#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "emotion_arbiter.hpp"
#include "../wifi_provisioner.hpp"

inline void applyCommandJsonDoc(
    const JsonDocument& doc,
    EmotionArbiterState& arb,
    DisplaySharedState& sharedState,
    portMUX_TYPE& stateMutex
) {
    const char* alert = doc["predicted_status"] | doc["command_emotion"];
    if (alert) {
        applyExternalEmotion(arb, sharedState, stateMutex, alert);
    }
    if (doc["command_view_mode"].is<const char*>()) {
        const char* vm = doc["command_view_mode"];
        if (vm) {
            portENTER_CRITICAL(&stateMutex);
            if (strcmp(vm, "HUD") == 0 || strcmp(vm, "VIEW_CIRCULAR_HUD") == 0) {
                sharedState.viewMode = VIEW_CIRCULAR_HUD;
            } else if (strcmp(vm, "FACE") == 0 || strcmp(vm, "VIEW_PET_FACE") == 0) {
                sharedState.viewMode = VIEW_PET_FACE;
            }
            portEXIT_CRITICAL(&stateMutex);
        }
    }
}

inline void populateProvisionerContext(
    WiFiProvisioner::TelemetryContext& provCtx,
    const DisplaySharedState& snapState,
    const SensorManager& sensors,
    size_t offlinePending,
    bool mqttConnected,
    bool isShakingNow
) {
    provCtx.ppm = snapState.ppm;
    provCtx.co_ppm = snapState.co_ppm;
    provCtx.temperature = snapState.temperature;
    provCtx.humidity = snapState.humidity;
    provCtx.offlinePending = offlinePending;
    provCtx.mqttConnected = mqttConnected;
    provCtx.hasMpu = sensors.isMpuConnected();
    provCtx.raw135 = (int)sensors.getRawAdc135();
    provCtx.raw7 = (int)sensors.getRawAdc7();
    provCtx.deltaPpm = sensors.getDeltaPPM();
    provCtx.emotion = emotionToString(snapState.emotion);
    provCtx.viewMode = (snapState.viewMode == VIEW_PET_FACE) ? "FACE" : "HUD";
    provCtx.eyeX = roundf((snapState.roll / 9.0f) * 100.0f) / 100.0f;
    provCtx.eyeY = roundf((snapState.pitch / 9.0f) * 100.0f) / 100.0f;
    provCtx.pitch = snapState.pitch;
    provCtx.roll = snapState.roll;
    provCtx.ax = snapState.ax;
    provCtx.ay = snapState.ay;
    provCtx.az = snapState.az;
    provCtx.isShaking = isShakingNow;
    provCtx.shakeEnergy = sensors.getShakeEnergy();
    provCtx.babyRockScore = sensors.getBabyRockScore();
}

inline String buildTelemetryPayload(
    const char* deviceId,
    uint32_t now,
    const DisplaySharedState& snapState,
    const SensorManager& sensors,
    size_t pendingCount,
    bool currentShaking
) {
    const char* emotionStr = emotionToString(snapState.emotion);
    const char* viewModeStr = (snapState.viewMode == VIEW_PET_FACE) ? "FACE" : "HUD";
    int rawAdc = (int)sensors.getRawAdc135();
    int rawAdc7 = (int)sensors.getRawAdc7();

    JsonDocument doc;
    doc["device_id"] = deviceId;
    doc["timestamp"] = now > 0 ? now : 1;
    doc["ppm"] = round(snapState.ppm * 10) / 10.0;
    doc["co_ppm"] = round(snapState.co_ppm * 10) / 10.0;
    doc["raw_adc"] = constrain(rawAdc, 0, 4095);
    doc["voltage"] = round(sensors.getVoltage135() * 100) / 100.0;
    doc["mq7_raw"] = rawAdc7;
    doc["raw_adc7"] = rawAdc7;
    doc["mq7_voltage"] = round(sensors.getVoltage7() * 100) / 100.0;
    doc["temperature"] = round(snapState.temperature * 10) / 10.0;
    doc["humidity"] = round(snapState.humidity * 10) / 10.0;
    doc["delta_ppm"] = round(sensors.getDeltaPPM() * 10) / 10.0;
    doc["Status"] = emotionStr;
    doc["emotion"] = emotionStr;
    doc["view_mode"] = viewModeStr;
    doc["mpu_connected"] = sensors.isMpuConnected();
    doc["eye_x"] = roundf((snapState.roll / 9.0f) * 100.0f) / 100.0f;
    doc["eye_y"] = roundf((snapState.pitch / 9.0f) * 100.0f) / 100.0f;
    doc["pitch"] = round(snapState.pitch * 10) / 10.0;
    doc["roll"] = round(snapState.roll * 10) / 10.0;
    doc["ax"] = round(snapState.ax * 100) / 100.0;
    doc["ay"] = round(snapState.ay * 100) / 100.0;
    doc["az"] = round(snapState.az * 100) / 100.0;
    doc["is_shaking"] = currentShaking;
    doc["shake_energy"] = round(sensors.getShakeEnergy() * 10) / 10.0;
    doc["baby_rock_score"] = round(sensors.getBabyRockScore() * 10) / 10.0;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();
    doc["uptime_s"] = now / 1000;
    doc["offline_queue"] = pendingCount;
    doc["offline_pending"] = pendingCount;

    String jsonPayload;
    serializeJson(doc, jsonPayload);
    return jsonPayload;
}

inline void publishLiveTelemetryHttp(
    HTTPClient& http,
    WiFiClientSecure& secureClient,
    WiFiClient& plainClient,
    String& activeIngestUrl,
    const String& ingestUrl,
    const String& jsonPayload,
    const char* emotionStr,
    uint32_t now,
    EmotionArbiterState& arb,
    DisplaySharedState& sharedState,
    portMUX_TYPE& stateMutex
) {
    if (activeIngestUrl != ingestUrl) {
        http.end();
        activeIngestUrl = ingestUrl;
    }

    if (ingestUrl.startsWith("https://")) {
        http.begin(secureClient, ingestUrl);
    } else {
        http.begin(plainClient, ingestUrl);
    }
    http.setReuse(true);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2500);
    int code = http.POST(jsonPayload);
    if (code == 200 || code == 201) {
        static uint32_t lastHttpLogMs = 0;
        if (now - lastHttpLogMs >= 1000) {
            lastHttpLogMs = now;
            Serial.printf("[HTTP-LIVE] Muestra publicada a %s (Code %d | Emotion: %s)\n", ingestUrl.c_str(), code, emotionStr);
        }
        String respPayload = http.getString();
        if (respPayload.length() > 0) {
            JsonDocument respDoc;
            if (!deserializeJson(respDoc, respPayload)) {
                applyCommandJsonDoc(respDoc, arb, sharedState, stateMutex);
            }
        }
        http.end();
    } else if (code < 0) {
        secureClient.stop();
        plainClient.stop();
        http.end();
        activeIngestUrl = "";
    } else {
        http.end();
    }
}
