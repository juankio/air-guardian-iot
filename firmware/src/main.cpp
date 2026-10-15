#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "display_renderer.hpp"
#include "sensors.hpp"
#include "offline_buffer.hpp"
#include "wifi_provisioner.hpp"
#include "core/emotion_arbiter.hpp"
#include "core/telemetry_publisher.hpp"

const char* DEFAULT_WIFI_SSID     = "TU_RED_WIFI";
const char* DEFAULT_WIFI_PASSWORD = "TU_PASSWORD";
const char* DEFAULT_MQTT_SERVER   = DEFAULT_SERVER;
const int   MQTT_PORT             = 1883;
const char* DEVICE_ID             = "air-guardian-01";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
PetDisplayRenderer renderer;
SensorManager sensors;
OfflineBufferManager offlineBuffer;
WiFiProvisioner wifiProvisioner;

DisplaySharedState sharedState;
portMUX_TYPE stateMutex = portMUX_INITIALIZER_UNLOCKED;
EmotionArbiterState emotionArbiter;

void TaskTelemetryCore0(void* parameter);
void TaskRenderCore1(void* parameter);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[256];
    if (length >= sizeof(message)) length = sizeof(message) - 1;
    memcpy(message, payload, length);
    message[length] = '\0';
    JsonDocument doc;
    if (!deserializeJson(doc, message)) applyCommandJsonDoc(doc, emotionArbiter, sharedState, stateMutex);
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    Serial.println("\n[AIR GUARDIAN] Iniciando Sistema Dual-Core (Emo-Mecha v3.0)...");
    sharedState = { EMOTION_OPTIMAL, VIEW_PET_FACE, 410.0f, 0.0f, 24.0f, 55.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, false };
    Serial.printf("[SYSTEM] Heap libre inicial: %u bytes\n", ESP.getFreeHeap());
    renderer.init();
    sensors.init();
    wifiProvisioner.init(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, DEFAULT_MQTT_SERVER);
    offlineBuffer.init();
    mqttClient.setBufferSize(2048);
    xTaskCreatePinnedToCore(TaskRenderCore1, "TaskRenderCore1", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(TaskTelemetryCore0, "TaskTelemetryCore0", 16384, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void TaskRenderCore1(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(16);
    DisplaySharedState localState;

    for (;;) {
        float eyeX = 0, eyeY = 0;
        bool shake = false;
        sensors.updateMPU(eyeX, eyeY, shake);
        uint32_t now = millis();

        portENTER_CRITICAL(&stateMutex);
        sharedState.targetEyeX = eyeX;
        sharedState.targetEyeY = eyeY;
        sharedState.pitch = sensors.getPitch();
        sharedState.roll = sensors.getRoll();
        sharedState.ax = sensors.getNormAx();
        sharedState.ay = sensors.getNormAy();
        sharedState.az = sensors.getNormAz();
        bool activeShake = false;
        sharedState.emotion = resolveActiveEmotion(emotionArbiter, sharedState, sensors, now, shake, activeShake);
        sharedState.shakeDetected = activeShake;
        localState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        renderer.updateAndRender(localState);

        TickType_t nowTicks = xTaskGetTickCount();
        if ((nowTicks - xLastWakeTime) < xFrequency) { vTaskDelayUntil(&xLastWakeTime, xFrequency); }
        else { xLastWakeTime = nowTicks; vTaskDelay(pdMS_TO_TICKS(2)); }
    }
}

void TaskTelemetryCore0(void* parameter) {
    wifiProvisioner.startConnection();
    String mqttHost = wifiProvisioner.getMqttHost();
    mqttClient.setServer(mqttHost.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    uint32_t lastPublishMs = 0, lastOfflineStoreMs = 0;
    bool wasOffline = true;
    WiFiClientSecure secureClient; secureClient.setInsecure();
    WiFiClient plainClient;
    HTTPClient http; http.setReuse(true);
    String activeIngestUrl = "";

    for (;;) {
        sensors.updateGasAndEnv();
        uint32_t now = millis();
        DisplaySharedState snapState;
        portENTER_CRITICAL(&stateMutex);
        sharedState.ppm = sensors.getPPM();
        sharedState.co_ppm = sensors.getCoPPM();
        sharedState.temperature = sensors.getTemp();
        sharedState.humidity = sensors.getHumidity();
        snapState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        bool isShakingNow = snapState.shakeDetected || sensors.isShaking();
        WiFiProvisioner::TelemetryContext provCtx;
        populateProvisionerContext(provCtx, snapState, sensors, offlineBuffer.getPendingCount(), mqttClient.connected(), isShakingNow);

        wifiProvisioner.processSerial(provCtx);
        if (wifiProvisioner.consumeCalibrateMpuFlag()) sensors.calibrateRestingOffset();
        String serialEmotion;
        if (wifiProvisioner.consumeEmotionCommand(serialEmotion)) {
            applyExternalEmotion(emotionArbiter, sharedState, stateMutex, serialEmotion.c_str());
        }
        wifiProvisioner.updateConnectionStatus();

        if (wifiProvisioner.consumeNewConfigFlag()) {
            if (mqttClient.connected()) mqttClient.disconnect();
            mqttHost = wifiProvisioner.getMqttHost();
            mqttClient.setServer(mqttHost.c_str(), MQTT_PORT);
        }

        if (WiFi.status() == WL_CONNECTED) {
            static uint32_t lastMqttAttemptMs = 0;
            String currentSrv = wifiProvisioner.getServer();
            bool isHttpsOrFunnel = currentSrv.startsWith("https://") || (currentSrv.indexOf(".ts.net") >= 0);
            if (!isHttpsOrFunnel) {
                if (!mqttClient.connected() && (now - lastMqttAttemptMs >= 20000)) {
                    lastMqttAttemptMs = now; espClient.setTimeout(1);
                    if (mqttClient.connect(DEVICE_ID)) {
                        mqttClient.subscribe("airguardian/commands/#");
                        mqttClient.subscribe("airguardian/predictions/#");
                    }
                } else if (mqttClient.connected()) mqttClient.loop();
            }

            static uint32_t lastOfflineFlushAttemptMs = 0;
            if (wasOffline || (offlineBuffer.hasPendingData() && (now - lastOfflineFlushAttemptMs >= 15000))) {
                lastOfflineFlushAttemptMs = now;
                offlineBuffer.flushToBackend(wifiProvisioner.getBackendUrl().c_str(), &mqttClient);
                wasOffline = false;
            }

            if (now - lastPublishMs >= 250) {
                lastPublishMs = now;
                portENTER_CRITICAL(&stateMutex); snapState = sharedState; portEXIT_CRITICAL(&stateMutex);
                bool currentShaking = snapState.shakeDetected || sensors.isShaking();
                String payload = buildTelemetryPayload(DEVICE_ID, now, snapState, sensors, offlineBuffer.getPendingCount(), currentShaking);
                publishLiveTelemetryHttp(http, secureClient, plainClient, activeIngestUrl, wifiProvisioner.getIngestUrl(), payload, emotionToString(snapState.emotion), now, emotionArbiter, sharedState, stateMutex);
            }
        } else {
            wasOffline = true;
            static uint32_t lastWifiRetryMs = 0;
            if (now - lastWifiRetryMs >= 25000) {
                lastWifiRetryMs = now;
                WiFi.begin(wifiProvisioner.getSSID().c_str(), wifiProvisioner.getPassword().c_str());
            }
            if (now - lastOfflineStoreMs >= 2000) {
                lastOfflineStoreMs = now;
                OfflineTelemetryRecord rec = { DEVICE_ID, now, snapState.ppm, snapState.co_ppm, snapState.temperature, snapState.humidity, sensors.getDeltaPPM(), (int)sensors.getRawAdc135() };
                offlineBuffer.storeRecord(rec);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
