#pragma once

#include <stdint.h>

#include "DeviceClass.h"

class SensorDeviceClass : public DeviceClass {
 public:
  // From https://www.home-assistant.io/integrations/sensor/ at 2023-01-17
  enum Type : uint8_t {
    NONE,
    APPARENT_POWER,
    AQI,
    ATMOSPHERIC_PRESSURE,
    BATTERY,
    CARBON_DIOXIDE,
    CARBON_MONOXIDE,
    CURRENT,
    DATA_RATE,
    DATA_SIZE,
    DATE,
    DISTANCE,
    DURATION,
    ENERGY,
    ENUM_CLASS,
    FREQUENCY,
    GAS,
    HUMIDITY,
    ILLUMINANCE,
    IRRADIANCE,
    MOISTURE,
    MONETARY,
    NITROGEN_DIOXIDE,
    NITROGEN_MONOXIDE,
    NITROUS_OXIDE,
    OZONE,
    PM1,
    PM10,
    PM25,
    POWER_FACTOR,
    POWER,
    PRECIPITATION,
    PRECIPITATION_INTENSITY,
    PRESSURE,
    REACTIVE_POWER,
    SIGNAL_STRENGTH,
    SOUND_PRESSURE,
    SPEED,
    SULPHUR_DIOXIDE,
    TEMPERATURE,
    TIMESTAMP,
    VOLATILE_ORGANIC_COMPOUNDS,
    VOLTAGE,
    VOLUME,
    WATER,
    WEIGHT,
    WIND_SPEED
  };

  SensorDeviceClass() : deviceClass(Type::NONE) {}
  SensorDeviceClass(Type d) : deviceClass(d) {}
  SensorDeviceClass(uint8_t d) : deviceClass(static_cast<Type>(d)) {}

  const char* getName() const override;
  Type getDeviceClass() const { return deviceClass; }
  uint8_t toByte() const override { return static_cast<uint8_t>(deviceClass); }
  DeviceClass* clone() const override { return new SensorDeviceClass(*this); }

 private:
  const Type deviceClass;
};
