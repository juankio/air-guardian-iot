#pragma once
#include <Arduino.h>
#include "lgfx_gc9a01.hpp"

enum PetEmotion {
    EMOTION_OPTIMAL,
    EMOTION_PREDICTIVE_WARNING,
    EMOTION_CRITICAL,
    EMOTION_RECOVERY,
    EMOTION_SLEEPING,
    EMOTION_DIZZY
};

enum DisplayViewMode {
    VIEW_PET_FACE,
    VIEW_CIRCULAR_HUD
};

struct DisplaySharedState {
    PetEmotion emotion;
    DisplayViewMode viewMode;
    float ppm;
    float co_ppm;
    float temperature;
    float humidity;
    float targetEyeX;
    float targetEyeY;
    float pitch;
    float roll;
    float ax;
    float ay;
    float az;
    bool shakeDetected;
};

class PetDisplayRenderer {
private:
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas;
    LovyanGFX* gfx = nullptr;
    bool useSprite = false;

    // Físicas de Resorte (Spring-Damper)
    float currentEyeX = 0;
    float currentEyeY = 0;
    float eyeVelX = 0;
    float eyeVelY = 0;
    const float stiffness = 0.14f;
    const float damping = 0.70f;

    // Parpadeo y Respiración
    float breathAngle = 0;
    float blinkProgress = 0;
    bool isBlinking = false;
    uint32_t nextBlinkMs = 3000;
    uint32_t lastFrameMs = 0;

    // Micro-sacadas
    float saccadeX = 0;
    float saccadeY = 0;
    uint32_t nextSaccadeMs = 2000;

    // Animación de Tos (Alerta Predictiva)
    float coughOffset = 0;
    uint32_t coughTimerMs = 0;

public:
    PetDisplayRenderer() : canvas(&lcd) {}

    void init() {
        Serial.println("[DISPLAY] Iniciando controlador GC9A01...");
        lcd.init();
        lcd.setRotation(0);
        lcd.setBrightness(255);

        // Prueba visual directa en el hardware (Verde Esmeralda Alquimia)
        lcd.startWrite();
        lcd.fillScreen(0x01E6); // Verde bosque profundo
        lcd.fillCircle(120, 120, 114, 0x02AC);
        lcd.drawCircle(120, 120, 115, 0x07E0);
        lcd.setTextDatum(lgfx::middle_center);
        lcd.setTextColor(0xFFFF, 0x02AC);
        lcd.setTextSize(2);
        lcd.drawString("DR. MASCOTA", 120, 105);
        lcd.setTextSize(1);
        lcd.setTextColor(0x7FFF, 0x02AC);
        lcd.drawString("AIR GUARDIAN 60FPS", 120, 135);
        lcd.endWrite();
        Serial.println("[DISPLAY] Pantalla de inicio proyectada con exito.");

        // Asignar Sprite en memoria interna (profundidad 8 bits = 57KB, cabe perfecto en RAM)
        canvas.setColorDepth(8);
        void* p = canvas.createSprite(240, 240);
        if (p != nullptr) {
            gfx = &canvas;
            useSprite = true;
            Serial.printf("[DISPLAY] Sprite 240x240 (8-bit, 57KB) asignado en %p\n", p);
        } else {
            // Si falta RAM contigua, dibuja directo en la pantalla por SPI DMA sin lag
            gfx = &lcd;
            useSprite = false;
            Serial.println("[DISPLAY] Modo Render Directo sobre LCD (Cero consumo de RAM).");
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

        gfx->startWrite();
        renderBackground(state.emotion);
        if (state.viewMode == VIEW_PET_FACE) {
            renderFace(state, now);
        } else {
            renderCircularHUD(state);
        }
        gfx->endWrite();

        if (useSprite) {
            canvas.pushSprite(0, 0);
        }
    }

private:
    void updatePhysics(const DisplaySharedState& state, float dt, uint32_t now) {
        float tx = state.targetEyeX + saccadeX;
        float ty = state.targetEyeY + saccadeY;

        float fx = -stiffness * (currentEyeX - tx);
        float fy = -stiffness * (currentEyeY - ty);

        eyeVelX = (eyeVelX + fx) * damping;
        eyeVelY = (eyeVelY + fy) * damping;

        currentEyeX += eyeVelX;
        currentEyeY += eyeVelY;

        breathAngle += dt * 2.5f;

        if (state.emotion == EMOTION_DIZZY) {
            isBlinking = false;
            blinkProgress = 0.0f;
        } else if (now >= nextBlinkMs && !isBlinking && state.emotion != EMOTION_SLEEPING) {
            isBlinking = true;
            blinkProgress = 0.0f;
        }

        if (isBlinking) {
            blinkProgress += dt * 8.0f;
            if (blinkProgress >= 1.0f) {
                isBlinking = false;
                blinkProgress = 0.0f;
                nextBlinkMs = now + 2500 + (esp_random() % 3500);
            }
        }

        if (now >= nextSaccadeMs) {
            nextSaccadeMs = now + 1500 + (esp_random() % 2500);
            if ((esp_random() % 10) > 4) {
                saccadeX = ((int)(esp_random() % 9) - 4) * 0.8f;
                saccadeY = ((int)(esp_random() % 7) - 3) * 0.8f;
            } else {
                saccadeX = 0;
                saccadeY = 0;
            }
        }

        if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
            coughTimerMs += (uint32_t)(dt * 1000.0f);
            if (coughTimerMs > 3500) {
                float phase = (coughTimerMs - 3500) / 400.0f;
                if (phase < 1.0f) {
                    coughOffset = sinf(phase * 3.14159f * 8.0f) * 3.5f;
                } else {
                    coughOffset = 0;
                    coughTimerMs = 0;
                }
            }
        } else {
            coughOffset = 0;
            coughTimerMs = 0;
        }
    }

