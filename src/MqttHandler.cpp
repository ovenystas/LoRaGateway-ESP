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

void MqttHandler::disconnect() {
  if (client.connected()) {
    client.disconnect();
  }
}

bool MqttHandler::isConnected() {
  return client.connected();
}

bool MqttHandler::publishSensorValue(uint16_t nodeId, uint8_t deviceId, const char* deviceName,
                                      const LoRaMessage& msg) {
  char topic[128];
  char payload[256];
  
  // Build state topic
  snprintf(topic, sizeof(topic), "lora_gateway/node_%u/device_%u/state", nodeId, deviceId);
  
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
  
  doc["device"] = deviceName;
  doc["timestamp"] = millis();
  
  size_t len = serializeJson(doc, payload, sizeof(payload));
  
  return client.publish(topic, payload);
}

bool MqttHandler::subscribeToCommands(uint16_t nodeId, uint8_t deviceId) {
  char topic[128];
  snprintf(topic, sizeof(topic), "lora_gateway/node_%u/device_%u/command", nodeId, deviceId);
  return client.subscribe(topic);
}

bool MqttHandler::publishDiscovery(const DeviceInfo& device, const char* nodePrefix) {
  char topic[256];
  char payload[512];
  
  const char* componentType = "sensor";  // Default
  const char* haType = "sensor";
  
  switch (device.type) {
    case DeviceType::BINARY_SENSOR:
      componentType = "binary_sensor";
      haType = "motion";
      break;
    case DeviceType::SENSOR:
      componentType = "sensor";
      haType = "sensor";
      break;
    case DeviceType::SWITCH:
      componentType = "switch";
      haType = "switch";
      break;
    case DeviceType::COVER:
      componentType = "cover";
      haType = "cover";
      break;
  }
  
  // Build discovery topic for Home Assistant
  snprintf(topic, sizeof(topic), "homeassistant/%s/lora_%u_%u/config",
           componentType, device.nodeId, device.deviceId);
  
  // Create discovery payload
  JsonDocument doc;
  doc["name"] = device.name;
  doc["unique_id"] = String("lora_") + device.nodeId + "_" + device.deviceId;
  doc["object_id"] = String("lora_") + device.nodeId + "_" + device.deviceId;
  
  // State topic
  char stateTopic[128];
  snprintf(stateTopic, sizeof(stateTopic), "lora_gateway/node_%u/device_%u/state",
           device.nodeId, device.deviceId);
  doc["state_topic"] = stateTopic;
  
  // Command topic (for switches and covers)
  if (device.type == DeviceType::SWITCH || device.type == DeviceType::COVER) {
    char cmdTopic[128];
    snprintf(cmdTopic, sizeof(cmdTopic), "lora_gateway/node_%u/device_%u/command",
             device.nodeId, device.deviceId);
    doc["command_topic"] = cmdTopic;
  }
  
  // Value template
  doc["value_template"] = "{{ value_json.value }}";
  
  // Device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = String("lora_node_") + device.nodeId;
  deviceObj["name"] = String("LoRa Node ") + device.nodeId;
  
  // Unit of measurement
  if (device.unit && strlen(device.unit) > 0) {
    doc["unit_of_measurement"] = device.unit;
  }
  
  size_t len = serializeJson(doc, payload, sizeof(payload));
  
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

void MqttHandler::buildTopic(char* buffer, size_t size, uint16_t nodeId, uint8_t deviceId,
                             const char* suffix) {
  snprintf(buffer, size, "lora_gateway/node_%u/device_%u/%s", nodeId, deviceId, suffix);
}
