#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "Types.h"
#include "LoRaHandler.h"
#include "MqttHandler.h"
#include "DeviceRegistry.h"

// Global instances
WiFiClient wifiClient;
LoRaHandler loRa(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO_PIN);
MqttHandler mqtt(wifiClient);
DeviceRegistry deviceRegistry;

// Timing variables
unsigned long lastMqttCheckTime = 0;
unsigned long lastDeviceTimeoutCheck = 0;
const unsigned long MQTT_CHECK_INTERVAL = 5000;        // Check MQTT connection every 5 seconds
const unsigned long DEVICE_TIMEOUT_CHECK_INTERVAL = 60000; // Check device timeouts every minute

// Forward declarations
void setupWiFi();
void handleLoRaMessage(const LoRaMessage& msg);
void handleMqttMessage(const char* topic, const byte* payload, unsigned int length);
void onDeviceDiscovered(uint16_t deviceId, const LoRaMessage& msg);
void sendMqttCommandToDevice(uint16_t deviceId, uint8_t entityId, CommandType cmd, const uint8_t* value);
void publishDeviceDiscovery(uint16_t deviceId, uint8_t entityId);

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n");
  Serial.println("====================================");
  Serial.println("LoRa Gateway starting up...");
  Serial.println("====================================");
  
  // Initialize LoRa
  Serial.print("Initializing LoRa...");
  if (!loRa.begin(LORA_FREQUENCY)) {
    Serial.println(" FAILED!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println(" OK");
  
  // Set up LoRa callback
  loRa.setOnMessageReceived(handleLoRaMessage);
  
  // Connect to WiFi
  setupWiFi();
  
  // Set up MQTT callback
  mqtt.setOnMessageReceived(handleMqttMessage);
  
  Serial.println("Gateway initialized successfully!");
}

void loop() {
  // Handle WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    setupWiFi();
  }
  
  // Handle MQTT connection
  unsigned long currentTime = millis();
  if (currentTime - lastMqttCheckTime >= MQTT_CHECK_INTERVAL) {
    lastMqttCheckTime = currentTime;
    
    if (!mqtt.isConnected()) {
      Serial.print("Connecting to MQTT broker at ");
      Serial.print(MQTT_BROKER);
      Serial.print(":");
      Serial.println(MQTT_PORT);
      
      if (mqtt.connect(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("MQTT connected!");
        
        // Resubscribe to all entity command topics
        uint8_t deviceCount = 0;
        DeviceInfo** devices = deviceRegistry.getAllDevices(deviceCount);
        if (devices) {
          for (uint8_t i = 0; i < deviceCount; i++) {
            for (uint8_t j = 0; j < devices[i]->entityCount; j++) {
              mqtt.subscribeToCommands(devices[i]->deviceId, devices[i]->entities[j].entityId);
            }
          }
        }
      } else {
        Serial.println("MQTT connection failed!");
      }
    }
  }
  
  // Handle device timeout checks
  if (currentTime - lastDeviceTimeoutCheck >= DEVICE_TIMEOUT_CHECK_INTERVAL) {
    lastDeviceTimeoutCheck = currentTime;
    
    uint8_t deviceCount = 0;
    DeviceInfo** devices = deviceRegistry.getAllDevices(deviceCount);
    if (devices) {
      for (uint8_t i = 0; i < deviceCount; i++) {
        if (currentTime - devices[i]->lastSeen > (NODE_TIMEOUT_SECONDS * 1000)) {
          Serial.print("Device ");
          Serial.print(devices[i]->deviceId);
          Serial.println(" has timed out - removing from registry");
          deviceRegistry.unregisterDevice(devices[i]->deviceId);
        }
      }
    }
  }
  
  // Process LoRa messages
  loRa.handle();
  
  // Process MQTT events
  mqtt.handle();
  
  delay(10);  // Small delay to prevent watchdog timeout
}

void setupWiFi() {
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Will retry in main loop.");
  }
}

void handleLoRaMessage(const LoRaMessage& msg) {
  Serial.print("LoRa message received from Device ");
  Serial.print(msg.deviceId);
  Serial.print(", Entity ");
  Serial.println(msg.entityId);
  
  // Check if this is a new device
  bool isNewDevice = !deviceRegistry.hasDevice(msg.deviceId);
  
  if (isNewDevice) {
    onDeviceDiscovered(msg.deviceId, msg);
  } else {
    // Update last seen time for existing device
    deviceRegistry.updateDeviceLastSeen(msg.deviceId);
  }
  
  // Forward sensor values to MQTT
  if (msg.messageType == 1) {  // Sensor update
    EntityInfo* entity = deviceRegistry.getEntity(msg.deviceId, msg.entityId);
    if (entity) {
      mqtt.publishSensorValue(msg.deviceId, msg.entityId, entity->name, msg);
      
      Serial.print("Published sensor value to MQTT: ");
      switch (msg.valueType) {
        case ValueType::BOOLEAN:
          Serial.println(msg.value.boolValue ? "ON" : "OFF");
          break;
        case ValueType::INT_VALUE:
          Serial.println(msg.value.intValue);
          break;
        case ValueType::FLOAT_VALUE:
          Serial.println(msg.value.floatValue);
          break;
        default:
          Serial.println("N/A");
      }
    }
  }
}

