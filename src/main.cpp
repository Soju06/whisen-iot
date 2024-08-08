
/*
  Project Name:    IOT LG Whisen
  Author:      soju06
  Created:     8/7/2024 9:40:37 PM
  Developed using NodeMCU v1.0 (ESP8266) 80MHz

  Pin Configuration:
  - IR LED: D2
  - TMP36 Sensor: A0
  
  Panics:
  - 3 short blinks: wrong WiFi password, cannot connect to WiFi
  - 4 short blinks: WiFi Initialization failed
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <ESP8266mDNS.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "packet.hpp"

constexpr uint32_t      SERIAL_BAUD_RATE    = 9600;

constexpr uint16_t      IR_LED_PIN          = D2;
constexpr uint16_t      TMP36_SENSOR_PIN    = A0;

constexpr float         TMP_OFFSET          = 0;
constexpr uint8_t       TMP_SAMPLES         = 60;

constexpr const char*   WIFI_SSID           = "soju";
constexpr const char*   WIFI_PASSWORD       = "password";
constexpr const char*   WIFI_HOSTNAME       = "WHISEN";

constexpr uint16_t      HTTP_PORT           = 80;
constexpr const char*   HTTP_USERNAME       = "soju";
constexpr const char*   HTTP_PASSWORD       = "password";

struct ACState {
  bool        power;
  ACMode      mode;
  uint8_t     temperature;
  ACFanSpeed  fan;
};

time_t            bootTime;
IRsend            ir(IR_LED_PIN);
WiFiUDP           ntpUdp;
NTPClient         ntpTime(ntpUdp, "kr.pool.ntp.org", 32400, 60000);
MDNSResponder     mdns;
ESP8266WebServer  server(HTTP_PORT);
ACState           state = {
                    .power = false,
                    .mode = ACMode::Cool,
                    .temperature = 23,
                    .fan = ACFanSpeed::Fan2
                  };
ACState           syncdState {
                    .power = false,
                    .mode = ACMode::Unknown,
                    .temperature = 0xFF,
                    .fan = ACFanSpeed::Unknown
                  };
float             temperatures[TMP_SAMPLES]{};
uint32_t          lastTemperatureUpdate = 0;
time_t            powerOffTime = 0;

constexpr uint32_t STORE_ADDRESS  = 0;
constexpr uint16_t STORE_MAGIC    = 0xACCA;

struct Store {
  uint16_t magic = STORE_MAGIC;
  ACState state;
  uint8_t checksum;

  uint8_t getChecksum() {
    return (
      state.power
      + (uint8_t)state.mode
      + state.temperature
      + (uint8_t)state.fan
    ) & 0xFF;
  }
};

void signal(ACEvent event, uint16_t data) {
  uint64_t packet = 0x8800000
    | (((uint8_t)event & 0xF) << 16)
    | ((data & 0xFFFFF) << 4);

  packet |= ( 
      ((packet >> 4) & 0xF)
      + ((packet >> 8) & 0xF)
      + ((packet >> 12) & 0xF)
      + ((packet >> 16) & 0xF)
    ) & 0xF;
  
  Serial.print("Sending IR packet: ");
  Serial.println(packet, HEX);
  ir.sendLG(packet);
}

void syncSetting(bool powerOn = false) {
  if (state.mode == ACMode::Jet) {
    if (!syncdState.power) {
      signal(ACEvent::Setting, ACSettingData {
        .modify       = 0,
        .mode         = (uint8_t)ACMode::Cool,
        .temperature  = 0,
        .fan          = (uint8_t)ACFanSpeed::Fan2,
      }.value());
      delay(500);
    }

    signal(ACEvent::Jet, ACJetData {
      .wind = 0,
      .mode = 0,
      .angle = 0x8,
    }.value());

    syncdState.power = true;
    syncdState.mode = ACMode::Jet;
  } else {
    auto mode         = state.mode == ACMode::Unknown ? ACMode::Cool : state.mode;
    auto temperature  = state.temperature == 0xFF ? 8 : min(max(state.temperature, (uint8_t)15) - 15, 0xF);
    auto fan          = state.fan == ACFanSpeed::Unknown ? ACFanSpeed::Fan2 : state.fan;

    signal(ACEvent::Setting, ACSettingData {
      .modify       = !powerOn,
      .mode         = (uint8_t)mode,
      .temperature  = (uint8_t)temperature,
      .fan          = (uint8_t)fan,
    }.value());
    
    syncdState.power        |= powerOn;
    syncdState.mode         = mode;
    syncdState.temperature  = temperature;
    syncdState.fan          = fan;
  }
}

void setTimer(uint16_t minutes) {
  if (!syncdState.power) {
    return;
  }

  signal(ACEvent::Timer, ACTimerData {
    .minutes = (uint16_t)(minutes & 0xFFF),
  }.value());
  powerOffTime = ntpTime.getEpochTime() + minutes * 60;
}

void handleTimer() {
  if (powerOffTime == 0 || ntpTime.getEpochTime() < powerOffTime) {
    return;
  }

  powerOffTime = 0;
  syncdState.power = false;
  state.power = false;
}

void powerOn() {
  syncSetting(true);
}

void powerOff() {
  Serial.print("Sending IR packet: ");
  Serial.println(POWER_OFF, HEX);
  ir.sendLG(POWER_OFF);
  syncdState.power = false;
}

void causePanic(uint8_t count = 3, uint16_t delayMs = 500, uint8_t repeat = 5) {
  Serial.print("Panic! Code: ");
  Serial.println(count);

  for (uint8_t i = 0; i < repeat; i++) {
    for (uint8_t j = 0; j < count; j++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(delayMs);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(delayMs);
    }
    delay(3000);
  }

  Serial.println("Restarting...");
  Serial.flush();

  ESP.restart();
}

bool restoreState() {
  Store store;
  EEPROM.get(0, store);

  if (store.magic != STORE_MAGIC) {
    return false;
  }

  if (store.checksum != store.getChecksum()) {
    Serial.println("Invalid store checksum");
    return false;
  }

  Serial.println("Restored state from EEPROM");
  state = store.state;
  return true;
}

void saveState() {
  Store store = {
    .magic = STORE_MAGIC,
    .state = state,
  };
  store.checksum = store.getChecksum();

  EEPROM.put(STORE_ADDRESS, store);
  EEPROM.commit();
  Serial.println("Saved state to EEPROM");
}

void sync() {
  if (!state.power) {
    if (syncdState.power) {
      powerOff();
    }
    
    return;
  }

  syncSetting(!syncdState.power);
}

void sendBadRequest(const char* message) {
  server.send(400, "application/json", "{\"error\":\"" + String(message) + "\",\"status\":\"error\"}");
}

void updateTemperature() {
  if (millis() - lastTemperatureUpdate < 1000) {
    return;
  }

  for (uint8_t i = TMP_SAMPLES - 1; i > 0; i--) {
    temperatures[i] = temperatures[i - 1];
  }
  
  float voltage = analogRead(TMP36_SENSOR_PIN) * 3.3f / 1024;
  temperatures[0] = (voltage - 0.5) * 100 + TMP_OFFSET;
  lastTemperatureUpdate = millis();
}

float readTemperature() {
  int count = 0;
  float temperature = 0;

  for (uint8_t i = 0; i < TMP_SAMPLES; i++) {
    if (temperatures[i] == 0) {
      continue;
    }

    temperature += temperatures[i];
    count++;
  }

  return count ? temperature / count : 0;
}

void stateJsonObject(JsonObject obj) {
  obj["power"] = state.power;
  obj["mode"] = ACModeToString(state.mode);
  obj["temperature"] = state.temperature;
  obj["fan"] = ACFanSpeedToString(state.fan);
}

void getState() {
  if (!server.authenticate(HTTP_USERNAME, HTTP_PASSWORD)) {
    return server.requestAuthentication();
  }

  JsonDocument resp;

  stateJsonObject(resp["state"].to<JsonObject>());
  resp["temperature"] = readTemperature();
  resp["uptime"] = ntpTime.getEpochTime() - bootTime;
  resp["status"] = "ok";

  server.send(200, "application/json", resp.as<String>());
}

void patchState() {
  if (!server.authenticate(HTTP_USERNAME, HTTP_PASSWORD)) {
    return server.requestAuthentication();
  }

  JsonDocument body;
  auto error = deserializeJson(body, server.arg("plain"));
  // power: bool
  // mode: "Cool" | "Dry" | "Fan" | "Heat" | "Jet"
  // temperature: number (15 - 30)
  // fan: "Fan0" | "Fan1" | "Fan2" | "Fan3" | "Fan4" | "NaturalWind"
  // timer: number (0 - 4095)
  // save: bool

  if (error) {
    sendBadRequest("Invalid JSON");
    return;
  }

  bool updated = false;

  if (body["mode"].is<std::string>()) {
    auto mode = ACModeFromString(body["mode"].as<std::string>());

    if (mode == ACMode::Unknown) {
      sendBadRequest("Invalid mode. Must be one of: Cool, Dry, Fan, Heat, Jet");
      return;
    }
    
    state.mode = mode;
    Serial.print("Mode: ");
    Serial.println(ACModeToString(mode));
    updated = true;
  }

  if (body["temperature"].is<int>()) {
    auto temperature = body["temperature"].as<int>();

    if (temperature < 15 || temperature > 30) {
      sendBadRequest("Invalid temperature. Must be between 15 and 30");
      return;
    }

    state.temperature = temperature;
    Serial.print("Temperature: ");
    Serial.println(temperature);
    updated = true;
  }

  if (body["fan"].is<std::string>()) {
    auto fan = ACFanSpeedFromString(body["fan"].as<std::string>());

    if (fan == ACFanSpeed::Unknown) {
      sendBadRequest("Invalid fan speed. Must be one of: Fan0, Fan1, Fan2, Fan3, Fan4, NaturalWind");
      return;
    }

    state.fan = fan;
    Serial.print("Fan: ");
    Serial.println(ACFanSpeedToString(fan));
    updated = true;
  }

  if (body["power"].is<bool>()) {
    state.power = body["power"].as<bool>();
    Serial.print("Power: ");
    Serial.println(state.power ? "On" : "Off");
    updated = true;
  }

  if (body["save"].as<bool>()) {
    saveState();
  }
  
  if (updated) {
    sync();
  }

  if (body["timer"].is<int>()) {
    if (body["timer"].as<int>() < 0 || body["timer"].as<int>() > 4095) {
      sendBadRequest("Invalid timer. Must be between 0 and 4095");
      return;
    }
    setTimer(body["timer"].as<int>());
  }
  
  JsonDocument resp;


  stateJsonObject(resp["state"].to<JsonObject>());
  resp["status"] = "ok";

  server.send(200, "application/json", resp.as<String>());
}

void setup() {
  ir.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("Hello, World!");

  EEPROM.begin(sizeof(Store));
  restoreState();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(WIFI_HOSTNAME);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    
    switch (WiFi.status()) {
    case WL_NO_SSID_AVAIL:
    case WL_CONNECT_FAILED:
    case WL_WRONG_PASSWORD:
      causePanic(3);
      return;

    default:
      break;
    }
  }

  Serial.print("Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());
  
  if (!mdns.begin(WIFI_HOSTNAME, WiFi.localIP())) {
    causePanic(4);
    return;
  }

  ntpTime.begin();
  ntpTime.forceUpdate();
  delay(1000);
  ntpTime.forceUpdate();

  Serial.print("Current time: ");
  
  {
    bootTime  = ntpTime.getEpochTime();
    tm ts     = *localtime(&bootTime);
    
    Serial.printf("%d/%d/%d %d:%02d:%02d %s\n",
      ts.tm_mon + 1, ts.tm_mday, 1900 + ts.tm_year,
      ts.tm_hour > 12 ? ts.tm_hour - 12 : ts.tm_hour, ts.tm_min, ts.tm_sec,
      ts.tm_hour > 12 ? "PM" : "AM");
  }

  server.on("/status", HTTP_GET, getState);

  server.on("/status", HTTP_PATCH, patchState);

  server.onNotFound([]() {
    server.send(404);
  });

  server.begin();

  Serial.print("HTTP server started: http://");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.print(HTTP_PORT);
  Serial.println("/");
  
  ESP.wdtEnable(10000);
  ESP.wdtFeed();
  
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  mdns.update();
  ntpTime.update();
  server.handleClient();
  updateTemperature();
  handleTimer();
}
