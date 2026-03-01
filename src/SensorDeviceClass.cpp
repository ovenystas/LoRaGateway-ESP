#include "SensorDeviceClass.h"

const char* SensorDeviceClass::getName() const {
  static const char* names[] = {"none",
                                "apparent_power",
                                "aqi",
                                "atmospheric_pressure",
                                "battery",
                                "carbon_dioxide",
                                "carbon_monoxide",
                                "current",
                                "data_rate",
                                "data_size",
                                "date",
                                "distance",
                                "duration",
                                "energy",
                                "enum",
                                "frequency",
                                "gas",
                                "humidity",
                                "illuminance",
                                "irradiance",
                                "moisture",
                                "monetary",
                                "nitrogen_dioxide",
                                "nitrogen_monoxide",
                                "nitrous_oxide",
                                "ozone",
                                "pm1",
                                "pm10",
                                "pm25",
                                "power_factor",
                                "power",
                                "precipitation",
                                "precipitation_intensity",
                                "pressure",
                                "reactive_power",
                                "signal_strength",
                                "sound_pressure",
                                "speed",
                                "sulphur_dioxide",
                                "temperature",
                                "timestamp",
                                "volatile_organic_compounds",
                                "voltage",
                                "volume",
                                "water",
                                "weight",
                                "wind_speed"};

  if (deviceClass < sizeof(names) / sizeof(names[0])) {
    return names[deviceClass];
  }

  return "unknown";
}
