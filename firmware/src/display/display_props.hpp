#pragma once
#include "display_overlays.hpp"

inline void drawMusicNote(LovyanGFX* gfx, int x, int y, uint16_t color) {
    gfx->fillCircle(x, y, 4, color);
    gfx->fillRect(x + 2, y - 12, 2, 12, color);
    gfx->fillRect(x + 2, y - 12, 6, 3, color);
}

inline void drawHeadphonesProp(LovyanGFX* gfx, int centerX, int centerY, uint32_t now) {
    int beat = (int)(fabsf(sinf(now * 0.012f)) * 4.0f);
    gfx->drawArc(centerX, centerY - 6, 86, 92, 200, 340, gfx->color565(56, 189, 248));
    gfx->fillRoundRect(centerX - 96 - beat, centerY - 26, 18, 44, 6, gfx->color565(236, 72, 153));
    gfx->fillRoundRect(centerX + 78 + beat, centerY - 26, 18, 44, 6, gfx->color565(236, 72, 153));
    gfx->drawRoundRect(centerX - 96 - beat, centerY - 26, 18, 44, 6, 0xFFFF);
    gfx->drawRoundRect(centerX + 78 + beat, centerY - 26, 18, 44, 6, 0xFFFF);
}

inline void drawGlassesBridgeAndLoupe(LovyanGFX* gfx, PetEmotion emo, int centerX, int centerY) {
    if (emo == EMOTION_GLASSES_COOL) {
        gfx->drawLine(centerX - 16, centerY - 16, centerX + 16, centerY - 16, gfx->color565(56, 189, 248));
        gfx->drawLine(centerX - 16, centerY - 15, centerX + 16, centerY - 15, 0xFFFF);
    } else if (emo == EMOTION_DETECTIVE) {
        int lx = centerX + 44;
        int ly = centerY - 10;
        gfx->drawCircle(lx, ly, 30, gfx->color565(250, 204, 21));
        gfx->drawCircle(lx, ly, 31, 0xFFFF);
        gfx->drawLine(lx + 21, ly + 21, lx + 42, ly + 42, gfx->color565(250, 204, 21));
        gfx->drawLine(lx + 20, ly + 22, lx + 41, ly + 43, gfx->color565(250, 204, 21));
    }
}

inline void drawBabyPacifierAndNotes(LovyanGFX* gfx, int centerX, int mouthY, uint32_t now) {
    int suck = (int)(sinf(now * 0.012f) * 2.0f);
    gfx->fillEllipse(centerX, mouthY + 3, 15 + suck, 11, gfx->color565(244, 114, 182));
    gfx->drawEllipse(centerX, mouthY + 3, 15 + suck, 11, 0xFFFF);
    gfx->fillCircle(centerX, mouthY + 3, 6, gfx->color565(250, 204, 21));
    gfx->drawCircle(centerX, mouthY + 10 + suck, 7, 0xFFFF);

    int noteY = 95 - ((now / 45) % 45);
    drawMusicNote(gfx, 38, noteY, gfx->color565(250, 204, 21));
    drawMusicNote(gfx, 196, noteY + 15, gfx->color565(244, 114, 182));
}

inline void drawCoffeeProp(LovyanGFX* gfx, int centerX, int mouthY, uint32_t now) {
    int mugX = centerX - 18;
    int mugY = mouthY - 2;
    gfx->fillRoundRect(mugX, mugY, 36, 30, 5, gfx->color565(180, 83, 9));
    gfx->drawRoundRect(mugX, mugY, 36, 30, 5, 0xFFFF);
    gfx->drawCircle(mugX + 38, mugY + 15, 7, 0xFFFF);
    int stOff = (now / 40) % 16;
    gfx->drawLine(centerX - 8, mugY - 4 - stOff, centerX - 5, mugY - 12 - stOff, 0xFFFF);
    gfx->drawLine(centerX + 8, mugY - 2 - stOff, centerX + 5, mugY - 10 - stOff, 0xFFFF);
}

