#pragma once

#include <Arduino.h>

#include "Types.h"

class LoRaHandler {
 public:
  LoRaHandler(int csPin, int rstPin, int dioPin);

  // Initialize LoRa module
  bool begin(long frequency = 868000000);

  // Send a message
  bool sendMessage(const LoRaTxMessage& msg);

  // Check if a message is available and read it
  bool readMessage(LoRaRxMessage& msg);

  // Set callback for received messages
  void setOnMessageReceived(void (*callback)(const LoRaRxMessage&));

  // Process LoRa events (should be called in main loop)
  void handle();

  // Encode message to bytes
  static uint8_t encodeMessage(const LoRaTxMessage& msg, uint8_t* buffer,
                               uint8_t maxLen);

  // Decode message from bytes
  static bool decodeMessage(const uint8_t* buffer, uint8_t len,
                            LoRaRxMessage& msg);

  // Helper to set default header
  static void setDefaultHeader(LoRaHeader& header, uint8_t dst, uint8_t src,
                               uint8_t id = 0,
                               LoRaMsgType msgType = LoRaMsgType::ping_req);

  static void printMessage(const LoRaTxMessage& msg);
  static void printMessage(const LoRaRxMessage& msg);

 private:
  int csPin;
  int rstPin;
  int dioPin;
  void (*onMessageReceived)(const LoRaRxMessage&);

  // Utility for CRC calculation
  uint16_t calculateCRC(const uint8_t* data, uint8_t len);
};
