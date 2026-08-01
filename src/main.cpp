#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "Config.h"
#include "DeviceRegistry.h"
#include "LoRaHandler.h"
#include "LoRaMsgHandler.h"
#include "Logger.h"
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
unsigned long lastGatewayStatusTime = 0;
uint8_t pingMessageId = 0;  // Track ping message ID
const unsigned long MQTT_CHECK_INTERVAL =
    5000;  // Check MQTT connection every 5 seconds
const unsigned long DEVICE_TIMEOUT_CHECK_INTERVAL =
    60000;  // Check device timeouts every minute
const unsigned long PING_INTERVAL =
    10000;  // Send ping request every 10 seconds

// Gateway status tracking
static bool gatewayDiscoveryPublished = false;
static bool loraInitialized = false;
static bool lastWifiConnected = false;
static bool lastMqttConnected = false;
static uint8_t lastDeviceCount = 0;

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

  // Register callbacks for LoRa message handling
  loRaMsg.setOnDiscoveryMessage(onDiscoveryMessage);
  loRaMsg.setOnValueMessage(onValueMessage);

  // Send initial discovery request to all devices
  logger.println(Logger::INF,
                 F("Sending initial discovery request to all devices..."));
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
          logger.printf(Logger::INF,
                        "Device %u has timed out - removing from registry",
                        devices[i]->deviceId);
          deviceRegistry.unregisterDevice(devices[i]->deviceId);
        }
      }
    }
  }

#if 0
  if (currentTime - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentTime;
    const uint8_t targetDeviceId = 1;
    loRaMsg.sendPingRequest(targetDeviceId);
    logger.printf(Logger::INF, "Sent ping request to Device %u", targetDeviceId);
  }
#endif

  // Process LoRa messages
  loRa.handle();

  // Process MQTT events
  mqtt.handle();

  publishGatewayStatus(currentTime);

  delay(10);  // Small delay to prevent watchdog timeout
}

void publishGatewayStatus(unsigned long currentTime) {
  // Publish gateway status periodically and on state changes
  {
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    bool mqttConnected = mqtt.isConnected();
    uint8_t deviceCount = deviceRegistry.getDeviceCount();

    bool stateChanged = (wifiConnected != lastWifiConnected) ||
                        (mqttConnected != lastMqttConnected) ||
                        (deviceCount != lastDeviceCount);

    // Publish gateway discovery once after MQTT connects
    if (mqttConnected && !gatewayDiscoveryPublished) {
      mqtt.publishGatewayDiscovery(MQTT_CLIENT_ID);
      gatewayDiscoveryPublished = true;
    }

    // Publish status on interval or state change
    if (mqttConnected && (stateChanged || currentTime - lastGatewayStatusTime >=
                                              GATEWAY_STATUS_INTERVAL)) {
      lastGatewayStatusTime = currentTime;
      lastWifiConnected = wifiConnected;
      lastMqttConnected = mqttConnected;
      lastDeviceCount = deviceCount;

      char versionStr[16];
      snprintf(versionStr, sizeof(versionStr), "%u.%u.%u", VERSION_MAJOR,
               VERSION_MINOR, VERSION_PATCH);

      mqtt.publishGatewayStatus(
          currentTime / 1000,               // uptime in seconds
          wifiConnected,                    // wifi_connected
          wifiConnected ? WiFi.RSSI() : 0,  // wifi_rssi
          wifiConnected ? WiFi.localIP().toString().c_str() : "0.0.0.0",  // ip
          mqttConnected,            // mqtt_connected
          ESP.getFreeHeap(),        // free_heap
          versionStr,               // version
          deviceCount,              // device_count
          loraInitialized,          // lora_ok
          loRa.getPacketRxCount(),  // lora_rx
          loRa.getPacketTxCount(),  // lora_tx
          mqtt.getMqttRxCount(),    // mqtt_rx
          mqtt.getMqttTxCount()     // mqtt_tx
      );
    }
  }
}

static void printWelcomeMessage(void) {
  logger.printf(Logger::INF, "\n\nLoRa Gateway Device v%u.%u.%u, Address=%u",
                VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, LORA_MY_ADDRESS);
}

