#pragma once

#include <cstdlib>
#include <cstring>

#include "DeviceRegistry.h"
#include "LoRaMsgHandler.h"
#include "MqttHandler.h"

// Cover command constants
enum class CoverCommand : uint8_t { OPEN = 0, CLOSE = 1, STOP = 2 };

// Rate limiting: maximum 10 messages per device per 500ms
#define MQTT_RATE_LIMIT_INTERVAL_MS 500
#define MQTT_RATE_LIMIT_MAX_PER_INTERVAL 10
#define INVALID_DEVICE_ID 255

class MqttMsgHandler {
 public:
  MqttMsgHandler(LoRaMsgHandler& loRaMsg, DeviceRegistry& registry)
      : loRaMsg(loRaMsg), registry(registry) {
    loRaMsgHandler = &loRaMsg;
    deviceRegistry = &registry;
    initRateLimiting();
  }

  // Handle an incoming MQTT message and perform appropriate actions
  static void handleMessage(const char* topic, const byte* payload,
                            unsigned int length);

 private:
  static LoRaMsgHandler* loRaMsgHandler;
  static DeviceRegistry* deviceRegistry;
  LoRaMsgHandler& loRaMsg;
  DeviceRegistry& registry;

  // Rate limiting structures
  static struct RateLimitEntry {
    uint8_t deviceId;
    uint32_t lastMessageTime;
    uint8_t messageCount;
  } rateLimitTable[DeviceRegistry::MAX_DEVICES];

  // Helper functions
  static void initRateLimiting();
  static bool checkRateLimit(uint8_t deviceId);
  static void cleanupStaleRateLimitEntries();
  static bool handleCoverCommand(const char* payloadStr, uint8_t deviceId,
                                 uint8_t entityId);
  static bool handleNumberCommand(const char* payloadStr, uint8_t deviceId,
                                  uint8_t entityId, const EntityInfo* entity);
};
