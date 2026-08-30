#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>

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

    // Físicas MPU
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

public:
    SensorManager() : dht(PIN_DHT, DHT_TYPE) {}

    bool init() {
        Wire.begin(21, 22); // I2C SDA=21, SCL=22
        Wire.setClock(400000); // 400kHz fast I2C

        mpu.initialize();
        dht.begin();

        // Configuración de pines analógicos
        analogSetAttenuation(ADC_11db); // Rango 0 - 3.3V
        pinMode(PIN_MQ135, INPUT);
        pinMode(PIN_MQ7, INPUT);

        // Lectura inicial
        emaMq135 = analogRead(PIN_MQ135);
        emaMq7   = analogRead(PIN_MQ7);
        lastPpmSampleMs = millis();

        return mpu.testConnection();
    }

    void updateMPU(float& targetEyeX, float& targetEyeY, bool& shake) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Normalizar aceleración a rango [-1.0, 1.0] G
        float normAx = (float)ax / 16384.0f;
        float normAy = (float)ay / 16384.0f;
        float normAz = (float)az / 16384.0f;

        // Asignar inclinación de ojos (físicas de gravedad)
        targetEyeX = constrain(normAy * 12.0f, -10.0f, 10.0f);
        targetEyeY = constrain(-normAx * 10.0f, -8.0f, 8.0f);

        // Detección de Gesto Sacudir (Shake > 2.2 G o rotación angular brusca)
        float totalAccel = sqrtf(normAx * normAx + normAy * normAy + normAz * normAz);
        shake = (totalAccel > 2.3f || abs(gx) > 25000 || abs(gy) > 25000);
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

        // 3. Leer DHT11
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if (!isnan(t)) currentTemp = t;
        if (!isnan(h)) currentHumidity = h;

        // 4. Compensación ambiental y estimación de PPM
        // Factor de corrección típico para curvas MQ según Temp y HR
        float envFactor = 1.0f + ((currentTemp - 20.0f) * 0.01f) + ((currentHumidity - 55.0f) * 0.005f);
        if (envFactor < 0.5f) envFactor = 0.5f;

        // Curva logarítmica aproximada MQ-135 (PPM CO2eq)
        float voltage135 = (emaMq135 / 4095.0f) * 3.3f;
        float rs135 = ((3.3f - voltage135) / max(0.01f, voltage135)) * 10.0f; // R_L = 10k
        currentPpm = 400.0f + powf(10.0f, (voltage135 * 1.3f)) * envFactor;
        currentPpm = constrain(currentPpm, 380.0f, 3000.0f);

        // Curva MQ-7 (PPM CO)
        float voltage7 = (emaMq7 / 4095.0f) * 3.3f;
        currentCoPpm = constrain(voltage7 * 15.0f * envFactor, 0.0f, 200.0f);

        // 5. Cálculo de Derivada Temporal (Tasa de cambio)
        uint32_t now = millis();
        if (now - lastPpmSampleMs >= 5000) { // Cada 5 segundos
            float timeSec = (now - lastPpmSampleMs) / 1000.0f;
            deltaPpm = ((currentPpm - lastPpm) / timeSec) * 60.0f; // PPM por minuto
            lastPpm = currentPpm;
            lastPpmSampleMs = now;
        }
    }

    float getPPM() const { return currentPpm; }
    float getCoPPM() const { return currentCoPpm; }
    float getTemp() const { return currentTemp; }
    float getHumidity() const { return currentHumidity; }
    float getDeltaPPM() const { return deltaPpm; }
    float getRawAdc135() const { return emaMq135; }
};