    void renderBackground(PetEmotion emotion) {
        uint16_t bgColor = 0x01E6; // Verde oscuro de laboratorio

        switch (emotion) {
            case EMOTION_OPTIMAL:
                bgColor = gfx->color565(4, 30, 20); // Esmeralda suave
                break;
            case EMOTION_PREDICTIVE_WARNING:
                bgColor = gfx->color565(40, 24, 4); // Ámbar cálido
                break;
            case EMOTION_CRITICAL:
                bgColor = gfx->color565(48, 6, 14); // Rojo alerta
                break;
            case EMOTION_RECOVERY:
                bgColor = gfx->color565(6, 28, 42); // Cian ventilación
                break;
            case EMOTION_SLEEPING:
                bgColor = gfx->color565(10, 14, 20); // Azul noche reposo
                break;
            case EMOTION_DIZZY:
                bgColor = gfx->color565(26, 34, 10); // Tono oliva/verde mareado cómico
                break;
        }

        gfx->fillCircle(120, 120, 120, bgColor);
        gfx->drawCircle(120, 120, 118, gfx->color565(55, 65, 80));
    }

    void renderFace(const DisplaySharedState& state, uint32_t now) {
        float breathY = sinf(breathAngle) * 1.5f + coughOffset;
        int centerY = 120 + (int)breathY;

        uint16_t mainColor = gfx->color565(16, 215, 140); // Verde brillante
        uint16_t pupilColor = gfx->color565(2, 40, 28);

        if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
            mainColor = gfx->color565(251, 191, 36); // Ámbar oro
            pupilColor = gfx->color565(65, 25, 2);
        } else if (state.emotion == EMOTION_CRITICAL) {
            mainColor = gfx->color565(244, 63, 94);  // Carmesí
            pupilColor = gfx->color565(75, 4, 22);
        } else if (state.emotion == EMOTION_RECOVERY) {
            mainColor = gfx->color565(56, 189, 248); // Cian agua
            pupilColor = gfx->color565(8, 45, 70);
        } else if (state.emotion == EMOTION_SLEEPING) {
            mainColor = gfx->color565(148, 163, 184); // Gris
        } else if (state.emotion == EMOTION_DIZZY) {
            mainColor = gfx->color565(234, 179, 8); // Amarillo lima mareado cómico
            pupilColor = gfx->color565(45, 55, 10);
        }

