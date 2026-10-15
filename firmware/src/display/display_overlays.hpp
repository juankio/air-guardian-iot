#pragma once
#include "display_types.hpp"

inline void drawHeart(LovyanGFX* gfx, int cx, int cy, int r, uint16_t color) {
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

inline void drawSparkle(LovyanGFX* gfx, int cx, int cy, int r, uint16_t color) {
    gfx->drawLine(cx - r, cy, cx + r, cy, color);
    gfx->drawLine(cx, cy - r, cx, cy + r, color);
    int d = max(1, r / 2);
    gfx->drawLine(cx - d, cy - d, cx + d, cy + d, color);
    gfx->drawLine(cx - d, cy + d, cx + d, cy - d, color);
    gfx->fillCircle(cx, cy, max(1, r / 3), 0xFFFF);
}

inline void drawTopFaceOverlays(LovyanGFX* gfx, PetEmotion emo, int centerX, int centerY, uint32_t now) {
    if (emo == EMOTION_UPSIDE_DOWN) {
        gfx->fillEllipse(120, 34, 72, 22, gfx->color565(160, 20, 45));
        gfx->fillEllipse(120, 24, 54, 14, gfx->color565(220, 38, 68));
        int dropY = 58 - ((now / 35) % 35);
        gfx->fillCircle(68, dropY, 4, gfx->color565(56, 189, 248));
        gfx->fillCircle(172, dropY + 8, 4, gfx->color565(56, 189, 248));
    } else if (emo == EMOTION_HOT_PANTING) {
        gfx->fillEllipse(120, 36, 66, 18, gfx->color565(150, 30, 20));
    } else if (emo == EMOTION_ZEN) {
        int haloY = 44 + (int)(sinf(now * 0.005f) * 3.0f);
        gfx->drawEllipse(centerX, haloY, 38, 8, gfx->color565(250, 204, 21));
        gfx->drawEllipse(centerX, haloY, 37, 7, 0xFFFF);
        gfx->fillCircle(centerX, centerY - 38, 4, gfx->color565(56, 189, 248));
        gfx->fillCircle(centerX, centerY - 38, 2, 0xFFFF);
    } else if (emo == EMOTION_FREEFALL) {
        for (int k = 0; k < 4; k++) {
            int ly = ((now / 12 + k * 55) % 180) + 30;
            gfx->drawLine(28 + k * 6, ly, 28 + k * 6, ly + 26, 0xFFFF);
            gfx->drawLine(212 - k * 6, ly + 10, 212 - k * 6, ly + 36, 0xFFFF);
        }
    }
}

inline void drawNauseaMouthAndSweat(LovyanGFX* gfx, int centerX, int centerY, int mouthY, uint16_t pupilColor, uint32_t now) {
    int puff = (int)(sinf(now * 0.02f) * 2.5f);
    uint16_t cheekCol = gfx->color565(163, 230, 53);
    gfx->fillCircle(centerX - 26, mouthY + 1, 13 + puff, cheekCol);
    gfx->fillCircle(centerX + 26, mouthY + 1, 13 + puff, cheekCol);
    gfx->drawCircle(centerX - 26, mouthY + 1, 13 + puff, 0xFFFF);
    gfx->drawCircle(centerX + 26, mouthY + 1, 13 + puff, 0xFFFF);
    gfx->drawLine(centerX - 14, mouthY + 1, centerX - 5, mouthY - 3, pupilColor);
    gfx->drawLine(centerX - 5, mouthY - 3, centerX + 5, mouthY + 4, pupilColor);
    gfx->drawLine(centerX + 5, mouthY + 4, centerX + 14, mouthY - 1, pupilColor);
    int dropOff = (now / 40) % 14;
    gfx->fillCircle(centerX + 68, centerY - 26 + dropOff, 4, gfx->color565(56, 189, 248));
}

inline void drawVomitOverlay(LovyanGFX* gfx, int centerX, int mouthY, uint16_t mainColor, uint32_t now) {
    gfx->fillEllipse(centerX, mouthY + 2, 22, 16, gfx->color565(15, 35, 8));
    gfx->drawEllipse(centerX, mouthY + 2, 22, 16, mainColor);

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

    gfx->fillCircle(62, 156, 15, vCol1);
    gfx->fillRoundRect(57, 156, 10, 26, 4, vCol1);
    gfx->fillCircle(176, 148, 17, vCol3);
    gfx->fillRoundRect(171, 148, 10, 30, 4, vCol3);
    gfx->fillCircle(84, 196, 12, vCol2);
    gfx->fillCircle(154, 192, 14, vCol1);
    gfx->fillCircle(48, 112, 9, vCol3);
}

inline void drawWiperOverlay(LovyanGFX* gfx, int centerX, int mouthY, uint16_t mainColor, uint32_t now) {
    gfx->drawArc(centerX, mouthY - 4, 10, 14, 20, 160, mainColor);

    float sweep = sinf(now * 0.006f);
    float angle = sweep * 1.12f;
    int pivotX = 120;
    int pivotY = 236;
    int bladeLen = 198;
    int tipX = pivotX + (int)(sinf(angle) * bladeLen);
    int tipY = pivotY - (int)(cosf(angle) * bladeLen);

    uint16_t slimeCol = gfx->color565(132, 245, 20);
    if (tipX < 62)  gfx->fillCircle(62, 156, 12, slimeCol);
    if (tipX < 154) gfx->fillCircle(154, 192, 11, slimeCol);
    if (tipX > 176) gfx->fillCircle(176, 148, 13, slimeCol);

    drawSparkle(gfx, 72, 138, 7, 0xFFFF);
    drawSparkle(gfx, 168, 132, 8, gfx->color565(56, 189, 248));
    drawSparkle(gfx, 120, 86, 6, 0xFFFF);

    for (int d = -3; d <= 3; d++) {
        gfx->drawLine(pivotX + d, pivotY, tipX + d, tipY, (abs(d) <= 1) ? 0xFFFF : gfx->color565(56, 189, 248));
    }
    gfx->fillCircle(pivotX, pivotY, 12, gfx->color565(71, 85, 105));
    gfx->drawCircle(pivotX, pivotY, 12, 0xFFFF);
}

inline void drawSnotBubbleAndZzz(LovyanGFX* gfx, int centerX, int centerY, int mouthY, float breathAngle, uint16_t mainColor, uint32_t now) {
    gfx->fillCircle(centerX, mouthY + 4, 4, mainColor);

    int bubbleR = 11 + (int)(sinf(breathAngle * 1.15f) * 6.0f);
    int bx = centerX + 24;
    int by = centerY + 10;
    gfx->fillCircle(bx, by, bubbleR, gfx->color565(28, 120, 200));
    gfx->drawCircle(bx, by, bubbleR, gfx->color565(125, 211, 252));
    gfx->drawCircle(bx, by, max(1, bubbleR - 1), 0xFFFF);
    gfx->fillCircle(bx - bubbleR / 3, by - bubbleR / 3, max(2, bubbleR / 4), 0xFFFF);

    gfx->setTextDatum(lgfx::middle_center);
    gfx->setTextSize(1);
    gfx->setTextColor(gfx->color565(125, 211, 252));
    int zy = 75 - ((now / 50) % 30);
    gfx->drawString("z", centerX - 52, zy + 14);
    gfx->setTextSize(2);
    gfx->drawString("Z", centerX - 38, zy);
}

inline void drawStartledPopAndAlert(LovyanGFX* gfx, int centerX, int centerY, int mouthY) {
    gfx->fillEllipse(centerX, mouthY + 4, 11, 14, 0x0000);
    gfx->drawEllipse(centerX, mouthY + 4, 11, 14, 0xFFFF);

    int bx = centerX + 24;
    int by = centerY + 10;
    for (int a = 0; a < 6; a++) {
        float ang = a * 1.047f;
        int dx = bx + (int)(cosf(ang) * 20.0f);
        int dy = by + (int)(sinf(ang) * 20.0f);
        gfx->fillCircle(dx, dy, 2, gfx->color565(125, 211, 252));
    }

    uint16_t alertCol = gfx->color565(244, 63, 94);
    gfx->fillRoundRect(centerX - 10, 34, 6, 18, 2, alertCol);
    gfx->fillCircle(centerX - 7, 58, 3, alertCol);
    gfx->fillRoundRect(centerX + 4, 34, 6, 18, 2, alertCol);
    gfx->fillCircle(centerX + 7, 58, 3, alertCol);
}

inline void drawBubblegumOverlay(LovyanGFX* gfx, int centerX, int mouthY, uint32_t now) {
    uint32_t cycle = now % 3000;
    if (cycle < 2550) {
        int gumR = 5 + (int)((cycle / 2550.0f) * 23.0f);
        int gx = centerX;
        int gy = mouthY + 4;
        gfx->fillCircle(gx, gy, gumR, gfx->color565(244, 114, 182));
        gfx->drawCircle(gx, gy, gumR, gfx->color565(251, 207, 232));
        gfx->fillCircle(gx - gumR / 3, gy - gumR / 3, max(2, gumR / 4), 0xFFFF);
    } else {
        gfx->drawCircle(centerX, mouthY + 4, 28, gfx->color565(244, 114, 182));
        drawSparkle(gfx, centerX - 24, mouthY, 5, gfx->color565(244, 114, 182));
        drawSparkle(gfx, centerX + 24, mouthY, 5, gfx->color565(244, 114, 182));
    }
}

inline void drawCryingTearsAndMouth(LovyanGFX* gfx, const int eyeXs[2], int centerX, int centerY, int mouthY, uint16_t mainColor, uint32_t now) {
    for (int i = 0; i < 2; i++) {
        int tx = eyeXs[i];
        int ty = centerY + 6;
        gfx->fillRoundRect(tx - 7, ty, 14, 72, 5, gfx->color565(56, 189, 248));
        int wave = (now / 30) % 18;
        gfx->fillCircle(tx, ty + 15 + wave, 5, 0xFFFF);
    }
    gfx->drawArc(centerX, mouthY + 12, 10, 14, 200, 340, mainColor);
}
