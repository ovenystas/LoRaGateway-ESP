#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "DeviceRegistry.h"
#include "LoRaMsgHandler.h"

class WebServerHandler {
 public:
  WebServerHandler(LoRaMsgHandler& loRaMsg, DeviceRegistry& registry,
                   uint16_t port = 80);

  // Initialize and start the async webserver and WebSocket.
  bool begin();

  // Stop the webserver.
  void stop();

 private:
  AsyncWebServer server;
  AsyncWebSocket ws;
  LoRaMsgHandler& loRaMsg;
  DeviceRegistry& deviceRegistry;
  String lastStatus;
  bool started;

  // WebSocket event handler
  void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                 AwsEventType type, void* arg, uint8_t* data, size_t len);

  // Send initial state (settings + log history) to a newly connected client
  void sendInitToClient(AsyncWebSocketClient* client);

  // Handle incoming WebSocket commands from clients
  void handleWsMessage(AsyncWebSocketClient* client, const String& message);

  // URL decode helper
  static String urlDecode(const String& input);
};