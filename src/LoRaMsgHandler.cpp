#include "LoRaMsgHandler.h"

#include <Arduino.h>

#include <vector>

// Initialize static instance
LoRaMsgHandler* LoRaMsgHandler::instance = nullptr;

LoRaMsgHandler::LoRaMsgHandler(LoRaHandler& loRa, uint8_t myAddress)
    : loRa(loRa), myAddress(myAddress) {
  LoRaMsgHandler::instance = this;
  loRa.setOnMessageReceived(&LoRaMsgHandler::handleMessageStatic);
}

void LoRaMsgHandler::setOnDiscoveryMessage(
    void (*callback)(uint8_t, const DiscoveryItem&)) {
  onDiscoveryMessage = callback;
}

void LoRaMsgHandler::setOnValueMessage(
    void (*callback)(uint8_t, const std::vector<ValueItem>& valueItems)) {
  onValueMessage = callback;
}

void LoRaMsgHandler::handleMessage(const LoRaRxMessage& msg) {
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
  Serial.print("    Raw payload: ");
  printArray(Serial, msg.payload, msg.payloadLength, HEX);
  Serial.println();

  // Handle different message types
  switch (msg.header.flags.msgType) {
    case LoRaMsgType::ping_req: {
      handlePingRequest(msg);
      break;
    }

    case LoRaMsgType::ping_msg: {
      handlePingMessage(msg);
      break;
    }

    case LoRaMsgType::discovery_req: {
      // handleDiscoveryRequest(msg);
      Serial.println(F("Discovery request handling not implemented yet"));
      break;
    }

    case LoRaMsgType::discovery_msg: {
      handleDiscoveryMessage(msg);
      break;
    }

    case LoRaMsgType::value_req: {
      // handleValueRequest(msg);
      Serial.println(F("Value request handling not implemented yet"));
      break;
    }

    case LoRaMsgType::value_msg: {
      handleValueMessage(msg);
      break;
    }

    case LoRaMsgType::valueSet_req: {
      // handleValueSetRequest(msg);
      Serial.println(F("Value set request handling not implemented yet"));
      break;
    }

    default:
      Serial.print("Unknown message type: ");
      Serial.println(static_cast<uint8_t>(msg.header.flags.msgType));
      break;
  }
}

void LoRaMsgHandler::handlePingMessage(const LoRaRxMessage& msg) {
  // Ping response received
  Serial.print("Ping response from Device ");
  Serial.print(msg.header.src);
  Serial.print(" - RSSI: ");
  Serial.print(msg.rssi);
  Serial.println(" dBm");

  // Extract device's RSSI if present in payload
  if (msg.payloadLength >= 2) {
    int16_t deviceRssi = ((int16_t)msg.payload[0] << 8) | msg.payload[1];
    Serial.print("    Device's signal strength: ");
    Serial.print(deviceRssi);
    Serial.println(" dBm");
  }
}

void LoRaMsgHandler::handlePingRequest(const LoRaRxMessage& msg) {
  // Send ping response
  LoRaTxMessage response;
  response.header = LoRaHeader(msg.header.src, msg.header.dst, msg.header.id,
                               LoRaMsgType::ping_msg);
  response.payloadLength = 2;
  response.payload[0] = ((-msg.rssi) >> 8) & 0xFF;
  response.payload[1] = (-msg.rssi) & 0xFF;
  loRa.sendMessage(response);
}

void LoRaMsgHandler::handleValueMessage(const LoRaRxMessage& msg) {
  Serial.print("Value message from Device ");
  Serial.println(msg.header.src);

  uint8_t deviceId = msg.header.src;
  //   deviceRegistry.updateDeviceLastSeen(deviceId);

  // Parse value payload
  if (msg.payloadLength > 0) {
    uint8_t numValues = msg.payload[0];
    Serial.print("    Number of values: ");
    Serial.println(numValues);
    if (msg.payloadLength != 1 + numValues * ValueItem::size()) {
      Serial.println(
          "    Payload size not correct for number of values, ignoring");
      {
        return;
      };
    }

    std::vector<ValueItem> valueItems;
    for (uint8_t i = 0; i < numValues; i++) {
      const uint8_t* valuePtr = &msg.payload[1 + i * ValueItem::size()];
      ValueItem valueItem;
      valueItem.fromByteArray(valuePtr);

      Serial.print("    #");
      Serial.print(i);
      Serial.print(": Entity ID=");
      Serial.print(valueItem.entityId);
      Serial.print(", Value=");
      Serial.println(valueItem.value);

      valueItems.push_back(valueItem);
    }
    onValueMessage(deviceId, valueItems);
  }
}

