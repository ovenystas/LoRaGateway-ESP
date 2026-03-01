#pragma once

#include <Arduino.h>
#include <Print.h>
#include <stdint.h>

#include <cstring>
#include <memory>
#include <vector>

#include "BinarySensorDeviceClass.h"
#include "CoverDeviceClass.h"
#include "DeviceClass.h"
#include "SensorDeviceClass.h"
#include "Unit.h"

#define LORA_MAX_MESSAGE_LENGTH 100
#define LORA_HEADER_LENGTH 4
#define LORA_MAX_PAYLOAD_LENGTH (LORA_MAX_MESSAGE_LENGTH - LORA_HEADER_LENGTH)

// LoRa Message Types
enum class LoRaMsgType : uint8_t {
  ping_req = 0,
  ping_msg = 1,
  discovery_req = 2,
  discovery_msg = 3,
  value_req = 4,
  value_msg = 5,
  config_req = 6,
  config_msg = 7,
  configSet_req = 8,
  service_req = 9
};

// Flag definitions for LoRa message header
#define FLAGS_ACK_MASK 0x80
#define FLAGS_ACK_SHIFT 7
#define FLAGS_REQ_ACK_MASK 0x40
#define FLAGS_REQ_ACK_SHIFT 6
#define FLAGS_MSG_TYPE_MASK 0x0F
#define FLAGS_MSG_TYPE_SHIFT 0

// LoRa Header with flags
struct LoRaHeaderFlags {
  bool ack_response{false};
  bool ack_request{false};
  LoRaMsgType msgType{LoRaMsgType::ping_req};

  uint8_t toByte() const {
    uint8_t b = (ack_response << FLAGS_ACK_SHIFT);
    b |= (ack_request << FLAGS_REQ_ACK_SHIFT);
    b |= (static_cast<uint8_t>(msgType) << FLAGS_MSG_TYPE_SHIFT);
    return b;
  }

  void fromByte(uint8_t b) {
    ack_response = (b & FLAGS_ACK_MASK) != 0;
    ack_request = (b & FLAGS_REQ_ACK_MASK) != 0;
    msgType = static_cast<LoRaMsgType>(b & FLAGS_MSG_TYPE_MASK);
  }
};

struct LoRaHeader {
  uint8_t dst;
  uint8_t src;
  uint8_t id;
  LoRaHeaderFlags flags;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = dst;
    buf[1] = src;
    buf[2] = id;
    buf[3] = flags.toByte();
    return 4;
  }

  uint8_t fromByteArray(const uint8_t* buf) {
    dst = buf[0];
    src = buf[1];
    id = buf[2];
    flags.fromByte(buf[3]);
    return 4;
  }
};

// Value item for value messages
struct ValueItem {
  uint8_t entityId;
  int32_t value;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = entityId;
    buf[1] = (value >> 24) & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 8) & 0xFF;
    buf[4] = value & 0xFF;
    return 5;
  }

  uint8_t fromByteArray(const uint8_t* buf) {
    entityId = buf[0];
    value = ((int32_t)buf[1] << 24) | ((int32_t)buf[2] << 16) |
            ((int32_t)buf[3] << 8) | buf[4];
    return 5;
  }

  static constexpr uint8_t size() { return 5; }
};

// Entity Domain definition
class EntityDomain {
 public:
  enum class Domain : uint8_t { BINARY_SENSOR = 0, SENSOR = 1, COVER = 2 };

  EntityDomain() : domain(Domain::SENSOR) {}
  EntityDomain(Domain d) : domain(d) {}
  EntityDomain(uint8_t d) : domain(static_cast<Domain>(d)) {}

  const char* getName() const;
  Domain getDomain() const { return domain; }
  uint8_t toByte() const { return static_cast<uint8_t>(domain); }

 private:
  Domain domain;
};

