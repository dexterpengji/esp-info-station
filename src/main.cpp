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
#include <TFT_eSPI.h>

// ST7789 Display & Double-Buffer Sprites for Parallax Carousel Slide Transitions
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

    // 3. Configure Hardware Buttons
    pinMode(PIN_BUTTON_1, INPUT_PULLUP); // Boot / Sleep button
    pinMode(PIN_BUTTON_2, INPUT_PULLUP); // Wi-Fi Setup button

    // 4. Initialize ST7789 TFT
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
    tft.setRotation(3); // 320x170 landscape
    tft.fillScreen(TFT_BLACK);

    // 5. Create Dual 320x170 16-bit Double-Buffer Sprites for Smooth Desk Slide Transitions
    spr.setColorDepth(16);
    spr_next.setColorDepth(16);
    if (!spr.createSprite(320, 170) || !spr_next.createSprite(320, 170)) {
        Serial.println("[Display] Error: Failed to allocate dual 320x170 Sprite buffers!");
    } else {
        Serial.println("[Display] Dual 320x170 Sprite buffers allocated!");
    }

    // 6. Initialize Touch Screen
    touchMgr.begin();

    // 7. Initialize Network Service
    NetworkService::init();

    // 8. Start Async Background Network Task on Core 0
    xTaskCreatePinnedToCore(
        NetworkService::backgroundTask,
        "NetWorker",
        8192,
        NULL,
        1,
        NULL,
        0 // Pinned to Core 0
    );

    // Register initial activity time
    NetworkService::lock();
    NetworkService::state.last_activity_ms = millis();
    NetworkService::unlock();

    Serial.println("[System] Setup complete, starting UI render loop on Core 1.");
}

