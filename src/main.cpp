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

// ST7789 Display & Sprite Buffer
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// Touch Manager
TouchManager touchMgr;

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

static const char* THEME_NAMES[4] = {
    "Cyberpunk HUD",
    "Modern Glass",
    "80s Synthwave",
    "Orbital Astro"
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

    // 5. Create 320x170 16-bit Double-Buffer Sprite
    spr.setColorDepth(16);
    if (!spr.createSprite(320, 170)) {
        Serial.println("[Display] Error: Failed to create 320x170 Sprite!");
    } else {
        Serial.println("[Display] 320x170 Sprite buffer allocated!");
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

    // 0b. Handle Button 2 (GPIO 14): Toggle Wi-Fi Setup Portal
    static uint32_t lastBtn2Press = 0;
    if (digitalRead(PIN_BUTTON_2) == LOW && (now - lastBtn2Press > 400)) {
        lastBtn2Press = now;
        NetworkService::lock();
        PowerManager::registerActivity(NetworkService::state);
        NetworkService::unlock();

        NetworkService::toggleWifiSetupPortal();
        Serial.println("[Button] Button 2 pressed: Toggled Wi-Fi Setup Portal");
    }

    // 1. Process Touch Input & Gestures
    int16_t touchX = 0, touchY = 0;
    GestureType gesture = touchMgr.processGestures(touchX, touchY);

    if (gesture != GESTURE_NONE) {
        NetworkService::lock();
        PowerManager::registerActivity(NetworkService::state);
        NetworkService::unlock();
    }

    NetworkService::lock();
    StockHUDState curStockState = NetworkService::state.stock_hud_state;
    NetworkService::unlock();

    if (gesture == GESTURE_SWIPE_DOWN) {
        NetworkService::lock();
        if (NetworkService::state.stock_hud_state == STOCK_HUD_CLOSED || NetworkService::state.stock_hud_state == STOCK_HUD_SLIDING_UP) {
            NetworkService::state.stock_hud_state = STOCK_HUD_SLIDING_DOWN;
            NetworkService::state.stock_hud_anim_start_ms = now;
            Serial.println("[UI] Action: Slide Down NVIDIA Stock HUD");
        }
        NetworkService::unlock();
    } else if (gesture == GESTURE_SWIPE_UP) {
        NetworkService::lock();
        if (NetworkService::state.stock_hud_state == STOCK_HUD_OPEN || NetworkService::state.stock_hud_state == STOCK_HUD_SLIDING_DOWN) {
            NetworkService::state.stock_hud_state = STOCK_HUD_SLIDING_UP;
            NetworkService::state.stock_hud_anim_start_ms = now;
            Serial.println("[UI] Action: Slide Up NVIDIA Stock HUD");
        } else {
            NetworkService::state.current_theme = (NetworkService::state.current_theme + 1) % 4;
            NetworkService::state.banner_text = String("Theme: ") + THEME_NAMES[NetworkService::state.current_theme];
            NetworkService::state.banner_until_ms = now + 1600;
            Serial.printf("[UI] Gesture Action: Changed Theme to %s\n", THEME_NAMES[NetworkService::state.current_theme]);
        }
        NetworkService::unlock();
    } else if (gesture == GESTURE_SWIPE_LEFT) {
        NetworkService::lock();
        NetworkService::state.current_palette = (NetworkService::state.current_palette + 1) % 6;
        NetworkService::state.banner_text = String("Palette: ") + PALETTES[NetworkService::state.current_palette].name;
        NetworkService::state.banner_until_ms = now + 1600;
        NetworkService::unlock();
        Serial.printf("[UI] Gesture Action: Changed Palette to %s\n", PALETTES[NetworkService::state.current_palette].name);
    } else if (gesture == GESTURE_SWIPE_RIGHT) {
        NetworkService::lock();
        NetworkService::state.current_palette = (NetworkService::state.current_palette + 5) % 6;
        NetworkService::state.banner_text = String("Palette: ") + PALETTES[NetworkService::state.current_palette].name;
        NetworkService::state.banner_until_ms = now + 1600;
        NetworkService::unlock();
        Serial.printf("[UI] Gesture Action: Changed Palette to %s\n", PALETTES[NetworkService::state.current_palette].name);
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

    // 3. Update Auto Dimming Power Manager
    PowerManager::update(localState);
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

    const ColorPalette &pal = PALETTES[localState.current_palette];

    // 5. Render Active Theme Layout or Wi-Fi Setup Screen
    if (localState.wifi_setup_mode) {
        Themes::drawWifiSetupScreen(spr, localState, pal);
    } else {
        switch (localState.current_theme) {
            case 0:
                Themes::drawTheme0_CyberHUD(spr, timeinfo, localGeo, localWeather, localState, pal);
                break;
            case 1:
                Themes::drawTheme1_MinimalModern(spr, timeinfo, localGeo, localWeather, localState, pal);
                break;
            case 2:
                Themes::drawTheme2_Synthwave80s(spr, timeinfo, localGeo, localWeather, localState, pal);
                break;
            case 3:
                Themes::drawTheme3_OrbitalAstro(spr, timeinfo, localGeo, localWeather, localState, pal);
                break;
        }
    }

    // 6. Render NVIDIA Stock Popup (Slide Down / Slide Up Interactive)
    if (localState.stock_hud_state != STOCK_HUD_CLOSED) {
        Themes::drawStockPopup(spr, localStock, localState, now, pal);
        NetworkService::lock();
        NetworkService::state.stock_hud_state = localState.stock_hud_state;
        NetworkService::unlock();
    }

    // 7. Render Toast Notification Banner
    if (localState.banner_text.length() > 0 && now < localState.banner_until_ms) {
        Themes::drawBannerToast(spr, localState.banner_text, pal);
    }

    // 8. Blit Sprite to Screen
    spr.pushSprite(0, 0);

    // 9. Increment Animation Frame Counter
    NetworkService::lock();
    NetworkService::state.anim_frame++;
    NetworkService::unlock();

    // Frame rate control (~50 FPS)
    uint32_t frame_time = millis() - now;
    if (frame_time < FRAME_DELAY_MS) {
        delay(FRAME_DELAY_MS - frame_time);
    }
}
