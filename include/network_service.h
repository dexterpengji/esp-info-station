#pragma once
#include "config.h"
#include "pin_config.h"
#include "data_models.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

class NetworkService {
public:
  static GeoData geo;
  static WeatherData weather;
  static StockData stock;
  static AppState state;
  static SemaphoreHandle_t dataMutex;

  static Preferences prefs;
  static WebServer webServer;
  static DNSServer dnsServer;
  static bool webServerRunning;

  static void init() {
    if (!dataMutex) {
      dataMutex = xSemaphoreCreateMutex();
    }
    analogReadResolution(12);
  }

  static void lock() {
    if (dataMutex)
      xSemaphoreTake(dataMutex, portMAX_DELAY);
  }

  static void unlock() {
    if (dataMutex)
      xSemaphoreGive(dataMutex);
  }

  // --- NVS Wi-Fi & Stock Credentials Helper ---
  static bool loadSavedWifiCredentials(String &ssid, String &pass) {
    prefs.begin("wifi_config", true);
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
    return (ssid.length() > 0);
  }

  static void saveWifiCredentials(const String &ssid, const String &pass) {
    prefs.begin("wifi_config", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    Serial.printf("[NVS] Saved Wi-Fi credentials for SSID: %s\n", ssid.c_str());
  }

  static bool loadSavedStockTickers(String &tickers) {
    prefs.begin("stock_config", true);
    tickers = prefs.getString("tickers", DEFAULT_STOCK_TICKERS);
    prefs.end();
    return (tickers.length() > 0);
  }

  static void saveStockTickers(const String &tickers) {
    prefs.begin("stock_config", false);
    prefs.putString("tickers", tickers);
    prefs.end();
    Serial.printf("[NVS] Saved Stock Watchlist: %s\n", tickers.c_str());
  }

  // --- Battery Telemetry ---
  static void updateBatteryTelemetry() {
    uint32_t raw_mv = analogReadMilliVolts(PIN_BAT_VOLT);
    float volt = (raw_mv * 2.0f) / 1000.0f; // 1:2 resistor voltage divider

    float clamped_v = volt;
    if (clamped_v < 3.3f) clamped_v = 3.3f;
    if (clamped_v > 4.2f) clamped_v = 4.2f;
    uint8_t pct = (uint8_t)(((clamped_v - 3.3f) / (4.2f - 3.3f)) * 100.0f);
    bool charging = (volt >= 4.25f);

    lock();
    state.battery_voltage = volt;
    state.battery_pct = pct;
    state.battery_charging = charging;
    unlock();
  }

  // --- Wi-Fi Setup & Stock Config Web Portal ---
  static void startWifiSetupPortal() {
    if (webServerRunning) return;

    lock();
    state.wifi_setup_mode = true;
    state.wifi_connected = false;
    unlock();

    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP-InfoStation-Setup");
    IPAddress apIP = WiFi.softAPIP();

    dnsServer.start(53, "*", apIP);

    webServer.on("/", HTTP_GET, []() {
      String savedSsid = "", savedPass = "", savedTickers = DEFAULT_STOCK_TICKERS;
      loadSavedWifiCredentials(savedSsid, savedPass);
      loadSavedStockTickers(savedTickers);

      String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>ESP InfoStation Setup</title>"
        "<style>"
        "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0d1117;color:#c9d1d9;margin:0;padding:20px;display:flex;justify-content:center;align-items:center;min-height:90vh;}"
        ".card{background:#161b22;padding:24px;border-radius:14px;box-shadow:0 8px 32px rgba(0,0,0,0.6);width:100%;max-width:380px;border:1px solid #30363d;}"
        "h2{color:#f85149;margin-top:0;text-align:center;font-size:22px;}"
        "label{font-size:13px;color:#8b949e;display:block;margin:14px 0 6px;text-transform:uppercase;letter-spacing:0.5px;}"
        "input,select{width:100%;padding:12px;border-radius:8px;border:1px solid #30363d;background:#0d1117;color:#fff;box-sizing:border-box;font-size:15px;}"
        "button{width:100%;padding:14px;background:#238636;color:#fff;border:none;border-radius:8px;font-weight:bold;font-size:16px;cursor:pointer;margin-top:20px;transition:background 0.2s;}"
        "button:hover{background:#2ea043;}"
        ".footer{text-align:center;font-size:12px;color:#484f58;margin-top:16px;}"
        "</style></head><body><div class='card'>"
        "<h2>ESP InfoStation</h2>"
        "<p style='text-align:center;font-size:14px;color:#8b949e;margin-bottom:20px;'>Configure Wi-Fi & Stock Watchlist</p>"
        "<form action='/save' method='POST'>"
        "<label>Wi-Fi SSID</label><input type='text' name='ssid' value='" + savedSsid + "' placeholder='Network Name' required>"
        "<label>Wi-Fi Password</label><input type='password' name='pass' value='" + savedPass + "' placeholder='Wi-Fi Password'>"
        "<label>Stock Watchlist (Comma Separated)</label><input type='text' name='tickers' value='" + savedTickers + "' placeholder='NVDA,AAPL,INTC,AMD,MU,TSLA' required>"
        "<button type='submit'>Save & Restart</button>"
        "</form><div class='footer'>LilyGo T-Display-S3 Portal</div></div></body></html>";
      webServer.send(200, "text/html", html);
    });

    webServer.on("/save", HTTP_POST, []() {
      String newSsid = webServer.arg("ssid");
      String newPass = webServer.arg("pass");
      String newTickers = webServer.arg("tickers");
      saveWifiCredentials(newSsid, newPass);
      saveStockTickers(newTickers);
      String html = "<!DOCTYPE html><html><body style='background:#0d1117;color:#58a6ff;font-family:sans-serif;text-align:center;padding-top:50px;'>"
        "<h2>Credentials & Watchlist Saved!</h2><p style='color:#c9d1d9;'>Restarting ESP InfoStation...</p></body></html>";
      webServer.send(200, "text/html", html);
      delay(1500);
      ESP.restart();
    });

    webServer.onNotFound([]() {
      webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
      webServer.send(302, "text/plain", "");
    });

    webServer.begin();
    webServerRunning = true;
    Serial.printf("[WiFi AP] Web Setup Portal started at http://%s\n", apIP.toString().c_str());
  }

  static void handleWebPortal() {
    if (webServerRunning) {
      dnsServer.processNextRequest();
      webServer.handleClient();
    }
  }

  static void stopWifiSetupPortal() {
    if (webServerRunning) {
      webServer.stop();
      dnsServer.stop();
      webServerRunning = false;
    }
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    lock();
    state.wifi_setup_mode = false;
    unlock();
  }

  static void toggleWifiSetupPortal() {
    lock();
    bool active = state.wifi_setup_mode;
    unlock();
    if (active) {
      stopWifiSetupPortal();
    } else {
      startWifiSetupPortal();
    }
  }

  static bool fetchGeoAndSyncTime() {
    if (WiFi.status() != WL_CONNECTED)
      return false;

    HTTPClient http;
    http.begin(IP_GEO_API);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error && doc["status"] == "success") {
        lock();
        geo.valid = true;
        geo.city = doc["city"] | "Unknown";
        geo.region = doc["regionName"] | "";
        geo.country = doc["country"] | "";
        geo.lat = doc["lat"] | 0.0f;
        geo.lon = doc["lon"] | 0.0f;
        geo.utc_offset_sec = doc["offset"] | 0;
        geo.timezone = doc["timezone"] | "UTC";
        unlock();

        Serial.printf("[Geo] %s, %s (%.2f, %.2f) Offset: %d sec\n",
                      geo.city.c_str(), geo.country.c_str(), geo.lat, geo.lon,
                      geo.utc_offset_sec);

        configTime(geo.utc_offset_sec, 0, NTP_SERVER1, NTP_SERVER2,
                   NTP_SERVER3);
        http.end();
        return true;
      }
    }
    http.end();
    return false;
  }

  static bool fetchWeather() {
    if (WiFi.status() != WL_CONNECTED)
      return false;

    float lat = 0.0f, lon = 0.0f;
    lock();
    lat = geo.lat;
    lon = geo.lon;
    unlock();

    char url[180];
    snprintf(url, sizeof(url),
             "http://api.open-meteo.com/v1/"
             "forecast?latitude=%.4f&longitude=%.4f&current_weather=true&"
             "hourly=relativehumidity_2m",
             lat, lon);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        float temp = doc["current_weather"]["temperature"] | 0.0f;
        float wind = doc["current_weather"]["windspeed"] | 0.0f;
        int code = doc["current_weather"]["weathercode"] | 0;
        int is_day = doc["current_weather"]["is_day"] | 1;

        int humidity = 50;
        JsonArray humArr = doc["hourly"]["relativehumidity_2m"];
        if (humArr.size() > 0) {
          humidity = humArr[0] | 50;
        }

        String cond = "Sunny";
        if (code == 0)
          cond = (is_day ? "Clear Sky" : "Starry Night");
        else if (code >= 1 && code <= 3)
          cond = "Partly Cloudy";
        else if (code >= 45 && code <= 48)
          cond = "Foggy";
        else if (code >= 51 && code <= 55)
          cond = "Drizzle";
        else if (code >= 61 && code <= 67)
          cond = "Rain";
        else if (code >= 71 && code <= 77)
          cond = "Snow";
        else if (code >= 80 && code <= 82)
          cond = "Rain Showers";
        else if (code >= 95 && code <= 99)
          cond = "Thunderstorm";

        lock();
        weather.valid = true;
        weather.temperature = temp;
        weather.humidity = humidity;
        weather.wind_speed = wind;
        weather.weather_code = code;
        weather.condition_text = cond;
        weather.is_day = (is_day != 0);
        unlock();

        Serial.printf("[Weather] %s: %.1f C, Hum: %d%%, Code: %d\n",
                      cond.c_str(), temp, humidity, code);
        http.end();
        return true;
      }
    }
    http.end();
    return false;
  }

  static bool fetchStockWatchlist() {
    if (WiFi.status() != WL_CONNECTED)
      return false;

    String tickerStr = DEFAULT_STOCK_TICKERS;
    loadSavedStockTickers(tickerStr);

    WiFiClientSecure client;
    client.setInsecure();

    uint8_t count = 0;
    int startIdx = 0;
    while (startIdx < tickerStr.length() && count < 16) {
      int commaIdx = tickerStr.indexOf(',', startIdx);
      if (commaIdx == -1) commaIdx = tickerStr.length();
      
      String sym = tickerStr.substring(startIdx, commaIdx);
      sym.trim();
      sym.toUpperCase();

      if (sym.length() > 0) {
        char url[180];
        snprintf(url, sizeof(url), YAHOO_FINANCE_URL, sym.c_str());

        HTTPClient https;
        if (https.begin(client, url)) {
          https.addHeader("User-Agent", "Mozilla/5.0");
          https.setTimeout(4000);
          int httpCode = https.GET();

          if (httpCode == HTTP_CODE_OK) {
            String payload = https.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
              JsonObject meta = doc["chart"]["result"][0]["meta"];
              float currentPrice = meta["regularMarketPrice"] | 0.0f;
              float prevClose = meta["chartPreviousClose"] | currentPrice;
              float change = currentPrice - prevClose;
              float changePct = (prevClose > 0.0f) ? (change / prevClose * 100.0f) : 0.0f;

              lock();
              state.configured_stock_tickers = tickerStr;
              stock.items[count].symbol = sym;
              stock.items[count].price = currentPrice;
              stock.items[count].change = change;
              stock.items[count].change_pct = changePct;
              stock.items[count].valid = true;
              unlock();

              count++;
            }
          }
          https.end();
        }
      }
      startIdx = commaIdx + 1;
    }

    if (count > 0) {
      lock();
      stock.valid = true;
      stock.count = count;
      unlock();
      Serial.printf("[Stock] Watchlist updated with %d tickers.\n", count);
      return true;
    }
    return false;
  }

  // --- Automatic GitHub OTA Firmware Updater ---
  static void checkForOtaUpdate() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("[OTA] Checking GitHub for firmware updates...");
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate validation for GitHub asset download

    if (http.begin(client, OTA_MANIFEST_URL)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
          const char* remoteVersion = doc["version"] | "";
          const char* downloadUrl = doc["url"] | "";

          Serial.printf("[OTA] Local Version: %s | Remote Version: %s\n", FIRMWARE_VERSION, remoteVersion);

          if (String(remoteVersion).length() > 0 && String(remoteVersion) != String(FIRMWARE_VERSION)) {
            Serial.printf("[OTA] New Firmware available! Updating to %s from %s\n", remoteVersion, downloadUrl);

            lock();
            state.ota_updating = true;
            state.banner_text = String("OTA Update: ") + remoteVersion;
            state.banner_until_ms = millis() + 60000;
            unlock();

            httpUpdate.onProgress([](int cur, int total) {
              int pct = (cur * 100) / total;
              lock();
              state.ota_progress_pct = pct;
              unlock();
              Serial.printf("[OTA Progress] %d%%\n", pct);
            });

            t_httpUpdate_return ret = httpUpdate.update(client, downloadUrl);

            switch (ret) {
              case HTTP_UPDATE_FAILED:
                Serial.printf("[OTA Error] HTTP Update Failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                lock();
                state.ota_updating = false;
                state.banner_text = "OTA Failed!";
                state.banner_until_ms = millis() + 4000;
                unlock();
                break;
              case HTTP_UPDATE_NO_UPDATES:
                Serial.println("[OTA] No updates available.");
                lock();
                state.ota_updating = false;
                unlock();
                break;
              case HTTP_UPDATE_OK:
                Serial.println("[OTA] Update Success! Restarting...");
                ESP.restart();
                break;
            }
          } else {
            Serial.println("[OTA] Firmware is up to date.");
          }
        }
      }
      http.end();
    }
  }

  // Core 0 background worker task
  static void backgroundTask(void *pvParameters) {
    Serial.println("[Task] Background Network Worker started on Core 0");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    uint32_t lastWeather = 0;
    uint32_t lastStock = 0;
    uint32_t lastTimeSync = 0;
    uint32_t lastOtaCheck = 0;
    uint32_t lastReconnect = 0;
    uint32_t lastBatteryCheck = 0;
    uint8_t failedAttempts = 0;
    uint8_t current_ssid_idx = 0;
    const char *target_ssids[] = {WIFI_SSID, "CMCC_2.4", "CMCC_2.4G"};

    while (true) {
      uint32_t now = millis();

      // Periodic Battery Telemetry Sampling (every 2s)
      if (now - lastBatteryCheck > 2000) {
        lastBatteryCheck = now;
        updateBatteryTelemetry();
      }

      // Handle Wi-Fi Setup Mode
      bool isSetupMode = false;
      lock();
      isSetupMode = state.wifi_setup_mode;
      unlock();

      if (isSetupMode) {
        handleWebPortal();
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }

      if (WiFi.status() == WL_CONNECTED) {
        failedAttempts = 0;
        lock();
        state.wifi_connected = true;
        state.ip_address = WiFi.localIP().toString();
        state.wifi_rssi = WiFi.RSSI();
        unlock();

        // 1. Initial or periodic Geo & NTP sync
        if (!geo.valid || (now - lastTimeSync > TIME_SYNC_INTERVAL_MS)) {
          if (fetchGeoAndSyncTime()) {
            lastTimeSync = now;
          }
        }

        // 2. Periodic Weather sync
        if (!weather.valid || (now - lastWeather > WEATHER_SYNC_INTERVAL_MS)) {
          if (fetchWeather()) {
            lastWeather = now;
          }
        }

        // 3. Periodic Stock Watchlist sync
        if (!stock.valid || (now - lastStock > STOCK_SYNC_INTERVAL_MS)) {
          if (fetchStockWatchlist()) {
            lastStock = now;
          }
        }

        // 4. Periodic GitHub OTA check (on boot & every 4 hours)
        if (lastOtaCheck == 0 || (now - lastOtaCheck > OTA_CHECK_INTERVAL_MS)) {
          lastOtaCheck = now;
          checkForOtaUpdate();
        }
      } else {
        lock();
        state.wifi_connected = false;
        unlock();

        if (now - lastReconnect > 6000) {
          lastReconnect = now;
          failedAttempts++;

          String savedSsid = "", savedPass = "";
          if (loadSavedWifiCredentials(savedSsid, savedPass)) {
            Serial.printf("[WiFi] Connecting to saved SSID %s...\n", savedSsid.c_str());
            WiFi.begin(savedSsid.c_str(), savedPass.c_str());
          } else {
            const char *ssid = target_ssids[current_ssid_idx];
            current_ssid_idx = (current_ssid_idx + 1) % 3;
            Serial.printf("[WiFi] Connecting to %s...\n", ssid);
            WiFi.begin(ssid, WIFI_PASSWORD);
          }

          if (failedAttempts >= 5) {
            Serial.println("[WiFi] Failed multiple connection attempts. Starting Setup Portal...");
            startWifiSetupPortal();
          }
        }
      }

      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
};

GeoData NetworkService::geo;
WeatherData NetworkService::weather;
StockData NetworkService::stock;
AppState NetworkService::state;
SemaphoreHandle_t NetworkService::dataMutex = NULL;
Preferences NetworkService::prefs;
WebServer NetworkService::webServer(80);
DNSServer NetworkService::dnsServer;
bool NetworkService::webServerRunning = false;