// Format for discovery/config messages
// Bit 4: 0=unsigned, 1=signed
// Bits 3-2: size
// Bits 1-0: precision
struct Format {
  bool isSigned;
  uint8_t size;       // 0=1 byte, 1=2 bytes, 2=4 bytes
  uint8_t precision;  // Number of decimal places (0-3) for floats

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
};

// Config item for config messages
struct ConfigItem {
  uint8_t configId;
  Unit unit;
  uint8_t format;  // Bits 4: 0=unsigned, 1=signed; Bits 3-2: size; Bits 1-0:
                   // precision
  int32_t minValue;
  int32_t maxValue;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = configId;
    buf[1] = unit.toByte();
    buf[2] = format;
    buf[3] = (minValue >> 24) & 0xFF;
    buf[4] = (minValue >> 16) & 0xFF;
    buf[5] = (minValue >> 8) & 0xFF;
    buf[6] = minValue & 0xFF;
    buf[7] = (maxValue >> 24) & 0xFF;
    buf[8] = (maxValue >> 16) & 0xFF;
    buf[9] = (maxValue >> 8) & 0xFF;
    buf[10] = maxValue & 0xFF;
    return 11;
  }

  uint8_t fromByteArray(const uint8_t* buf) {
    configId = buf[0];
    unit = Unit(buf[1]);
    format = buf[2];
    minValue = ((int32_t)buf[3] << 24) | ((int32_t)buf[4] << 16) |
               ((int32_t)buf[5] << 8) | buf[6];
    maxValue = ((int32_t)buf[7] << 24) | ((int32_t)buf[8] << 16) |
               ((int32_t)buf[9] << 8) | buf[10];
    return 11;
  }

  static constexpr uint8_t size() { return 11; }
};

// Discovery item for discovery messages
struct DiscoveryItem {
  uint8_t entityId;
  EntityDomain domain;
  std::unique_ptr<DeviceClass> deviceClass;
  Unit unit;
  Format format;
  std::vector<ConfigItem> configItems;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = entityId;
    buf[1] = domain.toByte();
    buf[2] = deviceClass ? deviceClass->toByte() : 0;
    buf[3] = unit.toByte();
    buf[4] = format.toByte();
    return 5;
  }

  uint8_t fromByteArray(const uint8_t* buf, uint8_t len) {
    uint8_t offset = 0;

    if (len < 6) {
      return 0;  // Not enough data
    }

    entityId = buf[offset++];
    domain = EntityDomain(buf[offset++]);
    deviceClass = createDeviceClass(domain, buf[offset++]);
    unit = Unit(buf[offset++]);
    format.fromByte(buf[offset++]);

    uint8_t numConfigItems = buf[offset++];
    configItems.clear();

    for (uint8_t i = 0; i < numConfigItems; i++) {
      if (offset + 11 > len) {
        break;  // Not enough data for config item
      }

      ConfigItem configItem;
      configItem.configId = buf[offset++];
      configItem.unit = Unit(buf[offset++]);
      configItem.format = buf[offset++];
      configItem.minValue = ((int32_t)buf[offset] << 24) |
                            ((int32_t)buf[offset + 1] << 16) |
                            ((int32_t)buf[offset + 2] << 8) | buf[offset + 3];
      offset += 4;
      configItem.maxValue = ((int32_t)buf[offset] << 24) |
                            ((int32_t)buf[offset + 1] << 16) |
                            ((int32_t)buf[offset + 2] << 8) | buf[offset + 3];
      offset += 4;

      configItems.push_back(configItem);
    }

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
      default:
        return std::unique_ptr<DeviceClass>(nullptr);
    }
  }

 public:
  static constexpr uint8_t size() { return 5; }
};

// Service item for service messages
struct ServiceItem {
  uint8_t entityId;
  uint8_t service;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = entityId;
    buf[1] = service;
    return 2;
  }

  uint8_t fromByteArray(const uint8_t* buf) {
    entityId = buf[0];
    service = buf[1];
    return 2;
  }

  static constexpr uint8_t size() { return 2; }
};

