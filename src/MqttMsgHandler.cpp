#include "MqttMsgHandler.h"

// Static member initialization
LoRaMsgHandler* MqttMsgHandler::loRaMsgHandler = nullptr;

void MqttMsgHandler::handleMessage(const char* topic, const byte* payload,
                                   unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.println(topic);

  // Parse topic: lora-gw/device_{deviceId}/entity_{entityId}/command
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
    entityStr += 7;  // Skip "entity_"

    deviceId = atoi(deviceStr);
    entityId = atoi(entityStr);

    if (length > 0 && loRaMsgHandler) {
      char payloadStr[256];
      strncpy(payloadStr, (const char*)payload, length);
      payloadStr[length] = '\0';

      Serial.print("Payload: ");
      Serial.println(payloadStr);

      uint8_t command = 0;

      // Parse command payload - handle common cover commands
      if (strstr(payloadStr, "OPEN")) {
        command = 0;  // Open
      } else if (strstr(payloadStr, "CLOSE")) {
        command = 1;  // Close
      } else if (strstr(payloadStr, "STOP")) {
        command = 2;  // Stop
      } else {
        // Try to parse numeric value
        char* valueStr = strstr(payloadStr, "\"value\":");
        if (valueStr) {
          command = atoi(valueStr + 8);
        }
      }

      // Forward command to device via LoRa
      if (loRaMsgHandler->sendServiceCommand(deviceId, entityId, command)) {
        Serial.println("Service command sent successfully");
      } else {
        Serial.println("Failed to send service command");
      }
    }
  }
}
