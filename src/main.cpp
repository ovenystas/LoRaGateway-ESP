#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "Config.h"
#include "DeviceRegistry.h"
#include "LoRaHandler.h"
#include "LoRaMsgHandler.h"
#include "MqttHandler.h"
#include "MqttMsgHandler.h"
#include "Types.h"
#include "Util.h"
#include "WebServerHandler.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#define LORA_MY_ADDRESS 0

// Global instances
WiFiClient wifiClient;
LoRaHandler loRa(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO_PIN);
LoRaMsgHandler loRaMsg(loRa, LORA_MY_ADDRESS);
MqttHandler mqtt(wifiClient);
DeviceRegistry deviceRegistry;
MqttMsgHandler mqttMsg(loRaMsg, deviceRegistry);
WebServerHandler webServer(loRaMsg, deviceRegistry);

// Timing variables
unsigned long lastMqttCheckTime = 0;
unsigned long lastDeviceTimeoutCheck = 0;
unsigned long lastPingTime = 0;
uint8_t pingMessageId = 0;  // Track ping message ID
const unsigned long MQTT_CHECK_INTERVAL =
    5000;  // Check MQTT connection every 5 seconds
const unsigned long DEVICE_TIMEOUT_CHECK_INTERVAL =
    60000;  // Check device timeouts every minute
const unsigned long PING_INTERVAL =
    10000;  // Send ping request every 10 seconds

// Non-blocking WiFi connection
enum class WiFiState { NEED_CONNECT, CONNECTING, CONNECTED, BACKOFF };
static WiFiState wifiState = WiFiState::NEED_CONNECT;
static unsigned long wifiLastActionTime = 0;
static uint8_t wifiConnectAttempts = 0;
static const unsigned long WIFI_POLL_INTERVAL =
    500;  // Poll connection status every 500 ms while connecting
static const unsigned long WIFI_BACKOFF_INTERVAL =
    10000;  // Wait 10 s before retrying after a failed attempt
static const uint8_t WIFI_MAX_POLL_ATTEMPTS =
    20;  // ~10 s of polling before giving up and backing off

// Forward declarations
static void printWelcomeMessage(void);
static void setupSpiffs();
static void setupLoRa(void);
static void handleWiFi(void);
static void setupOta(void);
static void setupMqtt(void);

static void onDiscoveryMessage(uint8_t deviceId,
                               const DiscoveryItem& discovery);
static void onValueMessage(uint8_t deviceId,
                           const std::vector<ValueItem>& valueItems);
static void sendMqttCommandToDevice(uint8_t deviceId, uint8_t entityId,
                                    int32_t value);
static void publishDeviceDiscovery(uint8_t deviceId,
                                   const DiscoveryItem& discovery);

// Main setup function
void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial) {
    // Do nothing
  }

  printWelcomeMessage();
  delay(5000);

  // Setup SPIFFS File system for web server files
  setupSpiffs();

  // LoRa setup and set message callback
  loRaMsg.setOnDiscoveryMessage(onDiscoveryMessage);
  loRaMsg.setOnValueMessage(onValueMessage);
  setupLoRa();

  // WiFi setup (kick off the non-blocking connection state machine)
  handleWiFi();

  // OTA setup (only meaningful once WiFi is connected)
  if (WiFi.status() == WL_CONNECTED) {
    setupOta();
  }

  // MQTT setup and set message callback
  mqtt.setOnMessageReceived(mqttMsg.handleMessage);
  if (WiFi.status() == WL_CONNECTED) {
    setupMqtt();
  }

  // Send initial discovery request to all devices
  Serial.println(F("Sending initial discovery request to all devices..."));
  loRaMsg.sendDiscoveryRequest(LoRaMsgHandler::LORA_BROADCAST_ADDRESS);
}

