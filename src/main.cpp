#include <Arduino.h>
#include "pin_config.h"
#include "config.h"
#include "data_models.h"
#include "palettes.h"
#include "weather_graphics.h"
#include "themes.h"
#include "touch_manager.h"
#include "network_service.h"
#include "power_manager.h"
#include "bluetooth_service.h"
#include <TFT_eSPI.h>

// ST7789 Display & Double-Buffer Sprites for Parallax Carousel Slide Transitions (170x320 Portrait)
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite spr_next = TFT_eSprite(&tft);

// Touch Manager
TouchManager touchMgr;

// Desk Parallax Slide Transition State
static bool is_desk_animating = false;
static uint8_t desk_from = DESK_TIME_WEATHER;
static uint8_t desk_to = DESK_TIME_WEATHER;
static int anim_direction = 0; // -1 for Swipe Left (slide left), +1 for Swipe Right (slide right)
static uint32_t anim_start_ms = 0;
static const uint32_t ANIM_DURATION_MS = 280; // 280ms smooth cubic linear carousel slide

// Vertical Rolling Scroll State
static float settingScrollY = 0.0f;
static float stockScrollY = 0.0f;
static bool stockScrollDown = true;

// Hardware ST7789 Initialization commands
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

static const lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0x05}, 1},
    {0xB2, {0x0B, 0x0B, 0x00, 0x33, 0x33}, 5},
    {0xB7, {0x75}, 1},
    {0xBB, {0x28}, 1},
    {0xC0, {0x2C}, 1},
    {0xC2, {0x01}, 1},
    {0xC3, {0x1F}, 1},
    {0xC6, {0x13}, 1},
    {0xD0, {0xA7}, 1},
    {0xD0, {0xA4, 0xA1}, 2},
    {0xD6, {0xA1}, 1},
    {0xE0, {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32}, 14},
    {0xE1, {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37}, 14},
};

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n====================================");
    Serial.println("   LilyGO T-Display-S3 Info Station  ");
    Serial.println("====================================");

    // 1. Power on LCD & Touch Peripherals (GPIO 15)
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    delay(50);

    // 2. Initialize Power Manager (PWM Backlight on GPIO 38)
    PowerManager::init();

    // 3. Configure Hardware Buttons (Located at the bottom of the board in rotation 0!)
    pinMode(PIN_BUTTON_1, INPUT_PULLUP); // Boot / Sleep button
    pinMode(PIN_BUTTON_2, INPUT_PULLUP); // Wi-Fi Setup button

    // 4. Initialize ST7789 TFT in Vertical Portrait Orientation (Rotation 0)
    tft.begin();
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7F); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }
        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }
    tft.setRotation(0); // Vertical Portrait Mode (170x320), Buttons at bottom!
    tft.fillScreen(TFT_BLACK);

    // 5. Create Dual 170x320 16-bit Double-Buffer Sprites for Smooth Desk Slide Transitions
    spr.setColorDepth(16);
    spr_next.setColorDepth(16);
    if (!spr.createSprite(170, 320) || !spr_next.createSprite(170, 320)) {
        Serial.println("[Display] Error: Failed to allocate dual 170x320 Sprite buffers!");
    } else {
        Serial.println("[Display] Dual 170x320 Sprite buffers allocated!");
    }

    // 6. Initialize Touch Screen
    touchMgr.begin();

    // 7. Initialize Network Service
    NetworkService::init();

    // 8. Initialize Dedicated Real-Time FreeRTOS Bluetooth Robot Controller Service
    BluetoothService::init();
}

