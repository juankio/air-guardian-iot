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
    {"GAS_MASK", EMOTION_GAS_MASK}, {"PREDICTIVE_WARNING", EMOTION_PREDICTIVE_WARNING},
    {"WARNING_PREDICTIVE", EMOTION_PREDICTIVE_WARNING}, {"CRITICAL", EMOTION_CRITICAL}, {"RECOVERY", EMOTION_RECOVERY}
};

static const PetEmotion IDLE_GREEN_GESTURES[] = {
    EMOTION_WINK, EMOTION_RECOVERY, EMOTION_SUSPICIOUS,
    EMOTION_WHISTLE, EMOTION_YAWN, EMOTION_LAUGHING
};
static constexpr size_t IDLE_GREEN_COUNT = sizeof(IDLE_GREEN_GESTURES) / sizeof(IDLE_GREEN_GESTURES[0]);

static const PetEmotion IDLE_WARN_GESTURES[] = {
    EMOTION_PREDICTIVE_WARNING, EMOTION_SUSPICIOUS, EMOTION_SNEEZE, EMOTION_CRYING
};
static constexpr size_t IDLE_WARN_COUNT = sizeof(IDLE_WARN_GESTURES) / sizeof(IDLE_WARN_GESTURES[0]);

static const PetEmotion IDLE_CRIT_GESTURES[] = {
    EMOTION_CRITICAL, EMOTION_HOT_PANTING, EMOTION_SNEEZE
};
static constexpr size_t IDLE_CRIT_COUNT = sizeof(IDLE_CRIT_GESTURES) / sizeof(IDLE_CRIT_GESTURES[0]);

inline const char* emotionToString(PetEmotion emotion) {
    if (emotion == EMOTION_PREDICTIVE_WARNING) return "WARNING_PREDICTIVE";
    if (emotion == EMOTION_SLEEPING) return "BABY_SLEEP";
    for (size_t i = 0; i < sizeof(EMOTION_MAP) / sizeof(EMOTION_MAP[0]); i++) {
        if (EMOTION_MAP[i].emotion == emotion) return EMOTION_MAP[i].name;
    }
    return "OPTIMAL";
}

inline bool parseEmotionName(const char* str, PetEmotion& out) {
    if (!str) return false;
    if (strcmp(str, "PREDICTIVE_WARNING") == 0 || strcmp(str, "WARNING_PREDICTIVE") == 0) {
        out = EMOTION_PREDICTIVE_WARNING;
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
    float shakeDurationMs = sensors.getShakeDurationMs();
    float babyRockScore = sensors.getBabyRockScore();
    float dynAccel = sensors.getDynamicAccel();
    float jerk = sensors.getJerk();
    bool freefall = sensors.isFreefalling();

    if (arb.vomitSeqStartMs == 0) {
        if (shakeDurationMs >= 3300.0f) {
            arb.vomitSeqStartMs = now;
            arb.nauseaUntilMs = 0; arb.dizzyUntilMs = 0;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
            sensors.resetShakeEnergy();
            sensors.resetBabyRockScore();
        } else if (shakeDurationMs >= 2200.0f) {
            if (now + 3500 > arb.nauseaUntilMs) arb.nauseaUntilMs = now + 3500;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
        } else if (shakeDurationMs >= 800.0f) {
            if (now + 3500 > arb.dizzyUntilMs) arb.dizzyUntilMs = now + 3500;
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
        }
    }

    if (arb.babySleepUntilMs > now) {
        if (dynAccel > 0.35f || jerk > 0.45f || shakeDurationMs >= 400.0f) {
            arb.babySleepUntilMs = 0; arb.babyRockUntilMs = 0;
            sensors.resetBabyRockScore();
            arb.startledUntilMs = now + 2200;
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
        if (elapsed < 3500) return EMOTION_VOMIT;
        if (elapsed < 6500) return EMOTION_WIPER;
        if (elapsed < 8500) { sensors.resetShakeEnergy(); return EMOTION_RECOVERY; }
        sensors.resetShakeEnergy();
        arb.vomitSeqStartMs = 0; outActiveShake = false; return EMOTION_OPTIMAL;
    }
    if (arb.nauseaUntilMs > now) { outActiveShake = true; sharedState.viewMode = VIEW_PET_FACE; return EMOTION_NAUSEA; }
    if (arb.dizzyUntilMs > now) { outActiveShake = true; sharedState.viewMode = VIEW_PET_FACE; return EMOTION_DIZZY; }
    if (arb.startledUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_STARTLED; }
    if (arb.babySleepUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_BABY_SLEEP; }
    if (arb.babyRockUntilMs > now) { sharedState.viewMode = VIEW_PET_FACE; return EMOTION_BABY_ROCKING; }
    if (freefall) return EMOTION_FREEFALL;

    if (!sensors.isWarmingUp()) {
        if (sharedState.ppm >= 1000.0f || sharedState.co_ppm >= 35.0f) {
            return ((now % 9000) >= 6200) ? IDLE_CRIT_GESTURES[(now / 9000) % IDLE_CRIT_COUNT] : EMOTION_CRITICAL;
        }
        if (sharedState.ppm >= 700.0f || sharedState.co_ppm >= 9.0f || sensors.getDeltaPPM() > 35.0f) {
            return ((now % 11000) >= 7800) ? IDLE_WARN_GESTURES[(now / 11000) % IDLE_WARN_COUNT] : EMOTION_PREDICTIVE_WARNING;
        }
    }

    if (now > 55000 && (now - sensors.getLastMotionMs() > 55000)) {
        return EMOTION_BABY_SLEEP;
    }
    return ((now % 16000) >= 13200) ? IDLE_GREEN_GESTURES[(now / 16000) % IDLE_GREEN_COUNT] : EMOTION_OPTIMAL;
}