void handleMqttMessage(const char* topic, const byte* payload, unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.println(topic);
  
  // Parse topic: lora_gateway/device_{deviceId}/entity_{entityId}/command
  uint16_t deviceId = 0;
  uint8_t entityId = 0;
  
  char topicCopy[256];
  strncpy(topicCopy, topic, sizeof(topicCopy) - 1);
  topicCopy[sizeof(topicCopy) - 1] = '\0';
  
  // Simple parsing - extract device ID and entity ID from topic
  char* deviceStr = strstr(topicCopy, "device_");
  char* entityStr = strstr(topicCopy, "entity_");
  
  if (deviceStr && entityStr) {
    deviceStr += 7;  // Skip "device_"
    entityStr += 7; // Skip "entity_"
    
    deviceId = atoi(deviceStr);
    entityId = atoi(entityStr);
    
    // Check if device and entity exist
    EntityInfo* entity = deviceRegistry.getEntity(deviceId, entityId);
    if (entity) {
      Serial.print("Command for Device ");
      Serial.print(deviceId);
      Serial.print(", Entity ");
      Serial.print(entityId);
      Serial.print(" (");
      Serial.print(entity->name);
      Serial.println(")");
      
      // Parse command payload
      if (length > 0) {
        char payloadStr[256];
        strncpy(payloadStr, (const char*)payload, length);
        payloadStr[length] = '\0';
        
        Serial.print("Payload: ");
        Serial.println(payloadStr);
        
        // For simple commands, parse the JSON payload
        // This is a basic example - could be extended for more complex payloads
        // Expected format: {"command": "ON"} or {"value": 128}
        
        uint8_t commandValue[4] = {0};
        CommandType cmd = CommandType::SET_STATE;
        
        // Simple string comparison for common commands
        if (strstr(payloadStr, "\"command\":\"ON\"") || strstr(payloadStr, "\"state\":\"on\"")) {
          cmd = CommandType::SET_STATE;
          commandValue[0] = 1;
        } else if (strstr(payloadStr, "\"command\":\"OFF\"") || strstr(payloadStr, "\"state\":\"off\"")) {
          cmd = CommandType::SET_STATE;
          commandValue[0] = 0;
        } else if (strstr(payloadStr, "\"command\":\"OPEN\"")) {
          cmd = CommandType::OPEN;
        } else if (strstr(payloadStr, "\"command\":\"CLOSE\"")) {
          cmd = CommandType::CLOSE;
        } else if (strstr(payloadStr, "\"command\":\"STOP\"")) {
          cmd = CommandType::STOP;
        } else if (strstr(payloadStr, "\"value\"")) {
          cmd = CommandType::SET_POSITION;
          // Extract numeric value - simplified parsing
          char* valueStr = strstr(payloadStr, "\"value\":");
          if (valueStr) {
            int value = atoi(valueStr + 8);
            commandValue[0] = value & 0xFF;
            commandValue[1] = (value >> 8) & 0xFF;
          }
        }
        
        sendMqttCommandToDevice(deviceId, entityId, cmd, commandValue);
      }
    }
  }
}

void onDeviceDiscovered(uint16_t deviceId, const LoRaMessage& msg) {
  Serial.print("New device discovered! Device ID: ");
  Serial.println(deviceId);
  
  // Register the device
  String deviceName = String("LoRa Device ") + deviceId;
  if (!deviceRegistry.registerDevice(deviceId, deviceName.c_str())) {
    Serial.println("Failed to register device!");
    return;
  }
  
  // Register the entity that announced itself
  EntityInfo entity;
  entity.deviceId = deviceId;
  entity.entityId = msg.entityId;
  entity.type = msg.entityType;
  entity.name = "Unknown Entity";
  entity.unit = "";
  entity.valueType = msg.valueType;
  entity.minValue = 0;
  entity.maxValue = 100;
  
  if (deviceRegistry.registerEntity(deviceId, entity)) {
    Serial.print("Entity registered: ");
    Serial.println(entity.name);
    
    // Publish Home Assistant discovery for this entity
    publishDeviceDiscovery(deviceId, msg.entityId);
  }
}

void sendMqttCommandToDevice(uint16_t deviceId, uint8_t entityId, CommandType cmd, const uint8_t* value) {
  LoRaMessage cmdMsg;
  cmdMsg.deviceId = deviceId;
  cmdMsg.entityId = entityId;
  cmdMsg.messageType = 3;  // Command message
  cmdMsg.command = cmd;
  
  memcpy(cmdMsg.commandValue, value, 4);
  
  // Determine entity type for the command
  EntityInfo* entity = deviceRegistry.getEntity(deviceId, entityId);
  if (entity) {
    cmdMsg.entityType = entity->type;
    cmdMsg.valueType = entity->valueType;
    
    if (loRa.sendMessage(cmdMsg)) {
      Serial.print("Command sent to Device ");
      Serial.print(deviceId);
      Serial.print(", Entity ");
      Serial.println(entityId);
    } else {
      Serial.println("Failed to send command!");
    }
  }
}

void publishDeviceDiscovery(uint16_t deviceId, uint8_t entityId) {
  EntityInfo* entity = deviceRegistry.getEntity(deviceId, entityId);
  if (entity) {
    if (mqtt.publishDiscovery(*entity, "lora_gateway")) {
      Serial.print("Published Home Assistant discovery for entity ");
      Serial.println(entity->name);
    } else {
      Serial.println("Failed to publish discovery!");
    }
  }
}