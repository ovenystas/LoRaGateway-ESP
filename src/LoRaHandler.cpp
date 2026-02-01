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

bool LoRaHandler::sendMessage(const LoRaTxMessage& msg) {
  uint8_t buffer[LORA_MAX_MESSAGE_LENGTH];
  uint8_t len = encodeMessage(msg, buffer, sizeof(buffer));
  
  if (len == 0) {
    return false;
  }
  
  LoRa.beginPacket();
  LoRa.write(buffer, len);
  LoRa.endPacket();
  
  return true;
}

bool LoRaHandler::readMessage(LoRaRxMessage& msg) {
  if (LoRa.parsePacket() == 0) {
    return false;
  }
  
  uint8_t buffer[LORA_MAX_MESSAGE_LENGTH];
  int len = 0;
  
  while (LoRa.available()) {
    buffer[len++] = LoRa.read();
    if (len >= (int)sizeof(buffer)) break;
  }
  
  if (decodeMessage(buffer, len, msg)) {
    msg.rssi = LoRa.packetRssi();
    return true;
  }
  
  return false;
}

void LoRaHandler::setOnMessageReceived(void (*callback)(const LoRaRxMessage&)) {
  onMessageReceived = callback;
}

void LoRaHandler::handle() {
  if (LoRa.parsePacket() > 0) {
    LoRaRxMessage msg;
    if (readMessage(msg) && onMessageReceived) {
      onMessageReceived(msg);
    }
  }
}

void LoRaHandler::setDefaultHeader(LoRaHeader& header, uint8_t dst, uint8_t src, uint8_t id,
                                     LoRaMsgType msgType) {
  header.dst = dst;
  header.src = src;
  header.id = id;
  header.flags.msgType = msgType;
  header.flags.ack_response = false;
  header.flags.ack_request = false;
}

uint8_t LoRaHandler::encodeMessage(const LoRaTxMessage& msg, uint8_t* buffer, uint8_t maxLen) {
  if (maxLen < LORA_HEADER_LENGTH + 2) {  // Header + CRC
    return 0;
  }
  
  uint8_t pos = 0;
  
  // Encode header
  pos += msg.header.toByteArray(&buffer[pos]);
  
  // Copy payload
  if (msg.payloadLength > 0) {
    if (pos + msg.payloadLength + 2 > maxLen) {
      return 0;
    }
    memcpy(&buffer[pos], msg.payload, msg.payloadLength);
    pos += msg.payloadLength;
  }
  
  // Calculate and append CRC16
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

bool LoRaHandler::decodeMessage(const uint8_t* buffer, uint8_t len, LoRaRxMessage& msg) {
  if (len < LORA_HEADER_LENGTH + 2) {  // Header + CRC minimum
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
  
  uint8_t pos = 0;
  
  // Decode header
  pos += msg.header.fromByteArray(&buffer[pos]);
  
  // Extract payload (everything between header and CRC)
  msg.payloadLength = len - LORA_HEADER_LENGTH - 2;  // Subtract header and CRC
  if (msg.payloadLength > LORA_MAX_PAYLOAD_LENGTH) {
    return false;
  }
  
  if (msg.payloadLength > 0) {
    memcpy(msg.payload, &buffer[pos], msg.payloadLength);
  }
  
  return true;
}
