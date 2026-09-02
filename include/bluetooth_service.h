#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "data_models.h"
#include "network_service.h"

// UUIDs for 16-Bit Dual-Axis Robot RC Yoke Service & Telemetry Characteristic
#define RC_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define RC_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class BluetoothService {
private:
    static BLEServer* pServer;
    static BLECharacteristic* pCharacteristic;
    static bool deviceConnected;
    static bool oldDeviceConnected;
    static TaskHandle_t bleTaskHandle;

    class ServerCallbacks: public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) override {
            deviceConnected = true;
            NetworkService::lock();
            NetworkService::state.ble_connected = true;
            NetworkService::unlock();
            Serial.println("[BLE] Client Robot Connected!");
        }

        void onDisconnect(BLEServer* pServer) override {
            deviceConnected = false;
            NetworkService::lock();
            NetworkService::state.ble_connected = false;
            NetworkService::unlock();
            Serial.println("[BLE] Client Robot Disconnected.");
        }
    };

    // Dedicated Real-Time FreeRTOS High-Priority Thread (50Hz Low Latency)
    static void bleWorkerTask(void* pvParameters) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = pdMS_TO_TICKS(20); // 50Hz (20ms) cycle

        for (;;) {
            bool isEnabled = false;
            int16_t rawX = 0, rawY = 0;

            NetworkService::lock();
            isEnabled = NetworkService::state.ble_enabled;
            rawX = NetworkService::state.yoke_raw_x;
            rawY = NetworkService::state.yoke_raw_y;
            NetworkService::unlock();

            if (isEnabled && deviceConnected && pCharacteristic) {
                // Construct 4-byte packed payload: [ int16_t X_signed, int16_t Y_signed ]
                uint8_t payload[4];
                payload[0] = (uint8_t)(rawX & 0xFF);
                payload[1] = (uint8_t)((rawX >> 8) & 0xFF);
                payload[2] = (uint8_t)(rawY & 0xFF);
                payload[3] = (uint8_t)((rawY >> 8) & 0xFF);

                pCharacteristic->setValue(payload, 4);
                pCharacteristic->notify();
            }

            // Handle advertising restart on disconnect
            if (!deviceConnected && oldDeviceConnected) {
                vTaskDelay(pdMS_TO_TICKS(500));
                pServer->startAdvertising();
                Serial.println("[BLE] Restarted Advertising...");
                oldDeviceConnected = deviceConnected;
            }
            if (deviceConnected && !oldDeviceConnected) {
                oldDeviceConnected = deviceConnected;
            }

            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
    }

public:
    static void init() {
        Serial.println("[BLE] Initializing Real-Time Bluetooth Robot Controller...");
        BLEDevice::init("ESP-InfoStation-RC");
        
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());

        BLEService *pService = pServer->createService(RC_SERVICE_UUID);

        pCharacteristic = pService->createCharacteristic(
            RC_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ   |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE
        );

        pCharacteristic->addDescriptor(new BLE2902());

        pService->start();

        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(RC_SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        Serial.println("[BLE] Service Started & Advertising Active!");

        // Launch Dedicated FreeRTOS High-Priority Task on Core 0 (Priority 5)
        xTaskCreatePinnedToCore(
            bleWorkerTask,
            "BLE_RC_Task",
            4096,
            NULL,
            5, // High priority to guarantee 0 lag for robot control
            &bleTaskHandle,
            0  // Pinned to Core 0
        );
    }
};

BLEServer* BluetoothService::pServer = nullptr;
BLECharacteristic* BluetoothService::pCharacteristic = nullptr;
bool BluetoothService::deviceConnected = false;
bool BluetoothService::oldDeviceConnected = false;
TaskHandle_t BluetoothService::bleTaskHandle = nullptr;
