#pragma once

#include <stdint.h>

class Unit {
public:
  enum class Type : uint8_t {
    none = 0,
    celsius = 1,
    fahrenheit = 2,
    kelvin = 3,
    percent = 4,
    kilometer = 5,
    meter = 6,
    decimeter = 7,
    centimeter = 8,
    millimeter = 9,
    micrometer = 10,
    second = 11,
    millisecond = 12
    };

  Unit() : type(Type::none) {}
  Unit(Type t) : type(t) {}
  Unit(uint8_t t) : type(static_cast<Type>(t)) {}

  Type getType() const { return type; }
  const char* getName() const;
  uint8_t toByte() const { return static_cast<uint8_t>(type); }

private:
  Type type;
};
