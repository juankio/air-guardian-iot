#pragma once
#include "display_overlays.hpp"
#include "display_props.hpp"

inline void renderMouthAndProps(
    LovyanGFX* gfx,
    PetEmotion emo,
    const int eyeXs[2],
    int centerX,
    int centerY,
    float breathAngle,
    uint16_t mainColor,
    uint16_t pupilColor,
    uint32_t now
) {
    int mouthY = centerY + 32;

    if (emo == EMOTION_NAUSEA) {
        drawNauseaMouthAndSweat(gfx, centerX, centerY, mouthY, pupilColor, now);
    } else if (emo == EMOTION_VOMIT) {
        drawVomitOverlay(gfx, centerX, mouthY, mainColor, now);
    } else if (emo == EMOTION_WIPER) {
        drawWiperOverlay(gfx, centerX, mouthY, mainColor, now);
    } else if (emo == EMOTION_BABY_ROCKING) {
        drawBabyPacifierAndNotes(gfx, centerX, mouthY, now);
    } else if (emo == EMOTION_BABY_SLEEP || emo == EMOTION_SLEEPING) {
        drawSnotBubbleAndZzz(gfx, centerX, centerY, mouthY, breathAngle, mainColor, now);
    } else if (emo == EMOTION_STARTLED) {
        drawStartledPopAndAlert(gfx, centerX, centerY, mouthY);
    } else if (emo == EMOTION_BUBBLEGUM) {
        drawBubblegumOverlay(gfx, centerX, mouthY, now);
    } else if (emo == EMOTION_DERP) {
        gfx->drawArc(centerX, mouthY - 4, 10, 13, 20, 160, mainColor);
        gfx->fillRoundRect(centerX - 7, mouthY + 2, 14, 16, 6, gfx->color565(244, 114, 182));
        gfx->drawLine(centerX, mouthY + 2, centerX, mouthY + 13, gfx->color565(190, 24, 93));
    } else if (emo == EMOTION_HOT_PANTING) {
        int pantY = (int)(sinf(now * 0.025f) * 3.5f);
        gfx->fillEllipse(centerX, mouthY + 2, 14, 10, 0x0000);
        gfx->drawEllipse(centerX, mouthY + 2, 14, 10, mainColor);
        gfx->fillRoundRect(centerX - 8, mouthY + 2 + pantY, 16, 18, 7, gfx->color565(244, 114, 182));
        gfx->drawLine(centerX, mouthY + 3 + pantY, centerX, mouthY + 15 + pantY, gfx->color565(190, 24, 93));
        int swY = ((now / 35) % 24);
        gfx->fillCircle(centerX + 66, centerY - 24 + swY, 5, gfx->color565(56, 189, 248));
    } else if (emo == EMOTION_COFFEE) {
        drawCoffeeProp(gfx, centerX, mouthY, now);
    } else if (emo == EMOTION_RAMEN) {
        drawRamenProp(gfx, centerX, mouthY, now);
    } else if (emo == EMOTION_MUSIC_VIBE) {
        drawMusicEqualizerAndSmile(gfx, centerX, mouthY, mainColor, now);
    } else if (emo == EMOTION_BUTTERFLY) {
        drawButterflyProp(gfx, centerX, mouthY, mainColor, now);
    } else if (emo == EMOTION_CRYING) {
        drawCryingTearsAndMouth(gfx, eyeXs, centerX, centerY, mouthY, mainColor, now);
    } else if (emo == EMOTION_LAUGHING) {
        gfx->fillArc(centerX, mouthY - 4, 0, 22, 0, 180, gfx->color565(80, 10, 25));
        gfx->fillRect(centerX - 18, mouthY - 4, 36, 6, 0xFFFF);
        gfx->fillArc(centerX, mouthY + 6, 0, 10, 0, 180, gfx->color565(244, 114, 182));
    } else if (emo == EMOTION_GAS_MASK) {
        drawGasMaskProp(gfx, centerX, mouthY, mainColor);
    } else if (emo == EMOTION_PREDICTIVE_WARNING) {
        if ((now % 3600) > 3200) {
            gfx->fillEllipse(centerX, mouthY + 4, 11, 8, mainColor);
        } else {
            gfx->drawArc(centerX, mouthY + 10, 10, 14, 200, 340, mainColor);
        }
    } else if (emo == EMOTION_WHISTLE) {
        gfx->fillCircle(centerX + 12, mouthY + 2, 5, mainColor);
        gfx->fillCircle(centerX + 12, mouthY + 2, 2, 0x0000);
        int ny = mouthY - 6 - ((now / 40) % 28);
        drawMusicNote(gfx, centerX + 34, ny, mainColor);
    } else if (emo == EMOTION_SNEEZE) {
        uint32_t sneezePhase = now % 2500;
        if (sneezePhase < 1600) {
            gfx->drawLine(centerX - 8, mouthY + 2, centerX + 8, mouthY + 2, mainColor);
        } else {
            gfx->fillEllipse(centerX, mouthY + 2, 12, 10, mainColor);
            for (int k = -2; k <= 2; k++) {
                gfx->fillCircle(centerX + k * 12, mouthY + 24 + abs(k) * 4, 3, 0xFFFF);
            }
        }
    } else if (emo == EMOTION_YAWN || emo == EMOTION_FREEFALL) {
        int ry = (emo == EMOTION_FREEFALL) ? 20 : (12 + (int)(sinf(now * 0.004f) * 6.0f));
        gfx->fillEllipse(centerX, mouthY + 4, 15, max(6, ry), gfx->color565(20, 15, 35));
        gfx->drawEllipse(centerX, mouthY + 4, 15, max(6, ry), mainColor);
    } else if (emo == EMOTION_DIZZY) {
        drawDizzyStarsAndMouth(gfx, centerX, centerY, mouthY, mainColor, now);
    } else if (emo == EMOTION_LOVE) {
        gfx->drawArc(centerX - 7, mouthY - 2, 6, 9, 20, 160, mainColor);
        gfx->drawArc(centerX + 7, mouthY - 2, 6, 9, 20, 160, mainColor);
        int hy = 58 - ((now / 45) % 24);
        drawHeart(gfx, centerX, hy, 8, gfx->color565(244, 63, 94));
    } else if (emo == EMOTION_SUSPICIOUS || emo == EMOTION_DETECTIVE) {
        gfx->drawLine(centerX - 12, mouthY + 2, centerX + 12, mouthY + 2, mainColor);
        gfx->drawLine(centerX - 12, mouthY + 3, centerX + 12, mouthY + 3, mainColor);
    } else if (emo == EMOTION_CRITICAL) {
        gfx->fillEllipse(centerX, mouthY + 4, 12, 9, mainColor);
    } else {
        gfx->drawArc(centerX, mouthY - 4, 11, 15, 20, 160, mainColor);
    }
}

inline void renderCheekBlush(LovyanGFX* gfx, PetEmotion emo, int centerX, int centerY, uint16_t mainColor) {
    if (emo != EMOTION_NAUSEA && emo != EMOTION_VOMIT && emo != EMOTION_GAS_MASK &&
        emo != EMOTION_CRITICAL && emo != EMOTION_BABY_SLEEP && emo != EMOTION_SLEEPING) {
        gfx->fillEllipse(centerX - 60, centerY + 12, 9, 5, mainColor);
        gfx->fillEllipse(centerX + 60, centerY + 12, 9, 5, mainColor);
    }
}
