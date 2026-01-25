#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "Types.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

class MqttHandler {
public:
  MqttHandler(WiFiClient& wifiClient);
  
  // Connect to MQTT broker
  bool connect(const char* broker, uint16_t port, const char* clientId);
  
  // Disconnect from MQTT broker
  void disconnect();
  
  // Check if connected
  bool isConnected();
  
  // Publish a sensor value to MQTT
  bool publishSensorValue(uint16_t nodeId, uint8_t deviceId, const char* deviceName, 
                          const LoRaMessage& msg);
  
  // Subscribe to command topics for a device
  bool subscribeToCommands(uint16_t nodeId, uint8_t deviceId);
  
  // Publish discovery message for Home Assistant
  bool publishDiscovery(const DeviceInfo& device, const char* nodePrefix);
  
  // Set callback for received MQTT messages
  void setOnMessageReceived(void (*callback)(const char* topic, const byte* payload, unsigned int length));
  
  // Process MQTT events (should be called in main loop)
  void handle();
  
private:
  PubSubClient client;
  WiFiClient& wifiClient;
  void (*onMessageReceived)(const char*, const byte*, unsigned int);
  
  // Helper to build MQTT topic strings
  static void buildTopic(char* buffer, size_t size, uint16_t nodeId, uint8_t deviceId, 
                        const char* suffix);
};

#endif // MQTTHANDLER_H