// Number of retries before giving up and restarting the device. A hard
// (infinite) lockup is undesirable for a gateway that is expected to run
// unattended - if SPIFFS or the LoRa module fail to initialize (e.g. due to
// a transient power-up glitch), we retry a few times and then perform a
// clean restart instead of hanging forever.
static const uint8_t INIT_MAX_ATTEMPTS = 5;
static const unsigned long INIT_RETRY_DELAY_MS = 1000;

static void setupSpiffs() {
  logger.print(Logger::INF, F("Initializing SPIFFS."));

  uint8_t attempts = 0;
  while (!SPIFFS.begin(true)) {
    attempts++;
    logger.print(Logger::INF, '.');

    if (attempts >= INIT_MAX_ATTEMPTS) {
      logger.println(Logger::ERR, F(" Failed! Restarting device..."));
      delay(INIT_RETRY_DELAY_MS);
      ESP.restart();
    }

    delay(INIT_RETRY_DELAY_MS);
  }

  logger.println(Logger::INF, F(" OK."));
}

static void setupLoRa() {
  logger.print(Logger::INF, F("Initializing LoRa."));

  uint8_t attempts = 0;
  while (!loRa.begin(LORA_FREQUENCY)) {
    attempts++;
    logger.print(Logger::INF, '.');

    if (attempts >= INIT_MAX_ATTEMPTS) {
      logger.println(Logger::ERR, F(" Failed! Restarting device..."));
      delay(INIT_RETRY_DELAY_MS);
      ESP.restart();
    }

    delay(INIT_RETRY_DELAY_MS);
  }

  logger.println(Logger::INF, F(" OK."));
  loraInitialized = true;
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
        logger.printf(Logger::INF, "OTA: Start updating %s", type.c_str());
      })
      .onEnd([]() {
        logger.println(Logger::INF, F("OTA: Update complete, rebooting."));
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        logger.printf(Logger::DBG, "OTA: Progress %u%%\r",
                      (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        logger.printf(Logger::ERR, "OTA: Error[%u]: ", error);
        switch (error) {
          case OTA_AUTH_ERROR:
            logger.println(Logger::ERR, F("Auth Failed"));
            break;
          case OTA_BEGIN_ERROR:
            logger.println(Logger::ERR, F("Begin Failed"));
            break;
          case OTA_CONNECT_ERROR:
            logger.println(Logger::ERR, F("Connect Failed"));
            break;
          case OTA_RECEIVE_ERROR:
            logger.println(Logger::ERR, F("Receive Failed"));
            break;
          case OTA_END_ERROR:
            logger.println(Logger::ERR, F("End Failed"));
            break;
        }
      });

  ArduinoOTA.begin();

  otaStarted = true;

  logger.printf(Logger::INF, "OTA ready. Hostname: %s.local", OTA_HOSTNAME);
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
      logger.printf(Logger::INF, "Connecting to WiFi SSID %s.", WIFI_SSID);

      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      wifiConnectAttempts = 0;
      wifiLastActionTime = now;
      wifiState = WiFiState::CONNECTING;
      break;
    }

    case WiFiState::CONNECTING: {
      if (WiFi.status() == WL_CONNECTED) {
        logger.printf(Logger::INF, " Connected. IP address=%s",
                      WiFi.localIP().toString().c_str());
        wifiState = WiFiState::CONNECTED;
        break;
      }

      if (now - wifiLastActionTime >= WIFI_POLL_INTERVAL) {
        wifiLastActionTime = now;
        wifiConnectAttempts++;
        logger.print(Logger::INF, '.');

        if (wifiConnectAttempts >= WIFI_MAX_POLL_ATTEMPTS) {
          logger.println(Logger::WRN, F(" Failed! Will retry after backoff."));
          wifiLastActionTime = now;
          wifiState = WiFiState::BACKOFF;
        }
      }
      break;
    }

    case WiFiState::CONNECTED: {
      if (WiFi.status() != WL_CONNECTED) {
        logger.println(Logger::WRN,
                       F("WiFi disconnected, attempting to reconnect..."));
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
  logger.printf(Logger::INF, "Connecting to MQTT broker at %s:%u.", MQTT_BROKER,
                MQTT_PORT);

  if (mqtt.connect(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME,
                   MQTT_PASSWORD)) {
    logger.println(Logger::INF, F(" Connected."));

    uint8_t deviceCount = 0;
    DeviceInfo** devices = deviceRegistry.getAllDevices(deviceCount);
    if (devices) {
      for (uint8_t i = 0; i < deviceCount; i++) {
        for (uint8_t j = 0; j < devices[i]->entityCount; j++) {
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
    logger.println(Logger::ERR, F(" Failed!"));
  }
}

static void onDiscoveryMessage(uint8_t deviceId,
                               const DiscoveryItem& discovery) {
  logger.printf(Logger::INF,
                "New entity discovered on Device %u! Entity ID: %u", deviceId,
                discovery.entityId);

  String deviceName = String("LoRa Device ") + deviceId;
  if (!deviceRegistry.registerDevice(deviceId, deviceName.c_str())) {
    logger.println(Logger::ERR, F("Failed to register device!"));
    return;
  }
  deviceRegistry.updateDeviceLastSeen(deviceId);
  logger.printf(Logger::INF, "Device registered: %s", deviceName.c_str());
  deviceRegistry.getDevice(deviceId)->print(logger, 2);

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
    logger.printf(Logger::INF, "Entity registered: %s", entity.name.c_str());
    entity.print(logger, 2);

    if (mqtt.isConnected()) {
      const EntityDomain::Domain domain = entity.domain.getDomain();
      if (domain == EntityDomain::Domain::COVER ||
          domain == EntityDomain::Domain::NUMBER) {
        mqtt.subscribeToCommands(deviceId, discovery.entityId, domain);
        logger.printf(Logger::INF, "Subscribed to command topic for Entity %u",
                      discovery.entityId);
      }
    }

    logger.println(Logger::INF, F("Publishing Home Assistant discovery..."));
    publishDeviceDiscovery(deviceId, discovery);
  }
}

static void onValueMessage(uint8_t deviceId,
                           const std::vector<ValueItem>& valueItems) {
  logger.printf(Logger::INF, "Value message from Device %u:", deviceId);

  deviceRegistry.updateDeviceLastSeen(deviceId);

  DeviceInfo* device = deviceRegistry.getDevice(deviceId);

  for (const ValueItem& valueItem : valueItems) {
    EntityInfo* entity = device->getEntity(valueItem.entityId);
    if (entity) {
      float scaledValue = entity->format.fromRawValue(valueItem.value);
      logger.printf(Logger::DBG, "    Value for entity %s: %f %s",
                    entity->name.c_str(), scaledValue, entity->unit.getName());
    } else {
      logger.println(Logger::WRN,
                     F("    Entity not found, sending discovery request"));
      loRaMsg.sendDiscoveryRequest(deviceId, valueItem.entityId);
    }
  }

  mqtt.publishSensorValues(*device, valueItems);
  logger.println(Logger::DBG, F("    Published sensor values"));
}

static void sendMqttCommandToDevice(uint8_t deviceId, uint8_t entityId,
                                    int32_t value) {
  LoRaTxMessage cmdMsg;
  cmdMsg.header = LoRaHeader(deviceId, 0, 0, LoRaMsgType::valueSet_req);

  cmdMsg.payloadLength = 0;
  cmdMsg.payload[cmdMsg.payloadLength++] = entityId;

  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 24) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 16) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = (value >> 8) & 0xFF;
  cmdMsg.payload[cmdMsg.payloadLength++] = value & 0xFF;

  if (loRa.sendMessage(cmdMsg)) {
    logger.printf(Logger::INF,
                  "Command sent to Device %u, Entity %u, Value: %d", deviceId,
                  entityId, value);
  } else {
    logger.println(Logger::ERR, F("Failed to send command!"));
  }
}

static void publishDeviceDiscovery(uint8_t deviceId,
                                   const DiscoveryItem& discovery) {
  EntityInfo* entity = deviceRegistry.getEntity(deviceId, discovery.entityId);
  if (entity) {
    if (mqtt.publishDiscovery(*entity, MQTT_CLIENT_ID)) {
      logger.printf(Logger::INF,
                    "Published Home Assistant discovery for entity %s",
                    entity->name.c_str());
    } else {
      logger.println(Logger::ERR, F("Failed to publish discovery!"));
    }
  } else {
    logger.println(Logger::WRN,
                   F("Entity not found in registry, cannot publish discovery"));
  }
}