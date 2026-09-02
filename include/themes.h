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
            stars[i].x = (float)(random(-85, 85));
            stars[i].y = (float)(random(-140, 140));
            stars[i].z = (float)(random(1, 100));
        }
        stars_inited = true;
    }

    // Format Date: MM DD YYYY weekday (e.g. "Wed, Sep 02")
    static void formatDate(const tm &t, char *buf, size_t max_len) {
        strftime(buf, max_len, "%a, %b %d", &t);
    }

    // Helper: Dynamic High-Contrast Sunlight Color Override
    static uint16_t getContrastColor(uint16_t defaultCol, const AppState &state) {
        if (state.backlight_brightness == 0) {
            return 0xFFFF; // Pure White font only at brightness zero
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

        // Battery Shell (16x8 px)
        spr.drawRect(x, y, 16, 8, borderCol);
        spr.fillRect(x + 16, y + 2, 2, 4, borderCol);

        // Level Fill
        int fillW = map(pct, 0, 100, 0, 12);
        fillW = constrain(fillW, 0, 12);

        uint16_t fillCol = color;
        if (charging) fillCol = TFT_YELLOW;
        else if (pct <= 15) fillCol = alertColor;
        else if (pct <= 30) fillCol = TFT_ORANGE;

        if (fillW > 0) {
            spr.fillRect(x + 2, y + 2, fillW, 4, fillCol);
        }

        char pctBuf[8];
        if (charging) {
            snprintf(pctBuf, sizeof(pctBuf), "CHG");
        } else {
            snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
        }
        spr.setTextColor(borderCol, bgDark);
        spr.drawString(pctBuf, x + 21, y, 1);
    }

    // =========================================================================
    // PERSISTENT BOTTOM STATUS BAR (Always visible at y=298..320, 170x22 px)
    // =========================================================================
    static void drawPersistentBottomBar(TFT_eSprite &spr, const AppState &state, const GeoData &geo, const ColorPalette &pal) {
        // Bottom bar container (170x22 px)
        spr.fillRect(0, 298, 170, 22, pal.bg_dark);
        spr.drawFastHLine(0, 298, 170, pal.primary);

        uint16_t highlightCol = getContrastColor(pal.highlight, state);

        // 1. Wi-Fi Signal & Battery (Left)
        drawWifiIcon(spr, 4, 304, state.wifi_rssi, highlightCol, pal.text_dim);
        drawBatteryIcon(spr, 20, 305, state.battery_pct, state.battery_charging, highlightCol, pal.bg_dark);

        // 2. Desk Indicator Dots / Status (Right)
        if (state.ota_updating) {
            char otaBuf[16];
            snprintf(otaBuf, sizeof(otaBuf), "OTA %d%%", state.ota_progress_pct);
            spr.setTextColor(highlightCol, pal.bg_dark);
            spr.drawRightString(otaBuf, 166, 304, 1);
        } else {
            char deskBuf[16];
            snprintf(deskBuf, sizeof(deskBuf), "[%d/4]", (state.current_desk % 4) + 1);
            spr.setTextColor(getContrastColor(pal.text_dim, state), pal.bg_dark);
            spr.drawRightString(deskBuf, 166, 304, 1);
        }
    }

    // =========================================================================
    // DESK 0: SETTINGS DESK (Scrollable 1-Line Clean Controls, Bigger Font 2)
    // =========================================================================
    static void drawDesk0_Settings(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal, float scrollY = 0.0f) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // Header Title (y=4..24)
        spr.fillRoundRect(6, 4, 158, 20, 4, pal.card_bg);
        spr.drawRoundRect(6, 4, 158, 20, 4, primCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString("SYSTEM SETTINGS", 85, 7, 1);

        // Scrollable List Area (y=28..294)
        int itemH = 34;
        int startY = 28 - (int)scrollY;

        // Item 0: Brightness Slider (1-Line, Bigger Font 2)
        int y0 = startY;
        if (y0 >= -30 && y0 <= 290) {
            spr.fillRoundRect(6, y0, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y0, 158, 30, 4, pal.secondary);
            int bPct = map(state.user_brightness_setting, 0, 255, 0, 100);
            char bBuf[24];
            snprintf(bBuf, sizeof(bBuf), "Bright: %d%%", bPct);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString(bBuf, 10, y0 + 7, 2);

            // Mini track slider bar
            spr.drawRect(98, y0 + 10, 60, 10, primCol);
            int barW = map(bPct, 0, 100, 0, 56);
            spr.fillRect(100, y0 + 12, constrain(barW, 0, 56), 6, hiCol);
        }

        // Item 1: Auto-Dimming (1-Line, Bigger Font 2)
        int y1 = startY + itemH;
        if (y1 >= -30 && y1 <= 290) {
            spr.fillRoundRect(6, y1, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y1, 158, 30, 4, state.auto_dim_enabled ? hiCol : dimCol);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("Auto-Dim:", 10, y1 + 7, 2);
            spr.setTextColor(state.auto_dim_enabled ? 0x07E0 : dimCol, pal.card_bg);
            spr.drawRightString(state.auto_dim_enabled ? "ON" : "OFF", 158, y1 + 7, 2);
        }

        // Item 2: Low Battery Sleep Threshold (1-Line, Bigger Font 2)
        int y2 = startY + (itemH * 2);
        if (y2 >= -30 && y2 <= 290) {
            spr.fillRoundRect(6, y2, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y2, 158, 30, 4, state.low_battery_sleep_pct > 0 ? hiCol : dimCol);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("LowBat Sleep:", 10, y2 + 7, 2);
            char sBuf[12];
            if (state.low_battery_sleep_pct == 0) snprintf(sBuf, sizeof(sBuf), "OFF");
            else snprintf(sBuf, sizeof(sBuf), "%d%%", state.low_battery_sleep_pct);
            spr.setTextColor(state.low_battery_sleep_pct > 0 ? 0x07E0 : dimCol, pal.card_bg);
            spr.drawRightString(sBuf, 158, y2 + 7, 2);
        }

        // Item 3: Bluetooth Robot Controller Link (1-Line, Bigger Font 2)
        int y3 = startY + (itemH * 3);
        if (y3 >= -30 && y3 <= 290) {
            spr.fillRoundRect(6, y3, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y3, 158, 30, 4, state.ble_enabled ? hiCol : dimCol);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("Bluetooth RC:", 10, y3 + 7, 2);
            spr.setTextColor(state.ble_connected ? 0x07E0 : (state.ble_enabled ? hiCol : dimCol), pal.card_bg);
            spr.drawRightString(state.ble_connected ? "LINKED" : (state.ble_enabled ? "ON" : "OFF"), 158, y3 + 7, 2);
        }

        // Item 4: AP Web Setup Portal Button (1-Line, Bigger Font 2)
        int y4 = startY + (itemH * 4);
        if (y4 >= -30 && y4 <= 290) {
            spr.fillRoundRect(6, y4, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y4, 158, 30, 4, (state.wifi_setup_mode ? hiCol : primCol));
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("AP Web Setup:", 10, y4 + 7, 2);
            spr.setTextColor(state.wifi_setup_mode ? hiCol : dimCol, pal.card_bg);
            spr.drawRightString(state.wifi_setup_mode ? "ACTIVE" : "START", 158, y4 + 7, 2);
        }

        // Item 5: Check OTA Firmware Update (1-Line, Bigger Font 2)
        int y5 = startY + (itemH * 5);
        if (y5 >= -30 && y5 <= 290) {
            spr.fillRoundRect(6, y5, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y5, 158, 30, 4, pal.secondary);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("OTA Check:", 10, y5 + 7, 2);
            spr.setTextColor(dimCol, pal.card_bg);
            spr.drawRightString(FIRMWARE_VERSION, 158, y5 + 7, 2);
        }

        // Item 6: Color Palette Theme Cycle (1-Line, Bigger Font 2)
        int y6 = startY + (itemH * 6);
        if (y6 >= -30 && y6 <= 290) {
            spr.fillRoundRect(6, y6, 158, 30, 4, pal.card_bg);
            spr.drawRoundRect(6, y6, 158, 30, 4, hiCol);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString("Theme:", 10, y6 + 7, 2);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawRightString(pal.name, 158, y6 + 7, 2);
        }

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // OTA UPDATE CONFIRMATION DIALOG MODAL (Portrait Mode with Bottom Buttons)
    // =========================================================================
    static void drawOtaConfirmModal(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        // Overlay dialog container (x=8..162, y=40..280)
        spr.fillRoundRect(8, 40, 154, 240, 8, pal.bg_dark);
        spr.drawRoundRect(8, 40, 154, 240, 8, pal.highlight);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("OTA FIRMWARE", 85, 52, 2);
        spr.drawCentreString("UPDATE FOUND", 85, 70, 2);

        char verBuf[32];
        snprintf(verBuf, sizeof(verBuf), "%s Ready", state.ota_new_version.c_str());
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(verBuf, 85, 96, 2);

        bool canUpdate = (state.battery_charging || state.battery_pct > 30 || state.battery_pct == 0);

        if (!canUpdate) {
            spr.setTextColor(0xF800, pal.bg_dark);
            spr.drawCentreString("LOW BATTERY!", 85, 124, 2);
            spr.drawCentreString("Plug USB Charger", 85, 142, 1);
        } else {
            spr.setTextColor(pal.text_dim, pal.bg_dark);
            spr.drawCentreString("Install update now?", 85, 134, 1);
        }

        // 2 Buttons at the bottom of the screen modal dialog!
        // Top Button: [ CONFIRM ] (y=175..215)
        if (canUpdate) {
            spr.fillRoundRect(16, 175, 138, 38, 6, 0x03E0);
            spr.drawRoundRect(16, 175, 138, 38, 6, 0x07E0);
            spr.setTextColor(TFT_WHITE, 0x03E0);
            spr.drawCentreString("CONFIRM", 85, 185, 2);
        } else {
            spr.fillRoundRect(16, 175, 138, 38, 6, 0x31A6);
            spr.drawRoundRect(16, 175, 138, 38, 6, 0x630C);
            spr.setTextColor(0x9492, 0x31A6);
            spr.drawCentreString("LOW BAT", 85, 185, 2);
        }

        // Bottom Button: [ CANCEL ] (y=222..262)
        spr.fillRoundRect(16, 222, 138, 38, 6, 0x8000);
        spr.drawRoundRect(16, 222, 138, 38, 6, 0xF800);
        spr.setTextColor(TFT_WHITE, 0x8000);
        spr.drawCentreString("CANCEL", 85, 232, 2);
    }

    // =========================================================================
    // OTA FLASHING VISUAL PROGRESS BAR OVERLAY (Portrait 170x320)
    // =========================================================================
    static void drawOtaProgressBar(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);
        spr.drawRoundRect(8, 20, 154, 270, 8, pal.highlight);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("FLASHING OTA", 85, 40, 2);
        spr.drawCentreString("FIRMWARE", 85, 60, 2);

        char statusBuf[32];
        snprintf(statusBuf, sizeof(statusBuf), "%d%%", state.ota_progress_pct);
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(statusBuf, 85, 100, 6);

        // Visual Progress Bar Track
        spr.drawRect(18, 170, 134, 20, pal.primary);
        int fillW = map(state.ota_progress_pct, 0, 100, 0, 130);
        spr.fillRect(20, 172, constrain(fillW, 0, 130), 16, pal.highlight);

        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString("Keep USB connected", 85, 210, 1);
        spr.drawCentreString("Do not power off", 85, 228, 1);
    }

    // =========================================================================
    // DESK 1: MAIN TIME & WEATHER DESK (Vertical Portrait 170x320)
    // =========================================================================
    static void drawDesk1_TimeWeather(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        if (!stars_inited) initStars();

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // 1. Starfield particles
        for (int i = 0; i < 45; i++) {
            stars[i].z -= 1.8f;
            if (stars[i].z <= 1.0f) {
                stars[i].x = (float)(random(-85, 85));
                stars[i].y = (float)(random(-140, 140));
                stars[i].z = 100.0f;
            }
            int px = 85 + (int)(stars[i].x * 60.0f / stars[i].z);
            int py = 140 + (int)(stars[i].y * 60.0f / stars[i].z);
            if (px >= 4 && px <= 166 && py >= 4 && py < 294) {
                uint16_t starColor = (stars[i].z < 40.0f) ? hiCol : dimCol;
                spr.drawPixel(px, py, starColor);
            }
        }

        // 2. Location Header (y=4..20) with Animated Map Pin Icon
        String shortCity = (geo.city.length() > 10) ? geo.city.substring(0, 10) : geo.city;
        WeatherGraphics::drawAnimatedLocationPin(spr, 24, 7, state.anim_frame, hiCol);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(shortCity.length() > 0 ? shortCity : "LOCATION", 88, 4, 2);

        // 3. Central Digital Clock & Dual Orbital Rings (y=28..135)
        int cx = 85, cy = 76, r1 = 44, r2 = 38;
        spr.drawCircle(cx, cy, r1, pal.card_bg);
        spr.drawCircle(cx, cy, r2, primCol);

        // Revolving Seconds Satellite (Inner Orbit r2)
        float angle1 = (t.tm_sec * 6.0f - 90.0f) * 0.0174533f;
        int satX1 = cx + (int)(cos(angle1) * r2);
        int satY1 = cy + (int)(sin(angle1) * r2);
        spr.drawCircle(satX1, satY1, 4, hiCol);
        spr.fillCircle(satX1, satY1, 2, hiCol);

        // Micro Pulse Satellite (Outer Orbit r1, Counter-Rotating)
        float angle2 = -(state.anim_frame * 0.08f);
        int satX2 = cx + (int)(cos(angle2) * r1);
        int satY2 = cy + (int)(sin(angle2) * r1);
        spr.fillCircle(satX2, satY2, 2, primCol);

        // Digital Clock HH:MM
        char mainTime[10];
        snprintf(mainTime, sizeof(mainTime), "%02d:%02d", t.tm_hour, t.tm_min);
        spr.setTextColor(primCol, pal.bg_dark);
        spr.drawCentreString(mainTime, 85, 52, 6);

        // Seconds Pill
        char secBuf[12];
        snprintf(secBuf, sizeof(secBuf), "%02d SEC", t.tm_sec);
        spr.fillRoundRect(58, 92, 54, 14, 6, pal.card_bg);
        spr.drawRoundRect(58, 92, 54, 14, 6, hiCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString(secBuf, 85, 95, 1);

        // Date
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(dateBuf, 85, 114, 2);

        // 4. Weather Panel (y=138..292)
        WeatherGraphics::drawWeatherBadge(spr, 85, 148, weather.weather_code, weather.is_day, state.anim_frame, pal);

        // Temperature with Animated Mercury Thermometer Icon
        WeatherGraphics::drawAnimatedThermometer(spr, 28, 175, state.anim_frame, hiCol, primCol);
        char tempBuf[16];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f°C", weather.temperature);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(tempBuf, 94, 172, 4);

        // High / Low Temp
        char hlBuf[24];
        snprintf(hlBuf, sizeof(hlBuf), "H:%.0f°  L:%.0f°", weather.temp_max, weather.temp_min);
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(hlBuf, 85, 204, 2);

        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(weather.condition_text, 85, 226, 2);

        // Environmental Metrics with Animated Water Droplet & Windmill Icons
        WeatherGraphics::drawAnimatedDroplet(spr, 10, 252, state.anim_frame, hiCol);
        char humBuf[12];
        snprintf(humBuf, sizeof(humBuf), "%d%%", weather.humidity);
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawString(humBuf, 24, 254, 2);

        WeatherGraphics::drawAnimatedWindmill(spr, 98, 254, state.anim_frame, hiCol);
        char windBuf[16];
        snprintf(windBuf, sizeof(windBuf), "%.0fk/h", weather.wind_speed);
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawString(windBuf, 112, 254, 2);

        // 5. Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // DESK 2: STOCK WATCHLIST DESK (Single Column Vertical List, 170x320)
    // =========================================================================
    static void drawDesk2_StockList(TFT_eSprite &spr, const StockData &stock, const AppState &state, const ColorPalette &pal, float scrollY = 0.0f) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        int totalCount = (stock.count > 0) ? stock.count : 11;
        const char *fallbackTickers[11] = {"NVDA", "AAPL", "INTC", "AMD", "MU", "TSLA", "GOOG", "META", "AMZN", "MSFT", "SNDK"};
        float fallbackPrices[11] = {128.5f, 224.3f, 30.2f, 155.8f, 108.4f, 218.2f, 176.4f, 512.6f, 186.2f, 448.9f, 48.6f};
        float fallbackChanges[11] = {3.4f, -1.8f, -0.4f, 2.1f, -2.3f, 8.6f, 1.2f, -4.5f, 2.8f, 1.9f, -0.7f};

        int rowHeight = 34;
        int startY = 4 - (int)scrollY;

        // Render Single Column Stock Cards (x=6..164, width=158)
        for (int i = 0; i < totalCount; i++) {
            int itemY = startY + (i * rowHeight);

            // Clip items outside visible list area (y=4..294)
            if (itemY < -30 || itemY > 290) continue;

            const char *symbol = stock.count > 0 ? stock.items[i].symbol.c_str() : fallbackTickers[i];
            float price = stock.count > 0 ? stock.items[i].price : fallbackPrices[i];
            float change = stock.count > 0 ? stock.items[i].change : fallbackChanges[i];

            spr.fillRoundRect(6, itemY, 158, 30, 4, pal.card_bg);

            uint16_t trendCol = (change >= 0.0f) ? 0x07E0 : 0xF800;
            if (state.backlight_brightness == 0) trendCol = 0xFFFF;

            spr.drawRoundRect(6, itemY, 158, 30, 4, trendCol);

            // 1. Ticker Symbol (Bigger Font 2, Green/Red)
            spr.setTextColor(trendCol, pal.card_bg);
            spr.drawString(symbol, 12, itemY + 7, 2);

            // 2. Price (Bigger Font 2)
            char pBuf[16];
            snprintf(pBuf, sizeof(pBuf), "$%.1f", price);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString(pBuf, 66, itemY + 7, 2);

            // 3. Change $ (Bigger Font 2, Green/Red)
            char cBuf[16];
            if (change >= 0.0f) {
                snprintf(cBuf, sizeof(cBuf), "+$%.1f", change);
            } else {
                snprintf(cBuf, sizeof(cBuf), "-$%.1f", fabs(change));
            }
            spr.setTextColor(trendCol, pal.card_bg);
            spr.drawRightString(cBuf, 158, itemY + 7, 2);
        }

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // DESK 3: BLUETOOTH VIRTUAL 2D YOKE RC CONTROLLER (Vertical 170x320)
    // =========================================================================
    static void drawDesk3_RcYoke(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // Header Title (y=4..24)
        spr.fillRoundRect(6, 4, 158, 20, 4, pal.card_bg);
        spr.drawRoundRect(6, 4, 158, 20, 4, primCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString("ROBOT RC YOKE (50Hz)", 85, 7, 1);

        // 1. TOP TELEMETRY PANEL (X & Y Numbers Clean) (y=28..58, height 30)
        spr.fillRoundRect(6, 28, 158, 30, 6, pal.card_bg);
        spr.drawRoundRect(6, 28, 158, 30, 6, primCol);

        char telemetryBuf[32];
        snprintf(telemetryBuf, sizeof(telemetryBuf), "X:%+d  Y:%+d", state.yoke_raw_x, state.yoke_raw_y);
        spr.setTextColor(state.yoke_active ? hiCol : dimCol, pal.card_bg);
        spr.drawCentreString(telemetryBuf, 85, 35, 2);

        // 2. ACTIVE YOKE CONTROL ZONE BOUNDING BOX (x=10..160, y=62..226)
        uint16_t zoneBorder = state.yoke_active ? hiCol : pal.secondary;
        spr.fillRoundRect(10, 62, 150, 164, 8, pal.card_bg);
        spr.drawRoundRect(10, 62, 150, 164, 8, zoneBorder);

        // Zone Label
        spr.setTextColor(state.yoke_active ? hiCol : dimCol, pal.card_bg);
        spr.drawCentreString("ACTIVE YOKE ZONE", 85, 67, 1);

        // Center Joystick Pad (cx = 85, cy = 148, r = 52)
        int cx = 85, cy = 148, r = 52;

        // Outer Cyber Bounding Ring
        spr.drawCircle(cx, cy, r, pal.card_bg);
        spr.drawCircle(cx, cy, r - 1, primCol);

        // Crosshair Dotted Axes
        for (int x = cx - r + 8; x <= cx + r - 8; x += 6) {
            spr.drawPixel(x, cy, dimCol);
        }
        for (int y = cy - r + 8; y <= cy + r - 8; y += 6) {
            spr.drawPixel(cx, y, dimCol);
        }

        // Calculate Knob Coordinates from 16-bit signed telemetry (-32768..+32767)
        int knobX = cx + (int)((state.yoke_raw_x / 32767.0f) * (r - 10));
        int knobY = cy + (int)((state.yoke_raw_y / 32767.0f) * (r - 10));

        // Draw Connecting Telemetry Vector Line
        spr.drawLine(cx, cy, knobX, knobY, state.yoke_active ? hiCol : dimCol);

        // Draw 2D Thumbstick Knob
        uint16_t knobColor = state.yoke_active ? hiCol : primCol;
        spr.fillCircle(knobX, knobY, 10, pal.card_bg);
        spr.drawCircle(knobX, knobY, 10, knobColor);
        spr.drawCircle(knobX, knobY, 9, knobColor);
        spr.fillCircle(knobX, knobY, 4, knobColor);

        // 3. SAFE SWIPE GUIDANCE BAR (y=232..262, height 30)
        spr.fillRoundRect(6, 232, 158, 30, 6, pal.card_bg);
        spr.drawRoundRect(6, 232, 158, 30, 6, primCol);
        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawCentreString("Swipe outside zone to switch", 85, 239, 1);

        // 4. LINK STATUS (y=268..292)
        spr.setTextColor(state.ble_connected ? 0x07E0 : dimCol, pal.bg_dark);
        spr.drawCentreString(state.ble_connected ? "BLE LINK: CONNECTED" : "BLE BROADCASTING (50Hz)", 85, 272, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // WI-FI SETUP SCREEN OVERLAY (Portrait Mode 170x320)
    // =========================================================================
    static void drawWifiSetupScreen(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);
        spr.drawRoundRect(6, 12, 158, 290, 8, pal.primary);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("WI-FI SETUP", 85, 24, 4);

        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString("1. Connect Phone to:", 85, 70, 2);

        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.fillRoundRect(12, 92, 146, 28, 4, pal.card_bg);
        spr.drawCentreString(state.ap_ssid, 85, 98, 2);

        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString("2. Open Browser at:", 85, 140, 2);

        spr.setTextColor(pal.highlight, pal.card_bg);
        spr.fillRoundRect(12, 162, 146, 28, 4, pal.card_bg);
        spr.drawCentreString(state.ap_ip, 85, 168, 2);

        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString("Press Button 2 to Exit", 85, 240, 1);
    }

    // =========================================================================
    // NOTIFICATION TOAST BANNER
    // =========================================================================
    static void drawBannerToast(TFT_eSprite &spr, const String &text, const ColorPalette &pal) {
        if (text.length() == 0) return;
        
        int text_w = text.length() * 8 + 24;
        if (text_w > 162) text_w = 162;
        int bx = 85 - (text_w / 2);
        int by = 6;
        spr.fillRoundRect(bx, by, text_w, 24, 6, pal.highlight);
        spr.setTextColor(pal.bg_dark, pal.highlight);
        spr.drawCentreString(text, 85, by + 4, 2);
    }
};

Themes::Star Themes::stars[45];
bool Themes::stars_inited = false;