// LoRa Message Tx
struct LoRaTxMessage {
  LoRaHeader header;
  uint8_t payloadLength;
  uint8_t payload[LORA_MAX_PAYLOAD_LENGTH];
};

// LoRa Message Rx
struct LoRaRxMessage {
  LoRaHeader header;
  uint8_t payloadLength;
  int16_t rssi;
  uint8_t payload[LORA_MAX_PAYLOAD_LENGTH];
};

// Entity metadata
struct EntityInfo {
  uint16_t deviceId;
  uint8_t entityId;
  EntityDomain domain;
  std::unique_ptr<DeviceClass> deviceClass;
  Format format;
  Unit unit;
  std::vector<ConfigItem> configItems;

  EntityInfo() = default;

  // Copy constructor for cloning deviceClass
  EntityInfo(const EntityInfo& other)
      : deviceId(other.deviceId),
        entityId(other.entityId),
        domain(other.domain),
        deviceClass(copyDeviceClass(other.deviceClass)),
        format(other.format),
        unit(other.unit),
        configItems(other.configItems) {}

  // Copy assignment operator
  EntityInfo& operator=(const EntityInfo& other) {
    if (this != &other) {
      deviceId = other.deviceId;
      entityId = other.entityId;
      domain = other.domain;
      deviceClass = copyDeviceClass(other.deviceClass);
      format = other.format;
      unit = other.unit;
      configItems = other.configItems;
    }
    return *this;
  }

  String getName() const {
    String name =
        deviceClass ? deviceClass->getName() : String("Entity ") + entityId;
    name[0] = static_cast<std::string::value_type>(toupper(name[0]));
    return name;
  }

  void setDeviceClass(const std::unique_ptr<DeviceClass>& source) {
    deviceClass = copyDeviceClass(source);
  }

  size_t print(Print& printer, size_t indent = 0) const {
    size_t n = 0;
    for (size_t i = 0; i < indent; i++) {
      n += printer.print(" ");
    }
    n += printer.print("Entity ID: ");
    n += printer.print(entityId);
    n += printer.print(", Name: ");
    n += printer.print(getName());
    n += printer.print(", Domain: ");
    n += printer.print(domain.getName());
    n += printer.print(", Device Class: ");
    n += printer.print(deviceClass ? deviceClass->getName() : "unknown");
    n += printer.print(", Unit: ");
    n += printer.println(unit.getName());
    return n;
  }

 private:
  static std::unique_ptr<DeviceClass> copyDeviceClass(
      const std::unique_ptr<DeviceClass>& source) {
    if (!source) {
      return nullptr;
    }
    return std::unique_ptr<DeviceClass>(source->clone());
  }
};

// Device metadata
struct DeviceInfo {
  uint16_t deviceId;
  uint32_t lastSeen;
  uint8_t entityCount;
  EntityInfo* entities;

  String getName() const { return String("LoRa Device ") + deviceId; }

  EntityInfo* getEntity(uint8_t entityId) const {
    for (uint8_t i = 0; i < entityCount; i++) {
      if (entities[i].entityId == entityId) {
        return &entities[i];
      }
    }
    return nullptr;
  }

  size_t print(Print& printer, size_t indent = 0) const {
    size_t n = 0;
    for (size_t i = 0; i < indent; i++) {
      n += printer.print(" ");
    }
    n += printer.print("Device ID: ");
    n += printer.print(deviceId);
    n += printer.print(", Name: ");
    n += printer.print(getName());
    n += printer.print(", Last Seen: ");
    n += printer.print(lastSeen);
    n += printer.print(", Entity Count: ");
    n += printer.println(entityCount);

    for (uint8_t i = 0; i < entityCount; i++) {
      n += entities[i].print(printer, indent + 2);
    }
    return n;
  }
};
