#pragma once
#include <Arduino.h>
#include <string.h>
#include "../display/display_types.hpp"
#include "../sensors.hpp"

struct EmotionNameEntry {
    const char* name;
    PetEmotion emotion;
};

static const EmotionNameEntry EMOTION_MAP[] = {
    {"OPTIMAL", EMOTION_OPTIMAL}, {"DIZZY", EMOTION_DIZZY}, {"NAUSEA", EMOTION_NAUSEA},
    {"VOMIT", EMOTION_VOMIT}, {"WIPER", EMOTION_WIPER}, {"BABY_ROCKING", EMOTION_BABY_ROCKING},
    {"BABY_SLEEP", EMOTION_BABY_SLEEP}, {"STARTLED", EMOTION_STARTLED}, {"UPSIDE_DOWN", EMOTION_UPSIDE_DOWN},
    {"WALL_LEFT", EMOTION_WALL_LEFT}, {"WALL_RIGHT", EMOTION_WALL_RIGHT}, {"FREEFALL", EMOTION_FREEFALL},
    {"WINK", EMOTION_WINK}, {"LOVE", EMOTION_LOVE}, {"BUBBLEGUM", EMOTION_BUBBLEGUM},
    {"WHISTLE", EMOTION_WHISTLE}, {"SNEEZE", EMOTION_SNEEZE}, {"YAWN", EMOTION_YAWN},
    {"DERP", EMOTION_DERP}, {"SUSPICIOUS", EMOTION_SUSPICIOUS}, {"GLASSES_COOL", EMOTION_GLASSES_COOL},
    {"COFFEE", EMOTION_COFFEE}, {"RAMEN", EMOTION_RAMEN}, {"MUSIC_VIBE", EMOTION_MUSIC_VIBE},
    {"BUTTERFLY", EMOTION_BUTTERFLY}, {"DETECTIVE", EMOTION_DETECTIVE}, {"ZEN", EMOTION_ZEN},
    {"CRYING", EMOTION_CRYING}, {"LAUGHING", EMOTION_LAUGHING}, {"HOT_PANTING", EMOTION_HOT_PANTING},
    {"GAS_MASK", EMOTION_GAS_MASK}, {"CRITICAL", EMOTION_CRITICAL}, {"RECOVERY", EMOTION_RECOVERY}
};

static const PetEmotion IDLE_CAROUSEL[] = {
    EMOTION_WINK, EMOTION_BUBBLEGUM, EMOTION_WHISTLE, EMOTION_LOVE,
    EMOTION_GLASSES_COOL, EMOTION_COFFEE, EMOTION_RAMEN, EMOTION_MUSIC_VIBE,
    EMOTION_BUTTERFLY, EMOTION_DETECTIVE, EMOTION_DERP, EMOTION_SUSPICIOUS,
    EMOTION_YAWN, EMOTION_SNEEZE, EMOTION_ZEN, EMOTION_LAUGHING, EMOTION_CRYING
};
static constexpr size_t IDLE_CAROUSEL_COUNT = sizeof(IDLE_CAROUSEL) / sizeof(IDLE_CAROUSEL[0]);

inline const char* emotionToString(PetEmotion emotion) {
    if (emotion == EMOTION_PREDICTIVE_WARNING) return "GAS_MASK";
    if (emotion == EMOTION_SLEEPING) return "BABY_SLEEP";
    for (size_t i = 0; i < sizeof(EMOTION_MAP) / sizeof(EMOTION_MAP[0]); i++) {
        if (EMOTION_MAP[i].emotion == emotion) return EMOTION_MAP[i].name;
    }
    return "OPTIMAL";
}

inline bool parseEmotionName(const char* str, PetEmotion& out) {
    if (!str) return false;
    if (strcmp(str, "PREDICTIVE_WARNING") == 0 || strcmp(str, "WARNING_PREDICTIVE") == 0) {
        out = EMOTION_GAS_MASK;
        return true;
    }
    if (strcmp(str, "SLEEPING") == 0) {
        out = EMOTION_BABY_SLEEP;
        return true;
    }
    for (size_t i = 0; i < sizeof(EMOTION_MAP) / sizeof(EMOTION_MAP[0]); i++) {
        if (strcmp(str, EMOTION_MAP[i].name) == 0) {
            out = EMOTION_MAP[i].emotion;
            return true;
        }
    }
    return false;
}

inline bool stringToEmotion(const char* str, PetEmotion& out) {
    return parseEmotionName(str, out);
}

struct EmotionArbiterState {
    volatile uint32_t manualEmotionUntilMs = 0;
    volatile PetEmotion manualEmotion = EMOTION_OPTIMAL;
    volatile uint32_t vomitSeqStartMs = 0;
    volatile uint32_t nauseaUntilMs = 0;
    volatile uint32_t dizzyUntilMs = 0;
    volatile uint32_t startledUntilMs = 0;
    volatile uint32_t babySleepUntilMs = 0;
    volatile uint32_t babyRockUntilMs = 0;
};

