#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

/**
 * @brief Gestor de aprovisionamiento WiFi y Servidor mediante Serial y NVS (Non-Volatile Storage).
 * Diseñado para operar en Core 0 de forma 100% no-bloqueante, protegiendo los 60 FPS del Core 1.
 */
class WiFiProvisioner {
public:
    struct TelemetryContext {
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
    };

private:
    Preferences prefs;
    const char* PREFS_NAMESPACE = "air-config";

    String ssid;
    String password;
    String server;

    // Buffer de recepción Serial no bloqueante
    static const size_t SERIAL_BUF_SIZE = 512;
    char serialBuffer[SERIAL_BUF_SIZE];
    size_t serialBufIndex = 0;

    // Estado de reconexión asíncrona
    bool isReconnecting = false;
    uint32_t reconnectStartMs = 0;
    const uint32_t RECONNECT_TIMEOUT_MS = 15000; // 15 segundos max para intentar handshake WiFi
    String pendingSsid;
    bool newConfigSaved = false;

public:
    WiFiProvisioner() 
        : serialBufIndex(0), 
          isReconnecting(false), 
          reconnectStartMs(0), 
          newConfigSaved(false) {}

    /**
     * @brief Inicializa NVS y carga credenciales guardadas o aplica defaults.
     */
    void init(const char* defaultSsid = "TU_RED_WIFI", 
              const char* defaultPass = "TU_PASSWORD", 
              const char* defaultServer = "192.168.1.100") {
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
            Serial.println("[PROVISIONER] [WARN] No se pudo abrir NVS. Utilizando credenciales por defecto.");
            ssid = defaultSsid;
            password = defaultPass;
            server = defaultServer;
            return;
        }

        ssid = prefs.getString("ssid", defaultSsid);
        password = prefs.getString("pass", defaultPass);
        server = prefs.getString("server", defaultServer);
        prefs.end();

        Serial.println("[PROVISIONER] [OK] Credenciales cargadas desde NVS ('air-config'):");
        Serial.printf("  - SSID: %s\n", ssid.c_str());
        Serial.printf("  - Pass: %s\n", password.length() > 0 ? "********" : "[VACIA]");
        Serial.printf("  - Server: %s\n", server.c_str());
    }

    const String& getSSID() const { return ssid; }
    const String& getPassword() const { return password; }
    const String& getServer() const { return server; }

    String getBackendUrl(uint16_t port = 3002) const {
        return String("http://") + server + ":" + String(port) + "/api/telemetry/batch";
    }

    /**
     * @brief Inicia la conexión inicial o tras boot en modo no-bloqueante
     */
    void startConnection() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(20);
        WiFi.begin(ssid.c_str(), password.c_str());
        isReconnecting = true;
        reconnectStartMs = millis();
        pendingSsid = ssid;
    }

    /**
     * @brief Guarda en NVS de forma persistente
     */
    bool saveCredentials(const String& newSsid, const String& newPass, const String& newServer) {
        if (!prefs.begin(PREFS_NAMESPACE, false)) {
            Serial.println("[PROVISIONER] [ERROR] Fallo al abrir NVS para escritura.");
            return false;
        }

        prefs.putString("ssid", newSsid);
        prefs.putString("pass", newPass);
        prefs.putString("server", newServer);
        prefs.end();

        ssid = newSsid;
        password = newPass;
        server = newServer;
        newConfigSaved = true;

        Serial.println("[PROVISIONER] [OK] Nuevas credenciales persistidas en NVS ('air-config').");
        return true;
    }

    /**
     * @brief Indica si se guardó una nueva configuración que requiera rearmar clientes (p. ej. MQTT)
     */
    bool consumeNewConfigFlag() {
        bool val = newConfigSaved;
        newConfigSaved = false;
        return val;
    }

    /**
     * @brief Revisa Serial.available() de forma completamente no-bloqueante
     */
    void processSerial(const TelemetryContext& ctx) {
        while (Serial.available() > 0) {
            char c = (char)Serial.read();
            if (c == '\n' || c == '\r') {
                if (serialBufIndex > 0) {
                    serialBuffer[serialBufIndex] = '\0';
                    handleJsonCommandLine(serialBuffer, ctx);
                    serialBufIndex = 0;
                }
            } else {
                if (serialBufIndex < SERIAL_BUF_SIZE - 1) {
                    serialBuffer[serialBufIndex++] = c;
                } else {
                    // Prevenir buffer overflow por saturación o ruido en la línea
                    serialBufIndex = 0;
                }
            }
        }
    }

    /**
     * @brief Monitorea el estado de la reconexión asíncrona e informa por Serial
     */
    void updateConnectionStatus() {
        if (!isReconnecting) return;

        if (WiFi.status() == WL_CONNECTED) {
            isReconnecting = false;
            JsonDocument res;
            res["status"] = "CONNECTED";
            res["ssid"] = pendingSsid;
            res["ip"] = WiFi.localIP().toString();
            res["rssi"] = WiFi.RSSI();

            serializeJson(res, Serial);
            Serial.println();
        } else if (millis() - reconnectStartMs >= RECONNECT_TIMEOUT_MS) {
            isReconnecting = false;
            JsonDocument res;
            res["status"] = "FAILED";
            res["ssid"] = pendingSsid;
            res["error"] = "TIMEOUT_CONNECTING";

            serializeJson(res, Serial);
            Serial.println();
        }
    }