void loop() {
    uint32_t now = millis();

    // 0a. Handle Button 1 (GPIO 0): Short press resets dimming, Long press (>1.2s) enters DEEP SLEEP
    static uint32_t btn1PressStart = 0;
    static bool btn1Pressed = false;

    if (digitalRead(PIN_BUTTON_1) == LOW) {
        if (!btn1Pressed) {
            btn1Pressed = true;
            btn1PressStart = now;
        } else if (now - btn1PressStart > 1200) {
            // Long Press -> Enter Deep Sleep
            NetworkService::lock();
            const ColorPalette &pal = PALETTES[NetworkService::state.current_palette];
            NetworkService::unlock();

            PowerManager::enterDeepSleep(spr, pal);
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
        uint8_t newDesk = (oldDesk + 1) % 3;
        NetworkService::state.current_desk = newDesk;
        const char* deskNames[3] = {"Settings Desk", "Main Time & Weather", "Stock Watchlist"};
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
        uint8_t newDesk = (oldDesk + 2) % 3;
        NetworkService::state.current_desk = newDesk;
        const char* deskNames[3] = {"Settings Desk", "Main Time & Weather", "Stock Watchlist"};
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

        int16_t screenX = 320 - touchX; // Converts raw touch sensor X to exact visual screen X coordinate

        if (isModal) {
            if (touchY >= 90 && touchY <= 124) {
                if (screenX >= 30 && screenX <= 142) {
                    // Touched Left Visual Button [ CONFIRM ] (x=30..142)
                    Serial.println("[OTA Modal] User Confirmed! Launching OTA update...");
                    NetworkService::performOtaUpdate();
                } else if (screenX >= 178 && screenX <= 290) {
                    // Touched Right Visual Button [ CANCEL ] (x=178..290)
                    Serial.println("[OTA Modal] User Cancelled OTA update.");
                    NetworkService::lock();
                    NetworkService::state.ota_confirm_modal = false;
                    NetworkService::state.banner_text = "OTA Cancelled";
                    NetworkService::state.banner_until_ms = now + 1500;
                    NetworkService::unlock();
                }
            }
        } else if (curDesk == DESK_SETTINGS) {
            if (touchY >= 28 && touchY <= 58) {
                // Item 1: 0-100% Brightness Drag/Touch Slider (x=100..240)
                int bPct = map(screenX, 100, 240, 0, 100);
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
                Serial.printf("[Settings] Touch Action: Set Brightness to %d%% (PWM %d)\n", bPct, pwmVal);
            } else if (touchY >= 60 && touchY <= 86) {
                // Item 2: Split Row
                if (screenX >= 8 && screenX <= 154) {
                    // Left Visual Button (x=8..154): Toggle Auto-Dimming
                    NetworkService::lock();
                    NetworkService::state.auto_dim_enabled = !NetworkService::state.auto_dim_enabled;
                    bool enabled = NetworkService::state.auto_dim_enabled;
                    PowerManager::registerActivity(NetworkService::state);
                    NetworkService::state.banner_text = enabled ? "Auto-Dimming: ON" : "Auto-Dimming: OFF";
                    NetworkService::state.banner_until_ms = now + 1400;
                    NetworkService::unlock();
                    Serial.printf("[Settings] Touch Action: Toggled Auto-Dimming to %s\n", enabled ? "ON" : "OFF");
                } else if (screenX >= 160 && screenX <= 306) {
                    // Right Visual Button (x=160..306): Low Battery Deep Sleep Threshold Cycle
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
                    Serial.printf("[Settings] Touch Action: Set LowBat Sleep Threshold to %d%%\n", cur);
                }
            } else if (touchY >= 90 && touchY <= 114) {
                // Item 3: AP Web Setup Portal Button
                NetworkService::toggleWifiSetupPortal();
                NetworkService::lock();
                bool isSetup = NetworkService::state.wifi_setup_mode;
                NetworkService::state.banner_text = isSetup ? "AP Setup Mode Active" : "Exited Setup Mode";
                NetworkService::state.banner_until_ms = now + 1800;
                NetworkService::unlock();
                Serial.println("[Settings] Touch Action: Toggled AP Web Setup Portal");
            } else if (touchY >= 118 && touchY <= 144) {
                // Item 4: Check OTA Firmware Update
                NetworkService::checkForOtaUpdate();
                NetworkService::lock();
                NetworkService::state.banner_text = "Checking GitHub OTA Updates...";
                NetworkService::state.banner_until_ms = now + 2000;
                NetworkService::unlock();
                Serial.println("[Settings] Touch Action: Checking OTA Updates");
            }
        }
    }

    // 2. Safely Snapshot Data from Background Worker
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

    const ColorPalette &pal = PALETTES[localState.current_palette];

    // 3. Update Auto Dimming & Low Battery Power Manager
    PowerManager::update(localState, spr, pal);
    NetworkService::lock();
    NetworkService::state.backlight_brightness = localState.backlight_brightness;
    NetworkService::state.is_dimmed = localState.is_dimmed;
    NetworkService::unlock();

    // 4. Obtain Local Time (HH:MM:SS)
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

    // Lambda helper to render a specific Desk into a target sprite buffer
    auto renderDeskToBuffer = [&](TFT_eSprite &targetSpr, uint8_t deskId) {
        switch (deskId) {
            case DESK_SETTINGS:
                Themes::drawDesk0_Settings(targetSpr, localState, pal);
                break;
            case DESK_TIME_WEATHER:
                Themes::drawDesk1_TimeWeather(targetSpr, timeinfo, localGeo, localWeather, localState, pal);
                break;
            case DESK_STOCKS:
                Themes::drawDesk2_StockList(targetSpr, localStock, localState, pal);
                break;
        }
    };

    // 5. Render Active Screen or Linear Carousel Slide Transition Animation
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
        int offset = (int)(ease * 320.0f);

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
            spr_next.pushSprite(320 - offset, 0);
        } else { // Sliding Right (Desk B -> Desk A)
            spr.pushSprite(offset, 0);
            spr_next.pushSprite(-320 + offset, 0);
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

    // 8. Increment Animation Frame Counter
    NetworkService::lock();
    NetworkService::state.anim_frame++;
    NetworkService::unlock();

    // Frame rate control (~50 FPS)
    uint32_t frame_time = millis() - now;
    if (frame_time < FRAME_DELAY_MS) {
        delay(FRAME_DELAY_MS - frame_time);
    }
}
