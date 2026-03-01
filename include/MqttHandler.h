#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include <vector>

#include "Types.h"

#define MQTT_BUFFER_SIZE 384

class MqttHandler {
 public:
  MqttHandler(WiFiClient& wifiClient);

  // Connect to MQTT broker
  bool connect(const char* broker, uint16_t port, const char* clientId);

  // Connect to MQTT broker with username and password
  bool connect(const char* broker, uint16_t port, const char* clientId,
               const char* username, const char* password);

  // Disconnect from MQTT broker
  void disconnect();

  // Check if connected
  bool isConnected();

  // Publish a sensor value to MQTT
  bool publishSensorValues(const DeviceInfo& device,
                           const std::vector<ValueItem>& valueItems);

  // Subscribe to command topics for an entity
  bool subscribeToCommands(uint8_t deviceId, uint8_t entityId);

  // Publish discovery message for Home Assistant
  bool publishDiscovery(const EntityInfo& entity, const char* nodePrefix);

  // Set callback for received MQTT messages
  void setOnMessageReceived(void (*callback)(const char* topic,
                                             const byte* payload,
                                             unsigned int length));

  // Process MQTT events (should be called in main loop)
  void handle();

 private:
  PubSubClient client;
  WiFiClient& wifiClient;
  void (*onMessageReceived)(const char*, const byte*, unsigned int);

  // Helper to build MQTT topic strings
  static void buildTopic(char* buffer, size_t size, uint8_t deviceId,
                         uint8_t entityId, const char* suffix);
};
