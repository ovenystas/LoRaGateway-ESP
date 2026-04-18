#pragma once

#include <Arduino.h>
#include <Print.h>
#include <stdint.h>

#include <cstring>
#include <memory>
#include <vector>

#include "BinarySensorDeviceClass.h"
#include "ConfigItem.h"
#include "CoverDeviceClass.h"
#include "DeviceClass.h"
#include "Entity.h"
#include "Format.h"
#include "SensorDeviceClass.h"
#include "Unit.h"

#define LORA_MAX_MESSAGE_LENGTH 150
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
  valueSet_req = 6,
  service_req = 7
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
