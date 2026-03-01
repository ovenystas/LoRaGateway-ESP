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
  for (uint8_t i = 0; i < msg.payloadLength; i++) {
    Serial.print(msg.payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Handle different message types
  switch (msg.header.flags.msgType) {
    case LoRaMsgType::discovery_msg: {
      Serial.print("Discovery message from Device ");
      Serial.println(msg.header.src);

      // Parse discovery payload
      if (msg.payloadLength >= DiscoveryItem::size()) {
        DiscoveryItem discovery;
        discovery.fromByteArray(msg.payload);

        Serial.print("    Entity ID: ");
        Serial.print(discovery.entityId);
        Serial.print(", Type: ");
        Serial.print(discovery.domain.getName());
        Serial.print(", Device Class: ");
        Serial.print(discovery.deviceClass ? discovery.deviceClass->getName()
                                           : "unknown");
        Serial.print(", Unit: ");
        Serial.print(discovery.unit.getName());
        Serial.print(", Format: (");
        Serial.print(discovery.format.isSigned ? "signed" : "unsigned");
        Serial.print(", ");
        Serial.print(1 << discovery.format.size);
        Serial.print(" bytes, ");
        Serial.print(discovery.format.precision);
        Serial.println(" decimals)");

        uint8_t deviceId = msg.header.src;
        onDiscoveryMessage(deviceId, discovery);
      }
      break;
    }

    case LoRaMsgType::value_msg: {
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
          break;
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
      break;
    }

    case LoRaMsgType::ping_req: {
      // Send ping response
      LoRaTxMessage response;
      LoRaHandler::setDefaultHeader(response.header, msg.header.src,
                                    msg.header.dst, msg.header.id,
                                    LoRaMsgType::ping_msg);
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
        Serial.print("    Device's signal strength: ");
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

bool LoRaMsgHandler::sendPingRequest(uint8_t targetDeviceId) {
  LoRaTxMessage msg;
  loRa.setDefaultHeader(msg.header, targetDeviceId, myAddress, 0,
                        LoRaMsgType::ping_req);
  msg.payloadLength = 0;

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendDiscoveryRequest(uint8_t targetDeviceId,
                                          uint8_t entityId) {
  LoRaTxMessage msg;
  loRa.setDefaultHeader(msg.header, targetDeviceId, myAddress, 0,
                        LoRaMsgType::discovery_req);
  msg.payloadLength = 1;
  msg.payload[0] = entityId;

  return loRa.sendMessage(msg);
}

bool LoRaMsgHandler::sendServiceCommand(uint8_t targetDeviceId,
                                        uint8_t entityId, uint8_t command) {
  LoRaTxMessage msg;
  loRa.setDefaultHeader(msg.header, targetDeviceId, myAddress, 0,
                        LoRaMsgType::service_req);
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
