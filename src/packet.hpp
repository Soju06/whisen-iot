#pragma once
#include <Arduino.h>

// AC Power Off
constexpr uint64_t POWER_OFF = 0x88C0051;

enum class ACEvent : uint8_t {
  Setting = 0,
  Jet = 1,
  Timer = 0xA,
  // PowerOff = 0xC
};

enum class ACMode : uint8_t {
  Cool = 0,
  Dry = 1,
  Fan = 2,
  Heat = 4,
  // jet is not a mode, it's a separate event
  Jet = 0xF0,
  Unknown = 0xFF
};

const char* ACModeToString(ACMode mode) {
  switch (mode) {
    case ACMode::Cool: return "Cool";
    case ACMode::Dry: return "Dry";
    case ACMode::Fan: return "Fan";
    case ACMode::Heat: return "Heat";
    case ACMode::Jet: return "Jet";
    default: return "Unknown";
  }
}

const ACMode ACModeFromString(std::string mode) {
  if (mode == "Cool") return ACMode::Cool;
  if (mode == "Dry") return ACMode::Dry;
  if (mode == "Fan") return ACMode::Fan;
  if (mode == "Heat") return ACMode::Heat;
  if (mode == "Jet") return ACMode::Jet;
  return ACMode::Unknown;
}

enum class ACFanSpeed : uint8_t {
  Fan0 = 0,
  Fan1 = 1,
  Fan2 = 2,
  Fan3 = 3,
  Fan4 = 4,
  NaturalWind = 5,
  Unknown = 0xFF
};

const char* ACFanSpeedToString(ACFanSpeed fan) {
  switch (fan) {
    case ACFanSpeed::Fan0: return "Fan0";
    case ACFanSpeed::Fan1: return "Fan1";
    case ACFanSpeed::Fan2: return "Fan2";
    case ACFanSpeed::Fan3: return "Fan3";
    case ACFanSpeed::Fan4: return "Fan4";
    case ACFanSpeed::NaturalWind: return "NaturalWind";
    default: return "Unknown";
  }
}

const ACFanSpeed ACFanSpeedFromString(std::string fan) {
  if (fan == "Fan0") return ACFanSpeed::Fan0;
  if (fan == "Fan1") return ACFanSpeed::Fan1;
  if (fan == "Fan2") return ACFanSpeed::Fan2;
  if (fan == "Fan3") return ACFanSpeed::Fan3;
  if (fan == "Fan4") return ACFanSpeed::Fan4;
  if (fan == "NaturalWind") return ACFanSpeed::NaturalWind;
  return ACFanSpeed::Unknown;
}

struct ACSettingData {
  /*
    0 - power on
    1 - modify 
  */
  uint16_t modify : 1;
  /* mode
    0 - cool
    1 - dry
    2 - fan
    4 - heat
  */
  uint16_t mode : 3;
  /* temperature : 0 - 15 */
  uint16_t temperature : 4;
  /* fan speed
    0 ~ 4 - fan speed
    5 - natural wind
  */
  uint16_t fan : 4;
  
  inline uint16_t value() {
    return (modify << 11) | (mode << 8) | (temperature << 4) | fan;
  }
};

struct ACTimerData {
  uint16_t minutes : 12;

  inline uint16_t value() {
    return minutes;
  }
};

struct ACJetData {
  /* wind mode
    0 - jet
    3 - swing
  */
  uint16_t wind : 4;
  /* swing mode
    0 - jet, horizontal
    1 - vertical
  */
  uint16_t mode : 4;
  /* angle
    8 - jet
  */
  uint16_t angle : 4;
  
  inline uint16_t value() {
    return (wind << 8) | (mode << 4) | angle;
  }
};
