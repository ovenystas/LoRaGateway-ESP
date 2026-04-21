#pragma once

#include <Print.h>
#include <stddef.h>
#include <stdint.h>

#include <cmath>

// Format for discovery/config messages
// Bit 4: 0=unsigned, 1=signed
// Bits 3-2: size (0=1 byte, 1=2 bytes or 2=4 bytes)
// Bits 1-0: precision (Number of decimals 0-3)
class Format {
 public:
  Format() : isSigned(false), size(0), precision(0) {}
  Format(bool isSigned, uint8_t size, uint8_t precision)
      : isSigned(isSigned), size(size), precision(precision) {}
  Format(uint8_t b) { fromByte(b); }

  uint8_t toByte() const {
    uint8_t b = (isSigned ? 0x10 : 0x00);
    b |= ((size & 0x03) << 2);
    b |= (precision & 0x03);
    return b;
  }

  void fromByte(uint8_t b) {
    isSigned = (b & 0x10) != 0;
    size = (b >> 2) & 0x03;
    precision = b & 0x03;
  }

  float fromRawValue(uint32_t rawValue) const {
    const float scaleFactor = pow(10, precision);

    if (isSigned) {
      // Interpret as signed value based on size
      int32_t signedValue;
      switch (size) {
        case 0:  // 1 byte signed: sign-extend from 8 bits
          signedValue = (int32_t)((int8_t)(rawValue & 0xFF));
          break;
        case 1:  // 2 bytes signed: sign-extend from 16 bits
          signedValue = (int32_t)((int16_t)(rawValue & 0xFFFF));
          break;
        case 2:  // 4 bytes signed: full 32-bit
          signedValue = (int32_t)rawValue;
          break;
        default:
          return 0.0f;
      }
      return static_cast<float>(signedValue) / scaleFactor;
    } else {
      // Unsigned: interpret as-is
      return static_cast<float>(rawValue) / scaleFactor;
    }
  }

  // Convert a float value to raw value (inverse of scaleValue)
  uint32_t toRawValue(float floatValue) const {
    const float scaleFactor = pow(10, precision);
    return (uint32_t)round(floatValue * scaleFactor);
  }

  size_t print(Print& printer, uint32_t rawValue) const {
    const float value = fromRawValue(rawValue);
    return printer.print(value, precision);
  }

  size_t println(Print& printer, uint32_t rawValue) const {
    const float value = fromRawValue(rawValue);
    return printer.println(value, precision);
  }

  size_t print(Print& printer) const {
    size_t n = 0;
    n += printer.print(isSigned ? "signed" : "unsigned");
    n += printer.print(", ");
    n += printer.print(1 << size);
    n += printer.print(" bytes, ");
    n += printer.print(precision);
    n += printer.print(" decimals");
    return n;
  }

  uint8_t getPrecision() const { return precision; }

  bool getIsSigned() const { return isSigned; }

  uint8_t getSize() const { return size; }

  // Validate a raw value against Format constraints (signedness and size)
  bool isValidRawValue(uint32_t rawValue) const {
    if (isSigned) {
      // For signed values, check if the value fits within the signed range
      // by casting to signed type and back - if it matches, it's valid
      switch (size) {
        case 0:  // 1 byte signed: -128 to 127
          return ((uint32_t)(int8_t)rawValue) == rawValue;
        case 1:  // 2 bytes signed: -32768 to 32767
          return ((uint32_t)(int16_t)rawValue) == rawValue;
        case 2:  // 4 bytes signed: full int32_t range
          return ((uint32_t)(int32_t)rawValue) == rawValue;
        default:
          return false;
      }
    } else {
      // For unsigned values, just check that it fits within the size
      switch (size) {
        case 0:  // 1 byte unsigned: 0 to 255
          return (rawValue <= 255);
        case 1:  // 2 bytes unsigned: 0 to 65535
          return (rawValue <= 65535);
        case 2:  // 4 bytes unsigned: full uint32_t range
          return true;
        default:
          return false;
      }
    }
  }

 private:
  bool isSigned;
  uint8_t size;       // 0=1 byte, 1=2 bytes, 2=4 bytes
  uint8_t precision;  // Number of decimal places (0-3) for floats
};
