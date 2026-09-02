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
    // =========================================================================
    // DESK 0: SETTINGS DESK (Leftmost Screen - Enhanced Controls)
    // =========================================================================
    static void drawDesk0_Settings(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // Title Header Card
        spr.fillRoundRect(8, 4, 304, 22, 4, pal.card_bg);
        spr.drawRoundRect(8, 4, 304, 22, 4, primCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString("SYSTEM SETTINGS & CONTROLS", 160, 8, 1);

        // Item 1: 0-100% Brightness Drag/Touch Slider (y=30..56)
        spr.fillRoundRect(8, 30, 304, 26, 4, pal.card_bg);
        spr.drawRoundRect(8, 30, 304, 26, 4, pal.secondary);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("Brightness:", 14, 35, 1);

        int bPct = map(state.user_brightness_setting, 0, 255, 0, 100);
        int barW = map(bPct, 0, 100, 0, 140);
        
        // Slider Track (x=100..240)
        spr.drawRect(100, 36, 144, 14, primCol);
        spr.fillRect(102, 38, constrain(barW, 0, 140), 10, hiCol);

        char bBuf[12];
        snprintf(bBuf, sizeof(bBuf), "%d%%", bPct);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawRightString(bBuf, 298, 35, 1);

        // Item 2: Split Row: Auto-Dimming (Left) & Low Battery Sleep (Right) (y=60..86)
        // Left: Auto Dimming (x=8..154)
        spr.fillRoundRect(8, 60, 146, 26, 4, pal.card_bg);
        spr.drawRoundRect(8, 60, 146, 26, 4, state.auto_dim_enabled ? hiCol : dimCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("Auto-Dim:", 14, 65, 1);
        spr.setTextColor(state.auto_dim_enabled ? 0x07E0 : dimCol, pal.card_bg);
        spr.drawRightString(state.auto_dim_enabled ? "ON" : "OFF", 146, 65, 1);

        // Right: Low Battery Deep Sleep Threshold (x=160..306)
        spr.fillRoundRect(160, 60, 152, 26, 4, pal.card_bg);
        spr.drawRoundRect(160, 60, 152, 26, 4, state.low_battery_sleep_pct > 0 ? hiCol : dimCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("LowBat Sleep:", 166, 65, 1);
        char sBuf[12];
        if (state.low_battery_sleep_pct == 0) snprintf(sBuf, sizeof(sBuf), "OFF");
        else snprintf(sBuf, sizeof(sBuf), "%d%%", state.low_battery_sleep_pct);
        spr.setTextColor(state.low_battery_sleep_pct > 0 ? 0x07E0 : dimCol, pal.card_bg);
        spr.drawRightString(sBuf, 304, 65, 1);

        // Item 3: AP Web Setup Portal Button (y=90..114)
        spr.fillRoundRect(8, 90, 304, 24, 4, pal.card_bg);
        spr.drawRoundRect(8, 90, 304, 24, 4, (state.wifi_setup_mode ? hiCol : primCol));
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("[1] Wi-Fi & Stock Web Setup", 14, 95, 1);
        spr.setTextColor(state.wifi_setup_mode ? hiCol : dimCol, pal.card_bg);
        spr.drawRightString(state.wifi_setup_mode ? "ACTIVE" : "LAUNCH", 298, 95, 1);

        // Item 4: GitHub OTA Check Button (y=118..142)
        spr.fillRoundRect(8, 118, 304, 24, 4, pal.card_bg);
        spr.drawRoundRect(8, 118, 304, 24, 4, pal.secondary);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawString("[2] Check OTA Firmware Update", 14, 123, 1);
        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawRightString(FIRMWARE_VERSION, 298, 123, 1);

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // OTA UPDATE CONFIRMATION DIALOG MODAL
    // =========================================================================
    static void drawOtaConfirmModal(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        // Semi-transparent dim background overlay
        spr.fillRoundRect(16, 12, 288, 132, 8, pal.bg_dark);
        spr.drawRoundRect(16, 12, 288, 132, 8, pal.highlight);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("FIRMWARE UPDATE FOUND", 160, 22, 2);

        char verBuf[40];
        snprintf(verBuf, sizeof(verBuf), "New Version: %s", state.ota_new_version.c_str());
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(verBuf, 160, 48, 2);

        bool canUpdate = (state.battery_charging || state.battery_pct > 30 || state.battery_pct == 0);

        if (!canUpdate) {
            spr.setTextColor(0xF800, pal.bg_dark);
            spr.drawCentreString("LOW BATTERY! Connect USB Charger", 160, 68, 1);

            // Greyed-out Disabled Confirm Button
            spr.fillRoundRect(30, 90, 112, 34, 6, 0x31A6);
            spr.drawRoundRect(30, 90, 112, 34, 6, 0x630C);
            spr.setTextColor(0x9492, 0x31A6);
            spr.drawCentreString("LOW BAT", 86, 99, 2);
        } else {
            spr.setTextColor(pal.text_dim, pal.bg_dark);
            spr.drawCentreString("Install update over Wi-Fi?", 160, 68, 1);

            // [ CONFIRM ] Green Button (x=30..142, y=90..124)
            spr.fillRoundRect(30, 90, 112, 34, 6, 0x03E0);
            spr.drawRoundRect(30, 90, 112, 34, 6, 0x07E0);
            spr.setTextColor(TFT_WHITE, 0x03E0);
            spr.drawCentreString("CONFIRM", 86, 99, 2);
        }

        // [ CANCEL ] Red Button (x=178..290, y=90..124)
        spr.fillRoundRect(178, 90, 112, 34, 6, 0x8000);
        spr.drawRoundRect(178, 90, 112, 34, 6, 0xF800);
        spr.setTextColor(TFT_WHITE, 0x8000);
        spr.drawCentreString("CANCEL", 234, 99, 2);
    }

    // =========================================================================
    // OTA FLASHING VISUAL PROGRESS BAR OVERLAY
    // =========================================================================
    static void drawOtaProgressBar(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);
        spr.drawRoundRect(12, 12, 296, 146, 8, pal.highlight);

        spr.setTextColor(pal.highlight, pal.bg_dark);
        spr.drawCentreString("FLASHING OTA FIRMWARE", 160, 24, 2);

        char statusBuf[48];
        snprintf(statusBuf, sizeof(statusBuf), "%s (%d%%)", state.ota_status_text.c_str(), state.ota_progress_pct);
        spr.setTextColor(pal.primary, pal.bg_dark);
        spr.drawCentreString(statusBuf, 160, 52, 2);

        // Full Visual Progress Bar Track (x=30..290)
        spr.drawRect(30, 80, 260, 24, pal.primary);
        int fillW = map(state.ota_progress_pct, 0, 100, 0, 256);
        spr.fillRect(32, 82, constrain(fillW, 0, 256), 20, pal.highlight);

        spr.setTextColor(pal.text_dim, pal.bg_dark);
        spr.drawCentreString("Do not power off or disconnect Wi-Fi", 160, 120, 1);
    }

    // =========================================================================
    // DESK 1: TIME & WEATHER DESK (Main Screen - Orbital Astro Cyber Glass)
    // =========================================================================
    static void drawDesk1_TimeWeather(TFT_eSprite &spr, const tm &t, const GeoData &geo, const WeatherData &weather, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        if (!stars_inited) initStars();

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // 1. Render 3D Warping Starfield Particles (Central Zone)
        for (int i = 0; i < 45; i++) {
            stars[i].z -= 1.8f;
            if (stars[i].z <= 1.0f) {
                stars[i].x = (float)(random(-160, 160));
                stars[i].y = (float)(random(-70, 70));
                stars[i].z = 100.0f;
            }
            int px = 160 + (int)(stars[i].x * 80.0f / stars[i].z);
            int py = 72 + (int)(stars[i].y * 80.0f / stars[i].z);
            if (px >= 78 && px <= 242 && py >= 0 && py < 144) {
                uint16_t starColor = (stars[i].z < 40.0f) ? hiCol : dimCol;
                spr.drawPixel(px, py, starColor);
            }
        }

        // 2. LEFT SIDE WEATHER INFO (Floating - No frame/bg)
        // Animated Weather Icon
        WeatherGraphics::drawWeatherBadge(spr, 42, 22, weather.weather_code, weather.is_day, state.anim_frame, pal);

        // Condition Text
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(weather.condition_text, 42, 48, 1);

        // Main Temperature
        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f", weather.temperature);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(tempBuf, 38, 64, 4);
        spr.setTextColor(pal.secondary, pal.bg_dark);
        spr.drawString("C", 62, 64, 2);

        // High / Low Temp Pills
        char maxBuf[10], minBuf[10];
        snprintf(maxBuf, sizeof(maxBuf), "^%.0f", weather.temp_max);
        snprintf(minBuf, sizeof(minBuf), "v%.0f", weather.temp_min);
        
        spr.fillRoundRect(12, 92, 60, 18, 4, pal.card_bg);
        spr.setTextColor(0x07E0, pal.card_bg); // Green up arrow
        spr.drawString(maxBuf, 16, 95, 1);

        spr.setTextColor(0x07FF, pal.card_bg); // Cyan down arrow
        spr.drawRightString(minBuf, 68, 95, 1);

        // Phase / Sun Status
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(weather.is_day ? "DAYTIME" : "NIGHT", 42, 118, 1);

        // 3. RIGHT SIDE ENVIRONMENT & TELEMETRY (Floating - No frame/bg)
        // City Header
        String shortCity = (geo.city.length() > 8) ? geo.city.substring(0, 8) : geo.city;
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(shortCity.length() > 0 ? shortCity : "LOCATION", 278, 12, 1);

        // Humidity Block
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString("HUMIDITY", 278, 30, 1);

        char humBuf[12];
        snprintf(humBuf, sizeof(humBuf), "%d%%", weather.humidity);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(humBuf, 278, 42, 2);

        // Mini Humidity Bar Track
        spr.drawRect(248, 60, 60, 6, primCol);
        int hW = map(constrain(weather.humidity, 0, 100), 0, 100, 0, 56);
        spr.fillRect(250, 62, hW, 2, hiCol);

        // Wind Speed Block
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString("WIND SPEED", 278, 74, 1);

        char windBuf[12];
        snprintf(windBuf, sizeof(windBuf), "%.0fk/h", weather.wind_speed);
        spr.setTextColor(hiCol, pal.bg_dark);
        spr.drawCentreString(windBuf, 278, 86, 2);

        // Mini Wind Gauge Bar Track
        spr.drawRect(248, 104, 60, 6, pal.secondary);
        int wW = map(constrain((int)weather.wind_speed, 0, 50), 0, 50, 0, 56);
        spr.fillRect(250, 106, wW, 2, pal.secondary);

        // 4. CENTRAL HOLOGRAPHIC CLOCK & DUAL ORBITAL RINGS (x=80..240)
        int cx = 160, cy = 66, r1 = 58, r2 = 52;
        // Outer dotted orbit ring
        spr.drawCircle(cx, cy, r1, pal.card_bg);
        // Inner glowing orbit ring
        spr.drawCircle(cx, cy, r2, primCol);

        // Revolving Seconds Satellite
        float angle = (t.tm_sec * 6.0f - 90.0f) * 0.0174533f;
        int satX = cx + (int)(cos(angle) * r2);
        int satY = cy + (int)(sin(angle) * r2);
        spr.drawCircle(satX, satY, 5, hiCol);
        spr.fillCircle(satX, satY, 3, hiCol);

        // Main Digital Clock (HH:MM) - Centered
        char mainTime[10];
        snprintf(mainTime, sizeof(mainTime), "%02d:%02d", t.tm_hour, t.tm_min);
        spr.setTextColor(primCol, pal.bg_dark);
        spr.drawCentreString(mainTime, 160, 36, 6);

        // Seconds Pill Capsule ( :SS )
        char secBuf[12];
        snprintf(secBuf, sizeof(secBuf), "%02d SEC", t.tm_sec);
        spr.fillRoundRect(134, 84, 52, 16, 8, pal.card_bg);
        spr.drawRoundRect(134, 84, 52, 16, 8, hiCol);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString(secBuf, 160, 87, 1);

        // Date Capsule
        char dateBuf[32];
        formatDate(t, dateBuf, sizeof(dateBuf));
        spr.setTextColor(dimCol, pal.bg_dark);
        spr.drawCentreString(dateBuf, 160, 110, 1);

        // 5. Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, geo, pal);
    }

    // =========================================================================
    // DESK 2: STOCK WATCHLIST DESK (Rightmost Screen - 2-Column Rolling)
    // =========================================================================
    static void drawDesk2_StockList(TFT_eSprite &spr, const StockData &stock, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t hiCol = getContrastColor(pal.highlight, state);

        // Default Stock Tickers (if network load pending)
        static const char *fallbackTickers[11] = {
            "NVDA", "AAPL", "INTC", "AMD",
            "MU",   "TSLA", "GOOG", "META",
            "AMZN", "MSFT", "SNDK"
        };
        static const float fallbackPrices[11] = {
            135.20f, 224.50f, 20.80f, 148.10f,
            96.40f,  210.60f, 165.40f, 510.20f,
            178.90f, 415.30f, 42.50f
        };
        static const float fallbackChanges[11] = {
            2.58f,  -0.45f, 1.20f,  3.10f,
            -1.15f, -2.40f, 1.05f,  4.20f,
            -0.80f, 1.75f,  -1.30f
        };

        uint8_t totalCount = stock.count > 0 ? stock.count : 11;

        // Auto-scrolling rolling motion calculation (~50 FPS)
        int rowHeight = 28;
        int totalRows = (totalCount + 1) / 2; // 2 items per row
        int maxScrollY = max(0, (totalRows * rowHeight) - 138);
        
        static float scrollY = 0.0f;
        static bool scrollDown = true;
        if (maxScrollY > 0) {
            if (scrollDown) {
                scrollY += 0.3f;
                if (scrollY >= maxScrollY + 8) scrollDown = false;
            } else {
                scrollY -= 0.3f;
                if (scrollY <= -8) scrollDown = true;
            }
        } else {
            scrollY = 0.0f;
        }

        int startY = 4 - (int)scrollY;

        // Render 2 Columns of Stock Cards
        for (int i = 0; i < totalCount; i++) {
            int row = i / 2;
            int col = i % 2;
            int itemX = (col == 0) ? 8 : 164;
            int itemY = startY + (row * rowHeight);

            // Clip items outside visible list area (y=2..144)
            if (itemY < -24 || itemY > 142) continue;

            const char *symbol = stock.count > 0 ? stock.items[i].symbol.c_str() : fallbackTickers[i];
            float price = stock.count > 0 ? stock.items[i].price : fallbackPrices[i];
            float change = stock.count > 0 ? stock.items[i].change : fallbackChanges[i];

            spr.fillRoundRect(itemX, itemY, 148, 26, 4, pal.card_bg);
            
            // Trend color: Green (rise) / Red (drop)
            uint16_t trendCol = (change >= 0.0f) ? 0x07E0 : 0xF800;
            if (state.is_dimmed || state.backlight_brightness < 80) trendCol = 0xFFFF;

            spr.drawRoundRect(itemX, itemY, 148, 26, 4, trendCol);

            // 1. Ticker Symbol (Bigger Font 2, Green/Red)
            spr.setTextColor(trendCol, pal.card_bg);
            spr.drawString(symbol, itemX + 6, itemY + 5, 2);

            // 2. Price (Bigger Font 2)
            char pBuf[16];
            snprintf(pBuf, sizeof(pBuf), "$%.1f", price);
            spr.setTextColor(hiCol, pal.card_bg);
            spr.drawString(pBuf, itemX + 54, itemY + 5, 2);

            // 3. Change $ (Replaced % with $ change, Bigger Font 2, Green/Red)
            char cBuf[16];
            if (change >= 0.0f) {
                snprintf(cBuf, sizeof(cBuf), "+$%.1f", change);
            } else {
                snprintf(cBuf, sizeof(cBuf), "-$%.1f", fabs(change));
            }
            spr.setTextColor(trendCol, pal.card_bg);
            spr.drawRightString(cBuf, itemX + 144, itemY + 5, 2);
        }

        // Render Persistent Bottom Status Bar
        drawPersistentBottomBar(spr, state, GeoData(), pal);
    }

    // =========================================================================
    // DESK 3: BLUETOOTH VIRTUAL 2D YOKE RC CONTROLLER (Far Right Desk)
    // =========================================================================
    static void drawDesk3_RcYoke(TFT_eSprite &spr, const AppState &state, const ColorPalette &pal) {
        spr.fillSprite(pal.bg_dark);

        uint16_t primCol = getContrastColor(pal.primary, state);
        uint16_t hiCol = getContrastColor(pal.highlight, state);
        uint16_t dimCol = getContrastColor(pal.text_dim, state);

        // 1. Center Yoke Controller Pad (cx = 160, cy = 68, R = 54)
        int cx = 160, cy = 68, r = 54;

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
        int knobX = cx + (int)((state.yoke_raw_x / 32767.0f) * (r - 12));
        int knobY = cy + (int)((state.yoke_raw_y / 32767.0f) * (r - 12));

        // Draw Connecting Telemetry Vector Line
        spr.drawLine(cx, cy, knobX, knobY, state.yoke_active ? hiCol : dimCol);

        // Draw 2D Thumbstick Knob (Double Neon Ring)
        uint16_t knobColor = state.yoke_active ? hiCol : primCol;
        spr.fillCircle(knobX, knobY, 10, pal.card_bg);
        spr.drawCircle(knobX, knobY, 10, knobColor);
        spr.drawCircle(knobX, knobY, 9, knobColor);
        spr.fillCircle(knobX, knobY, 4, knobColor);

        // 2. LEFT TELEMETRY PANEL (X-Axis 16-bit Readout) (x=8..86)
        spr.fillRoundRect(8, 12, 78, 122, 6, pal.card_bg);
        spr.drawRoundRect(8, 12, 78, 122, 6, primCol);

        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawCentreString("YOKE X", 47, 20, 1);

        char xBuf[16];
        snprintf(xBuf, sizeof(xBuf), "%+d", state.yoke_raw_x);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString(xBuf, 47, 36, 2);

        // Mini Horizontal Deflection Track (-32768..+32767)
        spr.drawRect(14, 62, 66, 8, primCol);
        int deflX = map(constrain((int)state.yoke_raw_x, -32768, 32767), -32768, 32767, 0, 62);
        spr.fillRect(16, 64, deflX, 4, hiCol);

        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawCentreString("16-BIT H-AXIS", 47, 78, 1);
        spr.setTextColor(state.yoke_active ? 0x07E0 : dimCol, pal.card_bg);
        spr.drawCentreString(state.yoke_active ? "ACTIVE" : "CENTER", 47, 94, 1);

        // 3. RIGHT TELEMETRY PANEL (Y-Axis 16-bit Readout) (x=234..312)
        spr.fillRoundRect(234, 12, 78, 122, 6, pal.card_bg);
        spr.drawRoundRect(234, 12, 78, 122, 6, pal.secondary);

        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawCentreString("YOKE Y", 273, 20, 1);

        char yBuf[16];
        snprintf(yBuf, sizeof(yBuf), "%+d", state.yoke_raw_y);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString(yBuf, 273, 36, 2);

        // Mini Vertical Deflection Track (-32768..+32767)
        spr.drawRect(240, 62, 66, 8, pal.secondary);
        int deflY = map(constrain((int)state.yoke_raw_y, -32768, 32767), -32768, 32767, 0, 62);
        spr.fillRect(242, 64, deflY, 4, hiCol);

        spr.setTextColor(dimCol, pal.card_bg);
        spr.drawCentreString("16-BIT V-AXIS", 273, 78, 1);
        spr.setTextColor(hiCol, pal.card_bg);
        spr.drawCentreString(state.ble_connected ? "BLE LINK" : "READY", 273, 94, 1);

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