        // Parpadeo elástico vertical
        float blinkH = 1.0f;
        if (state.emotion == EMOTION_DIZZY) {
            blinkH = 1.0f; // Mantener ojos abiertos desorbitados
        } else if (isBlinking) {
            float p = sinf(blinkProgress * 3.14159f);
            blinkH = max(0.08f, 1.0f - p * 0.92f);
        }

        // 1. Ojos Grandes Expresivos (Izquierdo: x=76, Derecho: x=164)
        int eyeXs[2] = { 76, 164 };
        for (int i = 0; i < 2; i++) {
            int ex = eyeXs[i];
            int ey = centerY - 10;

            if (state.emotion == EMOTION_SLEEPING) {
                gfx->drawArc(ex, ey + 4, 15, 18, 20, 160, mainColor);
            } else if (state.emotion == EMOTION_RECOVERY) {
                // Ojitos felices en semicírculo (^ ^)
                gfx->drawArc(ex, ey - 2, 16, 19, 200, 340, mainColor);
            } else if (state.emotion == EMOTION_CRITICAL) {
                // Ojos en X de peligro
                gfx->drawLine(ex - 14, ey - 14, ex + 14, ey + 14, mainColor);
                gfx->drawLine(ex + 14, ey - 14, ex - 14, ey + 14, mainColor);
                gfx->drawLine(ex - 13, ey - 14, ex + 15, ey + 14, mainColor);
                gfx->drawLine(ex + 15, ey - 14, ex - 13, ey + 14, mainColor);
            } else if (state.emotion == EMOTION_DIZZY) {
                // Ojos en espiral / giro orbital rápido y cejas arqueadas
                gfx->fillEllipse(ex, ey, 24, 32, mainColor);

                float dizzyAngle = now * 0.012f;
                int px = ex + (int)(cosf(dizzyAngle + i * 3.14159f) * 7.0f);
                int py = ey + (int)(sinf(dizzyAngle + i * 3.14159f) * 7.0f);

                gfx->fillCircle(px, py, 10, pupilColor);
                gfx->drawCircle(px, py, 6, 0xFFFF);
                gfx->drawCircle(px, py, 3, mainColor);

                // Cejas arqueadas cómicas
                int browY = ey - 22;
                gfx->drawArc(ex, browY, 13, 16, 200, 340, mainColor);
            } else {
                // Ojos Vivos con Esclerótica y Brillos Anime
                int currentH = (int)(32.0f * blinkH);
                if (currentH < 4) currentH = 4;
                gfx->fillEllipse(ex, ey, 24, currentH, mainColor);

                if (blinkH > 0.4f) {
                    // Pupila reactiva al sensor MPU
                    int px = ex + (int)currentEyeX;
                    int py = ey + (int)(currentEyeY * blinkH);
                    gfx->fillCircle(px, py, 11, pupilColor);

                    // Destellos de luz en los ojos (Catchlights)
                    gfx->fillCircle(px - 5, py - 5, 4, 0xFFFF);
                    gfx->fillCircle(px + 4, py + 4, 2, 0xFFFF);
                }

                // Cejas
                if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
                    int dir = (i == 0) ? 5 : -5;
                    gfx->drawLine(ex - 16, ey - 26 + dir, ex + 16, ey - 26 - dir, mainColor);
                    gfx->drawLine(ex - 16, ey - 25 + dir, ex + 16, ey - 25 - dir, mainColor);
                }
            }
        }

        // 2. Boca Expresiva
        int mouthY = centerY + 30;
        if (state.emotion == EMOTION_OPTIMAL) {
            // Sonrisa amplia
            gfx->drawArc(120, mouthY - 4, 10, 14, 20, 160, mainColor);
        } else if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
            // Boca ondulada de preocupación
            gfx->drawLine(110, mouthY, 120, mouthY - 3, mainColor);
            gfx->drawLine(120, mouthY - 3, 130, mouthY + 2, mainColor);
        } else if (state.emotion == EMOTION_CRITICAL) {
            // Boca abierta de asombro/gas
            gfx->fillEllipse(120, mouthY + 4, 8, 6, mainColor);
        } else if (state.emotion == EMOTION_RECOVERY) {
            gfx->drawArc(120, mouthY - 6, 12, 16, 20, 160, mainColor);
        } else if (state.emotion == EMOTION_DIZZY) {
            // Boca mareada ondulada / zig-zag cómico
            gfx->drawLine(108, mouthY + 1, 114, mouthY - 4, mainColor);
            gfx->drawLine(114, mouthY - 4, 120, mouthY + 4, mainColor);
            gfx->drawLine(120, mouthY + 4, 126, mouthY - 4, mainColor);
            gfx->drawLine(126, mouthY - 4, 132, mouthY + 1, mainColor);

            gfx->drawLine(108, mouthY + 2, 114, mouthY - 3, mainColor);
            gfx->drawLine(114, mouthY - 3, 120, mouthY + 5, mainColor);
            gfx->drawLine(120, mouthY + 5, 126, mouthY - 3, mainColor);
            gfx->drawLine(126, mouthY - 3, 132, mouthY + 2, mainColor);
        } else {
            gfx->fillCircle(120, mouthY + 2, 3, mainColor);
        }

        // 3. Rubor en las mejillas
        if (state.emotion == EMOTION_DIZZY) {
            // Mejillas amarillas/verdes mareadas
            uint16_t dizzyBlush = gfx->color565(195, 215, 35);
            gfx->fillEllipse(60, centerY + 10, 10, 6, dizzyBlush);
            gfx->fillEllipse(180, centerY + 10, 10, 6, dizzyBlush);
        } else if (state.emotion != EMOTION_SLEEPING && state.emotion != EMOTION_CRITICAL) {
            uint16_t blush = (state.emotion == EMOTION_OPTIMAL) ? gfx->color565(16, 120, 80) : gfx->color565(30, 110, 160);
            gfx->fillEllipse(60, centerY + 10, 9, 5, blush);
            gfx->fillEllipse(180, centerY + 10, 9, 5, blush);
        }
    }

    void renderCircularHUD(const DisplaySharedState& state) {
        float startAngle = 135.0f;
        float totalAngle = 270.0f;
        float pct = constrain((state.ppm - 380.0f) / (2000.0f - 380.0f), 0.0f, 1.0f);

        gfx->drawArc(120, 120, 92, 102, startAngle, startAngle + totalAngle, gfx->color565(30, 40, 55));

        uint16_t trackColor = gfx->color565(16, 215, 140);
        if (state.ppm > 800) trackColor = gfx->color565(251, 191, 36);
        if (state.ppm > 1400) trackColor = gfx->color565(244, 63, 94);

        gfx->drawArc(120, 120, 92, 102, startAngle, startAngle + (totalAngle * pct), trackColor);

        gfx->setTextDatum(textdatum_t::middle_center);
        gfx->setTextColor(0xFFFF);
        gfx->setTextSize(2);
        gfx->drawNumber((int)state.ppm, 120, 115);

        gfx->setTextSize(1);
        gfx->setTextColor(trackColor);
        gfx->drawString("PPM CO2eq", 120, 140);

        char envStr[32];
        snprintf(envStr, sizeof(envStr), "%.1f C | %d%% HR", state.temperature, (int)state.humidity);
        gfx->setTextColor(gfx->color565(160, 175, 195));
        gfx->drawString(envStr, 120, 175);
    }
};
