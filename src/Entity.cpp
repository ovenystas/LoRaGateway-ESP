#include "Entity.h"

const char *EntityDomain::getName() const {
  switch (domain) {
    case Domain::BINARY_SENSOR:
      return "binary_sensor";
    case Domain::BUTTON:
      return "button";
    case Domain::CAMERA:
      return "camera";
    case Domain::CLIMATE:
      return "climate";
    case Domain::COVER:
      return "cover";
    case Domain::DEVICE_TRACKER:
      return "device_tracker";
    case Domain::FAN:
      return "fan";
    case Domain::HUMIDIFIER:
      return "humidifier";
    case Domain::IMAGE:
      return "image";
    case Domain::LAWN_MOWER:
      return "lawn_mower";
    case Domain::LIGHT:
      return "light";
    case Domain::LOCK:
      return "lock";
    case Domain::NOTIFY:
      return "notify";
    case Domain::NUMBER:
      return "number";
    case Domain::ROOM_PRESENCE:
      return "room_presence";
    case Domain::SELECT:
      return "select";
    case Domain::SENSOR:
      return "sensor";
    case Domain::SIREN:
      return "siren";
    case Domain::SWITCH:
      return "switch";
    case Domain::TEXT:
      return "text";
    case Domain::VACUUM:
      return "vacuum";
    case Domain::VALVE:
      return "valve";
    case Domain::WATER_HEATER:
      return "water_heater";
    default:
      return "unknown";
  }
}

const char *EntityCategory::getName() const {
  switch (category) {
    case Category::NONE:
      return "none";
    case Category::CONFIG:
      return "config";
    case Category::DIAGNOSTIC:
      return "diagnostic";
    default:
      return "unknown";
  }
}
