#pragma once
#include <Arduino.h>
#include "pin_config.h"
#include "data_models.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "TFT_eSPI.h"

class PowerManager {
public:
    static float current_brightness;
    static float target_brightness;

    static void init() {
        pinMode(PIN_LCD_BL, OUTPUT);
        current_brightness = 255.0f;
        target_brightness = 255.0f;
        setHardwareBrightness(255);
    }

    static void setHardwareBrightness(uint8_t val) {
        analogWrite(PIN_LCD_BL, val);
    }

    static void setTargetBrightness(uint8_t val) {
        target_brightness = (float)val;
    }

    static void registerActivity(AppState &state) {
        state.last_activity_ms = millis();
        setTargetBrightness(state.user_brightness_setting);
        state.is_dimmed = false;
    }

    static void update(AppState &state) {
        uint32_t now = millis();
        if (state.last_activity_ms == 0) state.last_activity_ms = now;

        // 1. Determine Target Brightness based on Power & Charging State
        if (state.battery_charging) {
            setTargetBrightness(state.user_brightness_setting);
            state.is_dimmed = false;
        } else {
            uint32_t inactive_ms = now - state.last_activity_ms;
            if (inactive_ms > 30000) {        // >30s -> ~5% brightness
                setTargetBrightness(min((uint8_t)15, state.user_brightness_setting));
                state.is_dimmed = true;
            } else if (inactive_ms > 15000) { // >15s -> ~30% brightness
                setTargetBrightness((uint8_t)(state.user_brightness_setting * 0.3f));
                state.is_dimmed = true;
            } else {                          // <15s -> User brightness setting
                setTargetBrightness(state.user_brightness_setting);
                state.is_dimmed = false;
            }
        }

        // 2. Smoothly Interpolate Current Brightness towards Target Brightness (~50 FPS)
        if (abs(current_brightness - target_brightness) > 0.5f) {
            float step = 7.5f; // Gradual smooth transition (~400ms across full 0-255 range)
            if (current_brightness < target_brightness) {
                current_brightness = min(target_brightness, current_brightness + step);
            } else {
                current_brightness = max(target_brightness, current_brightness - step);
            }
            uint8_t pwmVal = (uint8_t)current_brightness;
            setHardwareBrightness(pwmVal);
            state.backlight_brightness = pwmVal;
        }
    }

    static void enterDeepSleep(TFT_eSprite &spr, const ColorPalette &pal) {
        // 1. Draw Deep Sleep Toast / Overlay
        spr.fillSprite(TFT_BLACK);
        spr.drawRoundRect(20, 20, 280, 130, 10, pal.primary);
        spr.setTextColor(pal.highlight, TFT_BLACK);
        spr.drawCentreString("DEEP SLEEP MODE", 160, 45, 4);
        spr.setTextColor(pal.text_dim, TFT_BLACK);
        spr.drawCentreString("Release Button to Sleep...", 160, 95, 2);
        spr.pushSprite(0, 0);

        // 2. WAIT FOR BUTTON 1 TO BE RELEASED
        uint32_t waitStart = millis();
        while (digitalRead(PIN_BUTTON_1) == LOW && (millis() - waitStart < 5000)) {
            delay(10);
        }
        delay(150); // Debounce button release

        // 3. Smoothly fade backlight to 0 (blackout) before power off
        while (current_brightness > 0.0f) {
            current_brightness = max(0.0f, current_brightness - 10.0f);
            setHardwareBrightness((uint8_t)current_brightness);
            delay(15);
        }

        digitalWrite(PIN_LCD_BL, LOW);
        digitalWrite(PIN_POWER_ON, LOW);

        // 4. Configure RTC Pull-up & Ext0 Wakeup on PIN_BUTTON_1 (GPIO 0, active LOW)
        rtc_gpio_pullup_en((gpio_num_t)PIN_BUTTON_1);
        rtc_gpio_pulldown_dis((gpio_num_t)PIN_BUTTON_1);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON_1, 0);

        Serial.println("[Power] Button released. Entering ESP32-S3 Deep Sleep Mode...");
        Serial.flush();

        esp_deep_sleep_start();
    }
};

float PowerManager::current_brightness = 255.0f;
float PowerManager::target_brightness = 255.0f;
