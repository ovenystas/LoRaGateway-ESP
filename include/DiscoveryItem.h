#pragma once

#include <Arduino.h>
#include <stdint.h>

#include <vector>

#include "DeviceClass.h"
#include "Entity.h"
#include "Format.h"
#include "Unit.h"
#include "Util.h"

// Discovery item for discovery messages
class DiscoveryItem {
 public:
  static constexpr uint8_t size() { return 5; }

  uint8_t entityId;
  EntityDomain domain;
  std::unique_ptr<DeviceClass> deviceClass;
  EntityCategory category;
  Unit unit;
  Format format;
  uint32_t minValue;
  uint32_t maxValue;
  char name[32];

  uint8_t toByteArray(uint8_t* buf) const {
    uint8_t offset = 0;
    buf[offset++] = entityId;
    buf[offset++] = domain.toByte();
    buf[offset++] = deviceClass ? deviceClass->toByte() : 0;
    buf[offset++] = category.toByte();
    buf[offset++] = unit.toByte();
    buf[offset++] = format.toByte();
    buf[offset++] = (minValue >> 24) & 0xFF;
    buf[offset++] = (minValue >> 16) & 0xFF;
    buf[offset++] = (minValue >> 8) & 0xFF;
    buf[offset++] = minValue & 0xFF;
    buf[offset++] = (maxValue >> 24) & 0xFF;
    buf[offset++] = (maxValue >> 16) & 0xFF;
    buf[offset++] = (maxValue >> 8) & 0xFF;
    buf[offset++] = maxValue & 0xFF;
    size_t nameLength = strlen(name);
    memcpy(&buf[offset], name, nameLength + 1);
    offset += nameLength;
    buf[offset++] = '\0';

    return offset;
  }

  uint8_t fromByteArray(const uint8_t* buf, uint8_t len) {
    uint8_t offset = 0;

    if (len < 6) {
      return 0;  // Not enough data
    }

    entityId = buf[offset++];
    domain = EntityDomain(buf[offset++]);
    deviceClass = createDeviceClass(domain, buf[offset++]);
    category = EntityCategory(buf[offset++]);
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

 private:
  std::unique_ptr<DeviceClass> createDeviceClass(EntityDomain domain,
                                                 uint8_t classValue) const {
    switch (domain.getDomain()) {
      case EntityDomain::Domain::BINARY_SENSOR:
        return std::unique_ptr<DeviceClass>(
            new BinarySensorDeviceClass(classValue));
      case EntityDomain::Domain::COVER:
        return std::unique_ptr<DeviceClass>(new CoverDeviceClass(classValue));
      case EntityDomain::Domain::SENSOR:
        return std::unique_ptr<DeviceClass>(new SensorDeviceClass(classValue));
      case EntityDomain::Domain::NUMBER:
        return std::unique_ptr<DeviceClass>(new NumberDeviceClass(classValue));
      default:
        return std::unique_ptr<DeviceClass>(nullptr);
    }
  }
};
