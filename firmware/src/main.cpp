#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "display_renderer.hpp"
#include "sensors.hpp"
#include "offline_buffer.hpp"
#include "wifi_provisioner.hpp"

// Configuración por defecto de Red WiFi y Backend (valores placeholder para repo público)
const char* DEFAULT_WIFI_SSID     = "TU_RED_WIFI";
const char* DEFAULT_WIFI_PASSWORD = "TU_PASSWORD";
const char* DEFAULT_MQTT_SERVER   = "192.168.1.100";
const int   MQTT_PORT             = 1883;
const char* DEVICE_ID             = "air-guardian-01";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Instancias principales
PetDisplayRenderer renderer;
SensorManager sensors;
OfflineBufferManager offlineBuffer;
WiFiProvisioner wifiProvisioner;

// Estado compartido protegido por Mutex de FreeRTOS
DisplaySharedState sharedState;
portMUX_TYPE stateMutex = portMUX_INITIALIZER_UNLOCKED;

// Variable de temporizador para emoción mareada (DIZZY)
volatile uint32_t dizzyUntilMs = 0;

// Conversor de emoción a cadena JSON
const char* emotionToString(PetEmotion emotion) {
    switch (emotion) {
        case EMOTION_OPTIMAL: return "OPTIMAL";
        case EMOTION_PREDICTIVE_WARNING: return "PREDICTIVE_WARNING";
        case EMOTION_CRITICAL: return "CRITICAL";
        case EMOTION_RECOVERY: return "RECOVERY";
        case EMOTION_SLEEPING: return "SLEEPING";
        case EMOTION_DIZZY: return "DIZZY";
        default: return "OPTIMAL";
    }
}

// Prototipos de Tareas FreeRTOS
void TaskTelemetryCore0(void* parameter);
void TaskRenderCore1(void* parameter);

// Callback MQTT para recibir alertas predictivas desde el backend
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[256];
    if (length >= sizeof(message)) length = sizeof(message) - 1;
    memcpy(message, payload, length);
    message[length] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
        const char* alert = doc["predicted_status"];
        if (alert) {
            portENTER_CRITICAL(&stateMutex);
            if (strcmp(alert, "WARNING_PREDICTIVE") == 0 || strcmp(alert, "PREDICTIVE_WARNING") == 0) {
                sharedState.emotion = EMOTION_PREDICTIVE_WARNING;
            } else if (strcmp(alert, "CRITICAL") == 0) {
                sharedState.emotion = EMOTION_CRITICAL;
            } else if (strcmp(alert, "OPTIMAL") == 0) {
                sharedState.emotion = EMOTION_OPTIMAL;
            } else if (strcmp(alert, "RECOVERY") == 0) {
                sharedState.emotion = EMOTION_RECOVERY;
            } else if (strcmp(alert, "DIZZY") == 0) {
                sharedState.emotion = EMOTION_DIZZY;
            }
            portEXIT_CRITICAL(&stateMutex);
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
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Desactivar detector de brownout por consumo de calentadores MQ
    Serial.begin(115200);
    Serial.println("\n[AIR GUARDIAN] Iniciando Sistema Dual-Core...");

    // Inicializar estado compartido
    sharedState.emotion = EMOTION_OPTIMAL;
    sharedState.viewMode = VIEW_PET_FACE;
    sharedState.ppm = 410.0f;
    sharedState.co_ppm = 0.0f;
    sharedState.temperature = 24.0f;
    sharedState.humidity = 55.0f;
    sharedState.targetEyeX = 0;
    sharedState.targetEyeY = 0;
    sharedState.shakeDetected = false;

    // Inicializar Hardware
    Serial.printf("[SYSTEM] Heap libre inicial: %u bytes\n", ESP.getFreeHeap());
    renderer.init();
    if (!sensors.init()) {
        Serial.println("[WARN] MPU-6050 no detectado. Revisa conexiones I2C.");
    } else {
        Serial.println("[OK] MPU-6050 Conectado.");
    }

    // Inicializar Aprovisionador NVS de WiFi y Servidor
    wifiProvisioner.init(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, DEFAULT_MQTT_SERVER);

    // Inicializar Buffer Persistente en LittleFS para modo sin Internet
    offlineBuffer.init();
    mqttClient.setBufferSize(2048);

    // Tarea 1: RENDER A 60 FPS en CORE 1 (Dedicado a la pantalla y MPU)
    xTaskCreatePinnedToCore(
        TaskRenderCore1,
        "TaskRenderCore1",
        8192,
        NULL,
        2, // Alta prioridad
        NULL,
        1  // Core 1
    );

    // Tarea 2: TELEMETRÍA Y COMUNICACIÓN en CORE 0 (WiFi, MQTT, MQ135, DHT11)
    xTaskCreatePinnedToCore(
        TaskTelemetryCore0,
        "TaskTelemetryCore0",
        8192,
        NULL,
        1, // Prioridad normal
        NULL,
        0  // Core 0
    );
}

