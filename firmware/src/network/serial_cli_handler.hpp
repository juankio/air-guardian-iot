#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>

struct TelemetryCliContext {
    float ppm;
    float co_ppm;
    float temperature;
    float humidity;
    size_t offlinePending;
    bool mqttConnected;
    bool hasMpu;
    int raw135;
    int raw7;
    float deltaPpm;
    const char* emotion;
    const char* viewMode;
    float eyeX;
    float eyeY;
    float pitch;
    float roll;
    float ax;
    float ay;
    float az;
    bool isShaking;
    float shakeEnergy;
    float babyRockScore;
};

namespace SerialCliHandler {

inline void sendQuickJsonAck(const char* cmd, const char* status, const char* extraKey = nullptr, const char* extraVal = nullptr) {
    JsonDocument res;
    if (cmd) res["cmd"] = cmd;
    if (status) res["status"] = status;
    if (extraKey && extraVal) res[extraKey] = extraVal;
    serializeJson(res, Serial);
    Serial.println();
}

inline void handleScanI2CCommand() {
    JsonDocument res;
    res["cmd"] = "I2C_RESULT";
    JsonArray arr = res["devices"].to<JsonArray>();
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            char hexStr[8];
            snprintf(hexStr, sizeof(hexStr), "0x%02X", a);
            arr.add(hexStr);
        }
    }
    serializeJson(res, Serial);
    Serial.println();
}

inline void sendStatusReport(
    const TelemetryCliContext& ctx,
    const String& fallbackSsid,
    const String& server,
    const String& ingestUrl
) {
    JsonDocument report;
    report["cmd"] = "STATUS";

    JsonObject net = report["network"].to<JsonObject>();
    bool connected = (WiFi.status() == WL_CONNECTED);
    net["connected"] = connected;
    net["ssid"] = connected ? WiFi.SSID() : fallbackSsid;
    net["ip"] = connected ? WiFi.localIP().toString() : "0.0.0.0";
    net["rssi"] = connected ? WiFi.RSSI() : 0;
    net["mac"] = WiFi.macAddress();
    net["server"] = server;
    net["ingest_url"] = ingestUrl;
    net["mqtt_connected"] = ctx.mqttConnected;

    JsonObject hw = report["hardware"].to<JsonObject>();
    hw["free_heap"] = ESP.getFreeHeap();
    hw["min_free_heap"] = ESP.getMinFreeHeap();
    hw["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    hw["uptime_s"] = millis() / 1000;
    hw["core"] = xPortGetCoreID();

    JsonObject sens = report["sensors"].to<JsonObject>();
    sens["ppm"] = round(ctx.ppm * 10) / 10.0;
    sens["co_ppm"] = round(ctx.co_ppm * 10) / 10.0;
    sens["temperature"] = round(ctx.temperature * 10) / 10.0;
    sens["humidity"] = round(ctx.humidity * 10) / 10.0;
    sens["delta_ppm"] = round(ctx.deltaPpm * 10) / 10.0;
    sens["raw_adc135"] = ctx.raw135;
    sens["raw_adc7"] = ctx.raw7;
    sens["mpu_connected"] = ctx.hasMpu;
    sens["emotion"] = ctx.emotion ? ctx.emotion : "OPTIMAL";
    sens["view_mode"] = ctx.viewMode ? ctx.viewMode : "FACE";
    sens["eye_x"] = round(ctx.eyeX * 100) / 100.0;
    sens["eye_y"] = round(ctx.eyeY * 100) / 100.0;
    sens["pitch"] = round(ctx.pitch * 10) / 10.0;
    sens["roll"] = round(ctx.roll * 10) / 10.0;
    sens["ax"] = round(ctx.ax * 100) / 100.0;
    sens["ay"] = round(ctx.ay * 100) / 100.0;
    sens["az"] = round(ctx.az * 100) / 100.0;
    sens["is_shaking"] = ctx.isShaking;
    sens["shake_energy"] = round(ctx.shakeEnergy * 10) / 10.0;
    sens["baby_rock_score"] = round(ctx.babyRockScore * 10) / 10.0;

    JsonObject storage = report["storage"].to<JsonObject>();
    storage["offline_pending"] = ctx.offlinePending;

    serializeJson(report, Serial);
    Serial.println();
}

} // namespace SerialCliHandler
