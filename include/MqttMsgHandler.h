#pragma once

#include "LoRaMsgHandler.h"
#include "MqttHandler.h"

class MqttMsgHandler {
 public:
  MqttMsgHandler(LoRaMsgHandler& loRaMsg) : loRaMsg(loRaMsg) {
    loRaMsgHandler = &loRaMsg;
  }

  // Handle an incoming MQTT message and perform appropriate actions
  static void handleMessage(const char* topic, const byte* payload,
                            unsigned int length);

 private:
  static LoRaMsgHandler* loRaMsgHandler;
  LoRaMsgHandler& loRaMsg;
};
