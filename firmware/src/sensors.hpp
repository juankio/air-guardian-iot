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
    float currentVoltage135 = 0.1f;
    float currentVoltage7 = 0.1f;

    // Derivada de PPM para predicción local
    float lastPpm = 400.0f;
    float deltaPpm = 0.0f;
    uint32_t lastPpmSampleMs = 0;
    bool hasMpu = false;

    // Control de tiempo de arranque (Warm-up)
    uint32_t bootTimeMs = 0;

    // Físicas MPU
    int16_t ax = 0, ay = 0, az = 0;
    int16_t gx = 0, gy = 0, gz = 0;
    float smoothAx = 0.0f, smoothAy = 0.0f, smoothAz = 8192.0f;
    bool smoothInitialized = false;
    float offsetAx = 0.0f;
    float offsetAy = 0.0f;
    bool calibrated = false;
    float normAx = 0.0f, normAy = 0.0f, normAz = 1.0f;
    float pitchDeg = 0.0f, rollDeg = 0.0f;
    bool shakingState = false;
    uint8_t shakeHitCount = 0;

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
                calibrateRestingOffset();
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

    void calibrateRestingOffset() {
        if (!hasMpu) return;
        float sumAx = 0.0f;
        float sumAy = 0.0f;
        float sumAz = 0.0f;
        const int samples = 20;
        for (int i = 0; i < samples; i++) {
            int16_t rAx = 0, rAy = 0, rAz = 0, rGx = 0, rGy = 0, rGz = 0;
            mpu.getMotion6(&rAx, &rAy, &rAz, &rGx, &rGy, &rGz);
            sumAx += (float)rAx / 8192.0f;
            sumAy += (float)rAy / 8192.0f;
            sumAz += (float)rAz / 8192.0f;
            if (i == samples - 1) {
                smoothAx = (float)rAx;
                smoothAy = (float)rAy;
                smoothAz = (float)rAz;
                smoothInitialized = true;
            }
            delay(4);
        }
        float avgAx = sumAx / (float)samples;
        float avgAy = sumAy / (float)samples;
        float avgAz = sumAz / (float)samples;
        if (fabsf(avgAx) < 0.45f && fabsf(avgAy) < 0.45f && avgAz > 0.75f) {
            offsetAx = avgAx;
            offsetAy = avgAy;
            calibrated = true;
            Serial.printf("[MPU-6050] Auto-Tare OK: offsetAx=%.3fG, offsetAy=%.3fG (avgAz=%.3fG)\n", offsetAx, offsetAy, avgAz);
        } else {
            offsetAx = 0.23f;
            offsetAy = 0.13f;
            calibrated = true;
            Serial.printf("[MPU-6050] Auto-Tare Fallback: offsetAx=%.3fG, offsetAy=%.3fG\n", offsetAx, offsetAy);
        }
    }

    void updateMPU(float& targetEyeX, float& targetEyeY, bool& shake) {
        if (!hasMpu) {
            targetEyeX = 0;
            targetEyeY = 0;
            shake = false;
            normAx = 0.0f;
            normAy = 0.0f;
            normAz = 1.0f;
            pitchDeg = 0.0f;
            rollDeg = 0.0f;
            shakingState = false;
            return;
        }

        int16_t rawAx = 0, rawAy = 0, rawAz = 0;
        mpu.getMotion6(&rawAx, &rawAy, &rawAz, &gx, &gy, &gz);
        ax = rawAx;
        ay = rawAy;
        az = rawAz;

        if (!smoothInitialized) {
            smoothAx = (float)rawAx;
            smoothAy = (float)rawAy;
            smoothAz = (float)rawAz;
            smoothInitialized = true;
        } else {
            smoothAx = (0.40f * (float)rawAx) + (0.60f * smoothAx);
            smoothAy = (0.40f * (float)rawAy) + (0.60f * smoothAy);
            smoothAz = (0.40f * (float)rawAz) + (0.60f * smoothAz);
        }

        float rawNormAx = (float)rawAx / 8192.0f;
        float rawNormAy = (float)rawAy / 8192.0f;
        float rawNormAz = (float)rawAz / 8192.0f;

        normAx = smoothAx / 8192.0f;
        normAy = smoothAy / 8192.0f;
        normAz = smoothAz / 8192.0f;

        float calAx = normAx - offsetAx;
        float calAy = normAy - offsetAy;

        if (fabsf(calAx) < 0.04f) calAx = 0.0f;
        if (fabsf(calAy) < 0.04f) calAy = 0.0f;

        float planarMag = sqrtf(calAx * calAx + calAy * calAy);
        if (planarMag < 0.08f && normAz > 0.65f) {
            rollDeg = 0.0f;
            pitchDeg = 0.0f;
        } else {
            float upY = -calAx + (normAz * 0.34f);
            rollDeg = atan2f(calAy, upY) * 180.0f / PI;

            float rawPitch = atan2f(calAx, fmaxf(0.15f, normAz)) * 180.0f / PI * 0.45f;
            float pitchFade = constrain(1.0f - (fabsf(rollDeg) - 85.0f) / 50.0f, 0.0f, 1.0f);
            pitchDeg = constrain(rawPitch * pitchFade, -60.0f, 60.0f);
        }

        if (fabsf(rollDeg) < 2.2f) rollDeg = 0.0f;
        if (fabsf(pitchDeg) < 2.2f) pitchDeg = 0.0f;

        targetEyeX = constrain(rollDeg / 9.0f, -10.0f, 10.0f);
        targetEyeY = constrain(pitchDeg / 9.0f, -10.0f, 10.0f);

        normAx = calAx;
        normAy = calAy;

        float totalG = sqrtf(rawNormAx * rawNormAx + rawNormAy * rawNormAy + rawNormAz * rawNormAz);
        float dynamicAccel = fabsf(totalG - 1.0f);
        if (dynamicAccel > 1.15f) {
            if (shakeHitCount < 10) shakeHitCount++;
        } else if (shakeHitCount > 0) {
            shakeHitCount--;
        }
        shake = (shakeHitCount >= 4);
        shakingState = shake;
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
        currentVoltage135 = voltage135;

        float rawV7 = (emaMq7 / 4095.0f) * 3.3f;
        float voltage7 = constrain(rawV7, 0.02f, 3.15f);
        currentVoltage7 = voltage7;

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
    float getVoltage135() const { return currentVoltage135; }
    float getVoltage7() const { return currentVoltage7; }
    float getPitch() const { return pitchDeg; }
    float getRoll() const { return rollDeg; }
    float getNormAx() const { return normAx; }
    float getNormAy() const { return normAy; }
    float getNormAz() const { return normAz; }
    bool isShaking() const { return shakingState; }
    bool isMpuConnected() const { return hasMpu; }
    void getRawMPU(int16_t& _ax, int16_t& _ay, int16_t& _az, int16_t& _gx, int16_t& _gy, int16_t& _gz) {
        _ax = ax; _ay = ay; _az = az;
        _gx = gx; _gy = gy; _gz = gz;
    }
};
