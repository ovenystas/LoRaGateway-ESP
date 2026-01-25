#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Config.h"
#include "Types.h"
#include "LoRaHandler.h"
#include "MqttHandler.h"
#include "NodeRegistry.h"

// Global instances
WiFiClient wifiClient;
LoRaHandler loRa(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO_PIN);
MqttHandler mqtt(wifiClient);
NodeRegistry nodeRegistry;

// Timing variables
unsigned long lastMqttCheckTime = 0;
unsigned long lastNodeTimeoutCheck = 0;
const unsigned long MQTT_CHECK_INTERVAL = 5000;        // Check MQTT connection every 5 seconds
const unsigned long NODE_TIMEOUT_CHECK_INTERVAL = 60000; // Check node timeouts every minute

// Forward declarations
void setupWiFi();
void handleLoRaMessage(const LoRaMessage& msg);
void handleMqttMessage(const char* topic, const byte* payload, unsigned int length);
void onNodeDiscovered(uint16_t nodeId, const LoRaMessage& msg);
void sendMqttCommandToNode(uint16_t nodeId, uint8_t deviceId, CommandType cmd, const uint8_t* value);
void publishNodeDiscovery(uint16_t nodeId, uint8_t deviceId);

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
      
      if (mqtt.connect(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID)) {
        Serial.println("MQTT connected!");
        
        // Resubscribe to all device command topics
        uint8_t nodeCount = 0;
        NodeInfo** nodes = nodeRegistry.getAllNodes(nodeCount);
        if (nodes) {
          for (uint8_t i = 0; i < nodeCount; i++) {
            for (uint8_t j = 0; j < nodes[i]->deviceCount; j++) {
              mqtt.subscribeToCommands(nodes[i]->nodeId, nodes[i]->devices[j].deviceId);
            }
          }
        }
      } else {
        Serial.println("MQTT connection failed!");
      }
    }
  }
  
  // Handle node timeout checks
  if (currentTime - lastNodeTimeoutCheck >= NODE_TIMEOUT_CHECK_INTERVAL) {
    lastNodeTimeoutCheck = currentTime;
    
    uint8_t nodeCount = 0;
    NodeInfo** nodes = nodeRegistry.getAllNodes(nodeCount);
    if (nodes) {
      for (uint8_t i = 0; i < nodeCount; i++) {
        if (currentTime - nodes[i]->lastSeen > (NODE_TIMEOUT_SECONDS * 1000)) {
          Serial.print("Node ");
          Serial.print(nodes[i]->nodeId);
          Serial.println(" has timed out - removing from registry");
          nodeRegistry.unregisterNode(nodes[i]->nodeId);
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
  Serial.print("LoRa message received from Node ");
  Serial.print(msg.nodeId);
  Serial.print(", Device ");
  Serial.println(msg.deviceId);
  
  // Check if this is a new node
  bool isNewNode = !nodeRegistry.hasNode(msg.nodeId);
  
  if (isNewNode) {
    onNodeDiscovered(msg.nodeId, msg);
  } else {
    // Update last seen time for existing node
    nodeRegistry.updateNodeLastSeen(msg.nodeId);
  }
  
  // Forward sensor values to MQTT
  if (msg.messageType == 1) {  // Sensor update
    DeviceInfo* device = nodeRegistry.getDevice(msg.nodeId, msg.deviceId);
    if (device) {
      mqtt.publishSensorValue(msg.nodeId, msg.deviceId, device->name, msg);
      
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
  
  // Parse topic: lora_gateway/node_{nodeId}/device_{deviceId}/command
  uint16_t nodeId = 0;
  uint8_t deviceId = 0;
  
  char topicCopy[256];
  strncpy(topicCopy, topic, sizeof(topicCopy) - 1);
  topicCopy[sizeof(topicCopy) - 1] = '\0';
  
  // Simple parsing - extract node ID and device ID from topic
  char* nodeStr = strstr(topicCopy, "node_");
  char* deviceStr = strstr(topicCopy, "device_");
  
  if (nodeStr && deviceStr) {
    nodeStr += 5;  // Skip "node_"
    deviceStr += 7; // Skip "device_"
    
    nodeId = atoi(nodeStr);
    deviceId = atoi(deviceStr);
    
    // Check if node and device exist
    DeviceInfo* device = nodeRegistry.getDevice(nodeId, deviceId);
    if (device) {
      Serial.print("Command for Node ");
      Serial.print(nodeId);
      Serial.print(", Device ");
      Serial.print(deviceId);
      Serial.print(" (");
      Serial.print(device->name);
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
        
        sendMqttCommandToNode(nodeId, deviceId, cmd, commandValue);
      }
    }
  }
}

void onNodeDiscovered(uint16_t nodeId, const LoRaMessage& msg) {
  Serial.print("New node discovered! Node ID: ");
  Serial.println(nodeId);
  
  // Register the node
  String nodeName = String("LoRa Node ") + nodeId;
  if (!nodeRegistry.registerNode(nodeId, nodeName.c_str())) {
    Serial.println("Failed to register node!");
    return;
  }
  
  // Register the device that announced itself
  DeviceInfo device;
  device.nodeId = nodeId;
  device.deviceId = msg.deviceId;
  device.type = msg.deviceType;
  device.name = "Unknown Device";
  device.unit = "";
  device.valueType = msg.valueType;
  device.minValue = 0;
  device.maxValue = 100;
  
  if (nodeRegistry.registerDevice(nodeId, device)) {
    Serial.print("Device registered: ");
    Serial.println(device.name);
    
    // Publish Home Assistant discovery for this device
    publishNodeDiscovery(nodeId, msg.deviceId);
  }
}

void sendMqttCommandToNode(uint16_t nodeId, uint8_t deviceId, CommandType cmd, const uint8_t* value) {
  LoRaMessage cmdMsg;
  cmdMsg.nodeId = nodeId;
  cmdMsg.deviceId = deviceId;
  cmdMsg.messageType = 3;  // Command message
  cmdMsg.command = cmd;
  
  memcpy(cmdMsg.commandValue, value, 4);
  
  // Determine device type for the command
  DeviceInfo* device = nodeRegistry.getDevice(nodeId, deviceId);
  if (device) {
    cmdMsg.deviceType = device->type;
    cmdMsg.valueType = device->valueType;
    
    if (loRa.sendMessage(cmdMsg)) {
      Serial.print("Command sent to Node ");
      Serial.print(nodeId);
      Serial.print(", Device ");
      Serial.println(deviceId);
    } else {
      Serial.println("Failed to send command!");
    }
  }
}

void publishNodeDiscovery(uint16_t nodeId, uint8_t deviceId) {
  DeviceInfo* device = nodeRegistry.getDevice(nodeId, deviceId);
  if (device) {
    if (mqtt.publishDiscovery(*device, "lora_gateway")) {
      Serial.print("Published Home Assistant discovery for device ");
      Serial.println(device->name);
    } else {
      Serial.println("Failed to publish discovery!");
    }
  }
}