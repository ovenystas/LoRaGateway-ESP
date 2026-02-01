#include "LoRaHandler.h"
#include <LoRa.h>
#include <crc.h>
#include <cstring>

LoRaHandler::LoRaHandler(int csPin, int rstPin, int dioPin)
    : csPin(csPin), rstPin(rstPin), dioPin(dioPin), onMessageReceived(nullptr) {}

bool LoRaHandler::begin(long frequency) {
  // Set LoRa pins
  LoRa.setPins(csPin, rstPin, dioPin);
  
  if (!LoRa.begin(frequency)) {
    return false;
  }
  
  // Configure LoRa parameters
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);
  LoRa.setCodingRate4(5);
  
  return true;
}

bool LoRaHandler::sendMessage(const LoRaMessage& msg) {
  uint8_t buffer[64];
  uint8_t len = encodeMessage(msg, buffer, sizeof(buffer));
  
  if (len == 0) {
    return false;
  }
  
  LoRa.beginPacket();
  LoRa.write(buffer, len);
  LoRa.endPacket();
  
  return true;
}

bool LoRaHandler::readMessage(LoRaMessage& msg) {
  if (LoRa.parsePacket() == 0) {
    return false;
  }
  
  uint8_t buffer[64];
  int len = 0;
  
  while (LoRa.available()) {
    buffer[len++] = LoRa.read();
    if (len >= (int)sizeof(buffer)) break;
  }
  
  return decodeMessage(buffer, len, msg);
}

void LoRaHandler::setOnMessageReceived(void (*callback)(const LoRaMessage&)) {
  onMessageReceived = callback;
}

void LoRaHandler::handle() {
  if (LoRa.parsePacket() > 0) {
    LoRaMessage msg;
    if (readMessage(msg) && onMessageReceived) {
      onMessageReceived(msg);
    }
  }
}

uint8_t LoRaHandler::encodeMessage(const LoRaMessage& msg, uint8_t* buffer, uint8_t maxLen) {
  if (maxLen < 20) {
    return 0;
  }
  
  uint8_t pos = 0;
  
  // Message header
  buffer[pos++] = 0xAA;  // Sync byte
  
  // Device and entity info
  buffer[pos++] = (msg.deviceId >> 8) & 0xFF;
  buffer[pos++] = msg.deviceId & 0xFF;
  buffer[pos++] = msg.entityId;
  buffer[pos++] = static_cast<uint8_t>(msg.entityType);
  buffer[pos++] = msg.messageType;
  
  // Value type and value
  buffer[pos++] = static_cast<uint8_t>(msg.valueType);
  
  switch (msg.valueType) {
    case ValueType::BOOLEAN:
      buffer[pos++] = msg.value.boolValue ? 1 : 0;
      break;
    case ValueType::INT_VALUE:
      buffer[pos++] = (msg.value.intValue >> 24) & 0xFF;
      buffer[pos++] = (msg.value.intValue >> 16) & 0xFF;
      buffer[pos++] = (msg.value.intValue >> 8) & 0xFF;
      buffer[pos++] = msg.value.intValue & 0xFF;
      break;
    case ValueType::FLOAT_VALUE: {
      uint32_t* fPtr = (uint32_t*)&msg.value.floatValue;
      buffer[pos++] = (*fPtr >> 24) & 0xFF;
      buffer[pos++] = (*fPtr >> 16) & 0xFF;
      buffer[pos++] = (*fPtr >> 8) & 0xFF;
      buffer[pos++] = *fPtr & 0xFF;
      break;
    }
    case ValueType::STRING:
      // For string, add length byte and string data
      buffer[pos++] = 0;  // Length (would need msg.stringValue for real implementation)
      break;
  }
  
  // Command type (if applicable)
  buffer[pos++] = static_cast<uint8_t>(msg.command);
  
  // Command value
  for (int i = 0; i < 4 && pos < maxLen; i++) {
    buffer[pos++] = msg.commandValue[i];
  }
  
  // Calculate and append CRC
  uint16_t crc = 0xFFFF;
  CRC16 crcCalculator(0x1021, true, true, 0xFFFF, 0);
  for (uint8_t i = 0; i < pos; i++) {
    crcCalculator.add(buffer[i]);
  }
  crc = crcCalculator.calc();
  
  buffer[pos++] = (crc >> 8) & 0xFF;
  buffer[pos++] = crc & 0xFF;
  
  return pos;
}

bool LoRaHandler::decodeMessage(const uint8_t* buffer, uint8_t len, LoRaMessage& msg) {
  if (len < 18 || buffer[0] != 0xAA) {
    return false;
  }
  
  // Verify CRC
  CRC16 crcCalculator(0x1021, true, true, 0xFFFF, 0);
  for (uint8_t i = 0; i < len - 2; i++) {
    crcCalculator.add(buffer[i]);
  }
  uint16_t calculatedCrc = crcCalculator.calc();
  uint16_t receivedCrc = ((uint16_t)buffer[len - 2] << 8) | buffer[len - 1];
  
  if (calculatedCrc != receivedCrc) {
    return false;
  }
  
  uint8_t pos = 1;
  
  msg.deviceId = ((uint16_t)buffer[pos] << 8) | buffer[pos + 1];
  pos += 2;
  msg.entityId = buffer[pos++];
  msg.entityType = static_cast<EntityType>(buffer[pos++]);
  msg.messageType = buffer[pos++];
  msg.valueType = static_cast<ValueType>(buffer[pos++]);
  
  switch (msg.valueType) {
    case ValueType::BOOLEAN:
      msg.value.boolValue = buffer[pos++] != 0;
      break;
    case ValueType::INT_VALUE:
      msg.value.intValue = ((int32_t)buffer[pos] << 24) | ((int32_t)buffer[pos + 1] << 16) |
                           ((int32_t)buffer[pos + 2] << 8) | buffer[pos + 3];
      pos += 4;
      break;
    case ValueType::FLOAT_VALUE: {
      uint32_t floatBits = ((uint32_t)buffer[pos] << 24) | ((uint32_t)buffer[pos + 1] << 16) |
                           ((uint32_t)buffer[pos + 2] << 8) | buffer[pos + 3];
      memcpy(&msg.value.floatValue, &floatBits, sizeof(float));
      pos += 4;
      break;
    }
    case ValueType::STRING:
      pos++;  // Skip length byte for now
      break;
  }
  
  msg.command = static_cast<CommandType>(buffer[pos++]);
  
  for (int i = 0; i < 4 && pos < len - 2; i++) {
    msg.commandValue[i] = buffer[pos++];
  }
  
  return true;
}
