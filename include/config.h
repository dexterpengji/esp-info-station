#pragma once

// Secrets Configuration
#if __has_include("secrets.h")
#include "secrets.h"
#else
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif
#endif

// Wi-Fi Configuration
#define WIFI_SSID "CMCC_2.4G"
#define WIFI_TIMEOUT_MS 15000

// API Endpoints
#define IP_GEO_API                                                             \
  "http://ip-api.com/json/"                                                    \
  "?fields=status,message,country,regionName,city,lat,lon,timezone,offset,"    \
  "query"
#define YAHOO_FINANCE_URL                                                      \
  "https://query1.finance.yahoo.com/v8/finance/chart/%s?interval=1d"
#define DEFAULT_STOCK_TICKERS                                                  \
  "NVDA,AAPL,INTC,AMD,MU,TSLA,GOOG,META,AMZN,MSFT,SNDK"

// NTP Time Servers
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.google.com"
#define NTP_SERVER3 "time.nist.gov"

// Sync Intervals (in milliseconds)
#define TIME_SYNC_INTERVAL_MS (3600 * 1000)       // 1 hour
#define WEATHER_SYNC_INTERVAL_MS (10 * 60 * 1000) // 10 minutes
#define STOCK_SYNC_INTERVAL_MS (60 * 1000)        // 1 minute

// Display & Animation Refresh
#define FPS_TARGET 50
#define FRAME_DELAY_MS (1000 / FPS_TARGET)

// Firmware & GitHub OTA Updates
#define FIRMWARE_VERSION "v1.3.3"
#define OTA_MANIFEST_URL                                                       \
  "https://raw.githubusercontent.com/dexterpengji/esp-info-station/main/"      \
  "version.json"
#define OTA_CHECK_INTERVAL_MS                                                  \
  (4 * 3600 * 1000) // Check for OTA update every 4 hours