void loop() {
    // El loop queda libre para el sistema de FreeRTOS
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// --------------------------------------------------------------------------
// CORE 1: MOTOR DE RENDER PROCEDURAL Y FÍSICAS (60 FPS)
// --------------------------------------------------------------------------
void TaskRenderCore1(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(16); // ~60 Hz (16.6ms)

    DisplaySharedState localState;
    uint32_t lastShakeToggle = 0;

    for (;;) {
        // 1. Leer MPU-6050 a alta velocidad
        float eyeX = 0, eyeY = 0;
        bool shake = false;
        sensors.updateMPU(eyeX, eyeY, shake);

        // 2. Gestionar cambio de vista si se sacude el dispositivo
        uint32_t now = millis();
        if (shake && (now - lastShakeToggle > 2000)) {
            lastShakeToggle = now;
            portENTER_CRITICAL(&stateMutex);
            sharedState.viewMode = (sharedState.viewMode == VIEW_PET_FACE) ? VIEW_CIRCULAR_HUD : VIEW_PET_FACE;
            dizzyUntilMs = now + 3500;
            sharedState.emotion = EMOTION_DIZZY;
            sharedState.shakeDetected = true;
            portEXIT_CRITICAL(&stateMutex);
        }

        // 3. Copiar estado atómicamente
        portENTER_CRITICAL(&stateMutex);
        sharedState.targetEyeX = eyeX;
        sharedState.targetEyeY = eyeY;
        localState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        // 4. Renderizar y enviar por DMA a la GC9A01
        renderer.updateAndRender(localState);

        // Alimentar watchdog y evitar starvation de IDLE1 en Core 1
        TickType_t nowTicks = xTaskGetTickCount();
        if ((nowTicks - xLastWakeTime) < xFrequency) {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        } else {
            // Si el render tomó >= 16ms, resetear referencia temporal y ceder CPU a IDLE1
            xLastWakeTime = nowTicks;
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

// --------------------------------------------------------------------------
// CORE 0: TELEMETRÍA, GASES, RED Y DATA LAKE INGEST (Cero freeze en pantalla)
// --------------------------------------------------------------------------
void TaskTelemetryCore0(void* parameter) {
    wifiProvisioner.startConnection();
    mqttClient.setServer(wifiProvisioner.getServer().c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    uint32_t lastPublishMs = 0;
    uint32_t lastOfflineStoreMs = 0;
    bool wasOffline = true;

    for (;;) {
        // 1. Muestreo y filtrado de sensores ambientales
        sensors.updateGasAndEnv();
        float currentPpm = sensors.getPPM();
        float currentCo = sensors.getCoPPM();
        float currentTemp = sensors.getTemp();
        float currentHum = sensors.getHumidity();
        float deltaPpm = sensors.getDeltaPPM();
        int rawAdc = (int)sensors.getRawAdc135();

        uint32_t now = millis();

        // 2. Actualizar variables compartidas para el render en Core 1
        portENTER_CRITICAL(&stateMutex);
        sharedState.ppm = currentPpm;
        sharedState.co_ppm = currentCo;
        sharedState.temperature = currentTemp;
        sharedState.humidity = currentHum;

        // Histéresis completa
        if (now < 25000 || sensors.isWarmingUp()) {
            // Durante warm-up (< 25s), mantener EMOTION_OPTIMAL
            sharedState.emotion = EMOTION_OPTIMAL;
        } else if (dizzyUntilMs > now) {
            // Si dizzyUntilMs > now, mantener EMOTION_DIZZY
            sharedState.emotion = EMOTION_DIZZY;
        } else if (sharedState.emotion != EMOTION_SLEEPING) {
            if (currentCo >= 30.0f || currentPpm >= 1500.0f) {
                sharedState.emotion = EMOTION_CRITICAL;
            } else if (sharedState.emotion == EMOTION_CRITICAL) {
                // Para EMOTION_CRITICAL, salir a RECOVERY si currentPpm < 1000 && currentCo < 25
                if (currentPpm < 1000.0f && currentCo < 25.0f) {
                    sharedState.emotion = EMOTION_RECOVERY;
                }
            } else if (deltaPpm > 45.0f || currentPpm > 1100.0f) {
                sharedState.emotion = EMOTION_PREDICTIVE_WARNING;
            } else if (currentPpm < 800.0f) {
                sharedState.emotion = EMOTION_OPTIMAL;
            } else {
                // Rama else que no quede bloqueada
                if (sharedState.emotion == EMOTION_DIZZY) {
                    sharedState.emotion = EMOTION_OPTIMAL;
                }
            }
        }
        portEXIT_CRITICAL(&stateMutex);

        // 3. Procesar comandos Serial CLI y Aprovisionamiento WiFi (No bloqueante)
        WiFiProvisioner::TelemetryContext provCtx;
        provCtx.ppm = currentPpm;
        provCtx.co_ppm = currentCo;
        provCtx.temperature = currentTemp;
        provCtx.humidity = currentHum;
        provCtx.offlinePending = offlineBuffer.getPendingCount();
        provCtx.mqttConnected = mqttClient.connected();
        provCtx.hasMpu = sensors.isMpuConnected();
        provCtx.raw135 = rawAdc;
        provCtx.raw7 = (int)sensors.getRawAdc7();
        provCtx.deltaPpm = deltaPpm;
        provCtx.emotion = emotionToString(sharedState.emotion);
        provCtx.viewMode = (sharedState.viewMode == VIEW_PET_FACE) ? "FACE" : "HUD";
        provCtx.eyeX = sharedState.targetEyeX;
        provCtx.eyeY = sharedState.targetEyeY;

        wifiProvisioner.processSerial(provCtx);
        wifiProvisioner.updateConnectionStatus();

        // Si se persistió un nuevo servidor vía comando SET_WIFI, reconfigurar MQTT
        if (wifiProvisioner.consumeNewConfigFlag()) {
            if (mqttClient.connected()) {
                mqttClient.disconnect();
            }
            mqttClient.setServer(wifiProvisioner.getServer().c_str(), MQTT_PORT);
        }

        bool isConnected = (WiFi.status() == WL_CONNECTED);

        // 4. Gestión de Conectividad y Flush Offline
        if (isConnected) {
            if (wasOffline) {
                Serial.printf("[WIFI] ¡CONECTADO CON ÉXITO A '%s'! IP: %s | RSSI: %d dBm\n", 
                              WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
            }

            // Quitar intento bloqueante de MQTT: reintento periódico con timeout ultracorto
            static uint32_t lastMqttAttemptMs = 0;
            if (!mqttClient.connected()) {
                if (now - lastMqttAttemptMs >= 20000) {
                    lastMqttAttemptMs = now;
                    espClient.setTimeout(150);
                    if (mqttClient.connect(DEVICE_ID)) {
                        Serial.println("[MQTT] Conectado exitosamente.");
                        mqttClient.subscribe("airguardian/commands/#");
                        mqttClient.subscribe("airguardian/predictions/#");
                    }
                }
            } else {
                mqttClient.loop();
            }

            // Si veníamos de estar desconectados o hay datos pendientes en LittleFS, vaciar hacia el servidor
            if (wasOffline || offlineBuffer.hasPendingData()) {
                Serial.println("[AIR GUARDIAN] Conexión detectada. Vaciando datos offline acumulados...");
                offlineBuffer.flushToBackend(wifiProvisioner.getBackendUrl().c_str(), &mqttClient);
                wasOffline = false;
            }

            // 5. Publicar Telemetría al Data Lake cada 2 segundos vía HTTP REST
            if (now - lastPublishMs >= 2000) {
                lastPublishMs = now;

                portENTER_CRITICAL(&stateMutex);
                PetEmotion currentEmotion = sharedState.emotion;
                DisplayViewMode currentViewMode = sharedState.viewMode;
                portEXIT_CRITICAL(&stateMutex);

                JsonDocument doc;
                doc["device_id"] = DEVICE_ID;
                doc["timestamp"] = now;
                doc["ppm"] = round(currentPpm * 10) / 10.0;
                doc["co_ppm"] = round(currentCo * 10) / 10.0;
                doc["temperature"] = round(currentTemp * 10) / 10.0;
                doc["humidity"] = round(currentHum * 10) / 10.0;
                doc["delta_ppm"] = round(deltaPpm * 10) / 10.0;
                doc["raw_adc"] = rawAdc;
                doc["raw_adc7"] = (int)sensors.getRawAdc7();
                doc["mpu_connected"] = sensors.isMpuConnected();
                doc["eye_x"] = round(sharedState.targetEyeX * 10) / 10.0;
                doc["eye_y"] = round(sharedState.targetEyeY * 10) / 10.0;
                doc["free_heap"] = ESP.getFreeHeap();
                doc["uptime_s"] = now / 1000;
                doc["offline_pending"] = offlineBuffer.getPendingCount();
                doc["emotion"] = emotionToString(currentEmotion);
                doc["view_mode"] = (currentViewMode == VIEW_PET_FACE) ? "FACE" : "HUD";

                String jsonPayload;
                serializeJson(doc, jsonPayload);

                // Ingesta directa HTTP a la API de NestJS
                String ingestUrl = "http://" + wifiProvisioner.getServer() + ":3002/api/telemetry/ingest";
                HTTPClient http;
                http.begin(ingestUrl);
                http.addHeader("Content-Type", "application/json");
                http.setTimeout(1200);
                int code = http.POST(jsonPayload);
                if (code == 200 || code == 201) {
                    Serial.printf("[HTTP-LIVE] Muestra publicada a %s (Code %d)\n", ingestUrl.c_str(), code);
                    String respPayload = http.getString();
                    if (respPayload.length() > 0) {
                        JsonDocument respDoc;
                        DeserializationError err = deserializeJson(respDoc, respPayload);
                        if (!err) {
                            portENTER_CRITICAL(&stateMutex);
                            if (respDoc["command_view_mode"].is<const char*>()) {
                                const char* vm = respDoc["command_view_mode"];
                                if (vm) {
                                    if (strcmp(vm, "HUD") == 0 || strcmp(vm, "VIEW_CIRCULAR_HUD") == 0) {
                                        sharedState.viewMode = VIEW_CIRCULAR_HUD;
                                    } else if (strcmp(vm, "FACE") == 0 || strcmp(vm, "VIEW_PET_FACE") == 0) {
                                        sharedState.viewMode = VIEW_PET_FACE;
                                    }
                                }
                            }
                            if (respDoc["command_emotion"].is<const char*>()) {
                                const char* emo = respDoc["command_emotion"];
                                if (emo) {
                                    if (strcmp(emo, "OPTIMAL") == 0) sharedState.emotion = EMOTION_OPTIMAL;
                                    else if (strcmp(emo, "PREDICTIVE_WARNING") == 0 || strcmp(emo, "WARNING_PREDICTIVE") == 0) sharedState.emotion = EMOTION_PREDICTIVE_WARNING;
                                    else if (strcmp(emo, "CRITICAL") == 0) sharedState.emotion = EMOTION_CRITICAL;
                                    else if (strcmp(emo, "RECOVERY") == 0) sharedState.emotion = EMOTION_RECOVERY;
                                    else if (strcmp(emo, "SLEEPING") == 0) sharedState.emotion = EMOTION_SLEEPING;
                                    else if (strcmp(emo, "DIZZY") == 0) sharedState.emotion = EMOTION_DIZZY;
                                }
                            }
                            portEXIT_CRITICAL(&stateMutex);
                        }
                    }
                }
                http.end();
            }
        } else {
            // MODO OFFLINE: Sin conexión WiFi/Internet
            wasOffline = true;

            // Reintentar conexión Wi-Fi periódicamente suavemente cada 25 segundos (sin WiFi.disconnect() destructivo)
            static uint32_t lastWifiRetryMs = 0;
            if (now - lastWifiRetryMs >= 25000) {
                lastWifiRetryMs = now;
                Serial.printf("[WIFI] Reintentando conexión suave a '%s'...\n", wifiProvisioner.getSSID().c_str());
                if (WiFi.status() != WL_CONNECTED) {
                    WiFi.begin(wifiProvisioner.getSSID().c_str(), wifiProvisioner.getPassword().c_str());
                }
            }

            // Guardar lecturas en buffer persistente de flash cada 2 segundos
            if (now - lastOfflineStoreMs >= 2000) {
                lastOfflineStoreMs = now;
                OfflineTelemetryRecord rec;
                rec.deviceId = DEVICE_ID;
                rec.timestamp = now;
                rec.ppm = currentPpm;
                rec.co_ppm = currentCo;
                rec.temperature = currentTemp;
                rec.humidity = currentHum;
                rec.delta_ppm = deltaPpm;
                rec.raw_adc = rawAdc;

                offlineBuffer.storeRecord(rec);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
