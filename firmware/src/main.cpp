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

// Configuración por defecto de Red WiFi y Backend (valores placeholder para repo público)
const char* DEFAULT_WIFI_SSID     = "TU_RED_WIFI";
const char* DEFAULT_WIFI_PASSWORD = "TU_PASSWORD";
const char* DEFAULT_MQTT_SERVER   = DEFAULT_SERVER;
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
    sharedState.targetEyeX = 0.0f;
    sharedState.targetEyeY = 0.0f;
    sharedState.pitch = 0.0f;
    sharedState.roll = 0.0f;
    sharedState.ax = 0.0f;
    sharedState.ay = 0.0f;
    sharedState.az = 1.0f;
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
        16384,
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

        // 2. Gestionar estado mareado si se sacude el dispositivo (manteniendo rostro expresivo)
        uint32_t now = millis();
        if (shake && (now - lastShakeToggle > 2000)) {
            lastShakeToggle = now;
            portENTER_CRITICAL(&stateMutex);
            sharedState.viewMode = VIEW_PET_FACE;
            dizzyUntilMs = now + 3500;
            sharedState.emotion = EMOTION_DIZZY;
            sharedState.shakeDetected = true;
            portEXIT_CRITICAL(&stateMutex);
        }

        // 3. Copiar estado atómicamente
        portENTER_CRITICAL(&stateMutex);
        sharedState.targetEyeX = eyeX;
        sharedState.targetEyeY = eyeY;
        sharedState.pitch = sensors.getPitch();
        sharedState.roll = sensors.getRoll();
        sharedState.ax = sensors.getNormAx();
        sharedState.ay = sensors.getNormAy();
        sharedState.az = sensors.getNormAz();
        if (shake) {
            sharedState.shakeDetected = true;
        } else if (now >= dizzyUntilMs) {
            sharedState.shakeDetected = false;
        }
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
    String mqttHost = wifiProvisioner.getMqttHost();
    mqttClient.setServer(mqttHost.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    uint32_t lastPublishMs = 0;
    uint32_t lastOfflineStoreMs = 0;
    bool wasOffline = true;

    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    WiFiClient plainClient;
    HTTPClient http;
    http.setReuse(true);
    String activeIngestUrl = "";

    for (;;) {
        // 1. Muestreo y filtrado de sensores ambientales
        sensors.updateGasAndEnv();
        float currentPpm = sensors.getPPM();
        float currentCo = sensors.getCoPPM();
        float currentTemp = sensors.getTemp();
        float currentHum = sensors.getHumidity();
        float deltaPpm = sensors.getDeltaPPM();
        int rawAdc = (int)sensors.getRawAdc135();
        int rawAdc7 = (int)sensors.getRawAdc7();
        float voltage135 = sensors.getVoltage135();
        float voltage7 = sensors.getVoltage7();

        uint32_t now = millis();

        // 2. Actualizar variables compartidas para el render en Core 1
        DisplaySharedState snapState;
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
        snapState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        bool isShakingNow = snapState.shakeDetected || sensors.isShaking() || (dizzyUntilMs > now) || (snapState.emotion == EMOTION_DIZZY);

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
        provCtx.raw7 = rawAdc7;
        provCtx.deltaPpm = deltaPpm;
        float encodedProvEyeX = roundf((snapState.roll / 9.0f) * 100.0f) / 100.0f;
        float encodedProvEyeY = roundf((snapState.pitch / 9.0f) * 100.0f) / 100.0f;
        provCtx.emotion = emotionToString(snapState.emotion);
        provCtx.viewMode = (snapState.viewMode == VIEW_PET_FACE) ? "FACE" : "HUD";
        provCtx.eyeX = encodedProvEyeX;
        provCtx.eyeY = encodedProvEyeY;
        provCtx.pitch = snapState.pitch;
        provCtx.roll = snapState.roll;
        provCtx.ax = snapState.ax;
        provCtx.ay = snapState.ay;
        provCtx.az = snapState.az;
        provCtx.isShaking = isShakingNow;

        wifiProvisioner.processSerial(provCtx);
        if (wifiProvisioner.consumeCalibrateMpuFlag()) {
            sensors.calibrateRestingOffset();
        }
        wifiProvisioner.updateConnectionStatus();

        // Si se persistió un nuevo servidor vía comando SET_WIFI, reconfigurar MQTT
        if (wifiProvisioner.consumeNewConfigFlag()) {
            if (mqttClient.connected()) {
                mqttClient.disconnect();
            }
            mqttHost = wifiProvisioner.getMqttHost();
            mqttClient.setServer(mqttHost.c_str(), MQTT_PORT);
        }

        bool isConnected = (WiFi.status() == WL_CONNECTED);

        // 4. Gestión de Conectividad y Flush Offline
        if (isConnected) {
            if (wasOffline) {
                Serial.printf("[WIFI] ¡CONECTADO CON ÉXITO A '%s'! IP: %s | RSSI: %d dBm\n", 
                              WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
            }

            // Quitar intento bloqueante de MQTT cuando se usa HTTPS / Tailscale Funnel (solo HTTP/HTTPS en puerto 10000)
            static uint32_t lastMqttAttemptMs = 0;
            String currentSrv = wifiProvisioner.getServer();
            bool isHttpsOrFunnel = currentSrv.startsWith("https://") || (currentSrv.indexOf(".ts.net") >= 0);
            if (!isHttpsOrFunnel) {
                if (!mqttClient.connected()) {
                    if (now - lastMqttAttemptMs >= 20000) {
                        lastMqttAttemptMs = now;
                        espClient.setTimeout(1);
                        if (mqttClient.connect(DEVICE_ID)) {
                            Serial.println("[MQTT] Conectado exitosamente.");
                            mqttClient.subscribe("airguardian/commands/#");
                            mqttClient.subscribe("airguardian/predictions/#");
                        }
                    }
                } else {
                    mqttClient.loop();
                }
            }

            // Si veníamos de estar desconectados o hay datos pendientes en LittleFS, vaciar hacia el servidor
            static uint32_t lastOfflineFlushAttemptMs = 0;
            if (wasOffline || (offlineBuffer.hasPendingData() && (now - lastOfflineFlushAttemptMs >= 15000))) {
                lastOfflineFlushAttemptMs = now;
                Serial.println("[AIR GUARDIAN] Conexión detectada. Vaciando datos offline acumulados...");
                offlineBuffer.flushToBackend(wifiProvisioner.getBackendUrl().c_str(), &mqttClient);
                wasOffline = false;
            }

            // 5. Publicar Telemetría al Data Lake cada 250ms (4 Hz) vía HTTP/HTTPS Keep-Alive
            if (now - lastPublishMs >= 250) {
                lastPublishMs = now;

                portENTER_CRITICAL(&stateMutex);
                snapState = sharedState;
                portEXIT_CRITICAL(&stateMutex);

                PetEmotion currentEmotion = snapState.emotion;
                DisplayViewMode currentViewMode = snapState.viewMode;
                bool currentShaking = snapState.shakeDetected || sensors.isShaking() || (dizzyUntilMs > now) || (currentEmotion == EMOTION_DIZZY);
                const char* emotionStr = emotionToString(currentEmotion);
                const char* viewModeStr = (currentViewMode == VIEW_PET_FACE) ? "FACE" : "HUD";
                size_t pendingCount = offlineBuffer.getPendingCount();

                JsonDocument doc;
                doc["device_id"] = DEVICE_ID;
                doc["timestamp"] = now > 0 ? now : 1;
                doc["ppm"] = round(currentPpm * 10) / 10.0;
                doc["co_ppm"] = round(currentCo * 10) / 10.0;
                doc["raw_adc"] = constrain(rawAdc, 0, 4095);
                doc["voltage"] = round(voltage135 * 100) / 100.0;
                doc["mq7_raw"] = rawAdc7;
                doc["raw_adc7"] = rawAdc7;
                doc["mq7_voltage"] = round(voltage7 * 100) / 100.0;
                doc["temperature"] = round(currentTemp * 10) / 10.0;
                doc["humidity"] = round(currentHum * 10) / 10.0;
                doc["delta_ppm"] = round(deltaPpm * 10) / 10.0;
                doc["Status"] = emotionStr;
                doc["emotion"] = emotionStr;
                doc["view_mode"] = viewModeStr;
                float encodedEyeX = roundf((snapState.roll / 9.0f) * 100.0f) / 100.0f;
                float encodedEyeY = roundf((snapState.pitch / 9.0f) * 100.0f) / 100.0f;
                doc["mpu_connected"] = sensors.isMpuConnected();
                doc["eye_x"] = encodedEyeX;
                doc["eye_y"] = encodedEyeY;
                doc["pitch"] = round(snapState.pitch * 10) / 10.0;
                doc["roll"] = round(snapState.roll * 10) / 10.0;
                doc["ax"] = round(snapState.ax * 100) / 100.0;
                doc["ay"] = round(snapState.ay * 100) / 100.0;
                doc["az"] = round(snapState.az * 100) / 100.0;
                doc["is_shaking"] = currentShaking;
                doc["free_heap"] = ESP.getFreeHeap();
                doc["min_free_heap"] = ESP.getMinFreeHeap();
                doc["uptime_s"] = now / 1000;
                doc["offline_queue"] = pendingCount;
                doc["offline_pending"] = pendingCount;

                String jsonPayload;
                serializeJson(doc, jsonPayload);

                // Ingesta directa HTTP/HTTPS a la API de NestJS con Keep-Alive (TLS reuse)
                String ingestUrl = wifiProvisioner.getIngestUrl();
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
                        Serial.printf("[HTTP-LIVE] Muestra publicada a %s (Code %d)\n", ingestUrl.c_str(), code);
                    }
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

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
