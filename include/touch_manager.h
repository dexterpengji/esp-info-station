#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "data_models.h"

// Include TouchLib implementation
#include "TouchLib.h"

class TouchManager {
private:
    TouchLib* touch = nullptr;
    bool initialized = false;
    uint8_t touch_i2c_addr = 0;

    // Gesture tracking state
    bool is_touching = false;
    int16_t start_x = 0;
    int16_t start_y = 0;
    int16_t last_x = 0;
    int16_t last_y = 0;
    uint32_t touch_start_time = 0;
    uint32_t last_touch_time = 0;

public:
    bool begin() {
        pinMode(PIN_TOUCH_RES, OUTPUT);
        digitalWrite(PIN_TOUCH_RES, LOW);
        delay(30);
        digitalWrite(PIN_TOUCH_RES, HIGH);
        delay(50);

        Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);

        // Probe I2C for touch device
        Wire.beginTransmission(0x15); // CST816 / CST820
        if (Wire.endTransmission() == 0) {
            touch_i2c_addr = 0x15;
            touch = new TouchLib(Wire, PIN_IIC_SDA, PIN_IIC_SCL, 0x15, PIN_TOUCH_RES);
            Serial.println("[Touch] Detected CST816 at 0x15");
        } else {
            Wire.beginTransmission(0x1A); // CST328
            if (Wire.endTransmission() == 0) {
                touch_i2c_addr = 0x1A;
                touch = new TouchLib(Wire, PIN_IIC_SDA, PIN_IIC_SCL, 0x1A, PIN_TOUCH_RES);
                Serial.println("[Touch] Detected CST328 at 0x1A");
            } else {
                Wire.beginTransmission(0x5A); // CST328 alternate
                if (Wire.endTransmission() == 0) {
                    touch_i2c_addr = 0x5A;
                    touch = new TouchLib(Wire, PIN_IIC_SDA, PIN_IIC_SCL, 0x5A, PIN_TOUCH_RES);
                    Serial.println("[Touch] Detected CST328 at 0x5A");
                } else {
                    // Default fallback
                    touch = new TouchLib(Wire, PIN_IIC_SDA, PIN_IIC_SCL, 0x15, PIN_TOUCH_RES);
                    Serial.println("[Touch] Probing fallback CST816");
                }
            }
        }

        if (touch && touch->init()) {
            initialized = true;
            touch->setRotation(1); // Set rotation for landscape 320x170
            Serial.println("[Touch] Touch screen successfully initialized!");
            return true;
        }

        Serial.println("[Touch] Warning: Touch init returned false, continuing...");
        return false;
    }

    GestureType processGestures(int16_t &outX, int16_t &outY) {
        if (!initialized || !touch) return GESTURE_NONE;

        bool touched = touch->read();
        uint32_t now = millis();
        GestureType detected = GESTURE_NONE;

        if (touched && touch->getPointNum() > 0) {
            TP_Point p = touch->getPoint(0);
            
            // Map coordinates if needed for 320x170 landscape
            int16_t cx = p.x;
            int16_t cy = p.y;
            
            // Boundary constraints
            if (cx < 0) cx = 0;
            if (cx > 320) cx = 320;
            if (cy < 0) cy = 0;
            if (cy > 170) cy = 170;

            outX = cx;
            outY = cy;

            if (!is_touching) {
                // Touch down event
                is_touching = true;
                start_x = cx;
                start_y = cy;
                last_x = cx;
                last_y = cy;
                touch_start_time = now;
                last_touch_time = now;
            } else {
                // Moving
                last_x = cx;
                last_y = cy;
                last_touch_time = now;
            }
        } else {
            // Touch released (or no touch)
            if (is_touching && (now - last_touch_time > 40)) {
                // Released! Analyze gesture
                int16_t dx = last_x - start_x;
                int16_t dy = last_y - start_y;
                uint32_t duration = last_touch_time - touch_start_time;

                outX = last_x;
                outY = last_y;

                if (duration < 550 && abs(dx) < 22 && abs(dy) < 22) {
                    detected = GESTURE_TAP;
                    Serial.printf("[Gesture] Single Tap detected at (%d, %d)\n", last_x, last_y);
                } else if (abs(dx) > 35 && abs(dx) > (int)(1.3f * abs(dy))) {
                    if (dx > 0) {
                        detected = GESTURE_SWIPE_RIGHT;
                        Serial.println("[Gesture] Horizontal Swipe Right");
                    } else {
                        detected = GESTURE_SWIPE_LEFT;
                        Serial.println("[Gesture] Horizontal Swipe Left");
                    }
                } else if (abs(dy) > 25 && abs(dy) > (int)(1.1f * abs(dx))) {
                    if (dy > 0) {
                        detected = GESTURE_SWIPE_DOWN;
                        Serial.println("[Gesture] Vertical Swipe Down");
                    } else {
                        detected = GESTURE_SWIPE_UP;
                        Serial.println("[Gesture] Vertical Swipe Up");
                    }
                }

                is_touching = false;
            }
        }

        return detected;
    }
};
