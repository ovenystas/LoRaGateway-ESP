#include "MqttHandler.h"

#include <ArduinoJson.h>
#include <stdio.h>

MqttHandler::MqttHandler(WiFiClient& wifiClient)
    : wifiClient(wifiClient), onMessageReceived(nullptr) {
  client.setClient(wifiClient);
  client.setBufferSize(MQTT_BUFFER_SIZE);
}

bool MqttHandler::connect(const char* broker, uint16_t port,
                          const char* clientId) {
  client.setServer(broker, port);
  client.setCallback([this](char* topic, byte* payload, unsigned int length) {
    if (onMessageReceived) {
      onMessageReceived(topic, payload, length);
    }
  });

  return client.connect(clientId);
}

bool MqttHandler::connect(const char* broker, uint16_t port,
                          const char* clientId, const char* username,
                          const char* password) {
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
          doc[key] = entity->format.scaleValue(valueItem.value);
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

  return client.publish(topic, payload);
}

bool MqttHandler::subscribeToCommands(uint8_t deviceId, uint8_t entityId) {
  char topic[128];

  snprintf(topic, sizeof(topic), "lora-gw/device_%u/entity_%u/command",
           deviceId, entityId);

  return client.subscribe(topic);
}

/*
 * Examples:
 *  topic: homeassistant/cover/lora_1/0_garage/config
 *  topic: homeassistant/sensor/lora_1/1_temperature/config
 */
bool MqttHandler::publishDiscovery(const EntityInfo& entity,
                                   const char* nodePrefix) {
  Serial.print(F("Publishing discovery for entity: "));
  Serial.print(entity.entityId);
  Serial.print(':');
  Serial.println(entity.name);

  char topic[128];
#if 1
  char payload[512];
#endif

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
  doc["device_class"] =
      entity.deviceClass ? entity.deviceClass->getName() : "unknown";

  // State topic
  char stateTopic[128];
  snprintf(stateTopic, sizeof(stateTopic), "%s/device_%u/state", nodePrefix,
           entity.deviceId);
  doc["state_topic"] = stateTopic;

  // Command topic (for covers)
  if (entity.domain.getDomain() == EntityDomain::Domain::COVER) {
    char cmdTopic[128];
    snprintf(cmdTopic, sizeof(cmdTopic), "%s/device_%u/entity_%u/command",
             nodePrefix, entity.deviceId, entity.entityId);
    doc["command_topic"] = cmdTopic;
    doc["payload_open"] = "OPEN";
    doc["payload_close"] = "CLOSE";
    doc["payload_stop"] = "STOP";
    doc["state_open"] = "open";
    doc["state_closed"] = "closed";
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
  Serial.print(F("Value template: "));
  Serial.println(valueTemplate);
#if 1
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

  // State class for sensors
  if (entity.domain.getDomain() == EntityDomain::Domain::SENSOR) {
    doc["state_class"] = "measurement";
  }

  serializeJson(doc, payload, sizeof(payload));
#endif
  Serial.print("Publishing discovery for entity: ");
  Serial.println(entity.name);
  Serial.print("    topic: ");
  Serial.println(topic);
#if 1
  Serial.print("    Payload: ");
  Serial.println(payload);
#endif
  const bool result =
      client.publish(topic, payload, true);  // Retain discovery message

  return result;
}

void MqttHandler::setOnMessageReceived(void (*callback)(const char*,
                                                        const byte*,
                                                        unsigned int)) {
  onMessageReceived = callback;
}

void MqttHandler::handle() {
  if (!client.connected()) {
    // Attempt reconnection could go here if needed
  } else {
    client.loop();
  }
}
