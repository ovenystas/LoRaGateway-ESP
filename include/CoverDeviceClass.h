#pragma once

#include <stdint.h>

#include "DeviceClass.h"

class CoverDeviceClass : public DeviceClass {
 public:
  // From https://www.home-assistant.io/integrations/cover/ at 2021-03-21
  enum Type : uint8_t {
    NONE,
    AWNING,
    BLIND,
    CURTAIN,
    DAMPER,
    DOOR,
    GARAGE,
    GATE,
    SHADE,
    SHUTTER,
    WINDOW
  };

  CoverDeviceClass() : deviceClass(Type::NONE) {}
  CoverDeviceClass(Type d) : deviceClass(d) {}
  CoverDeviceClass(uint8_t d) : deviceClass(static_cast<Type>(d)) {}

  const char* getName() const override;
  Type getDeviceClass() const { return deviceClass; }
  uint8_t toByte() const override { return static_cast<uint8_t>(deviceClass); }
  DeviceClass* clone() const override { return new CoverDeviceClass(*this); }

 private:
  Type deviceClass;
};
