#pragma once
#include <Arduino.h>
#include <math.h>
#include "../lgfx_gc9a01.hpp"

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
    EMOTION_PREDICTIVE_WARNING,
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
    float pitch;
    float roll;
    float ax;
    float ay;
    float az;
    bool shakeDetected;
};

struct EmotionPalette {
    uint16_t bgColor;
    uint16_t mainColor;
    uint16_t pupilColor;
};

inline EmotionPalette getEmotionPalette(LovyanGFX* gfx, PetEmotion emo) {
    EmotionPalette p;
    p.bgColor = gfx->color565(4, 30, 20);
    p.mainColor = gfx->color565(16, 215, 140);
    p.pupilColor = gfx->color565(2, 40, 28);

    switch (emo) {
        case EMOTION_DIZZY:
            p.bgColor = gfx->color565(28, 36, 8);
            p.mainColor = gfx->color565(250, 204, 21);
            p.pupilColor = gfx->color565(45, 55, 10);
            break;
        case EMOTION_NAUSEA:
        case EMOTION_VOMIT:
            p.bgColor = (emo == EMOTION_NAUSEA) ? gfx->color565(12, 46, 12) : gfx->color565(18, 42, 6);
            p.mainColor = gfx->color565(163, 230, 53);
            p.pupilColor = gfx->color565(20, 50, 8);
            break;
        case EMOTION_WIPER:
        case EMOTION_RECOVERY:
            p.bgColor = gfx->color565(6, 28, 42);
            p.mainColor = gfx->color565(56, 189, 248);
            p.pupilColor = gfx->color565(8, 45, 70);
            break;
        case EMOTION_BABY_ROCKING:
            p.bgColor = gfx->color565(22, 16, 40);
            p.mainColor = gfx->color565(125, 211, 252);
            p.pupilColor = gfx->color565(12, 35, 64);
            break;
        case EMOTION_BABY_SLEEP:
        case EMOTION_SLEEPING:
        case EMOTION_YAWN:
            p.bgColor = gfx->color565(8, 12, 26);
            p.mainColor = gfx->color565(148, 163, 184);
            p.pupilColor = gfx->color565(15, 23, 42);
            break;
        case EMOTION_STARTLED:
        case EMOTION_FREEFALL:
            p.bgColor = gfx->color565(38, 10, 28);
            break;
        case EMOTION_UPSIDE_DOWN:
        case EMOTION_HOT_PANTING:
            p.bgColor = gfx->color565(44, 12, 10);
            p.mainColor = gfx->color565(251, 146, 60);
            p.pupilColor = gfx->color565(65, 15, 10);
            break;
        case EMOTION_LOVE:
            p.bgColor = gfx->color565(42, 8, 28);
            p.mainColor = gfx->color565(244, 114, 182);
            p.pupilColor = gfx->color565(80, 10, 40);
            break;
        case EMOTION_GAS_MASK:
        case EMOTION_PREDICTIVE_WARNING:
        case EMOTION_SUSPICIOUS:
        case EMOTION_DETECTIVE:
            p.bgColor = gfx->color565(36, 24, 6);
            p.mainColor = gfx->color565(251, 191, 36);
            p.pupilColor = gfx->color565(65, 25, 2);
            break;
        case EMOTION_CRITICAL:
            p.bgColor = gfx->color565(52, 6, 14);
            p.mainColor = gfx->color565(244, 63, 94);
            p.pupilColor = gfx->color565(75, 4, 22);
            break;
        case EMOTION_MUSIC_VIBE:
        case EMOTION_GLASSES_COOL:
        case EMOTION_BUTTERFLY:
            p.bgColor = gfx->color565(10, 20, 38);
            break;
        case EMOTION_ZEN:
            p.bgColor = gfx->color565(10, 20, 38);
            p.mainColor = gfx->color565(250, 204, 21);
            p.pupilColor = gfx->color565(50, 30, 5);
            break;
        case EMOTION_CRYING:
        case EMOTION_SNEEZE:
            p.bgColor = gfx->color565(8, 24, 38);
            p.mainColor = gfx->color565(56, 189, 248);
            p.pupilColor = gfx->color565(8, 45, 70);
            break;
        case EMOTION_COFFEE:
        case EMOTION_RAMEN:
            p.bgColor = gfx->color565(30, 18, 8);
            break;
        case EMOTION_LAUGHING:
            p.bgColor = gfx->color565(30, 18, 8);
            p.mainColor = gfx->color565(250, 204, 21);
            p.pupilColor = gfx->color565(50, 30, 5);
            break;
        default:
            break;
    }
    return p;
}

inline void computeFaceOffsets(PetEmotion emo, uint32_t now, int& faceOffsetX, int& faceOffsetY) {
    faceOffsetX = 0;
    faceOffsetY = 0;
    if (emo == EMOTION_WALL_LEFT) faceOffsetX = -24;
    else if (emo == EMOTION_WALL_RIGHT) faceOffsetX = 24;
    else if (emo == EMOTION_BABY_ROCKING) {
        faceOffsetX = (int)(sinf(now * 0.0045f) * 8.0f);
        faceOffsetY = (int)(cosf(now * 0.009f) * 2.0f);
    } else if (emo == EMOTION_LAUGHING) {
        faceOffsetY = (int)(sinf(now * 0.026f) * 5.0f);
    } else if (emo == EMOTION_MUSIC_VIBE) {
        faceOffsetY = (int)(fabsf(sinf(now * 0.012f)) * 6.0f) - 3;
    } else if (emo == EMOTION_SNEEZE) {
        uint32_t sneezePhase = now % 2500;
        faceOffsetY = (sneezePhase < 1600) ? (-8 + (int)(sinf(now * 0.03f) * 2.0f)) : 12;
    } else if (emo == EMOTION_NAUSEA) {
        faceOffsetX = (int)(sinf(now * 0.025f) * 2.5f);
    }
}
