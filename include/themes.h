#pragma once
#include <Arduino.h>
#include "TFT_eSPI.h"
#include "data_models.h"
#include "palettes.h"
#include "weather_graphics.h"

class Themes {
public:
    // Starfield state for Theme 3 (Orbital Astro)
    struct Star {
        float x, y, z;
    };
    static Star stars[45];
    static bool stars_inited;

    static void initStars() {
        for (int i = 0; i < 45; i++) {
            stars[i].x = (float)(random(-160, 160));
            stars[i].y = (float)(random(-85, 85));
            stars[i].z = (float)(random(1, 100));
        }
        stars_inited = true;
    }

    // Format Date: MM DD YYYY weekday (e.g. "09 01 2026 Tuesday")
    static void formatDate(const tm &t, char *buf, size_t max_len) {
        strftime(buf, max_len, "%m %d %Y %A", &t);
    }

    // Helper: Draw Wi-Fi signal icon
    static void drawWifiIcon(TFT_eSprite &spr, int x, int y, int rssi, uint16_t color, uint16_t dimColor) {
        int bars = 1;
        if (rssi > -60) bars = 4;
        else if (rssi > -70) bars = 3;
        else if (rssi > -80) bars = 2;

        for (int i = 0; i < 4; i++) {
            int h = (i + 1) * 3;
            uint16_t c = (i < bars) ? color : dimColor;
            spr.fillRect(x + (i * 3), y + 10 - h, 2, h, c);
        }
    }

    // Helper: Draw Battery Icon & Percentage
    static void drawBatteryIcon(TFT_eSprite &spr, int x, int y, uint8_t pct, bool charging, uint16_t color, uint16_t bgDark) {
        uint16_t alertColor = TFT_RED;
        uint16_t borderCol = (pct <= 15 && !charging) ? alertColor : color;

        // Battery Shell (18x9 px)
        spr.drawRect(x, y, 18, 9, borderCol);
        spr.fillRect(x + 18, y + 2, 2, 5, borderCol);

        // Level Fill
        int fillW = map(pct, 0, 100, 0, 14);
        fillW = constrain(fillW, 0, 14);

        uint16_t fillCol = color;
        if (charging) fillCol = TFT_YELLOW;
        else if (pct <= 15) fillCol = alertColor;
        else if (pct <= 30) fillCol = TFT_ORANGE;

        if (fillW > 0) {
            spr.fillRect(x + 2, y + 2, fillW, 5, fillCol);
        }

        char pctBuf[8];
        if (charging) {
            snprintf(pctBuf, sizeof(pctBuf), "CHG");
        } else {
            snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
        }
        spr.setTextColor(borderCol, bgDark);
        spr.drawString(pctBuf, x + 23, y + 1, 1);
    }

    // =========================================================================
    // PERSISTENT BOTTOM STATUS BAR (Always visible at y=148..170 across all themes)
    // =========================================================================
    static void drawPersistentBottomBar(TFT_eSprite &spr, const AppState &state, const GeoData &geo, const ColorPalette &pal) {
        // Bottom bar container (320x22 px)
        spr.fillRect(0, 148, 320, 22, pal.bg_dark);
        spr.drawFastHLine(0, 148, 320, pal.primary);

        // 1. Wi-Fi Signal & City Name (Left)
        drawWifiIcon(spr, 6, 153, state.wifi_rssi, pal.highlight, pal.text_dim);
        spr.setTextColor(state.wifi_connected ? pal.primary : pal.text_dim, pal.bg_dark);
        spr.drawString(state.wifi_connected ? geo.city : "Offline", 22, 153, 1);

        // 2. Battery Telemetry (Center)
        drawBatteryIcon(spr, 122, 154, state.battery_pct, state.battery_charging, pal.highlight, pal.bg_dark);

        // 3. Firmware Version / OTA Progress (Right)
        if (state.ota_updating) {
            char otaBuf[16];
            snprintf(otaBuf, sizeof(otaBuf), "OTA %d%%", state.ota_progress_pct);
            spr.setTextColor(pal.highlight, pal.bg_dark);
            spr.drawRightString(otaBuf, 314, 153, 1);
        } else {
            spr.setTextColor(pal.text_dim, pal.bg_dark);
            spr.drawRightString(FIRMWARE_VERSION, 314, 153, 1);
        }
    }