inline void applyExternalEmotion(
    EmotionArbiterState& arb,
    DisplaySharedState& sharedState,
    portMUX_TYPE& stateMutex,
    const char* emoName
) {
    PetEmotion parsed = EMOTION_OPTIMAL;
    if (parseEmotionName(emoName, parsed)) {
        uint32_t now = millis();
        portENTER_CRITICAL(&stateMutex);
        sharedState.viewMode = VIEW_PET_FACE;
        if (parsed == EMOTION_OPTIMAL) {
            arb.manualEmotionUntilMs = 0; arb.vomitSeqStartMs = 0;
            arb.nauseaUntilMs = 0; arb.dizzyUntilMs = 0; arb.startledUntilMs = 0;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
            sharedState.emotion = EMOTION_OPTIMAL;
        } else {
            arb.manualEmotion = parsed;
            arb.manualEmotionUntilMs = now + 8000;
            sharedState.emotion = parsed;
        }
        portEXIT_CRITICAL(&stateMutex);
    }
}

inline PetEmotion resolveActiveEmotion(
    EmotionArbiterState& arb,
    DisplaySharedState& sharedState,
    SensorManager& sensors,
    uint32_t now,
    bool rawShake,
    bool& outActiveShake
) {
    float shakeEnergy = sensors.getShakeEnergy();
    float babyRockScore = sensors.getBabyRockScore();
    float dynAccel = sensors.getDynamicAccel();
    float jerk = sensors.getJerk();
    float roll = sensors.getRoll();
    bool freefall = sensors.isFreefalling();

    if (arb.vomitSeqStartMs == 0) {
        if (shakeEnergy >= 62.0f) {
            arb.vomitSeqStartMs = now;
            arb.nauseaUntilMs = 0; arb.dizzyUntilMs = 0;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
            sensors.resetBabyRockScore();
        } else if (shakeEnergy >= 35.0f) {
            if (now + 4000 > arb.nauseaUntilMs) arb.nauseaUntilMs = now + 4000;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
        } else if (shakeEnergy >= 12.0f) {
            if (now + 4000 > arb.dizzyUntilMs) arb.dizzyUntilMs = now + 4000;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
        }
    }

    if (arb.babySleepUntilMs > now) {
        if (dynAccel > 0.35f || jerk > 0.45f || shakeEnergy >= 12.0f) {
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
            sensors.resetBabyRockScore();
            arb.startledUntilMs = now + 2500;
        }
    } else if (arb.vomitSeqStartMs == 0 && arb.nauseaUntilMs <= now && arb.dizzyUntilMs <= now && arb.startledUntilMs <= now) {
        if (babyRockScore >= 55.0f) arb.babySleepUntilMs = now + 20000;
        else if (babyRockScore >= 22.0f && (now + 3500 > arb.babyRockUntilMs)) arb.babyRockUntilMs = now + 3500;
    }

    outActiveShake = rawShake;
    if (arb.manualEmotionUntilMs > now) return arb.manualEmotion;
    if (arb.vomitSeqStartMs > 0) {
        uint32_t elapsed = now - arb.vomitSeqStartMs;
        outActiveShake = true;
        sharedState.viewMode = VIEW_PET_FACE;
        if (elapsed < 4000) return EMOTION_VOMIT;
        if (elapsed < 7500) return EMOTION_WIPER;
        if (elapsed < 9500) { sensors.resetShakeEnergy(); return EMOTION_RECOVERY; }
        arb.vomitSeqStartMs = 0; outActiveShake = false; return EMOTION_OPTIMAL;
    }
    if (arb.nauseaUntilMs > now) { outActiveShake = true; sharedState.viewMode = VIEW_PET_FACE; return EMOTION_NAUSEA; }
    if (arb.dizzyUntilMs > now) { outActiveShake = true; sharedState.viewMode = VIEW_PET_FACE; return EMOTION_DIZZY; }
    if (arb.startledUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_STARTLED; }
    if (arb.babySleepUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_BABY_SLEEP; }
    if (arb.babyRockUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_BABY_ROCKING; }
    if (freefall) return EMOTION_FREEFALL;
    if (fabsf(roll) > 135.0f) return EMOTION_UPSIDE_DOWN;
    if (roll < -48.0f) return EMOTION_WALL_LEFT;
    if (roll > 48.0f) return EMOTION_WALL_RIGHT;
    if (!sensors.isWarmingUp() && (sharedState.co_ppm >= 30.0f || sharedState.ppm >= 1500.0f)) return EMOTION_CRITICAL;
    if (!sensors.isWarmingUp() && (sensors.getDeltaPPM() > 45.0f || sharedState.ppm > 1100.0f)) return EMOTION_GAS_MASK;
    if (!sensors.isWarmingUp() && sharedState.temperature >= 41.5f && ((now / 3500) % 12 == 0)) return EMOTION_HOT_PANTING;

    return ((now % 8000) >= 4500) ? IDLE_CAROUSEL[(now / 8000) % IDLE_CAROUSEL_COUNT] : EMOTION_OPTIMAL;
}
