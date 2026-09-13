#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>
#include <math.h>

#define PIN_MQ135 34
#define PIN_MQ7   35
#define PIN_DHT   15
#define DHT_TYPE  DHT11

class SensorManager {
private:
    MPU6050 mpu;
    DHT dht;

    // Filtros de Media Móvil Exponencial (EMA)
    float emaMq135 = 0;
    float emaMq7   = 0;
    const float alpha = 0.15f; // Factor de suavizado

    // Variables de estado
    float currentPpm = 400.0f;
    float currentCoPpm = 0.0f;
    float currentTemp = 24.0f;
    float currentHumidity = 50.0f;

    // Derivada de PPM para predicción local
    float lastPpm = 400.0f;
    float deltaPpm = 0.0f;
    uint32_t lastPpmSampleMs = 0;
    bool hasMpu = false;

    // Control de tiempo de arranque (Warm-up)
    uint32_t bootTimeMs = 0;

    // Físicas MPU
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

public:
    SensorManager() : dht(PIN_DHT, DHT_TYPE), bootTimeMs(0) {}

    bool isWarmingUp() const {
        return (millis() - bootTimeMs) < 25000;
    }

    bool init() {
        bootTimeMs = millis();

        // Asegurar pull-ups internos para bus I2C
        pinMode(21, INPUT_PULLUP);
        pinMode(22, INPUT_PULLUP);
        delay(60);

        Wire.begin(21, 22); // I2C SDA=21, SCL=22
        Wire.setClock(100000); // 100kHz standard I2C para mayor estabilidad de cables
        Wire.setTimeOut(50); // Blindaje I2C: timeout de 50ms para evitar cuelgues del bus
        delay(50);

        // Escaneo I2C exhaustivo en el bus
        Serial.println("[I2C SCAN] Escaneando bus I2C (SDA=21, SCL=22)...");
        uint8_t mpuAddr = 0;
        int devicesFound = 0;
        for (uint8_t address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            uint8_t error = Wire.endTransmission();
            if (error == 0) {
                Serial.printf("[I2C SCAN] -> Dispositivo I2C detectado en direccion 0x%02X\n", address);
                devicesFound++;
                if (address == 0x68 || address == 0x69) {
                    mpuAddr = address;
                }
            }
        }

        // Si no se encontro en (21, 22), probar si los cables estan invertidos (SDA=22, SCL=21)
        if (devicesFound == 0) {
            Serial.println("[I2C SCAN] Ningun dispositivo en SDA=21, SCL=22. Probando inversion (SDA=22, SCL=21)...");
            Wire.end();
            pinMode(22, INPUT_PULLUP);
            pinMode(21, INPUT_PULLUP);
            delay(50);
            Wire.begin(22, 21);
            Wire.setClock(100000);
            Wire.setTimeOut(50);
            delay(50);
            for (uint8_t address = 1; address < 127; address++) {
                Wire.beginTransmission(address);
                uint8_t error = Wire.endTransmission();
                if (error == 0) {
                    Serial.printf("[I2C SCAN] -> [PINES INVERTIDOS] Dispositivo detectado en 0x%02X\n", address);
                    devicesFound++;
                    if (address == 0x68 || address == 0x69) {
                        mpuAddr = address;
                    }
                }
            }
            if (devicesFound == 0) {
                // Volver a la configuracion original 21, 22
                Wire.end();
                Wire.begin(21, 22);
            }
        }

        if (mpuAddr != 0) {
            Serial.printf("[MPU-6050] Vinculando sensor en direccion 0x%02X...\n", mpuAddr);

            // Despertar sensor sacándolo de sleep mode explícitamente por I2C
            Wire.beginTransmission(mpuAddr);
            Wire.write(0x6B); // PWR_MGMT_1
            Wire.write(0x01); // Clock source PLL con eje X de gyro (despierto)
            Wire.endTransmission();
            delay(15);

            // Leer registro WHO_AM_I (0x75)
            Wire.beginTransmission(mpuAddr);
            Wire.write(0x75);
            Wire.endTransmission(false);
            Wire.requestFrom(mpuAddr, (uint8_t)1);
            uint8_t whoAmI = Wire.available() ? Wire.read() : 0xFF;
            Serial.printf("[MPU-6050] WHO_AM_I leido = 0x%02X\n", whoAmI);

            mpu = MPU6050(mpuAddr, &Wire);
            mpu.initialize();

            // Aceptar si testConnection() es verdadero O si respondió con ID de silicio válido
            hasMpu = mpu.testConnection() || (whoAmI != 0x00 && whoAmI != 0xFF);
            if (hasMpu) {
                mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
                mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);
                Serial.printf("[MPU-6050] OK: Sensor inicializado en 0x%02X (WHO_AM_I: 0x%02X)\n", mpuAddr, whoAmI);
            } else {
                Serial.printf("[MPU-6050] WARN: Fallo testConnection() (WHO_AM_I: 0x%02X)\n", whoAmI);
            }
        } else {
            hasMpu = false;
            Serial.println("[MPU-6050] ERROR: No se encontro MPU6050 en 0x68 ni 0x69.");
        }

        pinMode(PIN_DHT, INPUT_PULLUP);
        dht.begin();

        // Configuración de pines analógicos
        analogSetAttenuation(ADC_11db); // Rango 0 - 3.3V
        pinMode(PIN_MQ135, INPUT);
        pinMode(PIN_MQ7, INPUT);