private:
    void handleJsonCommandLine(const char* line, const TelemetryContext& ctx) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, line);
        if (err) {
            // Ignorar basura en la línea o líneas que no sean JSON
            return;
        }

        const char* cmd = doc["cmd"];
        if (!cmd) return;

        if (strcmp(cmd, "SET_WIFI") == 0) {
            handleSetWifiCommand(doc);
        } else if (strcmp(cmd, "GET_STATUS") == 0) {
            handleGetStatusCommand(ctx);
        } else if (strcmp(cmd, "SCAN_I2C") == 0) {
            handleScanI2CCommand();
        } else {
            JsonDocument errDoc;
            errDoc["status"] = "ERROR";
            errDoc["error"] = "UNKNOWN_COMMAND";
            errDoc["cmd"] = cmd;
            serializeJson(errDoc, Serial);
            Serial.println();
        }
    }

    void handleScanI2CCommand() {
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

    void handleSetWifiCommand(const JsonDocument& doc) {
        const char* newSsid = doc["ssid"] | "";
        const char* newPass = doc["pass"] | "";
        const char* newServer = doc["server"] | server.c_str();

        if (strlen(newSsid) == 0) {
            JsonDocument errDoc;
            errDoc["status"] = "ERROR";
            errDoc["error"] = "MISSING_SSID";
            serializeJson(errDoc, Serial);
            Serial.println();
            return;
        }

        // 1. Guardar en NVS
        saveCredentials(newSsid, newPass, newServer);

        // 2. Responder confirmación inmediata por Serial
        JsonDocument ack;
        ack["status"] = "CONNECTING";
        ack["ssid"] = newSsid;
        serializeJson(ack, Serial);
        Serial.println();

        // 3. Iniciar reconexión asíncrona de inmediato
        WiFi.disconnect();
        delay(20);
        WiFi.begin(newSsid, newPass);
        isReconnecting = true;
        reconnectStartMs = millis();
        pendingSsid = newSsid;
    }

    void handleGetStatusCommand(const TelemetryContext& ctx) {
        JsonDocument report;
        report["cmd"] = "STATUS";

        // Estado de red
        JsonObject net = report["network"].to<JsonObject>();
        bool connected = (WiFi.status() == WL_CONNECTED);
        net["connected"] = connected;
        net["ssid"] = connected ? WiFi.SSID() : ssid;
        net["ip"] = connected ? WiFi.localIP().toString() : "0.0.0.0";
        net["rssi"] = connected ? WiFi.RSSI() : 0;
        net["mac"] = WiFi.macAddress();
        net["server"] = server;
        net["mqtt_connected"] = ctx.mqttConnected;

        // Estado de hardware
        JsonObject hw = report["hardware"].to<JsonObject>();
        hw["free_heap"] = ESP.getFreeHeap();
        hw["min_free_heap"] = ESP.getMinFreeHeap();
        hw["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
        hw["uptime_s"] = millis() / 1000;
        hw["core"] = xPortGetCoreID();

        // Sensores
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
        sens["eye_x"] = round(ctx.eyeX * 10) / 10.0;
        sens["eye_y"] = round(ctx.eyeY * 10) / 10.0;

        // Almacenamiento LittleFS / Buffer Offline
        JsonObject storage = report["storage"].to<JsonObject>();
        storage["offline_pending"] = ctx.offlinePending;

        serializeJson(report, Serial);
        Serial.println();
    }
};
