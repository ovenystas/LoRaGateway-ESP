#include "LoRaHandler.h"

#include <LoRa.h>

#include <cstring>

#include "Util.h"

LoRaHandler::LoRaHandler(int csPin, int rstPin, int dioPin)
    : csPin(csPin),
      rstPin(rstPin),
      dioPin(dioPin),
      onMessageReceived(nullptr) {}

bool LoRaHandler::begin(long frequency) {
  // Set LoRa pins
  LoRa.setPins(csPin, rstPin, dioPin);

  if (!LoRa.begin(frequency)) {
    return false;
  }

  // Configure LoRa parameters
  // LoRa.setSignalBandwidth(125E3);
  // LoRa.setSpreadingFactor(7);
  // LoRa.setCodingRate4(5);

  return true;
}

bool LoRaHandler::sendMessage(const LoRaTxMessage& msg) {
  uint8_t buffer[LORA_MAX_MESSAGE_LENGTH];
  uint8_t len = encodeMessage(msg, buffer, sizeof(buffer));

  if (len == 0) {
    return false;
  }

  Serial.print("\nSending LoRa message, raw: ");
  printMessage(msg);
  Serial.println();

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
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    LoRaRxMessage msg;
    uint8_t buffer[LORA_MAX_MESSAGE_LENGTH];
    int len = 0;

    while (LoRa.available()) {
      buffer[len++] = LoRa.read();
      if (len >= (int)sizeof(buffer)) break;
    }

    if (decodeMessage(buffer, len, msg)) {
      Serial.print("\nLoRa message received, raw: ");
      printMessage(msg);
      Serial.println();
      msg.rssi = LoRa.packetRssi();
      if (onMessageReceived) {
        onMessageReceived(msg);
      }
    }
  }
}

void LoRaHandler::setDefaultHeader(LoRaHeader& header, uint8_t dst, uint8_t src,
                                   uint8_t id, LoRaMsgType msgType) {
  header.dst = dst;
  header.src = src;
  header.id = id;
  header.flags.msgType = msgType;
  header.flags.ack_response = false;
  header.flags.ack_request = false;
}

uint8_t LoRaHandler::encodeMessage(const LoRaTxMessage& msg, uint8_t* buffer,
                                   uint8_t maxLen) {
  if (maxLen < LORA_HEADER_LENGTH) {
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

  return pos;
}

bool LoRaHandler::decodeMessage(const uint8_t* buffer, uint8_t len,
                                LoRaRxMessage& msg) {
  if (len < LORA_HEADER_LENGTH) {
    return false;
  }

  uint8_t pos = 0;

  // Decode header
  pos += msg.header.fromByteArray(&buffer[pos]);

  // Extract payload
  msg.payloadLength = len - LORA_HEADER_LENGTH;

  if (msg.payloadLength > LORA_MAX_PAYLOAD_LENGTH) {
    return false;
  }

  if (msg.payloadLength > 0) {
    memcpy(msg.payload, &buffer[pos], msg.payloadLength);
  }

  return true;
}

void LoRaHandler::printMessage(const LoRaTxMessage& msg) {
  Serial.print(F("H: "));
  uint8_t buf[LORA_HEADER_LENGTH];
  msg.header.toByteArray(buf);
  printArray(Serial, buf, LORA_HEADER_LENGTH, HEX);

  Serial.print(F(" P: "));
  if (msg.payloadLength == 0) {
    Serial.print(F("--"));
  } else if (msg.header.flags.ack_response and msg.payloadLength == 1 and
             msg.payload[0] == '!') {
    Serial.print(F("(ACK)"));
  } else {
    printArray(Serial, msg.payload, msg.payloadLength, HEX);
  }
}

void LoRaHandler::printMessage(const LoRaRxMessage& msg) {
  Serial.print(F("H: "));
  uint8_t buf[LORA_HEADER_LENGTH];
  msg.header.toByteArray(buf);
  printArray(Serial, buf, LORA_HEADER_LENGTH, HEX);

  Serial.print(F(" P: "));
  if (msg.payloadLength == 0) {
    Serial.print(F("--"));
  } else if (msg.header.flags.ack_response and msg.payloadLength == 1 and
             msg.payload[0] == '!') {
    Serial.print(F("(ACK)"));
  } else {
    printArray(Serial, msg.payload, msg.payloadLength, HEX);
  }
}
