#pragma once

#include "Types.h"
#include <Arduino.h>

class LoRaHandler {
public:
  LoRaHandler(int csPin, int rstPin, int dioPin);
  
  // Initialize LoRa module
  bool begin(long frequency = 868000000);
  
  // Send a message to a node
  bool sendMessage(const LoRaMessage& msg);
  
  // Check if a message is available and read it
  bool readMessage(LoRaMessage& msg);
  
  // Set callback for received messages
  void setOnMessageReceived(void (*callback)(const LoRaMessage&));
  
  // Process LoRa events (should be called in main loop)
  void handle();
  
  // Encode message to bytes
  static uint8_t encodeMessage(const LoRaMessage& msg, uint8_t* buffer, uint8_t maxLen);
  
  // Decode message from bytes
  static bool decodeMessage(const uint8_t* buffer, uint8_t len, LoRaMessage& msg);
  
private:
  int csPin;
  int rstPin;
  int dioPin;
  void (*onMessageReceived)(const LoRaMessage&);
  
  // Utility for CRC calculation
  uint16_t calculateCRC(const uint8_t* data, uint8_t len);
};