void loop() {
    uint32_t now = millis();

    // Check Hardware Buttons
    static bool btn1Pressed = false;
    static uint32_t btn1PressStart = 0;

    if (digitalRead(PIN_BUTTON_1) == LOW) {
        if (!btn1Pressed) {
            btn1Pressed = true;
            btn1PressStart = now;
        } else if (now - btn1PressStart > 2000) {
            // Long press (>2s) -> Enter Deep Sleep immediately
            btn1Pressed = false;
            Serial.println("[Power] Manual Deep Sleep triggered via Button 1 long press.");
            NetworkService::saveUserSettings();
            PowerManager::enterDeepSleep();
        }
    } else {
        if (btn1Pressed) {
            uint32_t pressDur = now - btn1PressStart;
            btn1Pressed = false;
            if (pressDur <= 1200) {
                // Short Press -> Reset backlight activity timer
                NetworkService::lock();
                PowerManager::registerActivity(NetworkService::state);
                NetworkService::unlock();
                Serial.println("[Button] Button 1 short press: Restored full brightness");
            }
        }
    }

    // 1. Process Touch Input & Gestures
    int16_t touchX = 0, touchY = 0;
    GestureType gesture = touchMgr.processGestures(touchX, touchY);

    if (gesture != GESTURE_NONE) {
        NetworkService::lock();
        PowerManager::registerActivity(NetworkService::state);
        NetworkService::unlock();
    }

    if (gesture == GESTURE_SWIPE_LEFT && !is_desk_animating) {
        NetworkService::lock();
        uint8_t oldDesk = NetworkService::state.current_desk;
        uint8_t newDesk = (oldDesk + 1) % 4;
        NetworkService::state.current_desk = newDesk;
        const char* deskNames[4] = {"Settings Desk", "Main Time & Weather", "Stock Watchlist", "2D Yoke RC Controller"};
        NetworkService::state.banner_text = deskNames[newDesk];
        NetworkService::state.banner_until_ms = now + 1400;
        NetworkService::unlock();

        desk_from = oldDesk;
        desk_to = newDesk;
        anim_direction = -1; // Sliding Left (Desk A -> Desk B)
        is_desk_animating = true;
        anim_start_ms = now;
        Serial.printf("[UI] Carousel Slide Left: Desk %d -> %d\n", oldDesk, newDesk);
    } else if (gesture == GESTURE_SWIPE_RIGHT && !is_desk_animating) {
        NetworkService::lock();
        uint8_t oldDesk = NetworkService::state.current_desk;
        uint8_t newDesk = (oldDesk + 3) % 4;
        NetworkService::state.current_desk = newDesk;
        const char* deskNames[4] = {"Settings Desk", "Main Time & Weather", "Stock Watchlist", "2D Yoke RC Controller"};
        NetworkService::state.banner_text = deskNames[newDesk];
        NetworkService::state.banner_until_ms = now + 1400;
        NetworkService::unlock();

        desk_from = oldDesk;
        desk_to = newDesk;
        anim_direction = 1; // Sliding Right (Desk B -> Desk A)
        is_desk_animating = true;
        anim_start_ms = now;
        Serial.printf("[UI] Carousel Slide Right: Desk %d -> %d\n", oldDesk, newDesk);
    } else if (gesture == GESTURE_TAP && !is_desk_animating) {
        NetworkService::lock();
        uint8_t curDesk = NetworkService::state.current_desk;
        bool isModal = NetworkService::state.ota_confirm_modal;
        NetworkService::unlock();

        if (isModal) {
            // Modal dialog 2 buttons at the bottom!
            if (touchY >= 175 && touchY <= 215) {
                // Touched Top Button [ CONFIRM ]
                Serial.println("[OTA Modal] User Confirmed! Launching OTA update...");
                NetworkService::performOtaUpdate();
            } else if (touchY >= 222 && touchY <= 262) {
                // Touched Bottom Button [ CANCEL ]
                Serial.println("[OTA Modal] User Cancelled OTA update.");
                NetworkService::lock();
                NetworkService::state.ota_confirm_modal = false;
                NetworkService::state.banner_text = "OTA Cancelled";
                NetworkService::state.banner_until_ms = now + 1500;
                NetworkService::unlock();
            }
        } else if (curDesk == DESK_SETTINGS) {
            // Scrollable 1-Line Clean Setting Items (y=28..290)
            if (touchY >= 28 && touchY <= 58) {
                // Item 0: Brightness Slider (x=98..158)
                int bPct = map(touchX, 98, 158, 0, 100);
                bPct = constrain(bPct, 0, 100);
                uint8_t pwmVal = (uint8_t)map(bPct, 0, 100, 0, 255);

                NetworkService::lock();
                NetworkService::state.user_brightness_setting = pwmVal;
                PowerManager::registerActivity(NetworkService::state);

                char bBuf[24];
                snprintf(bBuf, sizeof(bBuf), "Brightness: %d%%", bPct);
                NetworkService::state.banner_text = bBuf;
                NetworkService::state.banner_until_ms = now + 1200;
                NetworkService::unlock();
                NetworkService::saveUserSettings();
            } else if (touchY >= 62 && touchY <= 92) {
                // Item 1: Auto-Dimming
                NetworkService::lock();
                NetworkService::state.auto_dim_enabled = !NetworkService::state.auto_dim_enabled;
                bool enabled = NetworkService::state.auto_dim_enabled;
                PowerManager::registerActivity(NetworkService::state);
                NetworkService::state.banner_text = enabled ? "Auto-Dimming: ON" : "Auto-Dimming: OFF";
                NetworkService::state.banner_until_ms = now + 1400;
                NetworkService::unlock();
                NetworkService::saveUserSettings();
            } else if (touchY >= 96 && touchY <= 126) {
                // Item 2: Low Battery Sleep Threshold Cycle
                NetworkService::lock();
                uint8_t cur = NetworkService::state.low_battery_sleep_pct;
                if (cur == 10) cur = 15;
                else if (cur == 15) cur = 20;
                else if (cur == 20) cur = 0; // OFF
                else if (cur == 0) cur = 5;
                else cur = 10;

                NetworkService::state.low_battery_sleep_pct = cur;
                char sBuf[24];
                if (cur == 0) snprintf(sBuf, sizeof(sBuf), "LowBat Sleep: OFF");
                else snprintf(sBuf, sizeof(sBuf), "LowBat Sleep: %d%%", cur);
                NetworkService::state.banner_text = sBuf;
                NetworkService::state.banner_until_ms = now + 1400;
                NetworkService::unlock();
                NetworkService::saveUserSettings();
            } else if (touchY >= 130 && touchY <= 160) {
                // Item 3: Bluetooth RC Link Toggle
                NetworkService::lock();
                NetworkService::state.ble_enabled = !NetworkService::state.ble_enabled;
                bool enabled = NetworkService::state.ble_enabled;
                NetworkService::state.banner_text = enabled ? "Bluetooth RC: ON" : "Bluetooth RC: OFF";
                NetworkService::state.banner_until_ms = now + 1400;
                NetworkService::unlock();
                NetworkService::saveUserSettings();
            } else if (touchY >= 164 && touchY <= 194) {
                // Item 4: AP Web Setup Portal Button
                NetworkService::toggleWifiSetupPortal();
                NetworkService::lock();
                bool isSetup = NetworkService::state.wifi_setup_mode;
                NetworkService::state.banner_text = isSetup ? "AP Setup Active" : "Exited Setup";
                NetworkService::state.banner_until_ms = now + 1800;
                NetworkService::unlock();
            } else if (touchY >= 198 && touchY <= 228) {
                // Item 5: Check OTA Firmware Update
                NetworkService::checkForOtaUpdate();
                NetworkService::lock();
                NetworkService::state.banner_text = "Checking OTA Updates...";
                NetworkService::state.banner_until_ms = now + 2000;
                NetworkService::unlock();
            } else if (touchY >= 232 && touchY <= 262) {
                // Item 6: Color Palette Cycle
                NetworkService::lock();
                NetworkService::state.current_palette = (NetworkService::state.current_palette + 1) % 6;
                const char* palName = PALETTES[NetworkService::state.current_palette].name;
                NetworkService::state.banner_text = String("Theme: ") + palName;
                NetworkService::state.banner_until_ms = now + 1400;
                NetworkService::unlock();
                NetworkService::saveUserSettings();
            }
        }
    }

    // 2. Real-Time Continuous Touch Tracking for Desk 3 (2D Virtual Yoke RC Controller)
    if (touchMgr.isTouching() && !is_desk_animating) {
        NetworkService::lock();
        uint8_t curDesk = NetworkService::state.current_desk;
        if (curDesk == DESK_RC_YOKE) {
            // Check if touch is inside Active Yoke Control Zone Box (x=10..160, y=62..226)
            if (touchX >= 10 && touchX <= 160 && touchY >= 62 && touchY <= 226) {
                int dx = touchX - 85;
                int dy = touchY - 148;
                float dist = sqrtf((float)(dx * dx + dy * dy));
                float maxR = 46.0f; // Max deflection radius
                if (dist > maxR && dist > 0.0f) {
                    dx = (int)((dx / dist) * maxR);
                    dy = (int)((dy / dist) * maxR);
                }
                int16_t rawX = (int16_t)constrain((int)((dx / maxR) * 32767.0f), -32768, 32767);
                int16_t rawY = (int16_t)constrain((int)((dy / maxR) * 32767.0f), -32768, 32767);

                NetworkService::state.yoke_raw_x = rawX;
                NetworkService::state.yoke_raw_y = rawY;
                NetworkService::state.yoke_active = true;
                PowerManager::registerActivity(NetworkService::state);
            } else {
                // Safe Swipe Zone outside box: Force neutral (0, 0) to prevent accidental robot movement!
                NetworkService::state.yoke_raw_x = 0;
                NetworkService::state.yoke_raw_y = 0;
                NetworkService::state.yoke_active = false;
            }
        }
        NetworkService::unlock();
    } else if (!touchMgr.isTouching()) {
        NetworkService::lock();
        if (NetworkService::state.yoke_active) {
            // Spring back to center (0, 0)
            NetworkService::state.yoke_raw_x = 0;
            NetworkService::state.yoke_raw_y = 0;
            NetworkService::state.yoke_active = false;
        }
        NetworkService::unlock();
    }

    // 3. Safely Snapshot Data from Background Worker
    GeoData localGeo;
    WeatherData localWeather;
    StockData localStock;
    AppState localState;

    NetworkService::lock();
    localGeo = NetworkService::geo;
    localWeather = NetworkService::weather;
    localStock = NetworkService::stock;
    localState = NetworkService::state;
    NetworkService::unlock();

    // 4. Update Power Manager & Auto Dimming
    PowerManager::update(NetworkService::state);

    NetworkService::lock();
    localState.backlight_brightness = NetworkService::state.backlight_brightness;
    localState.is_dimmed = NetworkService::state.is_dimmed;
    NetworkService::unlock();

    // 5. Obtain Local Time (HH:MM:SS)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        timeinfo.tm_hour = 12;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = (now / 1000) % 60;
        timeinfo.tm_wday = 2;
        timeinfo.tm_mon = 8;
        timeinfo.tm_mday = 1;
        timeinfo.tm_year = 126;
    }

    // Color Palette Selection
    const ColorPalette &pal = PALETTES[localState.current_palette % 6];

    // Lambda helper to render a specific Desk into a target sprite buffer
    auto renderDeskToBuffer = [&](TFT_eSprite &targetSpr, uint8_t deskId) {
        switch (deskId) {
            case DESK_SETTINGS:
                Themes::drawDesk0_Settings(targetSpr, localState, pal, settingScrollY);
                break;
            case DESK_TIME_WEATHER:
                Themes::drawDesk1_TimeWeather(targetSpr, timeinfo, localGeo, localWeather, localState, pal);
                break;
            case DESK_STOCKS:
                Themes::drawDesk2_StockList(targetSpr, localStock, localState, pal, stockScrollY);
                break;
            case DESK_RC_YOKE:
                Themes::drawDesk3_RcYoke(targetSpr, localState, pal);
                break;
        }
    };

    // Auto-scroll Stock List vertically
    if (localState.current_desk == DESK_STOCKS) {
        int totalStocks = localStock.count > 0 ? localStock.count : 11;
        float maxScrollY = (totalStocks * 34) - 260;
        if (maxScrollY > 0) {
            if (stockScrollDown) {
                stockScrollY += 0.4f;
                if (stockScrollY >= maxScrollY + 12) stockScrollDown = false;
            } else {
                stockScrollY -= 0.4f;
                if (stockScrollY <= -12) stockScrollDown = true;
            }
        }
    }

    // 6. Render Active Screen or Linear Carousel Slide Transition Animation
    if (localState.ota_updating) {
        Themes::drawOtaProgressBar(spr, localState, pal);
        if (localState.banner_text.length() > 0 && now < localState.banner_until_ms) {
            Themes::drawBannerToast(spr, localState.banner_text, pal);
        }
        spr.pushSprite(0, 0);
    } else if (localState.wifi_setup_mode) {
        Themes::drawWifiSetupScreen(spr, localState, pal);
        if (localState.banner_text.length() > 0 && now < localState.banner_until_ms) {
            Themes::drawBannerToast(spr, localState.banner_text, pal);
        }
        spr.pushSprite(0, 0);
    } else if (is_desk_animating) {
        uint32_t elapsed = now - anim_start_ms;
        float progress = (float)elapsed / (float)ANIM_DURATION_MS;
        if (progress >= 1.0f) {
            progress = 1.0f;
            is_desk_animating = false;
        }

        // Smooth cubic ease-out curve for natural linear perspective carousel slide
        float ease = 1.0f - powf(1.0f - progress, 2.5f);
        int offset = (int)(ease * 170.0f);

        // Render outgoing desk into spr and incoming desk into spr_next
        renderDeskToBuffer(spr, desk_from);
        renderDeskToBuffer(spr_next, desk_to);

        // Render Toast Notification Banner on incoming sprite if active
        if (localState.banner_text.length() > 0 && now < localState.banner_until_ms) {
            Themes::drawBannerToast(spr_next, localState.banner_text, pal);
        }

        // Blit both sprites onto display with parallax horizontal offsets
        if (anim_direction < 0) { // Sliding Left (Desk A -> Desk B)
            spr.pushSprite(-offset, 0);
            spr_next.pushSprite(170 - offset, 0);
        } else { // Sliding Right (Desk B -> Desk A)
            spr.pushSprite(offset, 0);
            spr_next.pushSprite(-170 + offset, 0);
        }
    } else {
        renderDeskToBuffer(spr, localState.current_desk);

        // Render OTA Confirmation Modal Dialog if update found
        if (localState.ota_confirm_modal) {
            Themes::drawOtaConfirmModal(spr, localState, pal);
        }

        // Render Toast Notification Banner
        if (localState.banner_text.length() > 0 && now < localState.banner_until_ms) {
            Themes::drawBannerToast(spr, localState.banner_text, pal);
        }

        // Blit Sprite to Screen
        spr.pushSprite(0, 0);
    }

    // 7. Increment Animation Frame Counter
    NetworkService::lock();
    NetworkService::state.anim_frame++;
    NetworkService::unlock();

    // Frame rate control (~50 FPS)
    uint32_t frame_time = millis() - now;
    if (frame_time < FRAME_DELAY_MS) {
        delay(FRAME_DELAY_MS - frame_time);
    }
}