// Main loop function
void loop() {
  handleWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    // Start WebServer and OTA if not already started and WiFi is connected
    static bool webServerStarted = false;
    if (!webServerStarted) {
      webServer.begin();
      setupOta();
      webServerStarted = true;
    }

    // Process OTA update requests
    ArduinoOTA.handle();
  }

  // Handle MQTT reconnection
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.isConnected()) {
      setupMqtt();
    }
  }

  // Handle device timeout checks
  unsigned long currentTime = millis();
  if (currentTime - lastDeviceTimeoutCheck >= DEVICE_TIMEOUT_CHECK_INTERVAL) {
    lastDeviceTimeoutCheck = currentTime;

    uint8_t deviceCount = 0;
    DeviceInfo** devices = deviceRegistry.getAllDevices(deviceCount);
    if (devices) {
      for (uint8_t i = 0; i < deviceCount; i++) {
        if (currentTime - devices[i]->lastSeen >
            (NODE_TIMEOUT_SECONDS * 1000)) {
          Serial.print(F("Device "));
          Serial.print(devices[i]->deviceId);
          Serial.println(F(" has timed out - removing from registry"));
          deviceRegistry.unregisterDevice(devices[i]->deviceId);
        }
      }
    }
  }

#if 0
  // Send ping request to device with address 1 every 3 seconds
  if (currentTime - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentTime;

    const uint8_t targetDeviceId = 1;
    loRaMsg.sendPingRequest(targetDeviceId);

    Serial.print(F("Sent ping request to Device "));
    Serial.println(targetDeviceId);
  }
#endif

  // Process LoRa messages
  loRa.handle();

  // Process MQTT events
  mqtt.handle();

  // Process WebServer events
  webServer.handle();

  delay(10);  // Small delay to prevent watchdog timeout
}

static void printVersion(uint8_t major, uint8_t minor, uint8_t patch) {
  Serial.print(major);
  Serial.print('.');
  Serial.print(minor);
  Serial.print('.');
  Serial.print(patch);
}

static void printWelcomeMessage(void) {
  Serial.print(F("\n\nLoRa Gateway Device v"));
  printVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  Serial.print(F(", Address="));
  Serial.println(LORA_MY_ADDRESS);
}

// Number of retries before giving up and restarting the device. A hard
// (infinite) lockup is undesirable for a gateway that is expected to run
// unattended - if SPIFFS or the LoRa module fail to initialize (e.g. due to
// a transient power-up glitch), we retry a few times and then perform a
// clean restart instead of hanging forever.
static const uint8_t INIT_MAX_ATTEMPTS = 5;
static const unsigned long INIT_RETRY_DELAY_MS = 1000;

static void setupSpiffs() {
  Serial.print(F("Initializing SPIFFS."));

  uint8_t attempts = 0;
  while (!SPIFFS.begin(true)) {
    attempts++;
    Serial.print('.');

    if (attempts >= INIT_MAX_ATTEMPTS) {
      Serial.println(F(" Failed! Restarting device..."));
      delay(INIT_RETRY_DELAY_MS);
      ESP.restart();
    }

    delay(INIT_RETRY_DELAY_MS);
  }

  Serial.println(F(" OK."));
}

static void setupLoRa() {
  Serial.print(F("Initializing LoRa."));

  uint8_t attempts = 0;
  while (!loRa.begin(LORA_FREQUENCY)) {
    attempts++;
    Serial.print('.');

    if (attempts >= INIT_MAX_ATTEMPTS) {
      Serial.println(F(" Failed! Restarting device..."));
      delay(INIT_RETRY_DELAY_MS);
      ESP.restart();
    }

    delay(INIT_RETRY_DELAY_MS);
  }

  Serial.println(F(" OK."));
}

