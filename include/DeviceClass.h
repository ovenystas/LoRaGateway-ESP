#pragma once

#include <stdint.h>

#include <memory>

// Base interface class for device classes
class DeviceClass {
 public:
  virtual ~DeviceClass() = default;

  virtual const char* getName() const = 0;
  virtual uint8_t toByte() const = 0;
  virtual DeviceClass* clone() const = 0;
};
