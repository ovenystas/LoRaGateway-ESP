#pragma once

#include <stdint.h>

// Entity types supported
enum class EntityType : uint8_t {
  BINARY_SENSOR = 0,
  SENSOR = 1,
  SWITCH = 2,
  COVER = 3
};

// Command types for switches and covers
enum class CommandType : uint8_t {
  SET_STATE = 0,      // For switches: on/off
  SET_POSITION = 1,   // For covers: 0-255
  OPEN = 2,           // For covers: open
  CLOSE = 3,          // For covers: close
  STOP = 4            // For covers: stop
};

// Sensor value types
enum class ValueType : uint8_t {
  BOOLEAN = 0,
  INT_VALUE = 1,
  FLOAT_VALUE = 2,
  STRING = 3
};

// LoRa Message structure
struct LoRaMessage {
  uint16_t deviceId;      // Device identifier
  uint8_t entityId;       // Entity ID within the device
  EntityType entityType;  // Type of entity
  uint8_t messageType;    // 0: announcement, 1: sensor update, 2: command response, 3: state update
  
  // For sensor updates
  ValueType valueType;
  union {
    bool boolValue;
    int32_t intValue;
    float floatValue;
  } value;
  
  // For commands (gateway -> node)
  CommandType command;
  uint8_t commandValue[4]; // Generic command data
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
