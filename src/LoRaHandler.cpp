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

void LoRaHandler::setOnMessageReceived(void (*callback)(const LoRaRxMessage&)) {
  onMessageReceived = callback;
}

void LoRaHandler::handle() {
  const int packetSize = LoRa.parsePacket();

  if (packetSize > 0) {
    LoRaRxMessage msg;
    if (readPacket(msg)) {
      Serial.print("\nLoRa message received, raw: ");
      printMessage(msg);
      Serial.println();

      if (onMessageReceived) {
        onMessageReceived(msg);
      }
    }
  }
}

bool LoRaHandler::readPacket(LoRaRxMessage& msg) {
  uint8_t buffer[LORA_MAX_MESSAGE_LENGTH];
  uint8_t len = 0;

  while (LoRa.available()) {
    buffer[len++] = LoRa.read();
    if ((size_t)len >= sizeof(buffer)) {
      break;
    }
  }

  if (decodeMessage(buffer, len, msg)) {
    msg.rssi = LoRa.packetRssi();
    msg.snr = LoRa.packetSnr();
    return true;
  }

  return false;
}

uint8_t LoRaHandler::encodeMessage(const LoRaTxMessage& msg, uint8_t* buffer,
                                   uint8_t maxLen) {
  if (maxLen < LORA_HEADER_LENGTH) {
    return 0;
  }

  if (!buffer) {
    return 0;
  }

  uint8_t pos = 0;

  // Encode header
  pos += msg.header.toByteArray(&buffer[pos]);

  // Copy payload
  if (msg.payloadLength > 0) {
    if (!msg.payload) {
      return 0;  // Payload pointer is null
    }
    if (pos + msg.payloadLength > maxLen) {
      return 0;  // Buffer too small
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

  // Decode header with validation
  uint8_t headerLen = msg.header.fromByteArray(&buffer[pos]);
  if (headerLen == 0) {
    return false;  // Header decoding failed
  }
  pos += headerLen;

  // Extract payload
  msg.payloadLength = len - pos;

  if (msg.payloadLength > LORA_MAX_PAYLOAD_LENGTH) {
    return false;  // Payload too large
  }

  if (msg.payloadLength > 0) {
    memcpy(msg.payload, &buffer[pos], msg.payloadLength);
  }

  return true;
}

void LoRaHandler::printMessage(const LoRaTxMessage& msg) {
  printMessagePayload(msg.header, msg.payload, msg.payloadLength,
                      msg.header.flags.ack_response);
}

void LoRaHandler::printMessage(const LoRaRxMessage& msg) {
  printMessagePayload(msg.header, msg.payload, msg.payloadLength,
                      msg.header.flags.ack_response);
}

void LoRaHandler::printMessagePayload(const LoRaHeader& header,
                                      const uint8_t* payload,
                                      uint8_t payloadLength, bool ackResponse) {
  Serial.print(F("H: "));
  uint8_t buf[LORA_HEADER_LENGTH];
  header.toByteArray(buf);
  printArray(Serial, buf, LORA_HEADER_LENGTH, HEX);

  Serial.print(F(" P: "));
  if (payloadLength == 0) {
    Serial.print(F("--"));
  } else if (ackResponse and payloadLength == 1 and payload[0] == '!') {
    Serial.print(F("(ACK)"));
  } else {
    printArray(Serial, payload, payloadLength, HEX);
  }
}
