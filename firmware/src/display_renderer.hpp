#pragma once
#include <Arduino.h>
#include "display/display_types.hpp"
#include "display/display_overlays.hpp"
#include "display/display_props.hpp"
#include "display/display_eyes.hpp"
#include "display/display_mouths.hpp"

class PetDisplayRenderer {
private:
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas;
    LovyanGFX* gfx = nullptr;
    bool useSprite = false;

    float currentEyeX = 0, currentEyeY = 0;
    float eyeVelX = 0, eyeVelY = 0;
    const float stiffness = 0.14f;
    const float damping = 0.70f;

    float breathAngle = 0;
    float blinkProgress = 0;
    bool isBlinking = false;
    uint32_t nextBlinkMs = 2500;
    uint32_t lastFrameMs = 0;

    float saccadeX = 0, saccadeY = 0;
    uint32_t nextSaccadeMs = 1800;

    float coughOffset = 0;
    uint32_t coughTimerMs = 0;

public:
    PetDisplayRenderer() : canvas(&lcd) {}

    void init() {
        Serial.println("[DISPLAY] Iniciando controlador GC9A01 (Emo-Mecha v3.0 - 32 Animaciones)...");
        lcd.init();
        lcd.setRotation(0);
        lcd.setBrightness(255);

        lcd.startWrite();
        lcd.fillScreen(0x01E6);
        lcd.fillCircle(120, 120, 114, 0x02AC);
        lcd.drawCircle(120, 120, 115, 0x07E0);
        lcd.setTextDatum(lgfx::middle_center);
        lcd.setTextColor(0xFFFF, 0x02AC);
        lcd.setTextSize(2);
        lcd.drawString("DR. MASCOTA", 120, 105);
        lcd.setTextSize(1);
        lcd.setTextColor(0x7FFF, 0x02AC);
        lcd.drawString("EMO-MECHA v3.0 60FPS", 120, 135);
        lcd.endWrite();

        canvas.setColorDepth(8);
        void* p = canvas.createSprite(240, 240);
        if (p != nullptr) {
            gfx = &canvas;
            useSprite = true;
            Serial.printf("[DISPLAY] Sprite 240x240 (8-bit, 57KB) asignado en %p\n", p);
        } else {
            gfx = &lcd;
            useSprite = false;
            Serial.println("[DISPLAY] Modo Render Directo sobre LCD.");
        }
        lastFrameMs = millis();
    }

    void updateAndRender(const DisplaySharedState& state) {
        if (!gfx) gfx = &lcd;
        uint32_t now = millis();
        float dt = (now - lastFrameMs) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        lastFrameMs = now;

        updatePhysics(state, dt, now);

        EmotionPalette palette = getEmotionPalette(gfx, state.emotion);
        gfx->startWrite();
        gfx->fillCircle(120, 120, 120, palette.bgColor);
        gfx->drawCircle(120, 120, 118, gfx->color565(55, 70, 88));

        if (state.viewMode == VIEW_PET_FACE) {
            renderFace(state, palette, now);
        } else {
            renderCircularHUDGauge(gfx, state);
        }
        gfx->endWrite();

        if (useSprite) canvas.pushSprite(0, 0);
    }

private:
    void updatePhysics(const DisplaySharedState& state, float dt, uint32_t now) {
        float tx = state.targetEyeX + saccadeX;
        float ty = state.targetEyeY + saccadeY;

        if (state.emotion == EMOTION_BUTTERFLY) {
            float bt = now * 0.0032f;
            tx = sinf(bt) * 9.0f;
            ty = sinf(bt * 2.0f) * 5.0f - 2.0f;
        } else if (state.emotion == EMOTION_WHISTLE) {
            tx = 6.0f; ty = -5.0f;
        } else if (state.emotion == EMOTION_SUSPICIOUS) {
            tx = sinf(now * 0.004f) * 7.0f; ty = 0.0f;
        }

        eyeVelX = (eyeVelX - stiffness * (currentEyeX - tx)) * damping;
        eyeVelY = (eyeVelY - stiffness * (currentEyeY - ty)) * damping;
        currentEyeX += eyeVelX;
        currentEyeY += eyeVelY;
        breathAngle += dt * 2.6f;

        bool noBlinkState = (
            state.emotion == EMOTION_DIZZY || state.emotion == EMOTION_NAUSEA ||
            state.emotion == EMOTION_VOMIT || state.emotion == EMOTION_STARTLED ||
            state.emotion == EMOTION_BABY_SLEEP || state.emotion == EMOTION_SLEEPING ||
            state.emotion == EMOTION_ZEN || state.emotion == EMOTION_LAUGHING
        );

        if (noBlinkState) {
            isBlinking = false; blinkProgress = 0.0f;
        } else if (now >= nextBlinkMs && !isBlinking) {
            isBlinking = true; blinkProgress = 0.0f;
        }

        if (isBlinking) {
            blinkProgress += dt * 8.5f;
            if (blinkProgress >= 1.0f) {
                isBlinking = false; blinkProgress = 0.0f;
                nextBlinkMs = ((esp_random() % 10) < 3) ? (now + 130) : (now + 2400 + (esp_random() % 3200));
            }
        }

        if (now >= nextSaccadeMs) {
            nextSaccadeMs = now + 1400 + (esp_random() % 2200);
            if ((esp_random() % 10) > 4) {
                saccadeX = ((int)(esp_random() % 9) - 4) * 0.85f;
                saccadeY = ((int)(esp_random() % 7) - 3) * 0.85f;
            } else {
                saccadeX = 0; saccadeY = 0;
            }
        }

        if (state.emotion == EMOTION_GAS_MASK || state.emotion == EMOTION_PREDICTIVE_WARNING) {
            coughTimerMs += (uint32_t)(dt * 1000.0f);
            if (coughTimerMs > 3200) {
                float phase = (coughTimerMs - 3200) / 400.0f;
                if (phase < 1.0f) coughOffset = sinf(phase * 3.14159f * 8.0f) * 3.5f;
                else { coughOffset = 0; coughTimerMs = 0; }
            }
        } else {
            coughOffset = 0; coughTimerMs = 0;
        }
    }

    void renderFace(const DisplaySharedState& state, const EmotionPalette& palette, uint32_t now) {
        PetEmotion emo = state.emotion;
        int faceOffsetX = 0, faceOffsetY = 0;
        computeFaceOffsets(emo, now, faceOffsetX, faceOffsetY);

        int centerX = 120 + faceOffsetX;
        int centerY = 120 + (int)(sinf(breathAngle) * 1.6f + coughOffset) + faceOffsetY;

        drawTopFaceOverlays(gfx, emo, centerX, centerY, now);
        if (emo == EMOTION_MUSIC_VIBE) drawHeadphonesProp(gfx, centerX, centerY, now);

        float blinkH = isBlinking ? max(0.08f, 1.0f - sinf(blinkProgress * 3.14159f) * 0.92f) : 1.0f;
        int eyeXs[2] = { centerX - 44, centerX + 44 };
        renderEyesAndBrows(gfx, emo, eyeXs, centerY, breathAngle, blinkH, currentEyeX, currentEyeY, palette.mainColor, palette.pupilColor, now);
        drawGlassesBridgeAndLoupe(gfx, emo, centerX, centerY);
        renderMouthAndProps(gfx, emo, eyeXs, centerX, centerY, breathAngle, palette.mainColor, palette.pupilColor, now);
        renderCheekBlush(gfx, emo, centerX, centerY);
    }
};
