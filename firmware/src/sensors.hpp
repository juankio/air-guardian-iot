#pragma once
#include <Arduino.h>
#include <DHT.h>
#include <math.h>
#include "sensors/mpu_motion_engine.hpp"

#define PIN_MQ135 34
#define PIN_MQ7   35
#define PIN_DHT   15
#define DHT_TYPE  DHT11

class SensorManager {
private:
    MpuMotionEngine mpuEngine;
    DHT dht;

    float emaMq135 = 0;
    float emaMq7   = 0;
    const float alpha = 0.15f;

    float currentPpm = 400.0f;
    float currentCoPpm = 0.0f;
    float currentTemp = 24.0f;
    float currentHumidity = 50.0f;
    float currentVoltage135 = 0.1f;
    float currentVoltage7 = 0.1f;

    float lastPpm = 400.0f;
    float deltaPpm = 0.0f;
    uint32_t lastPpmSampleMs = 0;
    uint32_t bootTimeMs = 0;

public:
    SensorManager() : dht(PIN_DHT, DHT_TYPE), bootTimeMs(0) {}

    bool isWarmingUp() const {
        return (millis() - bootTimeMs) < 25000;
    }

    bool init() {
        bootTimeMs = millis();
        bool hasMpu = mpuEngine.init();

        pinMode(PIN_DHT, INPUT_PULLUP);
        dht.begin();

        analogSetAttenuation(ADC_11db);
        pinMode(PIN_MQ135, INPUT);
        pinMode(PIN_MQ7, INPUT);

        emaMq135 = analogRead(PIN_MQ135);
        emaMq7   = analogRead(PIN_MQ7);
        lastPpmSampleMs = millis();

        return hasMpu;
    }

    void calibrateRestingOffset() {
        mpuEngine.calibrateRestingOffset();
    }

    void updateMPU(float& targetEyeX, float& targetEyeY, bool& shake) {
        mpuEngine.updateMPU(targetEyeX, targetEyeY, shake);
    }

    void updateGasAndEnv() {
        uint32_t sum135 = 0;
        uint32_t sum7 = 0;
        for (int i = 0; i < 16; i++) {
            sum135 += analogRead(PIN_MQ135);
            sum7   += analogRead(PIN_MQ7);
            delayMicroseconds(100);
        }
        float raw135 = sum135 / 16.0f;
        float raw7   = sum7 / 16.0f;

        emaMq135 = (alpha * raw135) + ((1.0f - alpha) * emaMq135);
        emaMq7   = (alpha * raw7)   + ((1.0f - alpha) * emaMq7);

        static uint32_t lastDhtMs = 0;
        uint32_t nowDhtMs = millis();
        if (nowDhtMs - lastDhtMs >= 2500) {
            lastDhtMs = nowDhtMs;
            float t = dht.readTemperature();
            float h = dht.readHumidity();
            if (!isnan(t) && t > -20.0f && t < 80.0f) currentTemp = t;
            if (!isnan(h) && h >= 0.0f && h <= 100.0f) currentHumidity = h;
        }

        float envFactor = 1.0f + ((currentTemp - 20.0f) * 0.01f) + ((currentHumidity - 55.0f) * 0.005f);
        envFactor = constrain(envFactor, 0.5f, 1.8f);

        float rawV135 = (emaMq135 / 4095.0f) * 3.3f;
        currentVoltage135 = constrain(rawV135, 0.05f, 3.15f);

        float rawV7 = (emaMq7 / 4095.0f) * 3.3f;
        currentVoltage7 = constrain(rawV7, 0.02f, 3.15f);

        float computedPpm = 400.0f + powf(10.0f, (currentVoltage135 * 1.25f)) * envFactor;
        computedPpm = constrain(computedPpm, 380.0f, 3000.0f);

        float computedCo = constrain(currentVoltage7 * 15.0f * envFactor, 0.0f, 200.0f);

        if (isWarmingUp()) {
            currentPpm = constrain(computedPpm, 380.0f, 450.0f);
            currentCoPpm = constrain(computedCo, 0.0f, 1.5f);
            deltaPpm = 0.0f;
        } else {
            currentPpm = computedPpm;
            currentCoPpm = computedCo;

            uint32_t now = millis();
            if (now - lastPpmSampleMs >= 5000) {
                float timeSec = (now - lastPpmSampleMs) / 1000.0f;
                deltaPpm = ((currentPpm - lastPpm) / timeSec) * 60.0f;
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
    float getPitch() const { return mpuEngine.getPitch(); }
    float getRoll() const { return mpuEngine.getRoll(); }
    float getNormAx() const { return mpuEngine.getNormAx(); }
    float getNormAy() const { return mpuEngine.getNormAy(); }
    float getNormAz() const { return mpuEngine.getNormAz(); }
    bool isShaking() const { return mpuEngine.isShaking(); }
    bool isMpuConnected() const { return mpuEngine.isMpuConnected(); }
    float getShakeEnergy() const { return mpuEngine.getShakeEnergy(); }
    void resetShakeEnergy() { mpuEngine.resetShakeEnergy(); }
    float getBabyRockScore() const { return mpuEngine.getBabyRockScore(); }
    void resetBabyRockScore() { mpuEngine.resetBabyRockScore(); }
    float getDynamicAccel() const { return mpuEngine.getDynamicAccel(); }
    float getJerk() const { return mpuEngine.getJerk(); }
    bool isFreefalling() const { return mpuEngine.isFreefalling(); }
    uint32_t getLastRockMs() const { return mpuEngine.getLastRockMs(); }
    void getRawMPU(int16_t& _ax, int16_t& _ay, int16_t& _az, int16_t& _gx, int16_t& _gy, int16_t& _gz) const {
        mpuEngine.getRawMPU(_ax, _ay, _az, _gx, _gy, _gz);
    }
};