void LoRaMsgHandler::handleDiscoveryMessage(const LoRaRxMessage& msg) {
  Serial.print("Discovery message from Device ");
  Serial.println(msg.header.src);

  // Parse discovery payload
  if (msg.payloadLength >= DiscoveryItem::size()) {
    DiscoveryItem discovery;
    discovery.fromByteArray(msg.payload, msg.payloadLength);

    Serial.print("    Entity ID: ");
    Serial.print(discovery.entityId);
    Serial.print(", Name: ");
    Serial.print(discovery.name);
    Serial.print(", Type: ");
    Serial.print(discovery.domain.getName());
    Serial.print(", Device Class: ");
    Serial.print(discovery.deviceClass ? discovery.deviceClass->getName()
                                       : "unknown");
    Serial.print(", Category: ");
    Serial.print(discovery.category.getName());
    Serial.print(", Unit: ");
    Serial.print(discovery.unit.getName());
    Serial.print(", Format: (");
    discovery.format.print(Serial);
    Serial.print("), Min=");
    Serial.print(discovery.format.fromRawValue(discovery.minValue),
                 discovery.format.getPrecision());
    Serial.print(", Max=");
    Serial.println(discovery.format.fromRawValue(discovery.maxValue),
                   discovery.format.getPrecision());

    const uint8_t deviceId = msg.header.src;
    onDiscoveryMessage(deviceId, discovery);
  }
}

bool LoRaMsgHandler::sendPingRequest(uint8_t targetDeviceId) {
  LoRaTxMessage msg;
  msg.header = LoRaHeader(targetDeviceId, myAddress, 0, LoRaMsgType::ping_req);
  msg.payloadLength = 0;

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendDiscoveryRequest(uint8_t targetDeviceId,
                                          uint8_t entityId) {
  LoRaTxMessage msg;
  msg.header =
      LoRaHeader(targetDeviceId, myAddress, 0, LoRaMsgType::discovery_req);
  msg.payloadLength = 1;
  msg.payload[0] = entityId;

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendValueGetRequest(uint8_t targetDeviceId,
                                         uint8_t entityId) {
  LoRaTxMessage msg;
  msg.header = LoRaHeader(targetDeviceId, myAddress, 0, LoRaMsgType::value_req);
  msg.payloadLength = 1;
  msg.payload[0] = entityId;

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendValueSetRequest(uint8_t targetDeviceId,
                                         uint8_t entityId, uint32_t value) {
  if (entityId == 255) {
    Serial.println("Entity ID cannot be 255 for value set request");
    return false;
  }

  LoRaTxMessage msg;
  msg.header =
      LoRaHeader(targetDeviceId, myAddress, 0, LoRaMsgType::valueSet_req);
  msg.payloadLength = 5;
  msg.payload[0] = entityId;
  msg.payload[1] = (value >> 24) & 0xFF;
  msg.payload[2] = (value >> 16) & 0xFF;
  msg.payload[3] = (value >> 8) & 0xFF;
  msg.payload[4] = value & 0xFF;

  Serial.print("Sending value set command to Device ");
  Serial.print(targetDeviceId);
  Serial.print(", Entity ");
  Serial.print(entityId);
  Serial.print(", Value ");
  Serial.println(value);

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendServiceCommand(uint8_t targetDeviceId,
                                        uint8_t entityId, uint8_t command) {
  LoRaTxMessage msg;
  msg.header =
      LoRaHeader(targetDeviceId, myAddress, 0, LoRaMsgType::service_req);
  msg.payloadLength = 2;
  msg.payload[0] = entityId;
  msg.payload[1] = command;

  Serial.print("Sending service command to Device ");
  Serial.print(targetDeviceId);
  Serial.print(", Entity ");
  Serial.print(entityId);
  Serial.print(", Command ");
  Serial.println(command);

  return loRa.sendMessage(msg);
}
