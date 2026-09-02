#pragma once
#include <Arduino.h>
#include "TFT_eSPI.h"
#include "data_models.h"
#include "palettes.h"
#include "weather_graphics.h"

class Themes {
public:
    // Starfield state for Orbital Astro
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

    // Helper: Dynamic High-Contrast Sunlight Color Override
    static uint16_t getContrastColor(uint16_t defaultCol, const AppState &state) {
        if (state.is_dimmed || state.backlight_brightness < 80) {
            return 0xFFFF; // Pure White font for readability under ambient sunlight
        }
        return defaultCol;
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
    // PERSISTENT BOTTOM STATUS BAR (Always visible at y=148..170 across all Desks)
    // =========================================================================
    static void drawPersistentBottomBar(TFT_eSprite &spr, const AppState &state, const GeoData &geo, const ColorPalette &pal) {
        // Bottom bar container (320x22 px)
        spr.fillRect(0, 148, 320, 22, pal.bg_dark);
        spr.drawFastHLine(0, 148, 320, pal.primary);

        uint16_t textCol = getContrastColor(pal.primary, state);
        uint16_t highlightCol = getContrastColor(pal.highlight, state);

        // 1. Wi-Fi Signal & City Name (Left)
        drawWifiIcon(spr, 6, 153, state.wifi_rssi, highlightCol, pal.text_dim);
        spr.setTextColor(state.wifi_connected ? textCol : pal.text_dim, pal.bg_dark);
        spr.drawString(state.wifi_connected ? geo.city : "Offline", 22, 153, 1);

        // 2. Battery Telemetry (Center)
        drawBatteryIcon(spr, 118, 154, state.battery_pct, state.battery_charging, highlightCol, pal.bg_dark);

        // 3. Desk Indicator & Version / OTA Status (Right)
        if (state.ota_updating) {
            char otaBuf[16];
            snprintf(otaBuf, sizeof(otaBuf), "OTA %d%%", state.ota_progress_pct);
            spr.setTextColor(highlightCol, pal.bg_dark);
            spr.drawRightString(otaBuf, 314, 153, 1);
        } else {
            char deskBuf[24];
            const char *deskNames[3] = {"SETTING [1/3]", "MAIN [2/3]", "STOCKS [3/3]"};
            snprintf(deskBuf, sizeof(deskBuf), "%s", deskNames[state.current_desk % 3]);
            spr.setTextColor(getContrastColor(pal.text_dim, state), pal.bg_dark);
            spr.drawRightString(deskBuf, 314, 153, 1);
        }
    }

    // =========================================================================
    // DESK 0: SETTINGS DESK (Leftmost Screen)
    // =========================================================================
    static void drawDesk0_Settings(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // Title Header Card
        spr.fillRoundRect(8, 6, 304, 26, 6, pal.card_bg);
        spr.drawRoundRect(8, 6, 304, 26, 6, primCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString("SYSTEM SETTINGS & CONTROLS", 160, 11, 2);

        // Item 1: Brightness Telemetry Gauge (y=36..62)
        spr.fillRoundRect(8, 36, 304, 30, 6, pal.card_bg);
        spr.drawRoundRect(8, 36, 304, 30, 6, pal.secondary);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("Brightness:", 16, 43, 2);

        int barW = map(state.backlight_brightness, 0, 255, 0, 130);
        spr.drawRect(110, 44, 134, 14, primCol);
        spr.fillRect(112, 46, constrain(barW, 0, 130), 10, hiCol);

        char pwmBuf[12];
        snprintf(pwmBuf, sizeof(pwmBuf), "%d%%", (state.backlight_brightness * 100) / 255);
        spr.drawString(pwmBuf, 252, 43, 2);

        // Item 2: AP Web Setup Portal Button (y=70..96)
        spr.fillRoundRect(8, 70, 304, 32, 6, pal.card_bg);
        spr.drawRoundRect(8, 70, 304, 32, 6, (state.wifi_setup_mode ? hiCol : primCol));
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("[1] Wi-Fi & Stock Web Setup", 16, 78, 2);
        spr.setTextColor(state.wifi_setup_mode ? hiCol : dimCol, pal.card_bg);
        spr.drawRightString(state.wifi_setup_mode ? "ACTIVE" : "TAP TO LAUNCH", 300, 78, 2);

        // Item 3: GitHub OTA Check Button (y=106..138)
        spr.fillRoundRect(8, 106, 304, 36, 6, pal.card_bg);
        spr.drawRoundRect(8, 106, 304, 36, 6, pal.secondary);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("[2] Check OTA Firmware Update", 16, 111, 2);
        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawString(FIRMWARE_VERSION, 16, 126, 1);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawRightString(state.ota_updating ? "UPDATING..." : "CHECK NOW", 300, 116, 2);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // DESK 1: TIME & WEATHER DESK (Main Screen - Orbital Astro Theme)
    // =========================================================================
    static void drawDesk1_TimeWeather(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        if (!stars_inited) initStars();

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

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
                uint16_t starColor = (stars[i].z < 40.0f) ? hiCol : dimCol;
                spr.drawPixel(px, py, starColor);
            }
        }

        // 2. Orbital Ring with Revolving Seconds Satellite
        int cx = 160, cy = 68, r = 62;
        spr.drawCircle(cx, cy, r, pal.card_bg);
        spr.drawCircle(cx, cy, r - 1, pal.secondary);

        float angle = (t.tm_sec * 6.0f - 90.0f) * 0.0174533f;
        int satX = cx + (int)(cos(angle) * r);
        int satY = cy + (int)(sin(angle) * r);
        spr.fillCircle(satX, satY, 4, hiCol);

        // 3. Central Time & Seconds Display (HH:MM:SS) - Dead Center, Unobscured
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
        spr.setTextColor(primCol, pal.bg_dark);
        spr.drawCentreString(timeBuf, 160, 48, 6);

        // Date text
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(dateBuf, 160, 96, 1);

        // 4. LEFT SIDE WEATHER PANEL (x=4..84 - No overlap with central clock)
        WeatherGraphics::drawWeatherBadge(spr, 44, 24, weather.weather_code, weather.is_day, state.anim_frame, pal);
        
        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f C", weather.temperature);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(tempBuf, 44, 52, 2);

        char hlBuf[20];
        snprintf(hlBuf, sizeof(hlBuf), "H:%.0f L:%.0f", weather.temp_max, weather.temp_min);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(hlBuf, 44, 72, 1);

        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(weather.condition_text, 44, 88, 1);

        // 5. RIGHT SIDE WEATHER & LOCATION PANEL (x=236..316 - No overlap with central clock)
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(geo.city.length() > 0 ? geo.city : "City", 276, 28, 2);

        char humBuf[16];
        snprintf(humBuf, sizeof(humBuf), "HUM %d%%", weather.humidity);
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(humBuf, 276, 52, 1);

        char windBuf[16];
        snprintf(windBuf, sizeof(windBuf), "WND %.0fk", weather.wind_speed);
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(windBuf, 276, 70, 1);

        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(weather.is_day ? "DAYTIME" : "NIGHT", 276, 88, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // DESK 2: STOCK WATCHLIST DESK (Rightmost Screen - 2-Column Rolling)
    // =========================================================================
    static void drawDesk2_StockList(TFT_eSprite &spr, const StockData &stock, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // Title Header Card
        spr.fillRoundRect(8, 4, 304, 22, 4, pal.card_bg);
        spr.drawRoundRect(8, 4, 304, 22, 4, primCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString("WATCHLIST (2-COLUMN ROLLING)", 160, 8, 1);

        // Default 12 Stock Tickers (if network load pending)
        static const char *fallbackTickers[12] = {
            "NVDA", "AAPL", "INTC", "AMD",
            "MU",   "WDC",  "TSLA", "GOOG",
            "META", "AMZN", "MSFT", "SNDK"
        };
        static const float fallbackPrices[12] = {
            135.20f, 224.50f, 20.80f, 148.10f,
            96.40f,  68.30f,  210.60f, 165.40f,
            510.20f, 178.90f, 415.30f, 42.50f
        };
        static const float fallbackChanges[12] = {
            2.58f,  -0.45f, 1.20f,  3.10f,
            -1.15f, 0.85f,  -2.40f, 1.05f,
            4.20f,  -0.80f, 1.75f,  -1.30f
        };

        uint8_t totalCount = stock.count > 0 ? stock.count : 12;

        // Auto-scrolling rolling motion calculation (~50 FPS)
        int rowHeight = 26;
        int totalRows = (totalCount + 1) / 2; // 2 items per row
        int maxScrollY = max(0, (totalRows * rowHeight) - 110);
        
        static float scrollY = 0.0f;
        static bool scrollDown = true;
        if (maxScrollY > 0) {
            if (scrollDown) {
                scrollY += 0.3f;
                if (scrollY >= maxScrollY + 10) scrollDown = false;
            } else {
                scrollY -= 0.3f;
                if (scrollY <= -10) scrollDown = true;
            }
        } else {
            scrollY = 0.0f;
        }

        int startY = 30 - (int)scrollY;

        // Render 2 Columns of Stock Cards
        for (int i = 0; i < totalCount; i++) {
            int row = i / 2;
            int col = i % 2;
            int itemX = (col == 0) ? 8 : 164;
            int itemY = startY + (row * rowHeight);

            // Clip items outside visible list area (y=28..144)
            if (itemY < 24 || itemY > 140) continue;

            const char *symbol = stock.count > 0 ? stock.items[i].symbol.c_str() : fallbackTickers[i];
            float price = stock.count > 0 ? stock.items[i].price : fallbackPrices[i];
            float changePct = stock.count > 0 ? stock.items[i].change_pct : fallbackChanges[i];

            spr.fillRoundRect(itemX, itemY, 148, 24, 4, pal.card_bg);
            spr.drawRoundRect(itemX, itemY, 148, 24, 4, pal.primary);

            // Symbol
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString(symbol, itemX + 6, itemY + 5, 2);

            // Price
            char pBuf[16];
            snprintf(pBuf, sizeof(pBuf), "$%.1f", price);
            spr.setTextColor(dimCol, pal.card_bg);
            spr.drawString(pBuf, itemX + 60, itemY + 6, 1);

            // Change %
            uint16_t cCol = (changePct >= 0.0f) ? 0x07E0 : 0xF800;
            if (state.is_dimmed || state.backlight_brightness < 80) cCol = 0xFFFF;

            char cBuf[16];
            snprintf(cBuf, sizeof(cBuf), "%+%.1f%%", changePct);
            spr.setTextColor(cCol, pal.card_bg);
            spr.drawRightString(cBuf, itemX + 142, itemY + 6, 1);
        }

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
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
