#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "display_renderer.hpp"
#include "sensors.hpp"

// Configuración de Red WiFi y MQTT
const char* WIFI_SSID     = "TU_RED_WIFI";
const char* WIFI_PASSWORD = "TU_PASSWORD";
const char* MQTT_SERVER   = "192.168.1.100"; // IP del servidor Backend / Broker
const int   MQTT_PORT     = 1883;
const char* DEVICE_ID     = "air-guardian-01";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Instancias principales
PetDisplayRenderer renderer;
SensorManager sensors;

// Estado compartido protegido por Mutex de FreeRTOS
DisplaySharedState sharedState;
portMUX_TYPE stateMutex = portMUX_INITIALIZER_UNLOCKED;

// Prototipos de Tareas FreeRTOS
void TaskTelemetryCore0(void* parameter);
void TaskRenderCore1(void* parameter);

// Callback MQTT para recibir alertas predictivas desde el backend
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[256];
    if (length >= sizeof(message)) length = sizeof(message) - 1;
    memcpy(message, payload, length);
    message[length] = '\0';

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (!error) {
        const char* alert = doc["predicted_status"];
        if (alert) {
            portENTER_CRITICAL(&stateMutex);
            if (strcmp(alert, "WARNING_PREDICTIVE") == 0) {
                sharedState.emotion = EMOTION_PREDICTIVE_WARNING;
            } else if (strcmp(alert, "CRITICAL") == 0) {
                sharedState.emotion = EMOTION_CRITICAL;
            } else if (strcmp(alert, "OPTIMAL") == 0) {
                sharedState.emotion = EMOTION_OPTIMAL;
            } else if (strcmp(alert, "RECOVERY") == 0) {
                sharedState.emotion = EMOTION_RECOVERY;
            }
            portEXIT_CRITICAL(&stateMutex);
        }
    }
}

void setup() {
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
    renderer.init();
    if (!sensors.init()) {
        Serial.println("[WARN] MPU-6050 no detectado. Revisa conexiones I2C.");
    } else {
        Serial.println("[OK] MPU-6050 Conectado.");
    }

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

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// --------------------------------------------------------------------------
// CORE 0: TELEMETRÍA, GASES, RED Y DATA LAKE INGEST (Cero freeze en pantalla)
// --------------------------------------------------------------------------
void TaskTelemetryCore0(void* parameter) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    uint32_t lastPublishMs = 0;

    for (;;) {
        // 1. Muestreo y filtrado de sensores ambientales
        sensors.updateGasAndEnv();
        float currentPpm = sensors.getPPM();
        float currentCo = sensors.getCoPPM();
        float currentTemp = sensors.getTemp();
        float currentHum = sensors.getHumidity();
        float deltaPpm = sensors.getDeltaPPM();

        // 2. Actualizar variables compartidas para el render
        portENTER_CRITICAL(&stateMutex);
        sharedState.ppm = currentPpm;
        sharedState.co_ppm = currentCo;
        sharedState.temperature = currentTemp;
        sharedState.humidity = currentHum;

        // Lógica de alerta reactiva local de contingencia (si no hay WiFi)
        if (sharedState.emotion != EMOTION_SLEEPING) {
            if (currentCo > 30.0f || currentPpm > 1500.0f) {
                sharedState.emotion = EMOTION_CRITICAL;
            } else if (deltaPpm > 45.0f || currentPpm > 850.0f) {
                sharedState.emotion = EMOTION_PREDICTIVE_WARNING;
            } else if (currentPpm < 600.0f && sharedState.emotion == EMOTION_CRITICAL) {
                sharedState.emotion = EMOTION_RECOVERY;
            } else if (currentPpm < 700.0f) {
                sharedState.emotion = EMOTION_OPTIMAL;
            }
        }
        portEXIT_CRITICAL(&stateMutex);

        // 3. Mantener conexión WiFi y MQTT
        if (WiFi.status() == WL_CONNECTED) {
            if (!mqttClient.connected()) {
                if (mqttClient.connect(DEVICE_ID)) {
                    mqttClient.subscribe("airguardian/commands/#");
                    mqttClient.subscribe("airguardian/predictions/#");
                }
            }
            mqttClient.loop();

            // 4. Publicar Telemetría al Data Lake cada 2 segundos
            uint32_t now = millis();
            if (now - lastPublishMs >= 2000 && mqttClient.connected()) {
                lastPublishMs = now;

                StaticJsonDocument<256> doc;
                doc["device_id"] = DEVICE_ID;
                doc["timestamp"] = now;
                doc["ppm"] = round(currentPpm * 10) / 10.0;
                doc["co_ppm"] = round(currentCo * 10) / 10.0;
                doc["temperature"] = round(currentTemp * 10) / 10.0;
                doc["humidity"] = round(currentHum * 10) / 10.0;
                doc["delta_ppm"] = round(deltaPpm * 10) / 10.0;
                doc["raw_adc"] = (int)sensors.getRawAdc135();

                char buffer[256];
                serializeJson(doc, buffer);
                mqttClient.publish("airguardian/telemetry/raw", buffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
