#pragma once
#include <Arduino.h>

/* ESP32-S3 LilyGO T-Display-S3 Pin Definitions */

// Power & Backlight
#define PIN_POWER_ON                 15   // Set HIGH to power LCD and Touch peripherals
#define PIN_LCD_BL                   38   // LCD Backlight PWM

// ST7789 8-bit Parallel Interface
#define PIN_LCD_D0                   39
#define PIN_LCD_D1                   40
#define PIN_LCD_D2                   41
#define PIN_LCD_D3                   42
#define PIN_LCD_D4                   45
#define PIN_LCD_D5                   46
#define PIN_LCD_D6                   47
#define PIN_LCD_D7                   48

#define PIN_LCD_RES                  5
#define PIN_LCD_CS                   6
#define PIN_LCD_DC                   7
#define PIN_LCD_WR                   8
#define PIN_LCD_RD                   9

// Onboard Buttons
#define PIN_BUTTON_1                 0    // Boot button
#define PIN_BUTTON_2                 14   // User button

// Battery ADC
#define PIN_BAT_VOLT                 4

// Touch Screen I2C (CST816 / CST328)
#define PIN_IIC_SDA                  18
#define PIN_IIC_SCL                  17
#define PIN_TOUCH_INT                16
#define PIN_TOUCH_RES                21

// LCD Resolution
#define LCD_WIDTH                    320
#define LCD_HEIGHT                   170
