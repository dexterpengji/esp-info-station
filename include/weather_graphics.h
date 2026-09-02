#pragma once
#include <Arduino.h>
#include "TFT_eSPI.h"

class WeatherGraphics {
public:
    static void drawAnimatedSun(TFT_eSprite &spr, int cx, int cy, int r, uint32_t frame, uint16_t primary, uint16_t secondary) {
        float angle_offset = (frame % 360) * 0.04f;
        
        // Rotating sun rays
        for (int i = 0; i < 8; i++) {
            float a = angle_offset + (i * TWO_PI / 8.0f);
            int x1 = cx + (int)(cos(a) * (r + 3));
            int y1 = cy + (int)(sin(a) * (r + 3));
            int x2 = cx + (int)(cos(a) * (r + 9));
            int y2 = cy + (int)(sin(a) * (r + 9));
            spr.drawLine(x1, y1, x2, y2, secondary);
        }
        // Sun glowing core
        spr.fillCircle(cx, cy, r + 1, primary);
        spr.drawCircle(cx, cy, r + 2, secondary);
    }

    static void drawAnimatedMoon(TFT_eSprite &spr, int cx, int cy, int r, uint32_t frame, uint16_t primary, uint16_t bg) {
        spr.fillCircle(cx, cy, r, primary);
        // Crescent mask
        int shadow_x = cx + (int)(r * 0.5f);
        int shadow_y = cy - (int)(r * 0.2f);
        spr.fillCircle(shadow_x, shadow_y, r - 1, bg);

        // Twinkling stars
        if ((frame / 10) % 2 == 0) spr.drawPixel(cx - r - 4, cy - 4, primary);
        if ((frame / 15) % 2 == 0) spr.drawPixel(cx + r + 5, cy + 2, primary);
    }

    static void drawAnimatedCloud(TFT_eSprite &spr, int cx, int cy, uint32_t frame, uint16_t color, uint16_t fill) {
        int float_x = cx + (int)(sin(frame * 0.05f) * 2.0f);
        
        // 3 cloud circles + bottom base
        spr.fillCircle(float_x - 8, cy + 2, 7, fill);
        spr.fillCircle(float_x + 2, cy - 3, 9, fill);
        spr.fillCircle(float_x + 10, cy + 3, 6, fill);
        spr.fillRect(float_x - 8, cy + 2, 18, 7, fill);

        spr.drawCircle(float_x - 8, cy + 2, 7, color);
        spr.drawCircle(float_x + 2, cy - 3, 9, color);
        spr.drawCircle(float_x + 10, cy + 3, 6, color);
        spr.drawLine(float_x - 8, cy + 9, float_x + 10, cy + 9, color);
    }

    static void drawAnimatedRain(TFT_eSprite &spr, int cx, int cy, uint32_t frame, uint16_t cloud_col, uint16_t rain_col, uint16_t fill) {
        drawAnimatedCloud(spr, cx, cy - 4, frame, cloud_col, fill);

        // Falling raindrops
        for (int i = -10; i <= 10; i += 7) {
            int drop_y = cy + 8 + ((frame * 2 + (i * 5)) % 16);
            spr.drawLine(cx + i - 1, drop_y, cx + i - 2, drop_y + 3, rain_col);
        }
    }

    static void drawAnimatedThunder(TFT_eSprite &spr, int cx, int cy, uint32_t frame, uint16_t cloud_col, uint16_t bolt_col, uint16_t fill) {
        drawAnimatedCloud(spr, cx, cy - 5, frame, cloud_col, fill);

        // Lightning flash
        if ((frame % 40) < 6) {
            spr.drawLine(cx, cy + 6, cx - 3, cy + 12, bolt_col);
            spr.drawLine(cx - 3, cy + 12, cx + 2, cy + 12, bolt_col);
            spr.drawLine(cx + 2, cy + 12, cx - 2, cy + 20, bolt_col);
        } else {
            // Light rain
            int drop_y = cy + 8 + ((frame * 3) % 12);
            spr.drawLine(cx - 6, drop_y, cx - 7, drop_y + 3, cloud_col);
            spr.drawLine(cx + 6, drop_y, cx + 5, drop_y + 3, cloud_col);
        }
    }

    static void drawAnimatedSnow(TFT_eSprite &spr, int cx, int cy, uint32_t frame, uint16_t cloud_col, uint16_t snow_col, uint16_t fill) {
        drawAnimatedCloud(spr, cx, cy - 5, frame, cloud_col, fill);

        // Floating snowflakes
        for (int i = -9; i <= 9; i += 9) {
            int snow_y = cy + 7 + ((frame + (i * 6)) % 14);
            int snow_x = cx + i + (int)(sin(frame * 0.1f + i) * 2.0f);
            spr.drawPixel(snow_x, snow_y, snow_col);
            spr.drawPixel(snow_x + 1, snow_y, snow_col);
            spr.drawPixel(snow_x, snow_y + 1, snow_col);
            spr.drawPixel(snow_x + 1, snow_y + 1, snow_col);
        }
    }

    static void drawWeatherBadge(TFT_eSprite &spr, int cx, int cy, int weather_code, bool is_day, uint32_t frame, const ColorPalette &pal) {
        if (weather_code == 0) {
            if (is_day) {
                drawAnimatedSun(spr, cx, cy, 10, frame, pal.primary, pal.secondary);
            } else {
                drawAnimatedMoon(spr, cx, cy, 10, frame, pal.primary, pal.bg_dark);
            }
        } else if (weather_code >= 1 && weather_code <= 3) {
            // Partly cloudy: sun/moon + cloud
            if (is_day) drawAnimatedSun(spr, cx - 5, cy - 5, 7, frame, pal.primary, pal.secondary);
            drawAnimatedCloud(spr, cx + 2, cy + 2, frame, pal.secondary, pal.card_bg);
        } else if (weather_code >= 51 && weather_code <= 67 || (weather_code >= 80 && weather_code <= 82)) {
            drawAnimatedRain(spr, cx, cy, frame, pal.text_dim, pal.primary, pal.card_bg);
        } else if (weather_code >= 95 && weather_code <= 99) {
            drawAnimatedThunder(spr, cx, cy, frame, pal.text_dim, pal.highlight, pal.card_bg);
        } else if (weather_code >= 71 && weather_code <= 77) {
            drawAnimatedSnow(spr, cx, cy, frame, pal.text_dim, pal.highlight, pal.card_bg);
        } else {
            drawAnimatedCloud(spr, cx, cy, frame, pal.primary, pal.card_bg);
        }
    }
};
