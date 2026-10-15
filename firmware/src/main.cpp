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

// Temporizadores de máquina de estados física y emocional (Emo-Mecha v3.0)
volatile uint32_t manualEmotionUntilMs = 0;
volatile PetEmotion manualEmotion = EMOTION_OPTIMAL;

volatile uint32_t vomitSeqStartMs = 0;
volatile uint32_t nauseaUntilMs = 0;
volatile uint32_t dizzyUntilMs = 0;
volatile uint32_t startledUntilMs = 0;
volatile uint32_t babySleepUntilMs = 0;
volatile uint32_t babyRockUntilMs = 0;

// Carrusel autónomo Tamagotchi (17 animaciones de vida cuando está en reposo)
static const PetEmotion IDLE_CAROUSEL[] = {
    EMOTION_WINK,
    EMOTION_BUBBLEGUM,
    EMOTION_WHISTLE,
    EMOTION_LOVE,
    EMOTION_GLASSES_COOL,
    EMOTION_COFFEE,
    EMOTION_RAMEN,
    EMOTION_MUSIC_VIBE,
    EMOTION_BUTTERFLY,
    EMOTION_DETECTIVE,
    EMOTION_DERP,
    EMOTION_SUSPICIOUS,
    EMOTION_YAWN,
    EMOTION_SNEEZE,
    EMOTION_ZEN,
    EMOTION_LAUGHING,
    EMOTION_CRYING
};
static constexpr size_t IDLE_CAROUSEL_COUNT = sizeof(IDLE_CAROUSEL) / sizeof(IDLE_CAROUSEL[0]);

void applyExternalEmotionCommand(const char* emoName) {
    PetEmotion parsed = EMOTION_OPTIMAL;
    if (stringToEmotion(emoName, parsed)) {
        uint32_t now = millis();
        portENTER_CRITICAL(&stateMutex);
        sharedState.viewMode = VIEW_PET_FACE;
        if (parsed == EMOTION_OPTIMAL) {
            manualEmotionUntilMs = 0;
            vomitSeqStartMs = 0;
            nauseaUntilMs = 0;
            dizzyUntilMs = 0;
            startledUntilMs = 0;
            babySleepUntilMs = 0;
            babyRockUntilMs = 0;
            sharedState.emotion = EMOTION_OPTIMAL;
        } else {
            manualEmotion = parsed;
            manualEmotionUntilMs = now + 8000;
            sharedState.emotion = parsed;
        }
        portEXIT_CRITICAL(&stateMutex);
    }
}

// Prototipos de Tareas FreeRTOS
void TaskTelemetryCore0(void* parameter);
void TaskRenderCore1(void* parameter);

