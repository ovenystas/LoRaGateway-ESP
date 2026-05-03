#include "MqttMsgHandler.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

// Static member initialization
LoRaMsgHandler* MqttMsgHandler::loRaMsgHandler = nullptr;
DeviceRegistry* MqttMsgHandler::deviceRegistry = nullptr;
MqttMsgHandler::RateLimitEntry
    MqttMsgHandler::rateLimitTable[DeviceRegistry::MAX_DEVICES];

void MqttMsgHandler::initRateLimiting() {
  for (uint8_t i = 0; i < DeviceRegistry::MAX_DEVICES; i++) {
    rateLimitTable[i].deviceId = INVALID_DEVICE_ID;
    rateLimitTable[i].lastMessageTime = 0;
    rateLimitTable[i].messageCount = 0;
  }
}

void MqttMsgHandler::cleanupStaleRateLimitEntries() {
  const uint32_t currentTime = millis();

  for (uint8_t i = 0; i < DeviceRegistry::MAX_DEVICES; i++) {
    if (rateLimitTable[i].deviceId != INVALID_DEVICE_ID) {
      // Use signed subtraction to handle wraparound correctly
      const int32_t timeDiff =
          (int32_t)(currentTime - rateLimitTable[i].lastMessageTime);

      // If no messages for 5 minutes, clear the entry
      if (timeDiff > 300000) {
        rateLimitTable[i].deviceId = INVALID_DEVICE_ID;
        rateLimitTable[i].lastMessageTime = 0;
        rateLimitTable[i].messageCount = 0;
      }
    }
  }
}

bool MqttMsgHandler::checkRateLimit(uint8_t deviceId) {
  const uint32_t currentTime = millis();

  // Find or create entry for this device
  for (uint8_t i = 0; i < DeviceRegistry::MAX_DEVICES; i++) {
    if (rateLimitTable[i].deviceId == deviceId) {
      // Use signed subtraction to handle millis() wraparound correctly
      const int32_t timeDiff =
          (int32_t)(currentTime - rateLimitTable[i].lastMessageTime);

      // Check if rate limit window has expired
      if (timeDiff >= (int32_t)MQTT_RATE_LIMIT_INTERVAL_MS) {
        // Reset counter for new window
        rateLimitTable[i].messageCount = 1;
        rateLimitTable[i].lastMessageTime = currentTime;
        return true;
      } else {
        // Still in same window
        if (rateLimitTable[i].messageCount < MQTT_RATE_LIMIT_MAX_PER_INTERVAL) {
          rateLimitTable[i].messageCount++;
          return true;
        } else {
          // Rate limit exceeded
          Serial.print("Warning: Rate limit exceeded for device ");
          Serial.println(deviceId);
          return false;
        }
      }
    }
  }

  // First message from this device - find empty slot
  for (uint8_t i = 0; i < DeviceRegistry::MAX_DEVICES; i++) {
    if (rateLimitTable[i].deviceId == INVALID_DEVICE_ID) {  // Empty slot
      rateLimitTable[i].deviceId = deviceId;
      rateLimitTable[i].lastMessageTime = currentTime;
      rateLimitTable[i].messageCount = 1;
      return true;
    }
  }

  // Rate limit table full - try cleanup first
  cleanupStaleRateLimitEntries();

  // Try again after cleanup
  for (uint8_t i = 0; i < DeviceRegistry::MAX_DEVICES; i++) {
    if (rateLimitTable[i].deviceId == INVALID_DEVICE_ID) {
      rateLimitTable[i].deviceId = deviceId;
      rateLimitTable[i].lastMessageTime = currentTime;
      rateLimitTable[i].messageCount = 1;
      return true;
    }
  }

  // Still full after cleanup
  Serial.println("Error: Rate limit table full");
  return false;
}

bool MqttMsgHandler::handleCoverCommand(const char* payloadStr,
                                        uint8_t deviceId, uint8_t entityId) {
  CoverCommand command;

  // Use exact string matching instead of substring matching
  if (strcmp(payloadStr, "OPEN") == 0) {
    command = CoverCommand::OPEN;
  } else if (strcmp(payloadStr, "CLOSE") == 0) {
    command = CoverCommand::CLOSE;
  } else if (strcmp(payloadStr, "STOP") == 0) {
    command = CoverCommand::STOP;
  } else {
    Serial.print("Error: Unknown cover command: ");
    Serial.println(payloadStr);
    return false;
  }

  // Forward command to device via LoRa
  if (loRaMsgHandler->sendServiceCommand(deviceId, entityId,
                                         static_cast<uint8_t>(command))) {
    Serial.print("Service command sent successfully (device=");
    Serial.print(deviceId);
    Serial.print(", entity=");
    Serial.print(entityId);
    Serial.print(", command=");
    Serial.print(payloadStr);
    Serial.println(")");
    return true;
  } else {
    Serial.println("Failed to send service command");
    return false;
  }
}

