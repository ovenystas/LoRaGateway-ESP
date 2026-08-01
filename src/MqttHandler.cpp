#include "MqttHandler.h"

#include <ArduinoJson.h>
#include <stdio.h>

#include "Logger.h"

MqttHandler::MqttHandler(WiFiClient& wifiClient)
    : wifiClient(wifiClient), onMessageReceived(nullptr) {
  client.setClient(wifiClient);
  client.setBufferSize(MQTT_BUFFER_SIZE);
}

void MqttHandler::setupCallback() {
  client.setCallback([this](char* topic, byte* payload, unsigned int length) {
    mqttRxCount++;
    if (onMessageReceived) {
      onMessageReceived(topic, payload, length);
    }
  });
}

bool MqttHandler::connect(const char* broker, uint16_t port,
                          const char* clientId) {
  if (!broker || !clientId) {
    logger.println(Logger::ERR, F("Error: broker or clientId is null"));
    return false;
  }

  client.setServer(broker, port);
  setupCallback();

  const char* lwtTopic = "lora-gw/status";
  const char* lwtPayload = "{\"status\":\"offline\"}";
  bool result =
      client.connect(clientId, nullptr, nullptr, lwtTopic, 0, true, lwtPayload);

  if (result) {
    client.publish(lwtTopic, "{\"status\":\"online\"}", true);
  }

  return result;
}

bool MqttHandler::connect(const char* broker, uint16_t port,
                          const char* clientId, const char* username,
                          const char* password) {
  if (!broker || !clientId || !username || !password) {
    logger.println(Logger::ERR,
                   F("Error: broker, clientId, username, or password is null"));
    return false;
  }

  client.setServer(broker, port);
  setupCallback();

  const char* lwtTopic = "lora-gw/status";
  const char* lwtPayload = "{\"status\":\"offline\"}";
  bool result = client.connect(clientId, username, password, lwtTopic, 0, true,
                               lwtPayload);

  if (result) {
    client.publish(lwtTopic, "{\"status\":\"online\"}", true);
  }

  return result;
}

void MqttHandler::disconnect() {
  if (client.connected()) {
    client.disconnect();
  }
}

bool MqttHandler::isConnected() { return client.connected(); }

bool MqttHandler::publishSensorValues(
    const DeviceInfo& device, const std::vector<ValueItem>& valueItems) {
  char topic[128];
  char payload[256];

  // Build state topic
  snprintf(topic, sizeof(topic), "lora-gw/device_%u/state", device.deviceId);

  // Create JSON payload
  JsonDocument doc;
  for (const ValueItem& valueItem : valueItems) {
    EntityInfo* entity = device.getEntity(valueItem.entityId);
    if (entity) {
      String key =
          String(entity->entityId) + "_" +
          (entity->deviceClass ? entity->deviceClass->getName() : "unknown");
      switch (entity->domain.getDomain()) {
        case EntityDomain::Domain::BINARY_SENSOR:
          doc[key] = valueItem.value == 0 ? "OFF" : "ON";
          break;
        case EntityDomain::Domain::SENSOR:
          doc[key] = entity->format.fromRawValue(valueItem.value);
          break;
        case EntityDomain::Domain::COVER:
          switch (valueItem.value) {
            case 0:
              doc[key] = "closed";
              break;
            case 1:
              doc[key] = "open";
              break;
            case 2:
              doc[key] = "opening";
              break;
            case 3:
              doc[key] = "closing";
              break;
            default:
              doc[key] = "unknown";
          }
          break;
        default:
          doc[key] = valueItem.value;
      }
    }
  }

  serializeJson(doc, payload, sizeof(payload));

  bool result = client.publish(topic, payload);
  if (result) {
    mqttTxCount++;
  }
  return result;
}

bool MqttHandler::subscribeToCommands(uint8_t deviceId, uint8_t entityId,
                                      EntityDomain::Domain domain) {
  char topic[128];

  const char* topicSuffix = "command";
  if (domain == EntityDomain::Domain::COVER) {
    topicSuffix = "service";
  } else if (domain == EntityDomain::Domain::NUMBER) {
    topicSuffix = "value";
  }

  snprintf(topic, sizeof(topic), "lora-gw/device_%u/entity_%u/%s", deviceId,
           entityId, topicSuffix);

  return client.subscribe(topic);
}

/*
 * Examples:
 *  topic: homeassistant/cover/lora_1/0_garage/config
 *  topic: homeassistant/sensor/lora_1/1_temperature/config
 */