inline void drawRamenProp(LovyanGFX* gfx, int centerX, int mouthY, uint32_t now) {
    int slurp = (now / 30) % 10;
    for (int n = -6; n <= 6; n += 6) {
        gfx->drawLine(centerX + n, mouthY - 2, centerX + n + ((slurp > 5) ? 2 : -2), mouthY + 18, gfx->color565(250, 204, 21));
    }
    gfx->fillArc(centerX, mouthY + 14, 0, 26, 0, 180, gfx->color565(220, 38, 38));
    gfx->drawArc(centerX, mouthY + 14, 24, 26, 0, 180, 0xFFFF);
}

inline void drawMusicEqualizerAndSmile(LovyanGFX* gfx, int centerX, int mouthY, uint16_t mainColor, uint32_t now) {
    gfx->drawArc(centerX, mouthY - 6, 10, 14, 20, 160, mainColor);
    for (int b = -3; b <= 3; b++) {
        int bh = 8 + (int)(fabsf(sinf(now * 0.015f + b * 0.9f)) * 18.0f);
        uint16_t bCol = (b % 2 == 0) ? gfx->color565(56, 189, 248) : gfx->color565(236, 72, 153);
        gfx->fillRoundRect(centerX + b * 12 - 4, 198 - bh, 8, bh, 2, bCol);
    }
}

inline void drawButterflyProp(LovyanGFX* gfx, int centerX, int mouthY, uint16_t mainColor, uint32_t now) {
    gfx->fillCircle(centerX, mouthY + 2, 5, mainColor);
    float bt = now * 0.0032f;
    int bx = 120 + (int)(sinf(bt) * 58.0f);
    int by = 82 + (int)(sinf(bt * 2.0f) * 24.0f);
    int wingW = 6 + (int)(fabsf(sinf(now * 0.03f)) * 7.0f);
    gfx->fillEllipse(bx - wingW / 2 - 2, by, wingW, 9, gfx->color565(244, 114, 182));
    gfx->fillEllipse(bx + wingW / 2 + 2, by, wingW, 9, gfx->color565(250, 204, 21));
    gfx->fillCircle(bx, by, 3, 0xFFFF);
}

inline void drawGasMaskProp(LovyanGFX* gfx, int centerX, int mouthY, uint16_t mainColor) {
    gfx->fillRoundRect(centerX - 26, mouthY - 12, 52, 36, 10, gfx->color565(51, 65, 85));
    gfx->drawRoundRect(centerX - 26, mouthY - 12, 52, 36, 10, mainColor);
    gfx->fillCircle(centerX - 34, mouthY + 8, 13, mainColor);
    gfx->fillCircle(centerX + 34, mouthY + 8, 13, mainColor);
    gfx->fillCircle(centerX - 34, mouthY + 8, 8, gfx->color565(30, 41, 59));
    gfx->fillCircle(centerX + 34, mouthY + 8, 8, gfx->color565(30, 41, 59));
    gfx->fillCircle(centerX, mouthY + 6, 10, gfx->color565(15, 23, 42));
    gfx->drawCircle(centerX, mouthY + 6, 10, 0xFFFF);
}

inline void drawDizzyStarsAndMouth(LovyanGFX* gfx, int centerX, int centerY, int mouthY, uint16_t mainColor, uint32_t now) {
    for (int d = 0; d <= 1; d++) {
        gfx->drawLine(centerX - 16, mouthY + d,     centerX - 8,  mouthY - 5 + d, mainColor);
        gfx->drawLine(centerX - 8,  mouthY - 5 + d, centerX,      mouthY + 5 + d, mainColor);
        gfx->drawLine(centerX,      mouthY + 5 + d, centerX + 8,  mouthY - 5 + d, mainColor);
        gfx->drawLine(centerX + 8,  mouthY - 5 + d, centerX + 16, mouthY + d,     mainColor);
    }
    for (int s = 0; s < 3; s++) {
        float sa = (now * 0.006f) + (s * 2.094f);
        int sx = centerX + (int)(cosf(sa) * 44.0f);
        int sy = centerY - 54 + (int)(sinf(sa) * 11.0f);
        drawSparkle(gfx, sx, sy, 6, gfx->color565(250, 204, 21));
    }
}

inline void renderCircularHUDGauge(LovyanGFX* gfx, const DisplaySharedState& state) {
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