// Sets up ArduinoOTA so firmware can be uploaded over WiFi using
// `pio run -e nodemcu-32s-ota -t upload`. Safe to call multiple times;
// initialization only happens once.
static void setupOta() {
  static bool otaStarted = false;
  if (otaStarted) {
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);

  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA
      .onStart([]() {
        String type =
            (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.print(F("OTA: Start updating "));
        Serial.println(type);
      })
      .onEnd([]() { Serial.println(F("\nOTA: Update complete, rebooting.")); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA: Progress %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("OTA: Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println(F("Auth Failed"));
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println(F("Begin Failed"));
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println(F("Connect Failed"));
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println(F("Receive Failed"));
        } else if (error == OTA_END_ERROR) {
          Serial.println(F("End Failed"));
        }
      });

  ArduinoOTA.begin();

  otaStarted = true;

  Serial.print(F("OTA ready. Hostname: "));
  Serial.print(OTA_HOSTNAME);
  Serial.println(F(".local"));
}

// Non-blocking WiFi connect/reconnect handler. This is a simple state
// machine driven by millis() so it never blocks the LoRa/MQTT/WebServer
// processing in loop():
//

//   NEED_CONNECT -> kick off WiFi.begin(), move to CONNECTING
//   CONNECTING   -> poll WiFi.status() every WIFI_POLL_INTERVAL ms;
//                   on success move to CONNECTED, on too many failed
//                   polls move to BACKOFF
//   CONNECTED    -> do nothing while connected; if the link drops, go
//                   back to NEED_CONNECT
//   BACKOFF      -> wait WIFI_BACKOFF_INTERVAL ms before trying again
static void handleWiFi() {
  const unsigned long now = millis();

  switch (wifiState) {
    case WiFiState::NEED_CONNECT: {
      Serial.print(F("Connecting to WiFi SSID "));
      Serial.print(WIFI_SSID);
      Serial.print('.');

      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      wifiConnectAttempts = 0;
      wifiLastActionTime = now;
      wifiState = WiFiState::CONNECTING;
      break;
    }

    case WiFiState::CONNECTING: {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F(" Connected. IP address="));
        Serial.print(WiFi.localIP());
        Serial.println('.');
        wifiState = WiFiState::CONNECTED;
        break;
      }

      if (now - wifiLastActionTime >= WIFI_POLL_INTERVAL) {
        wifiLastActionTime = now;
        wifiConnectAttempts++;
        Serial.print('.');

        if (wifiConnectAttempts >= WIFI_MAX_POLL_ATTEMPTS) {
          Serial.println(F(" Failed! Will retry after backoff."));
          wifiLastActionTime = now;
          wifiState = WiFiState::BACKOFF;
        }
      }
      break;
    }

    case WiFiState::CONNECTED: {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi disconnected, attempting to reconnect..."));
        wifiState = WiFiState::NEED_CONNECT;
      }
      break;
    }

    case WiFiState::BACKOFF: {
      if (now - wifiLastActionTime >= WIFI_BACKOFF_INTERVAL) {
        wifiState = WiFiState::NEED_CONNECT;
      }
      break;
    }
  }
}

static void setupMqtt() {
  Serial.print(F("Connecting to MQTT broker at "));
  Serial.print(MQTT_BROKER);
  Serial.print(':');
  Serial.print(MQTT_PORT);
  Serial.print('.');

  if (mqtt.connect(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME,
                   MQTT_PASSWORD)) {
    Serial.println(F(" Connected."));

    // Resubscribe to all entity command topics for entities that can receive
    // commands
    uint8_t deviceCount = 0;
    DeviceInfo** devices = deviceRegistry.getAllDevices(deviceCount);
    if (devices) {
      for (uint8_t i = 0; i < deviceCount; i++) {
        for (uint8_t j = 0; j < devices[i]->entityCount; j++) {
          // Only subscribe to command topics for entities that can receive
          // commands
          const EntityDomain::Domain domain =
              devices[i]->entities[j].domain.getDomain();
          if (domain == EntityDomain::Domain::COVER ||
              domain == EntityDomain::Domain::NUMBER) {
            mqtt.subscribeToCommands(devices[i]->deviceId,
                                     devices[i]->entities[j].entityId, domain);
          }
        }
      }
    }
  } else {
    Serial.println(F(" Failed!"));
  }
}

