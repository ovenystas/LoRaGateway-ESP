#include "MqttHandler.h"
#include <ArduinoJson.h>
#include <stdio.h>

MqttHandler::MqttHandler(WiFiClient& wifiClient)
    : wifiClient(wifiClient), onMessageReceived(nullptr) {
  client.setClient(wifiClient);
}

bool MqttHandler::connect(const char* broker, uint16_t port, const char* clientId) {
  client.setServer(broker, port);
  client.setCallback([this](char* topic, byte* payload, unsigned int length) {
    if (onMessageReceived) {
      onMessageReceived(topic, payload, length);
    }
  });
  
  return client.connect(clientId);
}

bool MqttHandler::connect(const char* broker, uint16_t port, const char* clientId, const char* username, const char* password) {
  client.setServer(broker, port);
  client.setCallback([this](char* topic, byte* payload, unsigned int length) {
    if (onMessageReceived) {
      onMessageReceived(topic, payload, length);
    }
  });
  
  return client.connect(clientId, username, password);
}

void MqttHandler::disconnect() {
  if (client.connected()) {
    client.disconnect();
  }
}

bool MqttHandler::isConnected() {
  return client.connected();
}

bool MqttHandler::publishSensorValue(uint16_t deviceId, uint8_t entityId, const char* entityName,
                                      const LoRaMessage& msg) {
  char topic[128];
  char payload[256];
  
  // Build state topic
  snprintf(topic, sizeof(topic), "lora_gateway/device_%u/entity_%u/state", deviceId, entityId);
  
  // Create JSON payload based on value type
  JsonDocument doc;
  
  switch (msg.valueType) {
    case ValueType::BOOLEAN:
      doc["value"] = msg.value.boolValue;
      doc["state"] = msg.value.boolValue ? "ON" : "OFF";
      break;
    case ValueType::INT_VALUE:
      doc["value"] = msg.value.intValue;
      break;
    case ValueType::FLOAT_VALUE:
      doc["value"] = msg.value.floatValue;
      break;
    case ValueType::STRING:
      doc["value"] = "N/A";
      break;
  }
  
  doc["entity"] = entityName;
  doc["timestamp"] = millis();
  
  serializeJson(doc, payload, sizeof(payload));
  
  return client.publish(topic, payload);
}

bool MqttHandler::subscribeToCommands(uint16_t deviceId, uint8_t entityId) {
  char topic[128];
  snprintf(topic, sizeof(topic), "lora_gateway/device_%u/entity_%u/command", deviceId, entityId);
  return client.subscribe(topic);
}

bool MqttHandler::publishDiscovery(const EntityInfo& entity, const char* nodePrefix) {
  char topic[256];
  char payload[512];
  
  const char* componentType = "sensor";  // Default
  
  switch (entity.type) {
    case EntityType::BINARY_SENSOR:
      componentType = "binary_sensor";
      break;
    case EntityType::SENSOR:
      componentType = "sensor";
      break;
    case EntityType::SWITCH:
      componentType = "switch";
      break;
    case EntityType::COVER:
      componentType = "cover";
      break;
  }
  
  // Build discovery topic for Home Assistant
  snprintf(topic, sizeof(topic), "homeassistant/%s/lora_%u_%u/config",
           componentType, entity.deviceId, entity.entityId);
  
  // Create discovery payload
  JsonDocument doc;
  doc["name"] = entity.name;
  doc["unique_id"] = String("lora_") + entity.deviceId + "_" + entity.entityId;
  doc["object_id"] = String("lora_") + entity.deviceId + "_" + entity.entityId;
  
  // State topic
  char stateTopic[128];
  snprintf(stateTopic, sizeof(stateTopic), "lora_gateway/device_%u/entity_%u/state",
           entity.deviceId, entity.entityId);
  doc["state_topic"] = stateTopic;
  
  // Command topic (for switches and covers)
  if (entity.type == EntityType::SWITCH || entity.type == EntityType::COVER) {
    char cmdTopic[128];
    snprintf(cmdTopic, sizeof(cmdTopic), "lora_gateway/device_%u/entity_%u/command",
             entity.deviceId, entity.entityId);
    doc["command_topic"] = cmdTopic;
  }
  
  // Value template
  doc["value_template"] = "{{ value_json.value }}";
  
  // Device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = String("lora_device_") + entity.deviceId;
  deviceObj["name"] = String("LoRa Device ") + entity.deviceId;
  
  // Unit of measurement
  if (entity.unit && strlen(entity.unit) > 0) {
    doc["unit_of_measurement"] = entity.unit;
  }
  
  serializeJson(doc, payload, sizeof(payload));
  
  return client.publish(topic, payload, true);  // Retain discovery message
}

void MqttHandler::setOnMessageReceived(void (*callback)(const char*, const byte*, unsigned int)) {
  onMessageReceived = callback;
}

void MqttHandler::handle() {
  if (!client.connected()) {
    // Attempt reconnection could go here if needed
  } else {
    client.loop();
  }
}

void MqttHandler::buildTopic(char* buffer, size_t size, uint16_t deviceId, uint8_t entityId,
                             const char* suffix) {
  snprintf(buffer, size, "lora_gateway/device_%u/entity_%u/%s", deviceId, entityId, suffix);
}
