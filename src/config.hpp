#pragma once
#include <Arduino.h>

constexpr uint32_t      SERIAL_BAUD_RATE    = 9600;

constexpr uint16_t      IR_LED_PIN          = D2;
constexpr uint16_t      TMP36_SENSOR_PIN    = A0;

constexpr float         TMP_OFFSET          = 0;
constexpr uint8_t       TMP_SAMPLES         = 60;

constexpr const char*   WIFI_SSID           = "soju";
constexpr const char*   WIFI_PASSWORD       = "password";
constexpr const char*   WIFI_HOSTNAME       = "WHISEN";

constexpr const char*   MDNS_HOSTNAME       = "whisen.local";

constexpr uint16_t      HTTP_PORT           = 80;
constexpr const char*   HTTP_USERNAME       = "soju";
constexpr const char*   HTTP_PASSWORD       = "password";
