#pragma once

#include <stdint.h>
#include <cstring>

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

// Entity types supported
enum class EntityType : uint8_t {
  BINARY_SENSOR = 0,
  SENSOR = 1,
  SWITCH = 2,
  COVER = 3
};

// Entity Device Classes
enum class BinarySensorDeviceClass : uint8_t {
  none = 0,
  battery = 1,
  cold = 2,
  heat = 3,
  connectivity = 4,
  door = 5,
  garageDoor = 6,
  opening = 7,
  window = 8,
  lock = 9,
  moisture = 10,
  gas = 11,
  motion = 12,
  occupancy = 13,
  smoke = 14,
  sound = 15,
  vibration = 16,
  presence = 17,
  problem = 18,
  safety = 19
};

enum class SensorDeviceClass : uint8_t {
  none = 0,
  battery = 1,
  humidity = 2,
  distance = 3
};

enum class CoverDeviceClass : uint8_t {
  none = 0,
  garage = 1
};

// Unit types
enum class Unit : uint8_t {
  none = 0,
  celsius = 1,
  fahrenheit = 2,
  percent = 3,
  mm = 4,
  cm = 5,
  m = 6,
  km = 7,
  unknown = 255
};

// Value types for metadata
enum class ValueType : uint8_t {
  BOOLEAN = 0,
  INT_VALUE = 1,
  FLOAT_VALUE = 2,
  STRING = 3
};

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

// Discovery item for discovery messages
struct DiscoveryItem {
  uint8_t entityId;
  EntityType type;
  uint8_t deviceClass;
  Unit unit;
  uint8_t format;  // Bits 4: 0=unsigned, 1=signed; Bits 3-2: size; Bits 1-0: precision

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = entityId;
    buf[1] = static_cast<uint8_t>(type);
    buf[2] = deviceClass;
    buf[3] = static_cast<uint8_t>(unit);
    buf[4] = format;
    return 5;
  }

  uint8_t fromByteArray(const uint8_t* buf) {
    entityId = buf[0];
    type = static_cast<EntityType>(buf[1]);
    deviceClass = buf[2];
    unit = static_cast<Unit>(buf[3]);
    format = buf[4];
    return 5;
  }

  static constexpr uint8_t size() { return 5; }
};

// Config item for config messages
struct ConfigItem {
  uint8_t configId;
  Unit unit;
  uint8_t format;  // Bits 4: 0=unsigned, 1=signed; Bits 3-2: size; Bits 1-0: precision
  int32_t minValue;
  int32_t maxValue;

  uint8_t toByteArray(uint8_t* buf) const {
    buf[0] = configId;
    buf[1] = static_cast<uint8_t>(unit);
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
    unit = static_cast<Unit>(buf[1]);
    format = buf[2];
    minValue = ((int32_t)buf[3] << 24) | ((int32_t)buf[4] << 16) |
               ((int32_t)buf[5] << 8) | buf[6];
    maxValue = ((int32_t)buf[7] << 24) | ((int32_t)buf[8] << 16) |
               ((int32_t)buf[9] << 8) | buf[10];
    return 11;
  }

  static constexpr uint8_t size() { return 11; }
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
  EntityType type;
  const char* name;
  const char* unit;      // Unit of measurement for sensors
  ValueType valueType;
  float minValue;
  float maxValue;
};

// Device metadata
struct DeviceInfo {
  uint16_t deviceId;
  const char* name;
  uint32_t lastSeen;
  uint8_t entityCount;
  EntityInfo* entities;
};
