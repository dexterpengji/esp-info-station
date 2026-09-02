#pragma once
#include <Arduino.h>

struct GeoData {
    bool valid = false;
    String city = "Detecting...";
    String region = "";
    String country = "";
    float lat = 0.0f;
    float lon = 0.0f;
    int utc_offset_sec = 0;
    String timezone = "UTC";
};

struct WeatherData {
    bool valid = false;
    float temperature = 0.0f;
    float temp_max = 0.0f;
    float temp_min = 0.0f;
    int humidity = 0;
    float wind_speed = 0.0f;
    int weather_code = 0;
    String condition_text = "Fetching...";
    bool is_day = true;
};

struct StockQuote {
    String symbol = "";
    float price = 0.0f;
    float change = 0.0f;
    float change_pct = 0.0f;
    bool valid = false;
};

struct StockData {
    bool valid = false;
    StockQuote items[16];
    uint8_t count = 0;
    float scroll_offset = 0.0f;
};

enum DeskType {
    DESK_SETTINGS = 0,
    DESK_TIME_WEATHER = 1,
    DESK_STOCKS = 2,
    DESK_RC_YOKE = 3
};

enum GestureType {
    GESTURE_NONE = 0,
    GESTURE_TAP,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN
};

struct ColorPalette {
    const char* name;
    uint16_t primary;       // Main vibrant color (16-bit RGB565)
    uint16_t secondary;     // Accent bright color
    uint16_t bg_dark;       // Dark background
    uint16_t card_bg;       // Semi-dark card/frame background
    uint16_t text_dim;      // Subtle/secondary text
    uint16_t highlight;     // Bright highlights / neon glow
};

struct AppState {
    bool wifi_connected = false;
    int wifi_rssi = -100;
    String ip_address = "0.0.0.0";
    
    // Battery Telemetry
    float battery_voltage = 0.0f;   // Volts (e.g., 3.7V - 4.2V)
    uint8_t battery_pct = 0;        // 0 - 100%
    bool battery_charging = false;  // USB / charging status

    // 2D Virtual Yoke RC Controller Telemetry (16-bit signed X, Y)
    int16_t yoke_raw_x = 0;         // -32768 .. +32767
    int16_t yoke_raw_y = 0;         // -32768 .. +32767
    bool yoke_active = false;
    bool ble_enabled = true;
    bool ble_connected = false;

    // Wi-Fi Setup Portal & Stock Config
    bool wifi_setup_mode = false;
    String ap_ssid = "ESP-InfoStation-Setup";
    String ap_ip = "192.168.4.1";
    String configured_stock_tickers = "NVDA,AAPL,INTC,AMD,MU,TSLA,GOOG,META,AMZN,MSFT,SNDK";
    
    // Power Management & Brightness Settings
    uint32_t last_activity_ms = 0;
    uint8_t user_brightness_setting = 255; // 0..255 PWM (0..100%)
    uint8_t backlight_brightness = 255; // Active 0..255 PWM
    bool is_dimmed = false;
    bool auto_dim_enabled = true;      // Toggle auto dimming ON/OFF
    uint8_t low_battery_sleep_pct = 10; // Auto Deep Sleep threshold (0=OFF, 5, 10, 15, 20%)

    // Multi-Desk Navigation (Desk 0: Settings, Desk 1: Time & Weather, Desk 2: Stocks)
    uint8_t current_desk = DESK_TIME_WEATHER;
    uint8_t current_palette = 0;

    // GitHub OTA Firmware Update Status & Confirm Modal
    bool ota_confirm_modal = false;
    String ota_new_version = "";
    String ota_new_url = "";
    bool ota_updating = false;
    String ota_status_text = "";
    uint8_t ota_progress_pct = 0;

    // Banner & Animation
    String banner_text = "";
    uint32_t banner_until_ms = 0;
    uint32_t anim_frame = 0;
};
