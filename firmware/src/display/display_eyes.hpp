#pragma once
#include "display_overlays.hpp"

inline void renderSingleEye(
    LovyanGFX* gfx,
    PetEmotion emo,
    int i,
    int ex,
    int ey,
    int baseRx,
    int baseRy,
    float blinkH,
    float currentEyeX,
    float currentEyeY,
    uint16_t mainColor,
    uint16_t pupilColor,
    uint32_t now
) {
    if (emo == EMOTION_BABY_SLEEP || emo == EMOTION_SLEEPING) {
        gfx->drawArc(ex, ey + 6, 15, 19, 20, 160, mainColor);
    } else if (emo == EMOTION_RECOVERY || emo == EMOTION_BABY_ROCKING || emo == EMOTION_ZEN) {
        gfx->drawArc(ex, ey - 2, 15, 19, 200, 340, mainColor);
    } else if (emo == EMOTION_LAUGHING || emo == EMOTION_VOMIT ||
               (emo == EMOTION_SNEEZE && (now % 2500) >= 1600)) {
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
        for (int d = -1; d <= 1; d++) {
            gfx->drawLine(ex - 14 + d, ey - 14, ex + 14 + d, ey + 14, mainColor);
            gfx->drawLine(ex + 14 + d, ey - 14, ex - 14 + d, ey + 14, mainColor);
        }
    } else if (emo == EMOTION_LOVE) {
        int pulseR = 22 + (int)(sinf(now * 0.012f + i) * 3.0f);
        drawHeart(gfx, ex + (int)(currentEyeX * 0.5f), ey + (int)(currentEyeY * 0.5f), pulseR, gfx->color565(244, 63, 94));
        gfx->fillCircle(ex - 6, ey - 8, 3, 0xFFFF);
    } else if (emo == EMOTION_DIZZY) {
        gfx->fillEllipse(ex, ey, 24, 31, mainColor);
        float dizzyAngle = now * 0.015f * (i == 0 ? 1.0f : -1.0f);
        int px = ex + (int)(cosf(dizzyAngle) * 7.5f);
        int py = ey + (int)(sinf(dizzyAngle) * 7.5f);
        gfx->fillCircle(px, py, 11, pupilColor);
        gfx->drawCircle(px, py, 7, 0xFFFF);
        gfx->drawCircle(px, py, 3, mainColor);
        int browOff = (i == 0) ? -4 : 4;
        gfx->drawArc(ex, ey - 24 + browOff, 14, 17, 200, 340, mainColor);
    } else if (emo == EMOTION_NAUSEA) {
        gfx->fillEllipse(ex, ey, 22, 18, mainColor);
        int jitterX = (int)(sinf(now * 0.04f + i) * 3.0f);
        gfx->fillCircle(ex + jitterX, ey + 2, 7, pupilColor);
        gfx->fillCircle(ex + jitterX - 2, ey, 2, 0xFFFF);
        int dir = (i == 0) ? -5 : 5;
        gfx->drawLine(ex - 16, ey - 20 - dir, ex + 16, ey - 20 + dir, mainColor);
        gfx->drawLine(ex - 16, ey - 19 - dir, ex + 16, ey - 19 + dir, mainColor);
    } else if (emo == EMOTION_STARTLED) {
        gfx->fillCircle(ex, ey, 27, 0xFFFF);
        gfx->drawCircle(ex, ey, 27, mainColor);
        int jx = ((now / 25 + i * 3) % 5) - 2;
        int jy = ((now / 20 + i * 7) % 5) - 2;
        gfx->fillCircle(ex + jx, ey + jy, 4, 0x0000);
        gfx->fillCircle(ex + jx - 1, ey + jy - 1, 1, 0xFFFF);
    } else if (emo == EMOTION_WINK && i == 0) {
        gfx->drawArc(ex, ey + 2, 15, 19, 200, 340, mainColor);
        drawSparkle(gfx, ex - 22, ey - 18, 6, gfx->color565(250, 204, 21));
    } else if (emo == EMOTION_SUSPICIOUS || (emo == EMOTION_SNEEZE && (now % 2500) < 1600)) {
        gfx->fillRoundRect(ex - 22, ey - 6, 44, 12, 4, mainColor);
        int px = ex + (int)currentEyeX;
        gfx->fillCircle(px, ey, 5, pupilColor);
        gfx->drawLine(ex - 22, ey - 12, ex + 22, ey - 12, mainColor);
    } else if (emo == EMOTION_GLASSES_COOL) {
        gfx->fillRoundRect(ex - 28, ey - 18, 56, 34, 7, gfx->color565(15, 23, 42));
        gfx->drawRoundRect(ex - 28, ey - 18, 56, 34, 7, gfx->color565(56, 189, 248));
        gfx->drawRoundRect(ex - 27, ey - 17, 54, 32, 6, 0xFFFF);
        int shineX = ((now / 18) % 46) - 18;
        gfx->drawLine(ex + shineX - 8, ey + 10, ex + shineX + 6, ey - 12, 0xFFFF);
        gfx->drawLine(ex + shineX - 7, ey + 10, ex + shineX + 7, ey - 12, 0xFFFF);
    } else {
        int rx = baseRx;
        int ry = max(4, (int)(baseRy * blinkH));

        if (emo == EMOTION_WALL_LEFT && i == 0) {
            rx = 13; ry = 36; ex -= 4;
            gfx->drawLine(ex - rx, ey - 28, ex - rx, ey + 28, 0xFFFF);
        } else if (emo == EMOTION_WALL_RIGHT && i == 1) {
            rx = 13; ry = 36; ex += 4;
            gfx->drawLine(ex + rx, ey - 28, ex + rx, ey + 28, 0xFFFF);
        } else if (emo == EMOTION_FREEFALL) {
            rx = 19; ry = 36;
        } else if (emo == EMOTION_YAWN || emo == EMOTION_HOT_PANTING || emo == EMOTION_CRYING) {
            ry = max(8, (int)(18.0f * blinkH));
        }

        gfx->fillEllipse(ex, ey, rx, ry, mainColor);

        if (blinkH > 0.35f) {
            int px = ex + (int)currentEyeX;
            int py = ey + (int)(currentEyeY * blinkH);
            if (emo == EMOTION_DERP) {
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

inline void renderEyesAndBrows(
    LovyanGFX* gfx,
    PetEmotion emo,
    const int eyeXs[2],
    int centerY,
    float breathAngle,
    float blinkH,
    float currentEyeX,
    float currentEyeY,
    uint16_t mainColor,
    uint16_t pupilColor,
    uint32_t now
) {
    int baseRx = 24 + (int)(sinf(breathAngle) * 1.5f);
    int baseRy = 32 - (int)(sinf(breathAngle) * 1.5f);
    for (int i = 0; i < 2; i++) {
        renderSingleEye(gfx, emo, i, eyeXs[i], centerY - 10, baseRx, baseRy, blinkH, currentEyeX, currentEyeY, mainColor, pupilColor, now);
    }
}
