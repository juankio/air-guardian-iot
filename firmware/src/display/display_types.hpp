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

inline EmotionPalette getEmotionPalette(LovyanGFX* gfx, const DisplaySharedState& state) {
    uint8_t br = 4, bg = 30, bb = 20;
    uint8_t mr = 16, mg = 215, mb = 140;
    uint8_t pr = 2, pg = 40, pb = 28;

    if (state.ppm >= 1000.0f || state.co_ppm >= 35.0f || state.emotion == EMOTION_CRITICAL) {
        br = 48; bg = 6;  bb = 14;
        mr = 244; mg = 63; mb = 94;
        pr = 75; pg = 4;  pb = 22;
    } else if (state.ppm >= 700.0f || state.co_ppm >= 9.0f ||
               state.emotion == EMOTION_PREDICTIVE_WARNING || state.emotion == EMOTION_GAS_MASK) {
        br = 38; bg = 24; bb = 4;
        mr = 251; mg = 191; mb = 36;
        pr = 65; pg = 25; pb = 2;
    }

    if (state.emotion == EMOTION_BABY_SLEEP || state.emotion == EMOTION_SLEEPING) {
        br = (uint8_t)(br * 0.65f); bg = (uint8_t)(bg * 0.65f); bb = (uint8_t)(bb * 0.65f);
        mr = (uint8_t)(mr * 0.65f); mg = (uint8_t)(mg * 0.65f); mb = (uint8_t)(mb * 0.65f);
        pr = (uint8_t)(pr * 0.65f); pg = (uint8_t)(pg * 0.65f); pb = (uint8_t)(pb * 0.65f);
    }

    EmotionPalette p;
    p.bgColor = gfx->color565(br, bg, bb);
    p.mainColor = gfx->color565(mr, mg, mb);
    p.pupilColor = gfx->color565(pr, pg, pb);
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