bool MqttHandler::publishDiscovery(const EntityInfo& entity,
                                   const char* nodePrefix) {
  if (!nodePrefix) {
    logger.println(Logger::ERR, F("Error: nodePrefix is null"));
    return false;
  }

  logger.printf(Logger::DBG, "Publishing discovery for entity: %u:%s",
                entity.entityId, entity.name.c_str());

  char topic[128];
  char payload[512];

  const char* componentType = entity.domain.getName();

  String entity_id_name =
      String(entity.entityId) + "_" +
      (entity.deviceClass ? entity.deviceClass->getName() : "unknown");

  // Build discovery topic for Home Assistant
  snprintf(topic, sizeof(topic), "homeassistant/%s/lora_%u/%s/config",
           componentType, entity.deviceId, entity_id_name.c_str());

  // Create discovery payload
  JsonDocument doc;
  doc["name"] = entity.name;
  doc["unique_id"] = String("lora_") + entity.deviceId + "_" + entity_id_name;
  doc["object_id"] = String("lora_") + entity.deviceId + "_" + entity_id_name;
  if (entity.deviceClass) {
    doc["device_class"] = entity.deviceClass->getName();
  }

  // State topic
  char stateTopic[128];
  snprintf(stateTopic, sizeof(stateTopic), "%s/device_%u/state", nodePrefix,
           entity.deviceId);
  doc["state_topic"] = stateTopic;

  // Command topic (for covers and config entities)
  if (entity.domain.getDomain() == EntityDomain::Domain::COVER) {
    char cmdTopic[128];
    snprintf(cmdTopic, sizeof(cmdTopic), "%s/device_%u/entity_%u/service",
             nodePrefix, entity.deviceId, entity.entityId);
    doc["command_topic"] = cmdTopic;
    doc["payload_open"] = "OPEN";
    doc["payload_close"] = "CLOSE";
    doc["payload_stop"] = "STOP";
    doc["state_open"] = "open";
    doc["state_closed"] = "closed";
  } else if (entity.domain.getDomain() == EntityDomain::Domain::NUMBER &&
             entity.category.getType() == EntityCategory::Category::CONFIG) {
    char cmdTopic[128];
    snprintf(cmdTopic, sizeof(cmdTopic), "%s/device_%u/entity_%u/value",
             nodePrefix, entity.deviceId, entity.entityId);
    doc["command_topic"] = cmdTopic;
  }

  // Payload mappings for binary sensors
  if (entity.domain.getDomain() == EntityDomain::Domain::BINARY_SENSOR) {
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
  }

  // Value template
  char valueTemplate[128];
  snprintf(valueTemplate, sizeof(valueTemplate), "{{ value_json['%u_%s'] }}",
           entity.entityId,
           entity.deviceClass ? entity.deviceClass->getName() : "unknown");
  logger.printf(Logger::DBG, "Value template: %s", valueTemplate);
  doc["value_template"] = valueTemplate;

  // Device info
  JsonObject deviceObj = doc["device"].to<JsonObject>();
  deviceObj["identifiers"][0] = String("lora_device_") + entity.deviceId;
  deviceObj["name"] = String("LoRa Device ") + entity.deviceId;

  // Unit of measurement
  const char* unit_name = entity.unit.getName();
  if (strlen(unit_name) > 0) {
    doc["unit_of_measurement"] = unit_name;
  }

  // Entity category (for config entities)
  if (entity.category.getType() == EntityCategory::Category::CONFIG) {
    doc["entity_category"] = "config";
  }

  // State class for sensors
  if (entity.domain.getDomain() == EntityDomain::Domain::SENSOR) {
    doc["state_class"] = "measurement";
  }

  // Min/max values for NUMBER entities
  if (entity.domain.getDomain() == EntityDomain::Domain::NUMBER) {
    doc["min"] = entity.format.fromRawValue(entity.minValue);
    doc["max"] = entity.format.fromRawValue(entity.maxValue);
  }

  serializeJson(doc, payload, sizeof(payload));

  logger.printf(Logger::DBG, "Publishing discovery for entity: %s",
                entity.name.c_str());
  logger.printf(Logger::DBG, "    topic: %s", topic);
  logger.printf(Logger::DBG, "    Payload: %s", payload);
  const bool result =
      client.publish(topic, payload, true);  // Retain discovery message

  if (result) {
    mqttTxCount++;
  }
  return result;
}

bool MqttHandler::publishGatewayStatus(
    uint32_t uptime, bool wifiConnected, int8_t wifiRssi, const char* ip,
    bool mqttConnected, uint32_t freeHeap, const char* version,
    uint8_t deviceCount, bool loraOk, uint32_t loraRxCount,
    uint32_t loraTxCount, uint32_t mqttRxCount, uint32_t mqttTxCount) {
  const char* topic = "lora-gw/status";
  char payload[512];

  JsonDocument doc;
  doc["uptime"] = uptime;
  doc["wifi_connected"] = wifiConnected ? "ON" : "OFF";
  doc["wifi_rssi"] = wifiRssi;
  doc["ip"] = ip ? ip : "0.0.0.0";
  doc["mqtt_connected"] = mqttConnected ? "ON" : "OFF";
  doc["free_heap"] = freeHeap;
  doc["version"] = version ? version : "unknown";
  doc["device_count"] = deviceCount;
  doc["lora_ok"] = loraOk ? "ON" : "OFF";
  doc["lora_rx"] = loraRxCount;
  doc["lora_tx"] = loraTxCount;
  doc["mqtt_rx"] = mqttRxCount;
  doc["mqtt_tx"] = mqttTxCount;

  serializeJson(doc, payload, sizeof(payload));

  bool result = client.publish(topic, payload, true);
  if (result) {
    mqttTxCount++;
  }
  return result;
}

