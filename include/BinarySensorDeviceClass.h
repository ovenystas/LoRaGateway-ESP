#pragma once

#include <stdint.h>

#include "DeviceClass.h"

class BinarySensorDeviceClass : public DeviceClass {
 public:
  // From https://www.home-assistant.io/integrations/binary_sensor/ at
  // 2021-03-21
  enum Type : uint8_t {
    NONE,
    BATTERY,
    BATTERYCHARGING,
    COLD,
    CONNECTIVITY,
    DOOR,
    GARAGEDOOR,
    GAS,
    HEAT,
    LIGHT,
    LOCK,
    MOISTURE,
    MOTION,
    MOVING,
    OCCUPANCY,
    OPENING,
    PLUG,
    POWER,
    PRESENCE,
    PROBLEM,
    SAFETY,
    SMOKE,
    SOUND,
    VIBRATION,
    WINDOW
  };

  BinarySensorDeviceClass() : deviceClass(Type::NONE) {}
  BinarySensorDeviceClass(Type d) : deviceClass(d) {}
  BinarySensorDeviceClass(uint8_t d) : deviceClass(static_cast<Type>(d)) {}

  const char* getName() const override;
  Type getDeviceClass() const { return deviceClass; }
  uint8_t toByte() const override { return static_cast<uint8_t>(deviceClass); }
  DeviceClass* clone() const override {
    return new BinarySensorDeviceClass(*this);
  }

 private:
  Type deviceClass;
};
