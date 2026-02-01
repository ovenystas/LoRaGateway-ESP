#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "Types.h"
#include "LoRaHandler.h"
#include "MqttHandler.h"
#include "DeviceRegistry.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define LORA_MY_ADDRESS 0

// Global instances
WiFiClient wifiClient;
LoRaHandler loRa(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO_PIN);
MqttHandler mqtt(wifiClient);
DeviceRegistry deviceRegistry;

// Timing variables
unsigned long lastMqttCheckTime = 0;
unsigned long lastDeviceTimeoutCheck = 0;
unsigned long lastPingTime = 0;
uint8_t pingMessageId = 0;  // Track ping message ID
const unsigned long MQTT_CHECK_INTERVAL = 5000;        // Check MQTT connection every 5 seconds
const unsigned long DEVICE_TIMEOUT_CHECK_INTERVAL = 60000; // Check device timeouts every minute
const unsigned long PING_INTERVAL = 10000;              // Send ping request every 10 seconds

// Forward declarations
static void printWelcomeMsg(void);
static void setupWiFi(void);
static void handleLoRaMessage(const LoRaRxMessage& msg);
static void handleMqttMessage(const char* topic, const byte* payload, unsigned int length);
static void onDeviceDiscovered(uint8_t deviceId, const DiscoveryItem& discovery);
static void sendMqttCommandToDevice(uint8_t deviceId, uint8_t entityId, int32_t value);
static void publishDeviceDiscovery(uint8_t deviceId, const DiscoveryItem& discovery);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Do nothing
  }

  printWelcomeMsg();

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

  // Send ping request to device with address 1 every 3 seconds
  if (currentTime - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentTime;

    LoRaTxMessage pingMsg;
    pingMsg.header.dst = 1;  // Target device address
    pingMsg.header.src = LORA_MY_ADDRESS;
    pingMsg.header.id = pingMessageId++;  // Use and increment message ID
    pingMsg.header.flags.msgType = LoRaMsgType::ping_req;
    pingMsg.payloadLength = 0;  // Ping has no payload

    loRa.sendMessage(pingMsg);
    Serial.print("Sent ping request to Device 1 (ID=");
    Serial.print(pingMsg.header.id);
    Serial.print(") at ");
    Serial.println(millis());
  }

  // Process LoRa messages (this will call handleLoRaMessage when a message is received)
  loRa.handle();

  // Process MQTT events
  mqtt.handle();

  delay(10);  // Small delay to prevent watchdog timeout
}

static void printVersion(Print& printer, uint8_t major, uint8_t minor, uint8_t patch) {
  printer.print(major);
  printer.print('.');
  printer.print(minor);
  printer.print('.');
  printer.print(patch);
}