bool MqttHandler::publishGatewayDiscovery(const char* nodePrefix) {
  if (!nodePrefix) {
    logger.println(Logger::ERR, F("Error: nodePrefix is null"));
    return false;
  }

  bool allOk = true;
  char topic[128];
  char payload[512];

  // Common device info for all gateway entities
  auto buildDeviceObj = [](JsonDocument& doc) {
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0] = "lora_gateway";
    dev["name"] = "LoRa Gateway";
    dev["manufacturer"] = "DIY";
    dev["model"] = "ESP32 + RFM95";
  };

  // Helper lambda to publish a single discovery config
  auto publishConfig = [&](const char* component, const char* entityId,
                           const char* name, const char* deviceClass,
                           const char* unit, const char* stateClass,
                           const char* valueTemplate, bool isBinarySensor) {
    snprintf(topic, sizeof(topic), "homeassistant/%s/lora_gw/%s/config",
             component, entityId);

    JsonDocument doc;
    doc["name"] = name;
    doc["unique_id"] = String("lora_gw_") + entityId;
    doc["object_id"] = String("lora_gw_") + entityId;
    doc["state_topic"] = "lora-gw/status";

    if (deviceClass && strlen(deviceClass) > 0) {
      doc["device_class"] = deviceClass;
    }
    if (unit && strlen(unit) > 0) {
      doc["unit_of_measurement"] = unit;
    }
    if (stateClass && strlen(stateClass) > 0) {
      doc["state_class"] = stateClass;
    }
    if (valueTemplate && strlen(valueTemplate) > 0) {
      doc["value_template"] = valueTemplate;
    }

    if (isBinarySensor) {
      doc["payload_on"] = "ON";
      doc["payload_off"] = "OFF";
    }

    buildDeviceObj(doc);

    serializeJson(doc, payload, sizeof(payload));

    logger.printf(Logger::DBG, "Publishing gateway discovery: %s -> %s", name,
                  topic);
    if (!client.publish(topic, payload, true)) {
      allOk = false;
    }
  };

  // Binary sensors
  publishConfig("binary_sensor", "wifi_connected", "WiFi Connected",
                "connectivity", nullptr, nullptr,
                "{{ value_json.wifi_connected }}", true);
  publishConfig("binary_sensor", "mqtt_connected", "MQTT Connected",
                "connectivity", nullptr, nullptr,
                "{{ value_json.mqtt_connected }}", true);
  publishConfig("binary_sensor", "lora_ok", "LoRa Radio OK", nullptr, nullptr,
                nullptr, "{{ value_json.lora_ok }}", true);

  // Sensors
  publishConfig("sensor", "uptime", "Uptime", "duration", "s", "measurement",
                "{{ value_json.uptime }}", false);
  publishConfig("sensor", "wifi_rssi", "WiFi RSSI", "signal_strength", "dBm",
                "measurement", "{{ value_json.wifi_rssi }}", false);
  publishConfig("sensor", "free_heap", "Free Heap", "data_size", "B",
                "measurement", "{{ value_json.free_heap }}", false);
  publishConfig("sensor", "device_count", "Connected Devices", nullptr, nullptr,
                "measurement", "{{ value_json.device_count }}", false);
  publishConfig("sensor", "lora_rx", "LoRa Packets Received", nullptr, nullptr,
                "total_increasing", "{{ value_json.lora_rx }}", false);
  publishConfig("sensor", "lora_tx", "LoRa Packets Sent", nullptr, nullptr,
                "total_increasing", "{{ value_json.lora_tx }}", false);

  // String sensors (no device_class/unit/state_class)
  publishConfig("sensor", "version", "Firmware Version", nullptr, nullptr,
                nullptr, "{{ value_json.version }}", false);
  publishConfig("sensor", "ip", "IP Address", nullptr, nullptr, nullptr,
                "{{ value_json.ip }}", false);

  // MQTT message counters
  publishConfig("sensor", "mqtt_rx", "MQTT Messages Received", nullptr, nullptr,
                "total_increasing", "{{ value_json.mqtt_rx }}", false);
  publishConfig("sensor", "mqtt_tx", "MQTT Messages Sent", nullptr, nullptr,
                "total_increasing", "{{ value_json.mqtt_tx }}", false);

  return allOk;
}

void MqttHandler::setOnMessageReceived(void (*callback)(const char*,
                                                        const byte*,
                                                        unsigned int)) {
  onMessageReceived = callback;
}

void MqttHandler::handle() {
  if (!client.connected()) {
    logger.println(Logger::INF, F("MQTT reconnecting..."));
    // Attempt reconnection (non-blocking, returns immediately if fails)
    client.connect("LoRaGateway");
  } else {
    client.loop();
  }
}