// Callback MQTT para recibir alertas predictivas o comandos desde el backend
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[256];
    if (length >= sizeof(message)) length = sizeof(message) - 1;
    memcpy(message, payload, length);
    message[length] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
        const char* alert = doc["predicted_status"] | doc["command_emotion"];
        if (alert) {
            applyExternalEmotionCommand(alert);
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
    Serial.println("\n[AIR GUARDIAN] Iniciando Sistema Dual-Core (Emo-Mecha v3.0)...");

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
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// --------------------------------------------------------------------------
// CORE 1: MOTOR DE RENDER PROCEDURAL Y FÍSICAS (60 FPS)
// --------------------------------------------------------------------------
void TaskRenderCore1(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(16); // ~60 Hz (16.6ms)

    DisplaySharedState localState;

    for (;;) {
        // 1. Leer MPU-6050 a 60 Hz y actualizar acumuladores de energía
        float eyeX = 0, eyeY = 0;
        bool shake = false;
        sensors.updateMPU(eyeX, eyeY, shake);

        uint32_t now = millis();
        float shakeEnergy = sensors.getShakeEnergy();
        float babyRockScore = sensors.getBabyRockScore();
        float dynAccel = sensors.getDynamicAccel();
        float jerk = sensors.getJerk();
        float roll = sensors.getRoll();
        float pitch = sensors.getPitch();
        bool freefall = sensors.isFreefalling();

        // 2. Evaluar Máquina de Estados Física Progresiva (Sacudida 4 Etapas + Mecer Bebé)
        if (vomitSeqStartMs == 0) {
            if (shakeEnergy >= 62.0f) {
                // Etapa 3: ¡Vómito + Limpiaparabrisas!
                vomitSeqStartMs = now;
                nauseaUntilMs = 0;
                dizzyUntilMs = 0;
                babySleepUntilMs = 0;
                babyRockUntilMs = 0;
                sensors.resetBabyRockScore();
            } else if (shakeEnergy >= 35.0f) {
                // Etapa 2: Náusea (cara verde aguantando las ganas de vomitar)
                if (now + 4000 > nauseaUntilMs) nauseaUntilMs = now + 4000;
                babySleepUntilMs = 0;
                babyRockUntilMs = 0;
            } else if (shakeEnergy >= 12.0f) {
                // Etapa 1: Mareo (ojos en espiral y estrellas)
                if (now + 4000 > dizzyUntilMs) dizzyUntilMs = now + 4000;
                babySleepUntilMs = 0;
                babyRockUntilMs = 0;
            }
        }

        // Detector de despertar con susto (STARTLED) si estaba durmiendo como bebé y lo sacuden
        if (babySleepUntilMs > now) {
            if (dynAccel > 0.35f || jerk > 0.45f || shakeEnergy >= 12.0f) {
                babySleepUntilMs = 0;
                babyRockUntilMs = 0;
                sensors.resetBabyRockScore();
                startledUntilMs = now + 2500;
            }
        } else if (vomitSeqStartMs == 0 && nauseaUntilMs <= now && dizzyUntilMs <= now && startledUntilMs <= now) {
            // Progresión de Mecer como un bebé
            if (babyRockScore >= 55.0f) {
                babySleepUntilMs = now + 20000;
            } else if (babyRockScore >= 22.0f) {
                if (now + 3500 > babyRockUntilMs) babyRockUntilMs = now + 3500;
            }
        }

        // 3. Resolver emoción prioritaria y sincronizar estado compartido
        portENTER_CRITICAL(&stateMutex);
        sharedState.targetEyeX = eyeX;
        sharedState.targetEyeY = eyeY;
        sharedState.pitch = pitch;
        sharedState.roll = roll;
        sharedState.ax = sensors.getNormAx();
        sharedState.ay = sensors.getNormAy();
        sharedState.az = sensors.getNormAz();

        float currentPpm = sharedState.ppm;
        float currentCo = sharedState.co_ppm;
        float currentTemp = sharedState.temperature;
        float deltaPpm = sensors.getDeltaPPM();
        bool warmingUp = sensors.isWarmingUp();

        PetEmotion resolvedEmotion = EMOTION_OPTIMAL;
        bool activeShakeFlag = shake;

        if (manualEmotionUntilMs > now) {
            resolvedEmotion = manualEmotion;
        } else if (vomitSeqStartMs > 0) {
            uint32_t elapsed = now - vomitSeqStartMs;
            activeShakeFlag = true;
            sharedState.viewMode = VIEW_PET_FACE;
            if (elapsed < 4000) {
                resolvedEmotion = EMOTION_VOMIT;
            } else if (elapsed < 7500) {
                resolvedEmotion = EMOTION_WIPER;
            } else if (elapsed < 9500) {
                resolvedEmotion = EMOTION_RECOVERY;
                sensors.resetShakeEnergy();
            } else {
                vomitSeqStartMs = 0;
                resolvedEmotion = EMOTION_OPTIMAL;
                activeShakeFlag = false;
            }
        } else if (nauseaUntilMs > now) {
            resolvedEmotion = EMOTION_NAUSEA;
            activeShakeFlag = true;
            sharedState.viewMode = VIEW_PET_FACE;
        } else if (dizzyUntilMs > now) {
            resolvedEmotion = EMOTION_DIZZY;
            activeShakeFlag = true;
            sharedState.viewMode = VIEW_PET_FACE;
        } else if (startledUntilMs > now) {
            resolvedEmotion = EMOTION_STARTLED;
            sharedState.viewMode = VIEW_PET_FACE;
        } else if (babySleepUntilMs > now) {
            resolvedEmotion = EMOTION_BABY_SLEEP;
            sharedState.viewMode = VIEW_PET_FACE;
        } else if (babyRockUntilMs > now) {
            resolvedEmotion = EMOTION_BABY_ROCKING;
            sharedState.viewMode = VIEW_PET_FACE;
        } else if (freefall) {
            resolvedEmotion = EMOTION_FREEFALL;
        } else if (fabsf(roll) > 135.0f) {
            resolvedEmotion = EMOTION_UPSIDE_DOWN;
        } else if (roll < -48.0f) {
            resolvedEmotion = EMOTION_WALL_LEFT;
        } else if (roll > 48.0f) {
            resolvedEmotion = EMOTION_WALL_RIGHT;
        } else if (!warmingUp && (currentCo >= 30.0f || currentPpm >= 1500.0f)) {
            resolvedEmotion = EMOTION_CRITICAL;
        } else if (!warmingUp && (deltaPpm > 45.0f || currentPpm > 1100.0f)) {
            resolvedEmotion = EMOTION_GAS_MASK;
        } else if (!warmingUp && currentTemp >= 41.5f && ((now / 3500) % 12 == 0)) {
            resolvedEmotion = EMOTION_HOT_PANTING;
        } else {
            // Carrusel Autónomo Tamagotchi: cada 8 segundos (4.5s OPTIMAL + 3.5s Animación Viva)
            uint32_t cyclePos = now % 8000;
            if (cyclePos >= 4500) {
                size_t idx = (now / 8000) % IDLE_CAROUSEL_COUNT;
                resolvedEmotion = IDLE_CAROUSEL[idx];
            } else {
                resolvedEmotion = EMOTION_OPTIMAL;
            }
        }

        sharedState.emotion = resolvedEmotion;
        sharedState.shakeDetected = activeShakeFlag;
        localState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        // 4. Renderizar y enviar por DMA a la GC9A01
        renderer.updateAndRender(localState);

        // Alimentar watchdog y evitar starvation de IDLE1 en Core 1
        TickType_t nowTicks = xTaskGetTickCount();
        if ((nowTicks - xLastWakeTime) < xFrequency) {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        } else {
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

        // 2. Actualizar variables ambientales compartidas (Core 1 resuelve la emoción a 60Hz)
        DisplaySharedState snapState;
        portENTER_CRITICAL(&stateMutex);
        sharedState.ppm = currentPpm;
        sharedState.co_ppm = currentCo;
        sharedState.temperature = currentTemp;
        sharedState.humidity = currentHum;
        snapState = sharedState;
        portEXIT_CRITICAL(&stateMutex);

        bool isShakingNow = snapState.shakeDetected || sensors.isShaking();

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
        provCtx.shakeEnergy = sensors.getShakeEnergy();
        provCtx.babyRockScore = sensors.getBabyRockScore();

        wifiProvisioner.processSerial(provCtx);
        if (wifiProvisioner.consumeCalibrateMpuFlag()) {
            sensors.calibrateRestingOffset();
        }
        String serialEmotion;
        if (wifiProvisioner.consumeEmotionCommand(serialEmotion)) {
            applyExternalEmotionCommand(serialEmotion.c_str());
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
                bool currentShaking = snapState.shakeDetected || sensors.isShaking();
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
                doc["shake_energy"] = round(sensors.getShakeEnergy() * 10) / 10.0;
                doc["baby_rock_score"] = round(sensors.getBabyRockScore() * 10) / 10.0;
                doc["free_heap"] = ESP.getFreeHeap();
                doc["min_free_heap"] = ESP.getMinFreeHeap();
                doc["uptime_s"] = now / 1000;
                doc["offline_queue"] = pendingCount;
                doc["offline_pending"] = pendingCount;

                String jsonPayload;
                serializeJson(doc, jsonPayload);

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
                        Serial.printf("[HTTP-LIVE] Muestra publicada a %s (Code %d | Emotion: %s)\n", ingestUrl.c_str(), code, emotionStr);
                    }
                    String respPayload = http.getString();
                    if (respPayload.length() > 0) {
                        JsonDocument respDoc;
                        DeserializationError err = deserializeJson(respDoc, respPayload);
                        if (!err) {
                            if (respDoc["command_view_mode"].is<const char*>()) {
                                const char* vm = respDoc["command_view_mode"];
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
                            if (respDoc["command_emotion"].is<const char*>()) {
                                const char* emo = respDoc["command_emotion"];
                                if (emo) {
                                    applyExternalEmotionCommand(emo);
                                }
                            }
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
            wasOffline = true;

            static uint32_t lastWifiRetryMs = 0;
            if (now - lastWifiRetryMs >= 25000) {
                lastWifiRetryMs = now;
                Serial.printf("[WIFI] Reintentando conexión suave a '%s'...\n", wifiProvisioner.getSSID().c_str());
                if (WiFi.status() != WL_CONNECTED) {
                    WiFi.begin(wifiProvisioner.getSSID().c_str(), wifiProvisioner.getPassword().c_str());
                }
            }

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