static void printWelcomeMsg(void) {
  Serial.print(F("\n\nLoRa Gateway Device v"));
  printVersion(Serial, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  Serial.print(F(", Address="));
  Serial.println(LORA_MY_ADDRESS);
}

static void setupWiFi() {
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

static void handleLoRaMessage(const LoRaRxMessage& msg) {
  Serial.print("LoRa message received from Device ");
  Serial.print(msg.header.src);
  Serial.print(", Type: ");
  Serial.print(static_cast<uint8_t>(msg.header.flags.msgType));
  Serial.print(", ID: ");
  Serial.print(msg.header.id);
  Serial.print(", Dst: ");
  Serial.print(msg.header.dst);
  Serial.print(", PayloadLen: ");
  Serial.print(msg.payloadLength);
  Serial.print(", RSSI: ");
  Serial.println(msg.rssi);

  // Handle different message types
  switch (msg.header.flags.msgType) {
    case LoRaMsgType::discovery_msg: {
      // Parse discovery payload
      if (msg.payloadLength >= DiscoveryItem::size()) {
        DiscoveryItem discovery;
        discovery.fromByteArray(msg.payload);

        uint8_t deviceId = msg.header.src;
        onDeviceDiscovered(deviceId, discovery);
      }
      break;
    }

    case LoRaMsgType::value_msg: {
      // Parse value payload
      if (msg.payloadLength > 0) {
        ValueItem valueItem;
        valueItem.fromByteArray(msg.payload);

        uint8_t deviceId = msg.header.src;
        deviceRegistry.updateDeviceLastSeen(deviceId);

        // Forward sensor value to MQTT
        EntityInfo* entity = deviceRegistry.getEntity(deviceId, valueItem.entityId);
        if (entity) {
          mqtt.publishSensorValue(deviceId, valueItem.entityId, entity->name, valueItem.value);
          Serial.print("Published sensor value: ");
          Serial.println(valueItem.value);
        }
      }
      break;
    }

    case LoRaMsgType::ping_req: {
      // Send ping response
      LoRaTxMessage response;
      LoRaHandler::setDefaultHeader(response.header, msg.header.src, msg.header.dst,
                                    msg.header.id, LoRaMsgType::ping_msg);
      response.payloadLength = 2;
      response.payload[0] = ((-msg.rssi) >> 8) & 0xFF;
      response.payload[1] = (-msg.rssi) & 0xFF;
      loRa.sendMessage(response);
      break;
    }

    case LoRaMsgType::ping_msg: {
      // Ping response received
      Serial.print("Ping response from Device ");
      Serial.print(msg.header.src);
      Serial.print(" - RSSI: ");
      Serial.print(msg.rssi);
      Serial.println(" dBm");

      // Extract device's RSSI if present in payload
      if (msg.payloadLength >= 2) {
        int16_t deviceRssi = ((int16_t)msg.payload[0] << 8) | msg.payload[1];
        Serial.print("Device's signal strength: ");
        Serial.print(deviceRssi);
        Serial.println(" dBm");
      }
      break;
    }

    default:
      Serial.print("Unknown message type: ");
      Serial.println(static_cast<uint8_t>(msg.header.flags.msgType));
      break;
  }
}

static void handleMqttMessage(const char* topic, const byte* payload, unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.println(topic);

  // Parse topic: lora_gateway/device_{deviceId}/entity_{entityId}/command
  uint8_t deviceId = 0;
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

      // Parse command payload - extract numeric value
      if (length > 0) {
        char payloadStr[256];
        strncpy(payloadStr, (const char*)payload, length);
        payloadStr[length] = '\0';

        Serial.print("Payload: ");
        Serial.println(payloadStr);

        int32_t value = 0;

        // Try to parse JSON "value" field
        char* valueStr = strstr(payloadStr, "\"value\":");
        if (valueStr) {
          value = atoi(valueStr + 8);
        } else if (strstr(payloadStr, "\"state\":\"on\"") || strstr(payloadStr, "\"command\":\"ON\"")) {
          value = 1;
        } else if (strstr(payloadStr, "\"state\":\"off\"") || strstr(payloadStr, "\"command\":\"OFF\"")) {
          value = 0;
        }

        sendMqttCommandToDevice(deviceId, entityId, value);
      }
    }
  }
}

static void onDeviceDiscovered(uint8_t deviceId, const DiscoveryItem& discovery) {
  Serial.print("New entity discovered on Device ");
  Serial.print(deviceId);
  Serial.print("! Entity ID: ");
  Serial.println(discovery.entityId);

  // Register the device if it doesn't exist
  if (!deviceRegistry.hasDevice(deviceId)) {
    String deviceName = String("LoRa Device ") + deviceId;
    if (!deviceRegistry.registerDevice(deviceId, deviceName.c_str())) {
      Serial.println("Failed to register device!");
      return;
    }
  }

  // Register the entity
  EntityInfo entity;
  entity.deviceId = deviceId;
  entity.entityId = discovery.entityId;
  entity.type = discovery.type;
  entity.name = "Unknown Entity";
  entity.unit = "";
  entity.valueType = ValueType::INT_VALUE;
  entity.minValue = 0;
  entity.maxValue = 100;

  if (deviceRegistry.registerEntity(deviceId, entity)) {
    Serial.print("Entity registered: ");
    Serial.println(entity.name);

    // Publish Home Assistant discovery for this entity
    publishDeviceDiscovery(deviceId, discovery);
  }
}

static void sendMqttCommandToDevice(uint8_t deviceId, uint8_t entityId, int32_t value) {
  LoRaTxMessage cmdMsg;
  LoRaHandler::setDefaultHeader(cmdMsg.header, deviceId, 0, 0, LoRaMsgType::configSet_req);

  // Create a simple command payload with the value
  cmdMsg.payloadLength = 0;
  cmdMsg.payload[cmdMsg.payloadLength++] = entityId;  // Target entity

  // Add the value (4 bytes big-endian)
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 24) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 16) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 8) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = value & 0xFF;

  if (loRa.sendMessage(cmdMsg)) {
    Serial.print("Command sent to Device ");
    Serial.print(deviceId);
    Serial.print(", Entity ");
    Serial.print(entityId);
    Serial.print(", Value: ");
    Serial.println(value);
  } else {
    Serial.println("Failed to send command!");
  }
}

static void publishDeviceDiscovery(uint8_t deviceId, const DiscoveryItem& discovery) {
  EntityInfo* entity = deviceRegistry.getEntity(deviceId, discovery.entityId);
  if (entity) {
    if (mqtt.publishDiscovery(*entity, "lora_gateway")) {
      Serial.print("Published Home Assistant discovery for entity ");
      Serial.println(entity->name);
    } else {
      Serial.println("Failed to publish discovery!");
    }
  }
}
