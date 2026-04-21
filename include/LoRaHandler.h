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

  static void printMessage(const LoRaTxMessage& msg);
  static void printMessage(const LoRaRxMessage& msg);

 private:
  const int csPin;
  const int rstPin;
  const int dioPin;
  void (*onMessageReceived)(const LoRaRxMessage&);

  // Helper to read and decode packet from LoRa module
  bool readPacket(LoRaRxMessage& msg);

  // Helper to print message payload (shared by both Tx and Rx formatters)
  static void printMessagePayload(const LoRaHeader& header,
                                  const uint8_t* payload, uint8_t payloadLength,
                                  bool ackResponse);
};