static void onDiscoveryMessage(uint8_t deviceId,
                               const DiscoveryItem& discovery) {
  Serial.print(F("New entity discovered on Device "));
  Serial.print(deviceId);
  Serial.print(F("! Entity ID: "));
  Serial.println(discovery.entityId);

  // Register the device
  String deviceName = String("LoRa Device ") + deviceId;
  if (!deviceRegistry.registerDevice(deviceId, deviceName.c_str())) {
    Serial.println(F("Failed to register device!"));
    return;
  }
  deviceRegistry.updateDeviceLastSeen(deviceId);
  Serial.println("Device registered: " + deviceName);
  deviceRegistry.getDevice(deviceId)->print(Serial, 2);

  // Register the entity
  EntityInfo entity;
  entity.deviceId = deviceId;
  entity.entityId = discovery.entityId;
  entity.domain = discovery.domain;
  entity.setDeviceClass(discovery.deviceClass);
  entity.category = discovery.category;
  entity.format = discovery.format;
  entity.unit = discovery.unit;
  entity.minValue = discovery.minValue;
  entity.maxValue = discovery.maxValue;
  entity.name = discovery.name;

  if (deviceRegistry.registerEntity(deviceId, entity)) {
    Serial.print(F("Entity registered: "));
    Serial.println(entity.name);
    entity.print(Serial, 2);

    // Subscribe to command topic for entities that can receive commands (e.g.,
    // covers and numbers)
    if (mqtt.isConnected()) {
      const EntityDomain::Domain domain = entity.domain.getDomain();
      if (domain == EntityDomain::Domain::COVER ||
          domain == EntityDomain::Domain::NUMBER) {
        mqtt.subscribeToCommands(deviceId, discovery.entityId, domain);
        Serial.print(F("Subscribed to command topic for Entity "));
        Serial.println(discovery.entityId);
      }
    }

    // Publish Home Assistant discovery for this entity
    Serial.println(F("Publishing Home Assistant discovery..."));
    publishDeviceDiscovery(deviceId, discovery);
  }
}

static void onValueMessage(uint8_t deviceId,
                           const std::vector<ValueItem>& valueItems) {
  Serial.print(F("Value message from Device "));
  Serial.print(deviceId);
  Serial.println(':');

  deviceRegistry.updateDeviceLastSeen(deviceId);

  DeviceInfo* device = deviceRegistry.getDevice(deviceId);

  for (const ValueItem& valueItem : valueItems) {
    EntityInfo* entity = device->getEntity(valueItem.entityId);
    if (entity) {
      Serial.print(F("    Value for entity "));
      Serial.print(entity->name);
      Serial.print(": ");
      float scaledValue = entity->format.fromRawValue(valueItem.value);
      Serial.print(scaledValue, entity->format.getPrecision());
      Serial.print(' ');
      Serial.println(entity->unit.getName());
    } else {
      Serial.println(F("    Entity not found, sending discovery request"));
      loRaMsg.sendDiscoveryRequest(deviceId, valueItem.entityId);
    }
  }

  // Forward sensor values to MQTT
  mqtt.publishSensorValues(*device, valueItems);
  Serial.println(F("    Published sensor values"));
}

static void sendMqttCommandToDevice(uint8_t deviceId, uint8_t entityId,
                                    int32_t value) {
  LoRaTxMessage cmdMsg;
  cmdMsg.header = LoRaHeader(deviceId, 0, 0, LoRaMsgType::valueSet_req);

  // Create a simple command payload with the value
  cmdMsg.payloadLength = 0;
  cmdMsg.payload[cmdMsg.payloadLength++] = entityId;  // Target entity

  // Add the value (4 bytes big-endian)
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 24) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 16) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 8) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = value & 0xFF;

  if (loRa.sendMessage(cmdMsg)) {
    Serial.print(F("Command sent to Device "));
    Serial.print(deviceId);
    Serial.print(F(", Entity "));
    Serial.print(entityId);
    Serial.print(F(", Value: "));
    Serial.println(value);
  } else {
    Serial.println(F("Failed to send command!"));
  }
}

static void publishDeviceDiscovery(uint8_t deviceId,
                                   const DiscoveryItem& discovery) {
  EntityInfo* entity = deviceRegistry.getEntity(deviceId, discovery.entityId);
  if (entity) {
    if (mqtt.publishDiscovery(*entity, MQTT_CLIENT_ID)) {
      Serial.print(F("Published Home Assistant discovery for entity "));
      Serial.println(entity->name);
    } else {
      Serial.println(F("Failed to publish discovery!"));
    }
  } else {
    Serial.println(
        F("WRN: Entity not found in registry, cannot publish discovery"));
  }
}
