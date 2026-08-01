#include "LoRaMsgHandler.h"

#include <Arduino.h>

#include <vector>

#include "Logger.h"

// Initialize static instance
LoRaMsgHandler* LoRaMsgHandler::instance = nullptr;

LoRaMsgHandler::LoRaMsgHandler(LoRaHandler& loRa, uint8_t myAddress)
    : loRa(loRa),
      myAddress(myAddress),
      onDiscoveryMessage(nullptr),
      onValueMessage(nullptr) {
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
  logger.printf(Logger::DBG,
                "LoRa message from Device %u, Type: %u, ID: %u, Dst: %u, "
                "PayloadLen: %u, RSSI: %d",
                msg.header.src, static_cast<uint8_t>(msg.header.flags.msgType),
                msg.header.id, msg.header.dst, msg.payloadLength, msg.rssi);
  logger.print(Logger::DBG, "    Raw payload: ");
  printArray(logger, msg.payload, msg.payloadLength, HEX);
  logger.println(Logger::DBG);

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
      logger.println(Logger::WRN,
                     F("Discovery request handling not implemented yet"));
      break;
    }

    case LoRaMsgType::discovery_msg: {
      handleDiscoveryMessage(msg);
      break;
    }

    case LoRaMsgType::value_req: {
      // handleValueRequest(msg);
      logger.println(Logger::WRN,
                     F("Value request handling not implemented yet"));
      break;
    }

    case LoRaMsgType::value_msg: {
      handleValueMessage(msg);
      break;
    }

    case LoRaMsgType::valueSet_req: {
      // handleValueSetRequest(msg);
      logger.println(Logger::WRN,
                     F("Value set request handling not implemented yet"));
      break;
    }

    default:
      logger.printf(Logger::WRN, "Unknown message type: %u",
                    static_cast<uint8_t>(msg.header.flags.msgType));
      break;
  }
}

void LoRaMsgHandler::handlePingMessage(const LoRaRxMessage& msg) {
  // Ping response received
  logger.printf(Logger::INF, "Ping response from Device %u - RSSI: %d dBm",
                msg.header.src, msg.rssi);

  // Extract device's RSSI if present in payload
  if (msg.payloadLength >= 2) {
    int16_t deviceRssi = ((int16_t)msg.payload[0] << 8) | msg.payload[1];
    logger.printf(Logger::DBG, "    Device's signal strength: %d dBm",
                  deviceRssi);
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
  logger.printf(Logger::INF, "Value message from Device %u", msg.header.src);

  uint8_t deviceId = msg.header.src;
  //   deviceRegistry.updateDeviceLastSeen(deviceId);

  // Parse value payload
  if (msg.payloadLength > 0) {
    uint8_t numValues = msg.payload[0];
    logger.printf(Logger::DBG, "    Number of values: %u", numValues);
    if (msg.payloadLength != 1 + numValues * ValueItem::size()) {
      logger.println(
          Logger::WRN,
          F("    Payload size not correct for number of values, ignoring"));
      return;
    }

    std::vector<ValueItem> valueItems;
    for (uint8_t i = 0; i < numValues; i++) {
      const uint8_t* valuePtr = &msg.payload[1 + i * ValueItem::size()];
      ValueItem valueItem;
      valueItem.fromByteArray(valuePtr);

      logger.printf(Logger::DBG, "    #%u: Entity ID=%u, Value=%d", i,
                    valueItem.entityId, valueItem.value);

      valueItems.push_back(valueItem);
    }
    if (onValueMessage) {
      onValueMessage(deviceId, valueItems);
    }
  }
}

void LoRaMsgHandler::handleDiscoveryMessage(const LoRaRxMessage& msg) {
  logger.printf(Logger::INF, "Discovery message from Device %u",
                msg.header.src);

  // Parse discovery payload
  if (msg.payloadLength >= DiscoveryItem::size()) {
    DiscoveryItem discovery;
    discovery.fromByteArray(msg.payload, msg.payloadLength);

    logger.printf(
        Logger::DBG,
        "    Entity ID: %u, Name: %s, Type: %s, Device Class: %s, Category: "
        "%s, Unit: %s",
        discovery.entityId, discovery.name, discovery.domain.getName(),
        discovery.deviceClass ? discovery.deviceClass->getName() : "unknown",
        discovery.category.getName(), discovery.unit.getName());
    logger.print(Logger::DBG, "    Format: (");
    discovery.format.print(logger);
    logger.printf(Logger::DBG, "), Min=%f, Max=%f",
                  discovery.format.fromRawValue(discovery.minValue),
                  discovery.format.fromRawValue(discovery.maxValue));

    const uint8_t deviceId = msg.header.src;
    if (onDiscoveryMessage) {
      onDiscoveryMessage(deviceId, discovery);
    }
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
    logger.println(Logger::ERR,
                   F("Entity ID cannot be 255 for value set request"));
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

  logger.printf(Logger::INF,
                "Sending value set command to Device %u, Entity %u, Value %u",
                targetDeviceId, entityId, value);

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

  logger.printf(Logger::INF,
                "Sending service command to Device %u, Entity %u, Command %u",
                targetDeviceId, entityId, command);

  return loRa.sendMessage(msg);
}
