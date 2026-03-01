#include "Unit.h"

const char* Unit::getName() const {
  switch (type) {
    case Type::celsius:
      return "°C";
    case Type::fahrenheit:
      return "°F";
    case Type::kelvin:
      return "K";
    case Type::percent:
      return "%";
    case Type::kilometer:
      return "km";
    case Type::meter:
      return "m";
    case Type::decimeter:
      return "dm";
    case Type::centimeter:
      return "cm";
    case Type::millimeter:
      return "mm";
    case Type::micrometer:
      return "μm";
    case Type::second:
      return "s";
    case Type::millisecond:
      return "ms";
    default:
      return "";
  }
}