    // =========================================================================
    // THEME 0: CYBERPUNK HUD STATION (Dramatic Crimson/Purple Tech Grid)
    // =========================================================================
    static void drawTheme0_CyberHUD(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        // Tech grid lines & glowing corner brackets
        spr.drawFastHLine(0, 14, 320, pal.card_bg);
        spr.drawLine(0, 0, 14, 0, pal.primary);
        spr.drawLine(0, 0, 0, 14, pal.primary);
        spr.drawLine(319, 0, 305, 0, pal.primary);
        spr.drawLine(319, 0, 319, 14, pal.primary);

        // Left Weather Telemetry Card [x=6..78, y=18..144]
        spr.fillRoundRect(6, 18, 72, 126, 6, pal.card_bg);
        spr.drawRoundRect(6, 18, 72, 126, 6, pal.primary);

        WeatherGraphics::drawWeatherBadge(spr, 42, 44, weather.weather_code, weather.is_day, state.anim_frame, pal);

        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f C", weather.temperature);
        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.drawCentreString(tempBuf, 42, 68, 2);

        spr.setTextColor(pal.text_dim, pal.card_bg);
        spr.drawCentreString(weather.condition_text, 42, 86, 1);

        char humBuf[16];
        snprintf(humBuf, sizeof(humBuf), "HUM %d%%", weather.humidity);
        spr.drawString(humBuf, 12, 104, 1);

        char windBuf[16];
        snprintf(windBuf, sizeof(windBuf), "WND %.0fk", weather.wind_speed);
        spr.drawString(windBuf, 12, 118, 1);

        // Center Giant Glowing Digital Clock [x=84..236, y=18..144]
        spr.fillRoundRect(84, 18, 152, 126, 6, pal.card_bg);
        spr.drawRoundRect(84, 18, 152, 126, 6, pal.secondary);

        char timeBuf[12];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);

        // Clock drop shadow glow
        spr.setTextColor(pal.primary, pal.card_bg);
        spr.drawCentreString(timeBuf, 161, 35, 7);
        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.drawCentreString(timeBuf, 160, 34, 7);

        // Seconds indicator sub-banner
        char secBuf[12];
        snprintf(secBuf, sizeof(secBuf), ": %02d SEC", t.tm_sec);
        spr.setTextColor(pal.secondary, pal.card_bg);
        spr.drawCentreString(secBuf, 160, 88, 2);

        // Date text
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(pal.text_dim, pal.card_bg);
        spr.drawCentreString(dateBuf, 160, 108, 1);

        // Right Stock Status Panel [x=242..314, y=18..144]
        spr.fillRoundRect(242, 18, 72, 126, 6, pal.card_bg);
        spr.drawRoundRect(242, 18, 72, 126, 6, pal.primary);

        spr.setTextColor(pal.primary, pal.card_bg);
        spr.drawCentreString("NVDA", 278, 24, 2);

        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.drawString("Swipe", 252, 48, 1);
        spr.drawString("DOWN", 252, 60, 2);

