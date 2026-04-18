#pragma once

#include <Print.h>
#include <stddef.h>
#include <stdint.h>

#include <cmath>

// Format for discovery/config messages
// Bit 4: 0=unsigned, 1=signed
// Bits 3-2: size
// Bits 1-0: precision
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

  float scaleValue(int32_t rawValue) const {
    return static_cast<float>(rawValue) / pow(10, precision);
  }

  size_t print(Print& printer, int32_t rawValue) const {
    float value = scaleValue(rawValue);
    return printer.print(value, precision);
  }

  size_t println(Print& printer, int32_t rawValue) const {
    float value = scaleValue(rawValue);
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

 private:
  bool isSigned;
  uint8_t size;       // 0=1 byte, 1=2 bytes, 2=4 bytes
  uint8_t precision;  // Number of decimal places (0-3) for floats
};
