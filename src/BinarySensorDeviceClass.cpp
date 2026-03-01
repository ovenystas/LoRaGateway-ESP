#include "BinarySensorDeviceClass.h"

const char* BinarySensorDeviceClass::getName() const {
  static const char* names[] = {
      "none",        "battery",      "battery_charging",
      "cold",        "connectivity", "door",
      "garage_door", "gas",          "heat",
      "light",       "lock",         "moisture",
      "motion",      "moving",       "occupancy",
      "opening",     "plug",         "power",
      "presence",    "problem",      "safety",
      "smoke",       "sound",        "vibration",
      "window"};

  if (deviceClass < sizeof(names) / sizeof(names[0])) {
    return names[deviceClass];
  }
  return "unknown";
}
