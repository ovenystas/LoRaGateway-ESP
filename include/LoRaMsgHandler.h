#pragma once

#include <vector>

#include "DiscoveryItem.h"
#include "LoRaHandler.h"

#define LORA_GATEWAY_ADDRESS_DEFAULT 0

class LoRaMsgHandler {
 public:
  static const uint8_t LORA_BROADCAST_ADDRESS = 255;

  LoRaMsgHandler(LoRaHandler& loRa,
                 uint8_t myAddress = LORA_GATEWAY_ADDRESS_DEFAULT);

  // Set callback for received discovery messages
  void setOnDiscoveryMessage(void (*callback)(uint8_t, const DiscoveryItem&));

  // Set callback for received value messages
  void setOnValueMessage(void (*callback)(uint8_t,
                                          const std::vector<ValueItem>&));

  // Handle an incoming LoRa message and perform appropriate actions
  void handleMessage(const LoRaRxMessage& msg);

  void handlePingMessage(const LoRaRxMessage& msg);

  void handlePingRequest(const LoRaRxMessage& msg);

  // Handle a received value message
  void handleValueMessage(const LoRaRxMessage& msg);

  // Handle a received discovery message
  void handleDiscoveryMessage(const LoRaRxMessage& msg);

  // Send a ping request to a specific device
  bool sendPingRequest(uint8_t targetDeviceId);

  // Send a discovery request to a specific device for a specific entity or all
  // entities
  bool sendDiscoveryRequest(uint8_t targetDeviceId, uint8_t entityId = 255);

  // Send a value set command to a specific device entity
  bool sendValueSet(uint8_t targetDeviceId, uint8_t entityId, uint32_t value);

  // Send a service command to a specific device entity
  bool sendServiceCommand(uint8_t targetDeviceId, uint8_t entityId,
                          uint8_t command);

 private:
  static LoRaMsgHandler* instance;
  LoRaHandler& loRa;
  uint8_t myAddress;
  void (*onDiscoveryMessage)(uint8_t, const DiscoveryItem&);
  void (*onValueMessage)(uint8_t, const std::vector<ValueItem>& valueItems);

  static void handleMessageStatic(const LoRaRxMessage& msg) {
    if (instance) {
      instance->handleMessage(msg);
    }
  }
};
