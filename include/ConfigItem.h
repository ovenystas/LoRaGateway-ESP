#pragma once

#include <Print.h>
#include <stdint.h>

#include "Format.h"
#include "Unit.h"

// Config item for config messages
struct ConfigItem {
  uint8_t configId;
  Unit unit;
  Format format;
  int32_t minValue;
  int32_t maxValue;
  char name[32];

  uint8_t fromByteArray(const uint8_t* buf, uint8_t len) {
    size_t offset = 0;
    configId = buf[offset++];
    unit = Unit(buf[offset++]);
    format.fromByte(buf[offset++]);
    minValue = ((int32_t)buf[offset] << 24) | ((int32_t)buf[offset + 1] << 16) |
               ((int32_t)buf[offset + 2] << 8) | buf[offset + 3];
    offset += 4;
    maxValue = ((int32_t)buf[offset] << 24) | ((int32_t)buf[offset + 1] << 16) |
               ((int32_t)buf[offset + 2] << 8) | buf[offset + 3];
    offset += 4;
    uint8_t nameLength = strlen((char*)&buf[offset]);
    if (nameLength > sizeof(name)) {
      nameLength = sizeof(name) - 1;
    }
    memcpy(name, &buf[offset], nameLength + 1);
    name[nameLength] = '\0';
    offset += nameLength + 1;
    return offset;
  }

  size_t print(Print& printer) const {
    size_t n = 0;
    n += printer.print("ConfigId=");
    n += printer.print(configId);
    n += printer.print(", Name=");
    n += printer.print(name);
    n += printer.print(", Unit=");
    n += printer.print(unit.getName());
    n += printer.print(", Format=(");
    n += format.print(printer);
    n += printer.print("), Min=");
    n += printer.print(format.fromRawValue(minValue), format.getPrecision());
    n += printer.print(", Max=");
    n += printer.println(format.fromRawValue(maxValue), format.getPrecision());
    return n;
  }
};
