#pragma once
#include <Arduino.h>
#include <string.h>
#include <math.h>
#include "lgfx_gc9a01.hpp"

enum PetEmotion {
    EMOTION_OPTIMAL = 0,
    EMOTION_DIZZY,
    EMOTION_NAUSEA,
    EMOTION_VOMIT,
    EMOTION_WIPER,
    EMOTION_BABY_ROCKING,
    EMOTION_BABY_SLEEP,
    EMOTION_STARTLED,
    EMOTION_UPSIDE_DOWN,
    EMOTION_WALL_LEFT,
    EMOTION_WALL_RIGHT,
    EMOTION_FREEFALL,
    EMOTION_WINK,
    EMOTION_LOVE,
    EMOTION_BUBBLEGUM,
    EMOTION_WHISTLE,
    EMOTION_SNEEZE,
    EMOTION_YAWN,
    EMOTION_DERP,
    EMOTION_SUSPICIOUS,
    EMOTION_GLASSES_COOL,
    EMOTION_COFFEE,
    EMOTION_RAMEN,
    EMOTION_MUSIC_VIBE,
    EMOTION_BUTTERFLY,
    EMOTION_DETECTIVE,
    EMOTION_ZEN,
    EMOTION_CRYING,
    EMOTION_LAUGHING,
    EMOTION_HOT_PANTING,
    EMOTION_GAS_MASK,
    EMOTION_CRITICAL,
    // Legacy aliases
    EMOTION_PREDICTIVE_WARNING,
    EMOTION_RECOVERY,
    EMOTION_SLEEPING
};

inline const char* emotionToString(PetEmotion emotion) {
    switch (emotion) {
        case EMOTION_OPTIMAL:            return "OPTIMAL";
        case EMOTION_DIZZY:              return "DIZZY";
        case EMOTION_NAUSEA:             return "NAUSEA";
        case EMOTION_VOMIT:              return "VOMIT";
        case EMOTION_WIPER:              return "WIPER";
        case EMOTION_BABY_ROCKING:       return "BABY_ROCKING";
        case EMOTION_BABY_SLEEP:         return "BABY_SLEEP";
        case EMOTION_STARTLED:           return "STARTLED";
        case EMOTION_UPSIDE_DOWN:        return "UPSIDE_DOWN";
        case EMOTION_WALL_LEFT:          return "WALL_LEFT";
        case EMOTION_WALL_RIGHT:         return "WALL_RIGHT";
        case EMOTION_FREEFALL:           return "FREEFALL";
        case EMOTION_WINK:               return "WINK";
        case EMOTION_LOVE:               return "LOVE";
        case EMOTION_BUBBLEGUM:          return "BUBBLEGUM";
        case EMOTION_WHISTLE:            return "WHISTLE";
        case EMOTION_SNEEZE:             return "SNEEZE";
        case EMOTION_YAWN:               return "YAWN";
        case EMOTION_DERP:               return "DERP";
        case EMOTION_SUSPICIOUS:         return "SUSPICIOUS";
        case EMOTION_GLASSES_COOL:       return "GLASSES_COOL";
        case EMOTION_COFFEE:             return "COFFEE";
        case EMOTION_RAMEN:              return "RAMEN";
        case EMOTION_MUSIC_VIBE:         return "MUSIC_VIBE";
        case EMOTION_BUTTERFLY:          return "BUTTERFLY";
        case EMOTION_DETECTIVE:          return "DETECTIVE";
        case EMOTION_ZEN:                return "ZEN";
        case EMOTION_CRYING:             return "CRYING";
        case EMOTION_LAUGHING:           return "LAUGHING";
        case EMOTION_HOT_PANTING:        return "HOT_PANTING";
        case EMOTION_GAS_MASK:           return "GAS_MASK";
        case EMOTION_CRITICAL:           return "CRITICAL";
        case EMOTION_PREDICTIVE_WARNING: return "GAS_MASK";
        case EMOTION_RECOVERY:           return "RECOVERY";
        case EMOTION_SLEEPING:           return "BABY_SLEEP";
        default:                         return "OPTIMAL";
    }
}