        // Lectura inicial
        emaMq135 = analogRead(PIN_MQ135);
        emaMq7   = analogRead(PIN_MQ7);
        lastPpmSampleMs = millis();

        return hasMpu;
    }

    void updateMPU(float& targetEyeX, float& targetEyeY, bool& shake) {
        if (!hasMpu) {
            targetEyeX = 0;
            targetEyeY = 0;
            shake = false;
            return;
        }

        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Normalizar aceleración a rango G con escala +/-4G (8192 LSB/G)
        float normAx = (float)ax / 8192.0f;
        float normAy = (float)ay / 8192.0f;
        float normAz = (float)az / 8192.0f;

        // Asignar inclinación de ojos (físicas de gravedad)
        targetEyeX = constrain(normAy * 12.0f, -10.0f, 10.0f);
        targetEyeY = constrain(-normAx * 10.0f, -8.0f, 8.0f);

        // Cálculo de shake sensible y multieje:
        // Calcular aceleración dinámica neta restando la gravedad (1.0G)
        float totalAccel = sqrtf(normAx * normAx + normAy * normAy + normAz * normAz);
        float dynamicAccel = fabsf(totalAccel - 1.0f);
        shake = (dynamicAccel > 0.65f || abs(gx) > 8000 || abs(gy) > 8000 || abs(gz) > 8000);
    }

    void updateGasAndEnv() {
        // 1. Leer ADC con sobremuestreo
        uint32_t sum135 = 0;
        uint32_t sum7 = 0;
        for (int i = 0; i < 16; i++) {
            sum135 += analogRead(PIN_MQ135);
            sum7   += analogRead(PIN_MQ7);
            delayMicroseconds(100);
        }
        float raw135 = sum135 / 16.0f;
        float raw7   = sum7 / 16.0f;

        // 2. Filtro EMA para eliminar picos de ruido
        emaMq135 = (alpha * raw135) + ((1.0f - alpha) * emaMq135);
        emaMq7   = (alpha * raw7)   + ((1.0f - alpha) * emaMq7);

        // 3. Leer DHT11 cada 2.5 segundos (evita bloquear interrupciones de WiFi en Core 0)
        static uint32_t lastDhtMs = 0;
        uint32_t nowDhtMs = millis();
        if (nowDhtMs - lastDhtMs >= 2500) {
            lastDhtMs = nowDhtMs;
            float t = dht.readTemperature();
            float h = dht.readHumidity();
            if (!isnan(t) && t > -20.0f && t < 80.0f) currentTemp = t;
            if (!isnan(h) && h >= 0.0f && h <= 100.0f) currentHumidity = h;
        }

        // 4. Compensación ambiental y estimación de PPM
        float envFactor = 1.0f + ((currentTemp - 20.0f) * 0.01f) + ((currentHumidity - 55.0f) * 0.005f);
        envFactor = constrain(envFactor, 0.5f, 1.8f);

        // Protección contra voltajes saturados/flotantes (evitar overflow y división por cero)
        float rawV135 = (emaMq135 / 4095.0f) * 3.3f;
        float voltage135 = constrain(rawV135, 0.05f, 3.15f);

        float rawV7 = (emaMq7 / 4095.0f) * 3.3f;
        float voltage7 = constrain(rawV7, 0.02f, 3.15f);

        // Curva robusta y acotada MQ-135 (PPM CO2eq)
        float computedPpm = 400.0f + powf(10.0f, (voltage135 * 1.25f)) * envFactor;
        computedPpm = constrain(computedPpm, 380.0f, 3000.0f);

        // Curva MQ-7 (PPM CO)
        float computedCo = constrain(voltage7 * 15.0f * envFactor, 0.0f, 200.0f);

        // Período de precalentamiento (warm-up): primeros 25s
        if (isWarmingUp()) {
            currentPpm = constrain(computedPpm, 380.0f, 450.0f);
            currentCoPpm = constrain(computedCo, 0.0f, 1.5f);
            deltaPpm = 0.0f;
        } else {
            currentPpm = computedPpm;
            currentCoPpm = computedCo;

            // 5. Cálculo de Derivada Temporal (Tasa de cambio)
            uint32_t now = millis();
            if (now - lastPpmSampleMs >= 5000) { // Cada 5 segundos
                float timeSec = (now - lastPpmSampleMs) / 1000.0f;
                deltaPpm = ((currentPpm - lastPpm) / timeSec) * 60.0f; // PPM por minuto
                lastPpm = currentPpm;
                lastPpmSampleMs = now;
            }
        }
    }

    float getPPM() const { return currentPpm; }
    float getCoPPM() const { return currentCoPpm; }
    float getTemp() const { return currentTemp; }
    float getHumidity() const { return currentHumidity; }
    float getDeltaPPM() const { return deltaPpm; }
    float getRawAdc135() const { return emaMq135; }
    float getRawAdc7() const { return emaMq7; }
    bool isMpuConnected() const { return hasMpu; }
    void getRawMPU(int16_t& _ax, int16_t& _ay, int16_t& _az, int16_t& _gx, int16_t& _gy, int16_t& _gz) {
        _ax = ax; _ay = ay; _az = az;
        _gx = gx; _gy = gy; _gz = gz;
    }
};
