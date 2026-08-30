#pragma once
#include <Arduino.h>
#include "lgfx_gc9a01.hpp"

enum PetEmotion {
    EMOTION_OPTIMAL,
    EMOTION_PREDICTIVE_WARNING,
    EMOTION_CRITICAL,
    EMOTION_RECOVERY,
    EMOTION_SLEEPING
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
    bool shakeDetected;
};

class PetDisplayRenderer {
private:
    LGFX_GC9A01 lcd;
    LGFX_Sprite canvas;

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
        lcd.init();
        lcd.setRotation(0);
        lcd.setBrightness(220);

        // Crear Sprite de 240x240 en memoria interna de 16 bits (RGB565)
        canvas.setColorDepth(16);
        canvas.createSprite(240, 240);
        lastFrameMs = millis();
    }

    void updateAndRender(const DisplaySharedState& state) {
        uint32_t now = millis();
        float dt = (now - lastFrameMs) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        lastFrameMs = now;

        updatePhysics(state, dt, now);

        // Limpiar Sprite
        canvas.startWrite();
        
        if (state.viewMode == VIEW_PET_FACE) {
            renderBackground(state.emotion);
            renderFace(state, now);
        } else {
            renderBackground(state.emotion);
            renderCircularHUD(state);
        }

        canvas.endWrite();
        // Vuelco por hardware DMA directo a la GC9A01
        canvas.pushSprite(0, 0);
    }