        spr.setTextColor(pal.text_dim, pal.card_bg);
        spr.drawString("for HUD", 252, 84, 1);
        spr.drawFastHLine(248, 102, 60, pal.primary);
        spr.drawString("STATION", 252, 112, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // THEME 1: MODERN GLASSMORPHIC MINIMAL
    // =========================================================================
    static void drawTheme1_MinimalModern(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        // Soft ambient background gradient glow circles
        spr.fillCircle(40, 30, 45, pal.card_bg);
        spr.fillCircle(280, 110, 55, pal.card_bg);

        // Main Glassmorphic Central Card [x=12..308, y=14..140]
        spr.fillRoundRect(12, 14, 296, 126, 10, pal.card_bg);
        spr.drawRoundRect(12, 14, 296, 126, 10, pal.primary);

        // Giant Main Clock Display
        char timeBuf[12];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.drawString(timeBuf, 28, 26, 7);

        // Seconds indicator
        char secBuf[8];
        snprintf(secBuf, sizeof(secBuf), ".%02d", t.tm_sec);
        spr.setTextColor(pal.secondary, pal.card_bg);
        spr.drawString(secBuf, 180, 56, 4);

        // Date String
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(pal.text_dim, pal.card_bg);
        spr.drawString(dateBuf, 30, 84, 2);

        // Weather side card
        WeatherGraphics::drawWeatherBadge(spr, 256, 42, weather.weather_code, weather.is_day, state.anim_frame, pal);

        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f C", weather.temperature);
        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.drawCentreString(tempBuf, 256, 68, 2);

        spr.setTextColor(pal.text_dim, pal.card_bg);
        spr.drawCentreString(weather.condition_text, 256, 86, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // THEME 2: 80s SYNTHWAVE HORIZON
    // =========================================================================
    static void drawTheme2_Synthwave80s(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        // 1. Rising Retro Neon Sun [cx=160, cy=70, r=36]
        int sunX = 160, sunY = 70, sunR = 34;
        spr.fillCircle(sunX, sunY, sunR, pal.highlight);

        // Sun horizontal scanline cutouts
        for (int i = sunY - 10; i < sunY + sunR; i += 4) {
            spr.drawFastHLine(sunX - sunR, i, sunR * 2, pal.bg_dark);
        }

        // Horizon line
        spr.drawFastHLine(0, 86, 320, pal.primary);

        // Perspective 3D Moving Grid Lines
        int animOffset = (state.anim_frame * 2) % 16;
        for (int y = 86; y < 148; y += (6 + (y - 86) / 6)) {
            int drawY = y + (animOffset * (y - 80)) / 40;
            if (drawY >= 86 && drawY < 148) {
                spr.drawFastHLine(0, drawY, 320, pal.card_bg);
            }
        }
        for (int x = -100; x <= 420; x += 35) {
            spr.drawLine(x, 148, 160 + (x - 160) / 4, 86, pal.card_bg);
        }

        // Central Retro Synthwave Clock Box [x=50..270, y=14..74]
        spr.fillRoundRect(50, 14, 220, 60, 8, pal.bg_dark);
        spr.drawRoundRect(50, 14, 220, 60, 8, pal.secondary);

        char timeBuf[12];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(timeBuf, 161, 24, 6);
        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString(timeBuf, 160, 23, 6);

        // Date banner
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString(dateBuf, 160, 96, 2);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // THEME 3: ORBITAL ASTRO SPACE DYNAMICS
    // =========================================================================
    static void drawTheme3_OrbitalAstro(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        if (!stars_inited) initStars();

        // 1. Render 3D Warping Starfield Particles
        for (int i = 0; i < 45; i++) {
            stars[i].z -= 1.8f;
            if (stars[i].z <= 1.0f) {
                stars[i].x = (float)(random(-160, 160));
                stars[i].y = (float)(random(-70, 70));
                stars[i].z = 100.0f;
            }
            int px = 160 + (int)(stars[i].x * 80.0f / stars[i].z);
            int py = 74 + (int)(stars[i].y * 80.0f / stars[i].z);
            if (px >= 0 && px < 320 && py >= 0 && py < 148) {
                uint16_t starColor = (stars[i].z < 40.0f) ? pal.highlight : pal.text_dim;
                spr.drawPixel(px, py, starColor);
            }
        }

        // 2. Orbital Ring with Revolving Seconds Satellite
        int cx = 160, cy = 72, r = 58;
        spr.drawCircle(cx, cy, r, pal.card_bg);
        spr.drawCircle(cx, cy, r - 1, pal.secondary);

        float angle = (t.tm_sec * 6.0f - 90.0f) * 0.0174533f;
        int satX = cx + (int)(cos(angle) * r);
        int satY = cy + (int)(sin(angle) * r);
        spr.fillCircle(satX, satY, 4, pal.highlight);

        // 3. Central Time String
        char timeBuf[12];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(timeBuf, 160, 52, 6);

        // Date text
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString(dateBuf, 160, 98, 1);

        // Weather badge on left
        WeatherGraphics::drawWeatherBadge(spr, 30, 48, weather.weather_code, weather.is_day, state.anim_frame, pal);
        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f C", weather.temperature);
        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString(tempBuf, 30, 72, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // OFFICIAL NVIDIA COMPANY LOGO & EMBLEM BADGE
    // =========================================================================
    static void drawNvidiaCompanyBadge(TFT_eSprite &spr, int x, int y) {
        // Outer Container Glass Badge (108x64 px)
        spr.fillRoundRect(x, y, 108, 64, 8, 0x0182); // Dark emerald tint card
        spr.drawRoundRect(x, y, 108, 64, 8, 0x75E0); // Official NVIDIA Green Border (#76B900)

        // NVIDIA Iconic Swirling Eye Contour (Vector Rendering)
        uint16_t nvGreen = 0x75E0;
        int iconX = x + 8;
        int iconY = y + 14;

        // Solid Green Icon Box (32x32 px)
        spr.fillRoundRect(iconX, iconY, 32, 32, 4, nvGreen);

        // White Inverted Eye Spiral Curve inside green icon box
        spr.fillCircle(iconX + 16, iconY + 16, 11, 0x0182);
        spr.fillCircle(iconX + 16, iconY + 16, 8, nvGreen);
        spr.fillRect(iconX + 4, iconY + 4, 12, 14, 0x0182);
        spr.fillCircle(iconX + 18, iconY + 14, 3, 0xFFFF);

        // "nvidia" Official Lowercase Typography
        spr.setTextColor(0xFFFF, 0x0182);
        spr.drawString("nvidia", x + 44, y + 14, 2);

        spr.setTextColor(nvGreen, 0x0182);
        spr.drawString("GEFORCE", x + 44, y + 34, 1);
    }

    // =========================================================================
    // NVIDIA STOCK POPUP OVERLAY (Interactive Slide-Down / Slide-Up)
    // =========================================================================
    static void drawStockPopup(TFT_eSprite &spr, const StockData &stock, AppState &state, uint32_t now, const ColorPalette &pal) {
        if (state.stock_hud_state == STOCK_HUD_CLOSED) return;

        int card_w = 296;
        int card_h = 146;
        int target_y = 12;
        int start_y = -card_h;
        int cur_y = target_y;

        const uint32_t anim_dur = 250; // 250ms smooth transition
        uint32_t elapsed = now - state.stock_hud_anim_start_ms;

        if (state.stock_hud_state == STOCK_HUD_SLIDING_DOWN) {
            if (elapsed >= anim_dur) {
                state.stock_hud_state = STOCK_HUD_OPEN;
                cur_y = target_y;
            } else {
                float t = (float)elapsed / (float)anim_dur;
                t = 1.0f - pow(1.0f - t, 3); // Cubic ease out
                cur_y = start_y + (int)(t * (target_y - start_y));
            }
        } else if (state.stock_hud_state == STOCK_HUD_SLIDING_UP) {
            if (elapsed >= anim_dur) {
                state.stock_hud_state = STOCK_HUD_CLOSED;
                return;
            } else {
                float t = (float)elapsed / (float)anim_dur;
                t = t * t * t; // Cubic ease in
                cur_y = target_y + (int)(t * (start_y - target_y));
            }
        } else { // STOCK_HUD_OPEN
            cur_y = target_y;
        }

        int x = 12;
        int y = cur_y;

        spr.fillRoundRect(x, y, card_w, card_h, 10, 0x0842);
        spr.drawRoundRect(x, y, card_w, card_h, 10, 0x75E0);
        spr.drawRoundRect(x + 1, y + 1, card_w - 2, card_h - 2, 9, 0x03E0);

        spr.fillRect(x + 10, y + 10, 6, 14, 0x75E0);
        spr.setTextColor(0xFFFF, 0x0842);
        spr.drawString("NVIDIA CORP · NVDA", x + 24, y + 10, 2);

        char priceBuf[24];
        snprintf(priceBuf, sizeof(priceBuf), "$%.2f", stock.price);
        spr.setTextColor(0xFFFF, 0x0842);
        spr.drawString(priceBuf, x + 16, y + 36, 6);

        uint16_t changeColor = (stock.change >= 0.0f) ? 0x07E0 : 0xF800;
        char changeBuf[32];
        snprintf(changeBuf, sizeof(changeBuf), "%s $%.2f (%+.2f%%)", 
            (stock.change >= 0.0f ? "[+]" : "[-]"), stock.change, stock.change_pct);
        
        spr.fillRoundRect(x + 16, y + 82, 148, 22, 4, (stock.change >= 0.0f ? 0x0200 : 0x4000));
        spr.setTextColor(changeColor, (stock.change >= 0.0f ? 0x0200 : 0x4000));
        spr.drawString(changeBuf, x + 22, y + 86, 2);

        // Draw Official NVIDIA Company Emblem Badge on Right Side
        drawNvidiaCompanyBadge(spr, x + 174, y + 10);

        // Sparkline Mini Chart under company badge
        int chart_x = x + 174;
        int chart_y = y + 80;
        int chart_w = 108;
        int chart_h = 34;
        spr.fillRoundRect(chart_x, chart_y, chart_w, chart_h, 4, 0x0182);
        spr.drawRoundRect(chart_x, chart_y, chart_w, chart_h, 4, 0x2124);

        if (stock.history_count > 1) {
            float minVal = stock.history[0];
            float maxVal = stock.history[0];
            for (int i = 1; i < stock.history_count; i++) {
                if (stock.history[i] < minVal) minVal = stock.history[i];
                if (stock.history[i] > maxVal) maxVal = stock.history[i];
            }
            float range = (maxVal - minVal > 0.01f) ? (maxVal - minVal) : 1.0f;

            for (int i = 0; i < stock.history_count - 1; i++) {
                int px1 = chart_x + (i * chart_w) / (stock.history_count - 1);
                int py1 = chart_y + chart_h - 4 - (int)(((stock.history[i] - minVal) / range) * (chart_h - 8));
                int px2 = chart_x + ((i + 1) * chart_w) / (stock.history_count - 1);
                int py2 = chart_y + chart_h - 4 - (int)(((stock.history[i + 1] - minVal) / range) * (chart_h - 8));
                spr.drawLine(px1, py1, px2, py2, changeColor);
                spr.drawLine(px1, py1 + 1, px2, py2 + 1, changeColor);
            }
        }

        spr.setTextColor(0x7BEF, 0x0842);
        spr.drawString("Swipe UP to Close", x + 16, y + 118, 1);
    }

    // =========================================================================
    // WI-FI SETUP SCREEN OVERLAY
    // =========================================================================
    static void drawWifiSetupScreen(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);
        spr.drawRoundRect(8, 8, 304, 154, 8, pal.primary);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("WI-FI SETUP PORTAL", 160, 16, 4);

        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString("1. Connect Phone/PC to AP:", 160, 48, 2);

        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.fillRoundRect(30, 64, 260, 22, 4, pal.card_bg);
        spr.drawCentreString(state.ap_ssid, 160, 67, 2);

        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString("2. Open Browser at:", 160, 94, 2);

        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.fillRoundRect(30, 110, 260, 22, 4, pal.card_bg);
        spr.drawCentreString(String("http://") + state.ap_ip, 160, 113, 2);

        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString("Press Button 2 to Exit", 160, 138, 1);
    }

    // =========================================================================
    // NOTIFICATION TOAST BANNER
    // =========================================================================
    static void drawBannerToast(TFT_eSprite &spr, const String &text, const ColorPalette &pal) {
        if (text.length() == 0) return;
        
        int text_w = text.length() * 8 + 32;
        int bx = 160 - (text_w / 2);
        int by = 8;
        spr.fillRoundRect(bx, by, text_w, 24, 6, pal.highlight);
        spr.setTextColor(pal.bg_dark, pal.highlight);
        spr.drawCentreString(text, 160, by + 4, 2);
    }
};

Themes::Star Themes::stars[45];
bool Themes::stars_inited = false;