bool MqttMsgHandler::handleNumberCommand(const char* payloadStr,
                                         uint8_t deviceId, uint8_t entityId,
                                         const EntityInfo* entity) {
  // Parse MQTT string as float (could be "23.45", "-30", or "200")
  char* endptr = nullptr;
  errno = 0;
  const float floatValue = strtof(payloadStr, &endptr);

  // Validate parsing
  if (endptr == payloadStr || *endptr != '\0') {
    Serial.print("Error: Invalid numeric value: ");
    Serial.println(payloadStr);
    return false;
  }

  if (errno == ERANGE) {
    Serial.print("Error: Float value out of range: ");
    Serial.println(payloadStr);
    return false;
  }

  // Convert float to raw value using Format's conversion
  const uint32_t rawValue = entity->format.toRawValue(floatValue);

  // Check if raw value is valid for this Format (signedness and size
  // constraints)
  if (!entity->format.isValidRawValue(rawValue)) {
    Serial.print("Error: Value ");
    Serial.print(rawValue);
    Serial.print(" out of Format bounds (");
    entity->format.print(Serial);
    Serial.println(")");
    return false;
  }

  // Check entity's min/max bounds
  if (rawValue < entity->minValue || rawValue > entity->maxValue) {
    Serial.print("Error: Value ");
    Serial.print(rawValue);
    Serial.print(" out of entity range [");
    Serial.print(entity->minValue);
    Serial.print(", ");
    Serial.print(entity->maxValue);
    Serial.println("]");
    return false;
  }

  // Forward value to device via LoRa
  if (loRaMsgHandler->sendValueSetRequest(deviceId, entityId, rawValue)) {
    Serial.print("Value set command sent successfully (device=");
    Serial.print(deviceId);
    Serial.print(", entity=");
    Serial.print(entityId);
    Serial.print(", mqtt_value=");
    Serial.print(floatValue, entity->format.getPrecision());
    Serial.print(", raw_value=");
    Serial.print(rawValue);
    Serial.println(")");
    return true;
  } else {
    Serial.println("Failed to send value set command");
    return false;
  }
}

void MqttMsgHandler::handleMessage(const char* topic, const byte* payload,
                                   unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.println(topic);

  // Validate inputs
  if (!topic || length == 0 || !loRaMsgHandler || !deviceRegistry) {
    Serial.println("Error: Invalid message parameters");
    return;
  }

  // Parse topic: lora-gw/device_{deviceId}/entity_{entityId}/[service|value]
  uint8_t deviceId = 0;
  uint8_t entityId = 0;
  char suffix[32];

  // Structured topic parsing with full validation
  if (sscanf(topic, "lora-gw/device_%u/entity_%u/%31s", &deviceId, &entityId,
             suffix) != 3) {
    Serial.println("Error: Invalid topic format");
    return;
  }

  // Validate topic suffix
  if (strcmp(suffix, "service") != 0 && strcmp(suffix, "value") != 0) {
    Serial.print("Error: Invalid topic suffix: ");
    Serial.println(suffix);
    return;
  }

  // Check rate limiting for this device
  if (!checkRateLimit(deviceId)) {
    return;
  }

  // Validate payload size
  if (length >= 256) {
    Serial.println("Error: Payload too large");
    return;
  }

  // Parse payload
  char payloadStr[256];
  strncpy(payloadStr, (const char*)payload, length);
  payloadStr[length] = '\0';

  // Trim leading and trailing whitespace
  int startIdx = 0;
  while (startIdx < (int)length && isspace(payloadStr[startIdx])) {
    startIdx++;
  }

  int endIdx = length - 1;
  while (endIdx >= startIdx && isspace(payloadStr[endIdx])) {
    endIdx--;
  }

  // Handle all-whitespace payload
  if (endIdx < startIdx) {
    Serial.println("Error: Payload is empty after trimming whitespace");
    return;
  }

  // Move trimmed string to the beginning and null-terminate
  if (startIdx > 0) {
    memmove(payloadStr, payloadStr + startIdx, endIdx - startIdx + 1);
  }
  payloadStr[endIdx - startIdx + 1] = '\0';

  Serial.print("Payload: ");
  Serial.println(payloadStr);

  // Look up the entity in the registry
  const EntityInfo* entity = deviceRegistry->getEntity(deviceId, entityId);
  if (!entity) {
    Serial.print("Error: Entity ");
    Serial.print(entityId);
    Serial.print(" not found on device ");
    Serial.println(deviceId);
    return;
  }

  // Log message acceptance
  Serial.print("Message accepted: device=");
  Serial.print(deviceId);
  Serial.print(", entity=");
  Serial.print(entityId);
  Serial.print(", domain=");
  Serial.print(entity->domain.getName());
  Serial.print(", payload=");
  Serial.println(payloadStr);

  // Determine entity type and send appropriate command
  if (entity->domain.getDomain() == EntityDomain::Domain::COVER) {
    handleCoverCommand(payloadStr, deviceId, entityId);
  } else if (entity->domain.getDomain() == EntityDomain::Domain::NUMBER) {
    handleNumberCommand(payloadStr, deviceId, entityId, entity);
  } else {
    Serial.print("Error: Entity type not supported for commands (domain: ");
    Serial.print(entity->domain.getName());
    Serial.println(")");
  }
}