inline bool stringToEmotion(const char* str, PetEmotion& out) {
    if (!str) return false;
    if (strcmp(str, "OPTIMAL") == 0)            { out = EMOTION_OPTIMAL; return true; }
    if (strcmp(str, "DIZZY") == 0)              { out = EMOTION_DIZZY; return true; }
    if (strcmp(str, "NAUSEA") == 0)             { out = EMOTION_NAUSEA; return true; }
    if (strcmp(str, "VOMIT") == 0)              { out = EMOTION_VOMIT; return true; }
    if (strcmp(str, "WIPER") == 0)              { out = EMOTION_WIPER; return true; }
    if (strcmp(str, "BABY_ROCKING") == 0)       { out = EMOTION_BABY_ROCKING; return true; }
    if (strcmp(str, "BABY_SLEEP") == 0)         { out = EMOTION_BABY_SLEEP; return true; }
    if (strcmp(str, "STARTLED") == 0)           { out = EMOTION_STARTLED; return true; }
    if (strcmp(str, "UPSIDE_DOWN") == 0)        { out = EMOTION_UPSIDE_DOWN; return true; }
    if (strcmp(str, "WALL_LEFT") == 0)          { out = EMOTION_WALL_LEFT; return true; }
    if (strcmp(str, "WALL_RIGHT") == 0)         { out = EMOTION_WALL_RIGHT; return true; }
    if (strcmp(str, "FREEFALL") == 0)           { out = EMOTION_FREEFALL; return true; }
    if (strcmp(str, "WINK") == 0)               { out = EMOTION_WINK; return true; }
    if (strcmp(str, "LOVE") == 0)               { out = EMOTION_LOVE; return true; }
    if (strcmp(str, "BUBBLEGUM") == 0)          { out = EMOTION_BUBBLEGUM; return true; }
    if (strcmp(str, "WHISTLE") == 0)            { out = EMOTION_WHISTLE; return true; }
    if (strcmp(str, "SNEEZE") == 0)             { out = EMOTION_SNEEZE; return true; }
    if (strcmp(str, "YAWN") == 0)               { out = EMOTION_YAWN; return true; }
    if (strcmp(str, "DERP") == 0)               { out = EMOTION_DERP; return true; }
    if (strcmp(str, "SUSPICIOUS") == 0)         { out = EMOTION_SUSPICIOUS; return true; }
    if (strcmp(str, "GLASSES_COOL") == 0)       { out = EMOTION_GLASSES_COOL; return true; }
    if (strcmp(str, "COFFEE") == 0)             { out = EMOTION_COFFEE; return true; }
    if (strcmp(str, "RAMEN") == 0)              { out = EMOTION_RAMEN; return true; }
    if (strcmp(str, "MUSIC_VIBE") == 0)         { out = EMOTION_MUSIC_VIBE; return true; }
    if (strcmp(str, "BUTTERFLY") == 0)          { out = EMOTION_BUTTERFLY; return true; }
    if (strcmp(str, "DETECTIVE") == 0)          { out = EMOTION_DETECTIVE; return true; }
    if (strcmp(str, "ZEN") == 0)                { out = EMOTION_ZEN; return true; }
    if (strcmp(str, "CRYING") == 0)             { out = EMOTION_CRYING; return true; }
    if (strcmp(str, "LAUGHING") == 0)           { out = EMOTION_LAUGHING; return true; }
    if (strcmp(str, "HOT_PANTING") == 0)        { out = EMOTION_HOT_PANTING; return true; }
    if (strcmp(str, "GAS_MASK") == 0)           { out = EMOTION_GAS_MASK; return true; }
    if (strcmp(str, "CRITICAL") == 0)           { out = EMOTION_CRITICAL; return true; }
    if (strcmp(str, "PREDICTIVE_WARNING") == 0 || strcmp(str, "WARNING_PREDICTIVE") == 0) {
        out = EMOTION_GAS_MASK;
        return true;
    }
    if (strcmp(str, "RECOVERY") == 0)           { out = EMOTION_RECOVERY; return true; }
    if (strcmp(str, "SLEEPING") == 0)           { out = EMOTION_BABY_SLEEP; return true; }
    return false;
}

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
    uint32_t nextBlinkMs = 2500;
    uint32_t lastFrameMs = 0;

    // Micro-sacadas
    float saccadeX = 0;
    float saccadeY = 0;
    uint32_t nextSaccadeMs = 1800;

    // Animación de Tos
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
    void drawHeart(int cx, int cy, int r, uint16_t color) {
        int lobeR = max(3, (r * 55) / 100);
        int offsetX = max(2, (r * 45) / 100);
        int lobeY = cy - (r / 4);
        gfx->fillCircle(cx - offsetX, lobeY, lobeR, color);
        gfx->fillCircle(cx + offsetX, lobeY, lobeR, color);
        gfx->fillTriangle(
            cx - offsetX - lobeR + 1, lobeY + (lobeR / 3),
            cx + offsetX + lobeR - 1, lobeY + (lobeR / 3),
            cx, cy + r,
            color
        );
    }

    void drawSparkle(int cx, int cy, int r, uint16_t color) {
        gfx->drawLine(cx - r, cy, cx + r, cy, color);
        gfx->drawLine(cx, cy - r, cx, cy + r, color);
        int d = max(1, r / 2);
        gfx->drawLine(cx - d, cy - d, cx + d, cy + d, color);
        gfx->drawLine(cx - d, cy + d, cx + d, cy - d, color);
        gfx->fillCircle(cx, cy, max(1, r / 3), 0xFFFF);
    }

    void drawMusicNote(int x, int y, uint16_t color) {
        gfx->fillCircle(x, y, 4, color);
        gfx->fillRect(x + 2, y - 12, 2, 12, color);
        gfx->fillRect(x + 2, y - 12, 6, 3, color);
    }

    void updatePhysics(const DisplaySharedState& state, float dt, uint32_t now) {
        float tx = state.targetEyeX + saccadeX;
        float ty = state.targetEyeY + saccadeY;

        if (state.emotion == EMOTION_BUTTERFLY) {
            float bt = now * 0.0032f;
            tx = sinf(bt) * 9.0f;
            ty = sinf(bt * 2.0f) * 5.0f - 2.0f;
        } else if (state.emotion == EMOTION_WHISTLE) {
            tx = 6.0f;
            ty = -5.0f;
        } else if (state.emotion == EMOTION_SUSPICIOUS) {
            tx = sinf(now * 0.004f) * 7.0f;
            ty = 0.0f;
        }

        float fx = -stiffness * (currentEyeX - tx);
        float fy = -stiffness * (currentEyeY - ty);

        eyeVelX = (eyeVelX + fx) * damping;
        eyeVelY = (eyeVelY + fy) * damping;

        currentEyeX += eyeVelX;
        currentEyeY += eyeVelY;

        breathAngle += dt * 2.6f;

        bool noBlinkState = (
            state.emotion == EMOTION_DIZZY ||
            state.emotion == EMOTION_NAUSEA ||
            state.emotion == EMOTION_VOMIT ||
            state.emotion == EMOTION_STARTLED ||
            state.emotion == EMOTION_BABY_SLEEP ||
            state.emotion == EMOTION_SLEEPING ||
            state.emotion == EMOTION_ZEN ||
            state.emotion == EMOTION_LAUGHING
        );

        if (noBlinkState) {
            isBlinking = false;
            blinkProgress = 0.0f;
        } else if (now >= nextBlinkMs && !isBlinking) {
            isBlinking = true;
            blinkProgress = 0.0f;
        }

        if (isBlinking) {
            blinkProgress += dt * 8.5f;
            if (blinkProgress >= 1.0f) {
                isBlinking = false;
                blinkProgress = 0.0f;
                // 30% probabilidad de doble parpadeo orgánico
                if ((esp_random() % 10) < 3) {
                    nextBlinkMs = now + 130;
                } else {
                    nextBlinkMs = now + 2400 + (esp_random() % 3200);
                }
            }
        }

        if (now >= nextSaccadeMs) {
            nextSaccadeMs = now + 1400 + (esp_random() % 2200);
            if ((esp_random() % 10) > 4) {
                saccadeX = ((int)(esp_random() % 9) - 4) * 0.85f;
                saccadeY = ((int)(esp_random() % 7) - 3) * 0.85f;
            } else {
                saccadeX = 0;
                saccadeY = 0;
            }
        }

        if (state.emotion == EMOTION_GAS_MASK || state.emotion == EMOTION_PREDICTIVE_WARNING) {
            coughTimerMs += (uint32_t)(dt * 1000.0f);
            if (coughTimerMs > 3200) {
                float phase = (coughTimerMs - 3200) / 400.0f;
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
        uint16_t bgColor = gfx->color565(4, 30, 20);

        switch (emotion) {
            case EMOTION_OPTIMAL:
            case EMOTION_WINK:
            case EMOTION_WHISTLE:
            case EMOTION_BUBBLEGUM:
            case EMOTION_DERP:
                bgColor = gfx->color565(4, 30, 20); // Esmeralda profundo
                break;
            case EMOTION_DIZZY:
                bgColor = gfx->color565(28, 36, 8); // Oliva mareo
                break;
            case EMOTION_NAUSEA:
                bgColor = gfx->color565(12, 46, 12); // Verde lima enfermo
                break;
            case EMOTION_VOMIT:
                bgColor = gfx->color565(18, 42, 6); // Verde tóxico vómito
                break;
            case EMOTION_WIPER:
            case EMOTION_RECOVERY:
                bgColor = gfx->color565(6, 28, 42); // Cian cristal limpio
                break;
            case EMOTION_BABY_ROCKING:
                bgColor = gfx->color565(22, 16, 40); // Violeta cuna
                break;
            case EMOTION_BABY_SLEEP:
            case EMOTION_SLEEPING:
            case EMOTION_YAWN:
                bgColor = gfx->color565(8, 12, 26); // Azul noche profundo
                break;
            case EMOTION_STARTLED:
            case EMOTION_FREEFALL:
                bgColor = gfx->color565(38, 10, 28); // Púrpura choque
                break;
            case EMOTION_UPSIDE_DOWN:
            case EMOTION_HOT_PANTING:
                bgColor = gfx->color565(44, 12, 10); // Carmesí calor/sangre en la cabeza
                break;
            case EMOTION_LOVE:
                bgColor = gfx->color565(42, 8, 28); // Magenta amor
                break;
            case EMOTION_GAS_MASK:
            case EMOTION_PREDICTIVE_WARNING:
            case EMOTION_SUSPICIOUS:
            case EMOTION_DETECTIVE:
                bgColor = gfx->color565(36, 24, 6); // Ámbar táctico
                break;
            case EMOTION_CRITICAL:
                bgColor = gfx->color565(52, 6, 14); // Rojo peligro crítico
                break;
            case EMOTION_MUSIC_VIBE:
            case EMOTION_GLASSES_COOL:
            case EMOTION_BUTTERFLY:
            case EMOTION_ZEN:
                bgColor = gfx->color565(10, 20, 38); // Azul neón cyber
                break;
            case EMOTION_CRYING:
            case EMOTION_SNEEZE:
                bgColor = gfx->color565(8, 24, 38); // Azul lluvia
                break;
            case EMOTION_COFFEE:
            case EMOTION_RAMEN:
            case EMOTION_LAUGHING:
                bgColor = gfx->color565(30, 18, 8); // Cálido acogedor
                break;
            default:
                bgColor = gfx->color565(4, 30, 20);
                break;
        }

        gfx->fillCircle(120, 120, 120, bgColor);
        gfx->drawCircle(120, 120, 118, gfx->color565(55, 70, 88));
    }

    void renderFace(const DisplaySharedState& state, uint32_t now) {
        PetEmotion emo = state.emotion;

        // Desplazamientos globales de rostro según física/emoción
        int faceOffsetX = 0;
        int faceOffsetY = 0;

        if (emo == EMOTION_WALL_LEFT) {
            faceOffsetX = -24;
        } else if (emo == EMOTION_WALL_RIGHT) {
            faceOffsetX = 24;
        } else if (emo == EMOTION_BABY_ROCKING) {
            faceOffsetX = (int)(sinf(now * 0.0045f) * 8.0f);
            faceOffsetY = (int)(cosf(now * 0.009f) * 2.0f);
        } else if (emo == EMOTION_LAUGHING) {
            faceOffsetY = (int)(sinf(now * 0.026f) * 5.0f);
        } else if (emo == EMOTION_MUSIC_VIBE) {
            faceOffsetY = (int)(fabsf(sinf(now * 0.012f)) * 6.0f) - 3;
        } else if (emo == EMOTION_SNEEZE) {
            uint32_t sneezePhase = now % 2500;
            if (sneezePhase < 1600) {
                faceOffsetY = -8 + (int)(sinf(now * 0.03f) * 2.0f);
            } else {
                faceOffsetY = 12;
            }
        } else if (emo == EMOTION_NAUSEA) {
            faceOffsetX = (int)(sinf(now * 0.025f) * 2.5f);
        }

        float breathY = sinf(breathAngle) * 1.6f + coughOffset;
        int centerX = 120 + faceOffsetX;
        int centerY = 120 + (int)breathY + faceOffsetY;

        // Paleta cromática principal según estado
        uint16_t mainColor = gfx->color565(16, 215, 140); // Esmeralda vibrante
        uint16_t pupilColor = gfx->color565(2, 40, 28);

        if (emo == EMOTION_GAS_MASK || emo == EMOTION_PREDICTIVE_WARNING || emo == EMOTION_SUSPICIOUS || emo == EMOTION_DETECTIVE) {
            mainColor = gfx->color565(251, 191, 36);
            pupilColor = gfx->color565(65, 25, 2);
        } else if (emo == EMOTION_CRITICAL) {
            mainColor = gfx->color565(244, 63, 94);
            pupilColor = gfx->color565(75, 4, 22);
        } else if (emo == EMOTION_RECOVERY || emo == EMOTION_WIPER || emo == EMOTION_CRYING || emo == EMOTION_SNEEZE) {
            mainColor = gfx->color565(56, 189, 248);
            pupilColor = gfx->color565(8, 45, 70);
        } else if (emo == EMOTION_BABY_SLEEP || emo == EMOTION_SLEEPING || emo == EMOTION_YAWN) {
            mainColor = gfx->color565(148, 163, 184);
            pupilColor = gfx->color565(15, 23, 42);
        } else if (emo == EMOTION_BABY_ROCKING) {
            mainColor = gfx->color565(125, 211, 252);
            pupilColor = gfx->color565(12, 35, 64);
        } else if (emo == EMOTION_DIZZY) {
            mainColor = gfx->color565(250, 204, 21);
            pupilColor = gfx->color565(45, 55, 10);
        } else if (emo == EMOTION_NAUSEA || emo == EMOTION_VOMIT) {
            mainColor = gfx->color565(163, 230, 53); // Verde ácido lima
            pupilColor = gfx->color565(20, 50, 8);
        } else if (emo == EMOTION_LOVE) {
            mainColor = gfx->color565(244, 114, 182);
            pupilColor = gfx->color565(80, 10, 40);
        } else if (emo == EMOTION_UPSIDE_DOWN || emo == EMOTION_HOT_PANTING) {
            mainColor = gfx->color565(251, 146, 60);
            pupilColor = gfx->color565(65, 15, 10);
        } else if (emo == EMOTION_ZEN || emo == EMOTION_LAUGHING) {
            mainColor = gfx->color565(250, 204, 21);
            pupilColor = gfx->color565(50, 30, 5);
        }

        // Efectos superiores de fondo de cara (Sangre subiendo a la cabeza / Calor / Halo Zen / Auriculares)
        if (emo == EMOTION_UPSIDE_DOWN) {
            // Rubor de sangre acumulada en la frente al estar de cabeza
            gfx->fillEllipse(120, 34, 72, 22, gfx->color565(160, 20, 45));
            gfx->fillEllipse(120, 24, 54, 14, gfx->color565(220, 38, 68));
            // Gotas de sudor cayendo hacia arriba (por gravedad invertida)
            int dropY = 58 - ((now / 35) % 35);
            gfx->fillCircle(68, dropY, 4, gfx->color565(56, 189, 248));
            gfx->fillCircle(172, dropY + 8, 4, gfx->color565(56, 189, 248));
        } else if (emo == EMOTION_HOT_PANTING) {
            gfx->fillEllipse(120, 36, 66, 18, gfx->color565(150, 30, 20));
        } else if (emo == EMOTION_ZEN) {
            // Halo dorado levitando
            int haloY = 44 + (int)(sinf(now * 0.005f) * 3.0f);
            gfx->drawEllipse(centerX, haloY, 38, 8, gfx->color565(250, 204, 21));
            gfx->drawEllipse(centerX, haloY, 37, 7, 0xFFFF);
            // Tercer ojo místico
            gfx->fillCircle(centerX, centerY - 38, 4, gfx->color565(56, 189, 248));
            gfx->fillCircle(centerX, centerY - 38, 2, 0xFFFF);
        } else if (emo == EMOTION_MUSIC_VIBE) {
            // Diadema y auriculares over-ear pulsando al ritmo
            int beat = (int)(fabsf(sinf(now * 0.012f)) * 4.0f);
            gfx->drawArc(centerX, centerY - 6, 86, 92, 200, 340, gfx->color565(56, 189, 248));
            gfx->fillRoundRect(centerX - 96 - beat, centerY - 26, 18, 44, 6, gfx->color565(236, 72, 153));
            gfx->fillRoundRect(centerX + 78 + beat, centerY - 26, 18, 44, 6, gfx->color565(236, 72, 153));
            gfx->drawRoundRect(centerX - 96 - beat, centerY - 26, 18, 44, 6, 0xFFFF);
            gfx->drawRoundRect(centerX + 78 + beat, centerY - 26, 18, 44, 6, 0xFFFF);
        } else if (emo == EMOTION_FREEFALL) {
            // Líneas de velocidad de caída libre en los bordes
            for (int k = 0; k < 4; k++) {
                int ly = ((now / 12 + k * 55) % 180) + 30;
                gfx->drawLine(28 + k * 6, ly, 28 + k * 6, ly + 26, 0xFFFF);
                gfx->drawLine(212 - k * 6, ly + 10, 212 - k * 6, ly + 36, 0xFFFF);
            }
        }

        // Squash & Stretch respiratorio y parpadeo
        float blinkH = 1.0f;
        if (isBlinking) {
            float p = sinf(blinkProgress * 3.14159f);
            blinkH = max(0.08f, 1.0f - p * 0.92f);
        }

        int baseRx = 24 + (int)(sinf(breathAngle) * 1.5f);
        int baseRy = 32 - (int)(sinf(breathAngle) * 1.5f);

        // ------------------------------------------------------------------
        // 1. RENDER DE OJOS Y CEJAS (Izquierdo: i=0, Derecho: i=1)
        // ------------------------------------------------------------------
        int eyeXs[2] = { centerX - 44, centerX + 44 };
        for (int i = 0; i < 2; i++) {
            int ex = eyeXs[i];
            int ey = centerY - 10;

            if (emo == EMOTION_BABY_SLEEP || emo == EMOTION_SLEEPING) {
                gfx->drawArc(ex, ey + 6, 15, 19, 20, 160, mainColor);
            } else if (emo == EMOTION_RECOVERY || emo == EMOTION_BABY_ROCKING || emo == EMOTION_ZEN) {
                gfx->drawArc(ex, ey - 2, 15, 19, 200, 340, mainColor);
            } else if (emo == EMOTION_LAUGHING || emo == EMOTION_VOMIT ||
                       (emo == EMOTION_SNEEZE && (now % 2500) >= 1600)) {
                // Ojos apretados > <
                if (i == 0) {
                    gfx->drawLine(ex - 14, ey - 10, ex + 10, ey, mainColor);
                    gfx->drawLine(ex - 14, ey - 9,  ex + 10, ey + 1, mainColor);
                    gfx->drawLine(ex - 14, ey + 10, ex + 10, ey, mainColor);
                    gfx->drawLine(ex - 14, ey + 9,  ex + 10, ey - 1, mainColor);
                } else {
                    gfx->drawLine(ex + 14, ey - 10, ex - 10, ey, mainColor);
                    gfx->drawLine(ex + 14, ey - 9,  ex - 10, ey + 1, mainColor);
                    gfx->drawLine(ex + 14, ey + 10, ex - 10, ey, mainColor);
                    gfx->drawLine(ex + 14, ey + 9,  ex - 10, ey - 1, mainColor);
                }
            } else if (emo == EMOTION_CRITICAL) {
                // Ojos en X de peligro
                for (int d = -1; d <= 1; d++) {
                    gfx->drawLine(ex - 14 + d, ey - 14, ex + 14 + d, ey + 14, mainColor);
                    gfx->drawLine(ex + 14 + d, ey - 14, ex - 14 + d, ey + 14, mainColor);
                }
            } else if (emo == EMOTION_LOVE) {
                int pulseR = 22 + (int)(sinf(now * 0.012f + i) * 3.0f);
                drawHeart(ex + (int)(currentEyeX * 0.5f), ey + (int)(currentEyeY * 0.5f), pulseR, gfx->color565(244, 63, 94));
                gfx->fillCircle(ex - 6, ey - 8, 3, 0xFFFF);
            } else if (emo == EMOTION_DIZZY) {
                gfx->fillEllipse(ex, ey, 24, 31, mainColor);
                float dizzyAngle = now * 0.015f * (i == 0 ? 1.0f : -1.0f);
                int px = ex + (int)(cosf(dizzyAngle) * 7.5f);
                int py = ey + (int)(sinf(dizzyAngle) * 7.5f);
                gfx->fillCircle(px, py, 11, pupilColor);
                gfx->drawCircle(px, py, 7, 0xFFFF);
                gfx->drawCircle(px, py, 3, mainColor);
                // Cejas asimétricas de mareo
                int browOff = (i == 0) ? -4 : 4;
                gfx->drawArc(ex, ey - 24 + browOff, 14, 17, 200, 340, mainColor);
            } else if (emo == EMOTION_NAUSEA) {
                // Ojos entrecerrados temblorosos aguantando las ganas de vomitar
                gfx->fillEllipse(ex, ey, 22, 18, mainColor);
                int jitterX = (int)(sinf(now * 0.04f + i) * 3.0f);
                gfx->fillCircle(ex + jitterX, ey + 2, 7, pupilColor);
                gfx->fillCircle(ex + jitterX - 2, ey, 2, 0xFFFF);
                // Ceja fruncida de sufrimiento
                int dir = (i == 0) ? -5 : 5;
                gfx->drawLine(ex - 16, ey - 20 - dir, ex + 16, ey - 20 + dir, mainColor);
                gfx->drawLine(ex - 16, ey - 19 - dir, ex + 16, ey - 19 + dir, mainColor);
            } else if (emo == EMOTION_STARTLED) {
                // Ojos blancos gigantes en shock con pupilas diminutas vibrando
                gfx->fillCircle(ex, ey, 27, 0xFFFF);
                gfx->drawCircle(ex, ey, 27, mainColor);
                int jx = ((now / 25 + i * 3) % 5) - 2;
                int jy = ((now / 20 + i * 7) % 5) - 2;
                gfx->fillCircle(ex + jx, ey + jy, 4, 0x0000);
                gfx->fillCircle(ex + jx - 1, ey + jy - 1, 1, 0xFFFF);
            } else if (emo == EMOTION_WINK && i == 0) {
                // Guiño en ojo izquierdo
                gfx->drawArc(ex, ey + 2, 15, 19, 200, 340, mainColor);
                drawSparkle(ex - 22, ey - 18, 6, gfx->color565(250, 204, 21));
            } else if (emo == EMOTION_SUSPICIOUS || (emo == EMOTION_SNEEZE && (now % 2500) < 1600)) {
                // Ojos entrecerrados (-_-)
                gfx->fillRoundRect(ex - 22, ey - 6, 44, 12, 4, mainColor);
                int px = ex + (int)currentEyeX;
                gfx->fillCircle(px, ey, 5, pupilColor);
                gfx->drawLine(ex - 22, ey - 12, ex + 22, ey - 12, mainColor);
            } else if (emo == EMOTION_GLASSES_COOL) {
                // Gafas de sol oscuras con reflejo diagonal animado
                gfx->fillRoundRect(ex - 28, ey - 18, 56, 34, 7, gfx->color565(15, 23, 42));
                gfx->drawRoundRect(ex - 28, ey - 18, 56, 34, 7, gfx->color565(56, 189, 248));
                gfx->drawRoundRect(ex - 27, ey - 17, 54, 32, 6, 0xFFFF);
                int shineX = ((now / 18) % 46) - 18;
                gfx->drawLine(ex + shineX - 8, ey + 10, ex + shineX + 6, ey - 12, 0xFFFF);
                gfx->drawLine(ex + shineX - 7, ey + 10, ex + shineX + 7, ey - 12, 0xFFFF);
            } else {
                // Ojo estándar expresivo con Squash & Stretch
                int rx = baseRx;
                int ry = (int)(baseRy * blinkH);
                if (ry < 4) ry = 4;

                // Aplastamiento contra el borde del cristal en WALL_LEFT / WALL_RIGHT
                if (emo == EMOTION_WALL_LEFT && i == 0) {
                    rx = 13;
                    ry = 36;
                    ex -= 4;
                    gfx->drawLine(ex - rx, ey - 28, ex - rx, ey + 28, 0xFFFF);
                } else if (emo == EMOTION_WALL_RIGHT && i == 1) {
                    rx = 13;
                    ry = 36;
                    ex += 4;
                    gfx->drawLine(ex + rx, ey - 28, ex + rx, ey + 28, 0xFFFF);
                } else if (emo == EMOTION_FREEFALL) {
                    rx = 19;
                    ry = 36;
                } else if (emo == EMOTION_YAWN || emo == EMOTION_HOT_PANTING || emo == EMOTION_CRYING) {
                    ry = max(8, (int)(18.0f * blinkH));
                }

                gfx->fillEllipse(ex, ey, rx, ry, mainColor);

                if (blinkH > 0.35f) {
                    int px = ex + (int)currentEyeX;
                    int py = ey + (int)(currentEyeY * blinkH);

                    if (emo == EMOTION_DERP) {
                        // Ojos bizcos divertidos
                        px = ex + (i == 0 ? 9 : -9);
                        py = ey + (i == 0 ? -3 : 3);
                    } else if (emo == EMOTION_UPSIDE_DOWN) {
                        py = ey - 9;
                    }

                    px = constrain(px, ex - max(2, rx - 9), ex + max(2, rx - 9));
                    py = constrain(py, ey - max(2, ry - 9), ey + max(2, ry - 9));

                    int pr = (emo == EMOTION_DETECTIVE && i == 1) ? 14 : 11;
                    gfx->fillCircle(px, py, pr, pupilColor);
                    gfx->fillCircle(px - 5, py - 5, 4, 0xFFFF);
                    gfx->fillCircle(px + 4, py + 4, 2, 0xFFFF);
                }

                // Cejas expresivas flotantes
                int browY = ey - ry - 7;
                if (emo == EMOTION_GAS_MASK || emo == EMOTION_PREDICTIVE_WARNING) {
                    int dir = (i == 0) ? 5 : -5;
                    gfx->drawLine(ex - 16, browY + dir, ex + 16, browY - dir, mainColor);
                    gfx->drawLine(ex - 16, browY + 1 + dir, ex + 16, browY + 1 - dir, mainColor);
                } else if (emo == EMOTION_CRYING) {
                    int dir = (i == 0) ? -6 : 6;
                    gfx->drawLine(ex - 16, browY - dir, ex + 16, browY + dir, mainColor);
                } else if (emo == EMOTION_DETECTIVE && i == 0) {
                    gfx->drawArc(ex, browY - 4, 14, 17, 200, 340, mainColor);
                } else {
                    gfx->drawArc(ex, browY + 4, 14, 16, 215, 325, mainColor);
                }
            }
        }

        // Puente de las gafas de sol
        if (emo == EMOTION_GLASSES_COOL) {
            gfx->drawLine(centerX - 16, centerY - 16, centerX + 16, centerY - 16, gfx->color565(56, 189, 248));
            gfx->drawLine(centerX - 16, centerY - 15, centerX + 16, centerY - 15, 0xFFFF);
        }

        // Lupa de Detective sobre el ojo derecho
        if (emo == EMOTION_DETECTIVE) {
            int lx = centerX + 44;
            int ly = centerY - 10;
            gfx->drawCircle(lx, ly, 30, gfx->color565(250, 204, 21));
            gfx->drawCircle(lx, ly, 31, 0xFFFF);
            gfx->drawLine(lx + 21, ly + 21, lx + 42, ly + 42, gfx->color565(250, 204, 21));
            gfx->drawLine(lx + 20, ly + 22, lx + 41, ly + 43, gfx->color565(250, 204, 21));
        }

        // ------------------------------------------------------------------
        // 2. BOCA Y ACCESORIOS CENTRALES
        // ------------------------------------------------------------------
        int mouthY = centerY + 32;

        if (emo == EMOTION_NAUSEA) {
            // Mejillas súper infladas aguantando el vómito + boca en ~ apretada
            int puff = (int)(sinf(now * 0.02f) * 2.5f);
            uint16_t cheekCol = gfx->color565(163, 230, 53);
            gfx->fillCircle(centerX - 26, mouthY + 1, 13 + puff, cheekCol);
            gfx->fillCircle(centerX + 26, mouthY + 1, 13 + puff, cheekCol);
            gfx->drawCircle(centerX - 26, mouthY + 1, 13 + puff, 0xFFFF);
            gfx->drawCircle(centerX + 26, mouthY + 1, 13 + puff, 0xFFFF);
            // Boca ondulada apretada
            gfx->drawLine(centerX - 14, mouthY + 1, centerX - 5, mouthY - 3, pupilColor);
            gfx->drawLine(centerX - 5, mouthY - 3, centerX + 5, mouthY + 4, pupilColor);
            gfx->drawLine(centerX + 5, mouthY + 4, centerX + 14, mouthY - 1, pupilColor);
            // Gota de sudor frío en la sien
            int dropOff = (now / 40) % 14;
            gfx->fillCircle(centerX + 68, centerY - 26 + dropOff, 4, gfx->color565(56, 189, 248));
        } else if (emo == EMOTION_VOMIT) {
            // ¡VOMITANDO EN LA PANTALLA!
            gfx->fillEllipse(centerX, mouthY + 2, 22, 16, gfx->color565(15, 35, 8));
            gfx->drawEllipse(centerX, mouthY + 2, 22, 16, mainColor);

            // Cascada de vómito verde/ácido hacia el borde inferior de la pantalla
            uint16_t vCol1 = gfx->color565(132, 245, 20);
            uint16_t vCol2 = gfx->color565(234, 250, 45);
            uint16_t vCol3 = gfx->color565(34, 197, 94);
            int waveShift = (now / 25) % 24;
            for (int y = mouthY + 4; y < 236; y += 6) {
                int w = 16 + ((y - mouthY) / 5);
                int wx = centerX + (int)(sinf((y + waveShift * 3) * 0.12f) * 5.0f);
                gfx->fillEllipse(wx, y, w, 6, ((y / 6) % 2 == 0) ? vCol1 : vCol3);
                gfx->fillCircle(wx + ((y % 3) - 1) * 6, y, 4, vCol2);
            }

            // Salpicaduras de vómito pegadas en el cristal de la pantalla
            gfx->fillCircle(62, 156, 15, vCol1);
            gfx->fillRoundRect(57, 156, 10, 26, 4, vCol1);
            gfx->fillCircle(176, 148, 17, vCol3);
            gfx->fillRoundRect(171, 148, 10, 30, 4, vCol3);
            gfx->fillCircle(84, 196, 12, vCol2);
            gfx->fillCircle(154, 192, 14, vCol1);
            gfx->fillCircle(48, 112, 9, vCol3);
        } else if (emo == EMOTION_WIPER) {
            // Boca de alivio mientras el limpiaparabrisas limpia el cristal
            gfx->drawArc(centerX, mouthY - 4, 10, 14, 20, 160, mainColor);

            // Ángulo del brazo limpiaparabrisas (-65° a +65°)
            float sweep = sinf(now * 0.006f);
            float angle = sweep * 1.12f; // rad
            int pivotX = 120;
            int pivotY = 236;
            int bladeLen = 198;
            int tipX = pivotX + (int)(sinf(angle) * bladeLen);
            int tipY = pivotY - (int)(cosf(angle) * bladeLen);

            // Manchas residuales que desaparecen al pasar la escobilla
            uint16_t slimeCol = gfx->color565(132, 245, 20);
            if (tipX < 62)  gfx->fillCircle(62, 156, 12, slimeCol);
            if (tipX < 154) gfx->fillCircle(154, 192, 11, slimeCol);
            if (tipX > 176) gfx->fillCircle(176, 148, 13, slimeCol);

            // Estrellitas de limpieza brillante detrás del limpiaparabrisas
            drawSparkle(72, 138, 7, 0xFFFF);
            drawSparkle(168, 132, 8, gfx->color565(56, 189, 248));
            drawSparkle(120, 86, 6, 0xFFFF);

            // Brazo mecánico y goma azul neón del limpiaparabrisas
            for (int d = -3; d <= 3; d++) {
                gfx->drawLine(pivotX + d, pivotY, tipX + d, tipY, (abs(d) <= 1) ? 0xFFFF : gfx->color565(56, 189, 248));
            }
            gfx->fillCircle(pivotX, pivotY, 12, gfx->color565(71, 85, 105));
            gfx->drawCircle(pivotX, pivotY, 12, 0xFFFF);
        } else if (emo == EMOTION_BABY_ROCKING) {
            // Chupete de bebé animado + notas de canción de cuna
            int suck = (int)(sinf(now * 0.012f) * 2.0f);
            gfx->fillEllipse(centerX, mouthY + 3, 15 + suck, 11, gfx->color565(244, 114, 182));
            gfx->drawEllipse(centerX, mouthY + 3, 15 + suck, 11, 0xFFFF);
            gfx->fillCircle(centerX, mouthY + 3, 6, gfx->color565(250, 204, 21));
            gfx->drawCircle(centerX, mouthY + 10 + suck, 7, 0xFFFF);

            // Notas musicales flotando
            int noteY = 95 - ((now / 45) % 45);
            drawMusicNote(38, noteY, gfx->color565(250, 204, 21));
            drawMusicNote(196, noteY + 15, gfx->color565(244, 114, 182));
        } else if (emo == EMOTION_BABY_SLEEP || emo == EMOTION_SLEEPING) {
            // Boquita dormida + BURBUJA DE MOCO AZUL inflándose y desinflándose
            gfx->fillCircle(centerX, mouthY + 4, 4, mainColor);

            int bubbleR = 11 + (int)(sinf(breathAngle * 1.15f) * 6.0f);
            int bx = centerX + 24;
            int by = centerY + 10;
            gfx->fillCircle(bx, by, bubbleR, gfx->color565(28, 120, 200));
            gfx->drawCircle(bx, by, bubbleR, gfx->color565(125, 211, 252));
            gfx->drawCircle(bx, by, max(1, bubbleR - 1), 0xFFFF);
            gfx->fillCircle(bx - bubbleR / 3, by - bubbleR / 3, max(2, bubbleR / 4), 0xFFFF);

            // Zzz flotando
            gfx->setTextDatum(lgfx::middle_center);
            gfx->setTextSize(1);
            gfx->setTextColor(gfx->color565(125, 211, 252));
            int zy = 75 - ((now / 50) % 30);
            gfx->drawString("z", centerX - 52, zy + 14);
            gfx->setTextSize(2);
            gfx->drawString("Z", centerX - 38, zy);
        } else if (emo == EMOTION_STARTLED) {
            // Boca abierta de susto + gotitas de la burbuja de moco recién explotada + !!
            gfx->fillEllipse(centerX, mouthY + 4, 11, 14, 0x0000);
            gfx->drawEllipse(centerX, mouthY + 4, 11, 14, 0xFFFF);

            // Gotitas de burbuja explotada (POP!)
            int bx = centerX + 24;
            int by = centerY + 10;
            for (int a = 0; a < 6; a++) {
                float ang = a * 1.047f;
                int dx = bx + (int)(cosf(ang) * 20.0f);
                int dy = by + (int)(sinf(ang) * 20.0f);
                gfx->fillCircle(dx, dy, 2, gfx->color565(125, 211, 252));
            }

            // Signos de exclamación !! arriba
            uint16_t alertCol = gfx->color565(244, 63, 94);
            gfx->fillRoundRect(centerX - 10, 34, 6, 18, 2, alertCol);
            gfx->fillCircle(centerX - 7, 58, 3, alertCol);
            gfx->fillRoundRect(centerX + 4, 34, 6, 18, 2, alertCol);
            gfx->fillCircle(centerX + 7, 58, 3, alertCol);
        } else if (emo == EMOTION_BUBBLEGUM) {
            // Globo de chicle rosa que se infla y explota
            uint32_t cycle = now % 3000;
            if (cycle < 2550) {
                int gumR = 5 + (int)((cycle / 2550.0f) * 23.0f);
                int gx = centerX;
                int gy = mouthY + 4;
                gfx->fillCircle(gx, gy, gumR, gfx->color565(244, 114, 182));
                gfx->drawCircle(gx, gy, gumR, gfx->color565(251, 207, 232));
                gfx->fillCircle(gx - gumR / 3, gy - gumR / 3, max(2, gumR / 4), 0xFFFF);
            } else {
                // POP!
                gfx->drawCircle(centerX, mouthY + 4, 28, gfx->color565(244, 114, 182));
                drawSparkle(centerX - 24, mouthY, 5, gfx->color565(244, 114, 182));
                drawSparkle(centerX + 24, mouthY, 5, gfx->color565(244, 114, 182));
            }
        } else if (emo == EMOTION_DERP) {
            // Sonrisa tonta + lengua rosa afuera
            gfx->drawArc(centerX, mouthY - 4, 10, 13, 20, 160, mainColor);
            gfx->fillRoundRect(centerX - 7, mouthY + 2, 14, 16, 6, gfx->color565(244, 114, 182));
            gfx->drawLine(centerX, mouthY + 2, centerX, mouthY + 13, gfx->color565(190, 24, 93));
        } else if (emo == EMOTION_HOT_PANTING) {
            // Jadeando de calor con lengua moviéndose rápido y gota de sudor
            int pantY = (int)(sinf(now * 0.025f) * 3.5f);
            gfx->fillEllipse(centerX, mouthY + 2, 14, 10, 0x0000);
            gfx->drawEllipse(centerX, mouthY + 2, 14, 10, mainColor);
            gfx->fillRoundRect(centerX - 8, mouthY + 2 + pantY, 16, 18, 7, gfx->color565(244, 114, 182));
            gfx->drawLine(centerX, mouthY + 3 + pantY, centerX, mouthY + 15 + pantY, gfx->color565(190, 24, 93));
            // Sudor
            int swY = ((now / 35) % 24);
            gfx->fillCircle(centerX + 66, centerY - 24 + swY, 5, gfx->color565(56, 189, 248));
        } else if (emo == EMOTION_COFFEE) {
            // Taza de café humeante
            int mugX = centerX - 18;
            int mugY = mouthY - 2;
            gfx->fillRoundRect(mugX, mugY, 36, 30, 5, gfx->color565(180, 83, 9));
            gfx->drawRoundRect(mugX, mugY, 36, 30, 5, 0xFFFF);
            gfx->drawCircle(mugX + 38, mugY + 15, 7, 0xFFFF);
            // Vapor animado
            int stOff = (now / 40) % 16;
            gfx->drawLine(centerX - 8, mugY - 4 - stOff, centerX - 5, mugY - 12 - stOff, 0xFFFF);
            gfx->drawLine(centerX + 8, mugY - 2 - stOff, centerX + 5, mugY - 10 - stOff, 0xFFFF);
        } else if (emo == EMOTION_RAMEN) {
            // Tazón de ramen + fideos subiendo a la boca
            int slurp = (now / 30) % 10;
            for (int n = -6; n <= 6; n += 6) {
                gfx->drawLine(centerX + n, mouthY - 2, centerX + n + ((slurp > 5) ? 2 : -2), mouthY + 18, gfx->color565(250, 204, 21));
            }
            gfx->fillArc(centerX, mouthY + 14, 0, 26, 0, 180, gfx->color565(220, 38, 38));
            gfx->drawArc(centerX, mouthY + 14, 24, 26, 0, 180, 0xFFFF);
        } else if (emo == EMOTION_MUSIC_VIBE) {
            // Sonrisa + barras de ecualizador neón en la parte inferior
            gfx->drawArc(centerX, mouthY - 6, 10, 14, 20, 160, mainColor);
            for (int b = -3; b <= 3; b++) {
                int bh = 8 + (int)(fabsf(sinf(now * 0.015f + b * 0.9f)) * 18.0f);
                uint16_t bCol = (b % 2 == 0) ? gfx->color565(56, 189, 248) : gfx->color565(236, 72, 153);
                gfx->fillRoundRect(centerX + b * 12 - 4, 198 - bh, 8, bh, 2, bCol);
            }
        } else if (emo == EMOTION_BUTTERFLY) {
            // Boca de asombro + Mariposa brillante volando en 8
            gfx->fillCircle(centerX, mouthY + 2, 5, mainColor);
            float bt = now * 0.0032f;
            int bx = 120 + (int)(sinf(bt) * 58.0f);
            int by = 82 + (int)(sinf(bt * 2.0f) * 24.0f);
            int wingW = 6 + (int)(fabsf(sinf(now * 0.03f)) * 7.0f);
            gfx->fillEllipse(bx - wingW / 2 - 2, by, wingW, 9, gfx->color565(244, 114, 182));
            gfx->fillEllipse(bx + wingW / 2 + 2, by, wingW, 9, gfx->color565(250, 204, 21));
            gfx->fillCircle(bx, by, 3, 0xFFFF);
        } else if (emo == EMOTION_CRYING) {
            // Cascadas de lágrimas azules + boca triste
            for (int i = 0; i < 2; i++) {
                int tx = eyeXs[i];
                int ty = centerY + 6;
                gfx->fillRoundRect(tx - 7, ty, 14, 72, 5, gfx->color565(56, 189, 248));
                int wave = (now / 30) % 18;
                gfx->fillCircle(tx, ty + 15 + wave, 5, 0xFFFF);
            }
            gfx->drawArc(centerX, mouthY + 12, 10, 14, 200, 340, mainColor);
        } else if (emo == EMOTION_LAUGHING) {
            // Carcajada abierta con dientes blancos y lengua
            gfx->fillArc(centerX, mouthY - 4, 0, 22, 0, 180, gfx->color565(80, 10, 25));
            gfx->fillRect(centerX - 18, mouthY - 4, 36, 6, 0xFFFF);
            gfx->fillArc(centerX, mouthY + 6, 0, 10, 0, 180, gfx->color565(244, 114, 182));
        } else if (emo == EMOTION_GAS_MASK || emo == EMOTION_PREDICTIVE_WARNING) {
            // Máscara antigás táctica con doble filtro lateral
            gfx->fillRoundRect(centerX - 26, mouthY - 12, 52, 36, 10, gfx->color565(51, 65, 85));
            gfx->drawRoundRect(centerX - 26, mouthY - 12, 52, 36, 10, mainColor);
            // Filtros laterales
            gfx->fillCircle(centerX - 34, mouthY + 8, 13, mainColor);
            gfx->fillCircle(centerX + 34, mouthY + 8, 13, mainColor);
            gfx->fillCircle(centerX - 34, mouthY + 8, 8, gfx->color565(30, 41, 59));
            gfx->fillCircle(centerX + 34, mouthY + 8, 8, gfx->color565(30, 41, 59));
            // Rejilla central
            gfx->fillCircle(centerX, mouthY + 6, 10, gfx->color565(15, 23, 42));
            gfx->drawCircle(centerX, mouthY + 6, 10, 0xFFFF);
        } else if (emo == EMOTION_WHISTLE) {
            // Boca silbando hacia un lado + notas musicales
            gfx->fillCircle(centerX + 12, mouthY + 2, 5, mainColor);
            gfx->fillCircle(centerX + 12, mouthY + 2, 2, 0x0000);
            int ny = mouthY - 6 - ((now / 40) % 28);
            drawMusicNote(centerX + 34, ny, mainColor);
        } else if (emo == EMOTION_SNEEZE) {
            uint32_t sneezePhase = now % 2500;
            if (sneezePhase < 1600) {
                gfx->drawLine(centerX - 8, mouthY + 2, centerX + 8, mouthY + 2, mainColor);
            } else {
                // ¡ACHOO! Spray de gotitas
                gfx->fillEllipse(centerX, mouthY + 2, 12, 10, mainColor);
                for (int k = -2; k <= 2; k++) {
                    gfx->fillCircle(centerX + k * 12, mouthY + 24 + abs(k) * 4, 3, 0xFFFF);
                }
            }
        } else if (emo == EMOTION_YAWN || emo == EMOTION_FREEFALL) {
            // Bostezo grande / Grito de caída libre
            int ry = (emo == EMOTION_FREEFALL) ? 20 : (12 + (int)(sinf(now * 0.004f) * 6.0f));
            gfx->fillEllipse(centerX, mouthY + 4, 15, max(6, ry), gfx->color565(20, 15, 35));
            gfx->drawEllipse(centerX, mouthY + 4, 15, max(6, ry), mainColor);
        } else if (emo == EMOTION_DIZZY) {
            // Boca en zig-zag + 3 estrellas orbitando en la frente
            for (int d = 0; d <= 1; d++) {
                gfx->drawLine(centerX - 16, mouthY + d,     centerX - 8,  mouthY - 5 + d, mainColor);
                gfx->drawLine(centerX - 8,  mouthY - 5 + d, centerX,      mouthY + 5 + d, mainColor);
                gfx->drawLine(centerX,      mouthY + 5 + d, centerX + 8,  mouthY - 5 + d, mainColor);
                gfx->drawLine(centerX + 8,  mouthY - 5 + d, centerX + 16, mouthY + d,     mainColor);
            }
            // 3 Estrellas girando alrededor de la frente
            for (int s = 0; s < 3; s++) {
                float sa = (now * 0.006f) + (s * 2.094f);
                int sx = centerX + (int)(cosf(sa) * 44.0f);
                int sy = centerY - 54 + (int)(sinf(sa) * 11.0f);
                drawSparkle(sx, sy, 6, gfx->color565(250, 204, 21));
            }
        } else if (emo == EMOTION_LOVE) {
            // Sonrisa tierna tipo gatito (:3 / w) + corazoncitos flotando
            gfx->drawArc(centerX - 7, mouthY - 2, 6, 9, 20, 160, mainColor);
            gfx->drawArc(centerX + 7, mouthY - 2, 6, 9, 20, 160, mainColor);
            int hy = 58 - ((now / 45) % 24);
            drawHeart(centerX, hy, 8, gfx->color565(244, 63, 94));
        } else if (emo == EMOTION_SUSPICIOUS || emo == EMOTION_DETECTIVE) {
            gfx->drawLine(centerX - 12, mouthY + 2, centerX + 12, mouthY + 2, mainColor);
            gfx->drawLine(centerX - 12, mouthY + 3, centerX + 12, mouthY + 3, mainColor);
        } else if (emo == EMOTION_CRITICAL) {
            gfx->fillEllipse(centerX, mouthY + 4, 12, 9, mainColor);
        } else {
            // Sonrisa amplia por defecto (OPTIMAL, WINK, GLASSES_COOL, WALL_LEFT/RIGHT, etc.)
            gfx->drawArc(centerX, mouthY - 4, 11, 15, 20, 160, mainColor);
        }

        // ------------------------------------------------------------------
        // 3. RUBOR EN LAS MEJILLAS
        // ------------------------------------------------------------------
        if (emo != EMOTION_NAUSEA && emo != EMOTION_VOMIT && emo != EMOTION_GAS_MASK &&
            emo != EMOTION_CRITICAL && emo != EMOTION_BABY_SLEEP && emo != EMOTION_SLEEPING) {
            uint16_t blush = gfx->color565(16, 120, 80);
            if (emo == EMOTION_LOVE || emo == EMOTION_BABY_ROCKING || emo == EMOTION_WINK || emo == EMOTION_LAUGHING) {
                blush = gfx->color565(236, 72, 153);
            } else if (emo == EMOTION_DIZZY) {
                blush = gfx->color565(195, 215, 35);
            }
            gfx->fillEllipse(centerX - 60, centerY + 12, 10, 6, blush);
            gfx->fillEllipse(centerX + 60, centerY + 12, 10, 6, blush);
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
