#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <math.h>

class MpuMotionEngine {
private:
    MPU6050 mpu;
    bool hasMpu = false;
    int16_t ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    float smoothAx = 0.0f, smoothAy = 0.0f, smoothAz = 8192.0f;
    bool smoothInitialized = false;
    float offsetAx = 0.23f, offsetAy = 0.13f, uprightOffsetAx = -0.04f;
    bool calibrated = false;
    float normAx = 0.0f, normAy = 0.0f, normAz = 1.0f;
    float pitchDeg = 0.0f, rollDeg = 0.0f;
    bool shakingState = false;
    float prevRawAx = 0.0f, prevRawAy = 0.0f, prevRawAz = 1.0f, prevRollDeg = 0.0f;
    bool prevRawInitialized = false;
    float currentDynamicAccel = 0.0f, currentJerk = 0.0f, shakeEnergy = 0.0f;
    uint32_t lastShakeInputMs = 0, lastRockMs = 0;
    float babyRockScore = 0.0f;
    bool freefallDetected = false;

    uint8_t scanBus(int sdaPin, int sclPin, int& devicesFound) {
        pinMode(sdaPin, INPUT_PULLUP);
        pinMode(sclPin, INPUT_PULLUP);
        delay(50);
        Wire.begin(sdaPin, sclPin);
        Wire.setClock(100000);
        Wire.setTimeOut(50);
        delay(50);
        uint8_t mpuAddr = 0;
        for (uint8_t address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            if (Wire.endTransmission() == 0) {
                Serial.printf("[I2C SCAN] -> Dispositivo I2C en 0x%02X (SDA=%d, SCL=%d)\n", address, sdaPin, sclPin);
                devicesFound++;
                if (address == 0x68 || address == 0x69) mpuAddr = address;
            }
        }
        return mpuAddr;
    }

public:
    bool init() {
        Serial.println("[I2C SCAN] Escaneando bus I2C (SDA=21, SCL=22)...");
        int devicesFound = 0;
        uint8_t mpuAddr = scanBus(21, 22, devicesFound);
        if (devicesFound == 0) {
            Serial.println("[I2C SCAN] Probando inversion (SDA=22, SCL=21)...");
            Wire.end();
            mpuAddr = scanBus(22, 21, devicesFound);
            if (devicesFound == 0) { Wire.end(); Wire.begin(21, 22); }
        }
        if (mpuAddr == 0) {
            hasMpu = false;
            Serial.println("[MPU-6050] ERROR: No se encontro MPU6050 en 0x68 ni 0x69.");
            return false;
        }
        Wire.beginTransmission(mpuAddr); Wire.write(0x6B); Wire.write(0x01); Wire.endTransmission();
        delay(15);
        Wire.beginTransmission(mpuAddr); Wire.write(0x75); Wire.endTransmission(false);
        Wire.requestFrom(mpuAddr, (uint8_t)1);
        uint8_t whoAmI = Wire.available() ? Wire.read() : 0xFF;

        mpu = MPU6050(mpuAddr, &Wire);
        mpu.initialize();
        hasMpu = mpu.testConnection() || (whoAmI != 0x00 && whoAmI != 0xFF);
        if (hasMpu) {
            mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
            mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);
            Serial.printf("[MPU-6050] OK en 0x%02X (WHO_AM_I: 0x%02X)\n", mpuAddr, whoAmI);
            calibrateRestingOffset();
        }
        return hasMpu;
    }

    void calibrateRestingOffset() {
        if (!hasMpu) return;
        float sumAx = 0.0f, sumAy = 0.0f, sumAz = 0.0f;
        const int samples = 20;
        for (int i = 0; i < samples; i++) {
            int16_t rAx = 0, rAy = 0, rAz = 0, rGx = 0, rGy = 0, rGz = 0;
            mpu.getMotion6(&rAx, &rAy, &rAz, &rGx, &rGy, &rGz);
            sumAx += (float)rAx / 8192.0f; sumAy += (float)rAy / 8192.0f; sumAz += (float)rAz / 8192.0f;
            if (i == samples - 1) { smoothAx = (float)rAx; smoothAy = (float)rAy; smoothAz = (float)rAz; smoothInitialized = true; }
            delay(4);
        }
        float avgAx = sumAx / samples, avgAy = sumAy / samples, avgAz = sumAz / samples;
        if (fabsf(avgAx) < 0.45f && fabsf(avgAy) < 0.45f && avgAz > 0.75f) { offsetAx = avgAx; offsetAy = avgAy; }
        else if (fabsf(avgAx) < 0.35f && avgAy > 0.60f) { uprightOffsetAx = avgAx; }
        calibrated = true;
    }

    void updateMPU(float& targetEyeX, float& targetEyeY, bool& shake) {
        if (!hasMpu) {
            targetEyeX = 0; targetEyeY = 0; shake = false;
            normAx = 0.0f; normAy = 0.0f; normAz = 1.0f; pitchDeg = 0.0f; rollDeg = 0.0f;
            shakingState = false; shakeEnergy = 0.0f; babyRockScore = 0.0f; freefallDetected = false;
            return;
        }
        int16_t rawAx = 0, rawAy = 0, rawAz = 0;
        mpu.getMotion6(&rawAx, &rawAy, &rawAz, &gx, &gy, &gz);
        ax = rawAx; ay = rawAy; az = rawAz;

        if (!smoothInitialized) {
            smoothAx = (float)rawAx; smoothAy = (float)rawAy; smoothAz = (float)rawAz; smoothInitialized = true;
        } else {
            smoothAx = (0.40f * (float)rawAx) + (0.60f * smoothAx);
            smoothAy = (0.40f * (float)rawAy) + (0.60f * smoothAy);
            smoothAz = (0.40f * (float)rawAz) + (0.60f * smoothAz);
        }

        float rawNormAx = (float)rawAx / 8192.0f, rawNormAy = (float)rawAy / 8192.0f, rawNormAz = (float)rawAz / 8192.0f;
        normAx = smoothAx / 8192.0f; normAy = smoothAy / 8192.0f; normAz = smoothAz / 8192.0f;

        float tableWeight = constrain((normAz - 0.60f) / 0.30f, 0.0f, 1.0f);
        float calAx = normAx - ((uprightOffsetAx * (1.0f - tableWeight)) + (offsetAx * tableWeight));
        float calAy = normAy - (offsetAy * tableWeight);
        if (fabsf(calAx) < 0.04f) calAx = 0.0f;
        if (fabsf(calAy) < 0.04f) calAy = 0.0f;

        float planarMag = sqrtf(calAx * calAx + calAy * calAy);
        if (planarMag < 0.08f && normAz > 0.65f) {
            rollDeg = 0.0f; pitchDeg = 0.0f;
        } else {
            rollDeg = atan2f(-calAx, calAy + (normAz * 0.35f)) * 180.0f / PI;
            float rawPitch = (normAz < -0.15f) ? (atan2f(-normAz - 0.15f, fmaxf(0.2f, planarMag)) * 180.0f / PI * 0.55f) : 0.0f;
            pitchDeg = constrain(rawPitch, -35.0f, 35.0f);
        }
        if (fabsf(rollDeg) < 2.5f) rollDeg = 0.0f;
        if (fabsf(pitchDeg) < 2.5f) pitchDeg = 0.0f;

        targetEyeX = constrain(rollDeg / 9.0f, -10.0f, 10.0f);
        targetEyeY = constrain(pitchDeg / 9.0f, -10.0f, 10.0f);
        normAx = calAx; normAy = calAy;

        float totalG = sqrtf(rawNormAx * rawNormAx + rawNormAy * rawNormAy + rawNormAz * rawNormAz);
        currentDynamicAccel = fabsf(totalG - 1.0f);
        if (!prevRawInitialized) {
            prevRawAx = rawNormAx; prevRawAy = rawNormAy; prevRawAz = rawNormAz; prevRollDeg = rollDeg;
            prevRawInitialized = true;
        }
        currentJerk = fabsf(rawNormAx - prevRawAx) + fabsf(rawNormAy - prevRawAy) + fabsf(rawNormAz - prevRawAz);
        prevRawAx = rawNormAx; prevRawAy = rawNormAy; prevRawAz = rawNormAz;
        freefallDetected = (totalG < 0.22f);

        uint32_t nowMs = millis();
        if (currentDynamicAccel > 0.28f || currentJerk > 0.42f) {
            shakeEnergy = fminf(100.0f, shakeEnergy + (currentDynamicAccel * 4.2f + currentJerk * 2.5f));
            lastShakeInputMs = nowMs;
            babyRockScore = fmaxf(0.0f, babyRockScore - 4.0f);
        } else if (nowMs - lastShakeInputMs > 1200) {
            shakeEnergy = fmaxf(0.0f, shakeEnergy - 0.45f);
        }

        float rollDelta = fabsf(rollDeg - prevRollDeg);
        prevRollDeg = rollDeg;
        float gyroRate = (fabsf((float)gx) + fabsf((float)gy) + fabsf((float)gz)) / 65.5f;
        bool gentleRock = (shakeEnergy < 10.0f) && (currentDynamicAccel < 0.26f) && (currentJerk < 0.36f) && (fabsf(rollDeg) < 62.0f) &&
                          ((rollDelta > 0.28f && rollDelta < 4.8f) || (gyroRate > 14.0f && gyroRate < 115.0f && currentDynamicAccel > 0.035f));
        if (gentleRock) { babyRockScore = fminf(100.0f, babyRockScore + 0.90f); lastRockMs = nowMs; }
        else if (nowMs - lastRockMs > 2500) { babyRockScore = fmaxf(0.0f, babyRockScore - 0.35f); }

        shake = (shakeEnergy >= 12.0f) || (currentDynamicAccel > 0.45f);
        shakingState = shake;
    }

    float getPitch() const { return pitchDeg; }
    float getRoll() const { return rollDeg; }
    float getNormAx() const { return normAx; }
    float getNormAy() const { return normAy; }
    float getNormAz() const { return normAz; }
    bool isShaking() const { return shakingState; }
    bool isMpuConnected() const { return hasMpu; }
    float getShakeEnergy() const { return shakeEnergy; }
    void resetShakeEnergy() { shakeEnergy = 0.0f; }
    float getBabyRockScore() const { return babyRockScore; }
    void resetBabyRockScore() { babyRockScore = 0.0f; }
    float getDynamicAccel() const { return currentDynamicAccel; }
    float getJerk() const { return currentJerk; }
    bool isFreefalling() const { return freefallDetected; }
    uint32_t getLastRockMs() const { return lastRockMs; }
    void getRawMPU(int16_t& _ax, int16_t& _ay, int16_t& _az, int16_t& _gx, int16_t& _gy, int16_t& _gz) const {
        _ax = ax; _ay = ay; _az = az; _gx = gx; _gy = gy; _gz = gz;
    }
};