private:
    void updatePhysics(const DisplaySharedState& state, float dt, uint32_t now) {
        // Físicas del resorte para las pupilas
        float tx = state.targetEyeX + saccadeX;
        float ty = state.targetEyeY + saccadeY;

        float fx = -stiffness * (currentEyeX - tx);
        float fy = -stiffness * (currentEyeY - ty);

        eyeVelX = (eyeVelX + fx) * damping;
        eyeVelY = (eyeVelY + fy) * damping;

        currentEyeX += eyeVelX;
        currentEyeY += eyeVelY;

        // Respiración senoidal
        breathAngle += dt * 2.5f;

        // Temporizador de Parpadeo
        if (now >= nextBlinkMs && !isBlinking && state.emotion != EMOTION_SLEEPING) {
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

        // Micro-sacadas oculares
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

        // Animación de Tos periódica en estado PREDICTIVE_WARNING
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
        uint16_t bgColor = 0x0842; // Negro pizarra

        switch (emotion) {
            case EMOTION_OPTIMAL:
                bgColor = canvas.color565(4, 30, 20); // Verde oscuro
                break;
            case EMOTION_PREDICTIVE_WARNING:
                bgColor = canvas.color565(36, 22, 2); // Ámbar oscuro
                break;
            case EMOTION_CRITICAL:
                bgColor = canvas.color565(45, 6, 12); // Rojo borgoña
                break;
            case EMOTION_RECOVERY:
                bgColor = canvas.color565(4, 25, 40); // Cyan marino
                break;
            case EMOTION_SLEEPING:
                bgColor = canvas.color565(12, 16, 24); // Azul noche
                break;
        }

        canvas.fillCircle(120, 120, 120, bgColor);
        // Anillo perimetral sutil
        canvas.drawCircle(120, 120, 118, canvas.color565(50, 55, 65));
    }

    void renderFace(const DisplaySharedState& state, uint32_t now) {
        float breathY = sinf(breathAngle) * 1.5f + coughOffset;
        int centerY = 120 + (int)breathY;

        uint16_t mainColor = canvas.color565(16, 185, 129); // Verde
        uint16_t pupilColor = canvas.color565(2, 44, 34);

        if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
            mainColor = canvas.color565(245, 158, 11); // Ámbar
            pupilColor = canvas.color565(69, 26, 3);
        } else if (state.emotion == EMOTION_CRITICAL) {
            mainColor = canvas.color565(244, 63, 94);  // Rojo
            pupilColor = canvas.color565(76, 5, 25);
        } else if (state.emotion == EMOTION_RECOVERY) {
            mainColor = canvas.color565(56, 189, 248); // Cyan
            pupilColor = canvas.color565(8, 47, 73);
        } else if (state.emotion == EMOTION_SLEEPING) {
            mainColor = canvas.color565(148, 163, 184); // Gris
        }

        // Parpadeo vertical (Squash)
        float blinkH = 1.0f;
        if (isBlinking) {
            float p = sinf(blinkProgress * 3.14159f);
            blinkH = max(0.08f, 1.0f - p * 0.92f);
        }

        // Dibujar Ojos (Izquierdo: 78, Derecho: 162)
        int eyeXs[2] = { 78, 162 };
        for (int i = 0; i < 2; i++) {
            int ex = eyeXs[i];
            int ey = centerY - 12;

            if (state.emotion == EMOTION_SLEEPING) {
                // Arcos de sueño
                canvas.drawArc(ex, ey + 4, 14, 16, 20, 160, mainColor);
            } else if (state.emotion == EMOTION_RECOVERY) {
                // Medialuna feliz (^ ^)
                canvas.drawArc(ex, ey - 2, 14, 17, 200, 340, mainColor);
            } else if (state.emotion == EMOTION_CRITICAL) {
                // Ojos en X (Mareo/Peligro)
                canvas.drawLine(ex - 12, ey - 12, ex + 12, ey + 12, mainColor);
                canvas.drawLine(ex + 12, ey - 12, ex - 12, ey + 12, mainColor);
                canvas.drawLine(ex - 11, ey - 12, ex + 13, ey + 12, mainColor);
                canvas.drawLine(ex + 13, ey - 12, ex - 11, ey + 12, mainColor);
            } else {
                // Ojo Vivo con Anti-Aliasing
                int currentH = (int)(22 * blinkH);
                if (currentH < 2) currentH = 2;
                
                canvas.fillEllipse(ex, ey, 22, currentH, mainColor);

                if (blinkH > 0.4f) {
                    // Pupila Inercial
                    int px = ex + constrain((int)currentEyeX, -8, 8);
                    int py = ey + constrain((int)currentEyeY, -7, 7);
                    canvas.fillCircle(px, py, 10, pupilColor);

                    // Reflejos Especulares (Brillos)
                    canvas.fillCircle(px - 4, py - 4, 4, 0xFFFF);
                    canvas.fillCircle(px + 3, py + 3, 2, 0xFFFF);
                }

                // Ceja en alerta predictiva
                if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
                    int dir = (i == 0) ? 3 : -3;
                    canvas.drawLine(ex - 14, ey - 24 + dir, ex + 14, ey - 24 - dir, mainColor);
                    canvas.drawLine(ex - 14, ey - 23 + dir, ex + 14, ey - 23 - dir, mainColor);
                }
            }
        }

        // Dibujar Boca
        int mouthY = centerY + 18;
        if (state.emotion == EMOTION_OPTIMAL) {
            canvas.drawArc(120, mouthY - 4, 10, 13, 20, 160, mainColor);
        } else if (state.emotion == EMOTION_PREDICTIVE_WARNING) {
            canvas.drawLine(110, mouthY, 120, mouthY - 3, mainColor);
            canvas.drawLine(120, mouthY - 3, 130, mouthY + 2, mainColor);
        } else if (state.emotion == EMOTION_CRITICAL) {
            canvas.fillEllipse(120, mouthY + 4, 7, 5, mainColor);
        } else if (state.emotion == EMOTION_RECOVERY) {
            canvas.drawArc(120, mouthY - 8, 14, 17, 20, 160, mainColor);
        } else if (state.emotion == EMOTION_SLEEPING) {
            canvas.fillCircle(120, mouthY + 2, 3, mainColor);
            // ZZZ animados
            float zP = (now % 2000) / 2000.0f;
            canvas.setTextColor(canvas.color565(140, 150, 170));
            canvas.drawString("Z", 155 + (int)(zP * 15), 85 - (int)(zP * 25));
        }

        // Mejillas sonrosadas en estado óptimo o recuperación
        if (state.emotion == EMOTION_OPTIMAL || state.emotion == EMOTION_RECOVERY) {
            uint16_t blush = (state.emotion == EMOTION_OPTIMAL) ? canvas.color565(16, 120, 80) : canvas.color565(30, 110, 160);
            canvas.fillEllipse(62, centerY + 8, 8, 4, blush);
            canvas.fillEllipse(178, centerY + 8, 8, 4, blush);
        }
    }

    void renderCircularHUD(const DisplaySharedState& state) {
        // Tacómetro Circular
        float startAngle = 135.0f;
        float totalAngle = 270.0f;
        float pct = constrain((state.ppm - 350.0f) / 1650.0f, 0.0f, 1.0f);

        // Pista de fondo
        canvas.drawArc(120, 120, 92, 102, startAngle, startAngle + totalAngle, canvas.color565(30, 40, 55));

        // Color del arco según gas
        uint16_t trackColor = canvas.color565(16, 185, 129);
        if (state.ppm > 800) trackColor = canvas.color565(245, 158, 11);
        if (state.ppm > 1400) trackColor = canvas.color565(244, 63, 94);

        canvas.drawArc(120, 120, 92, 102, startAngle, startAngle + (totalAngle * pct), trackColor);

        // Lectura Digital Central
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(0xFFFF);
        canvas.setTextSize(2);
        canvas.drawNumber((int)state.ppm, 120, 115);

        canvas.setTextSize(1);
        canvas.setTextColor(trackColor);
        canvas.drawString("PPM CO2eq", 120, 140);

        // DHT11 Datos
        canvas.setTextColor(canvas.color565(160, 175, 195));
        char envStr[32];
        snprintf(envStr, sizeof(envStr), "%.1f C  |  %.0f%% HR", state.temperature, state.humidity);
        canvas.drawString(envStr, 120, 175);

        // Estado Predictivo Superior
        if (state.emotion == EMOTION_OPTIMAL) canvas.drawString("AIRE OPTIMO", 120, 65);
        else if (state.emotion == EMOTION_PREDICTIVE_WARNING) canvas.drawString("ALERTA ALZA", 120, 65);
        else if (state.emotion == EMOTION_CRITICAL) canvas.drawString("PELIGRO GAS", 120, 65);
        else if (state.emotion == EMOTION_RECOVERY) canvas.drawString("VENTILANDO", 120, 65);
    }
};
