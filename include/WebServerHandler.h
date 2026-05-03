#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "DeviceRegistry.h"
#include "LoRaMsgHandler.h"

class WebServerHandler {
 public:
  WebServerHandler(LoRaMsgHandler& loRaMsg, DeviceRegistry& registry,
                   uint16_t port = 80);

  // Initialize and start the webserver
  bool begin();

  // Handle webserver events (should be called in main loop)
  void handle();

  // Stop the webserver
  void stop();

  // Route handlers
  void handleRoot();
  void handlePing();
  void handleDiscovery();
  void handleValueGet();
  void handleValueSet();
  void handleGetStatus();
  void handleNotFound();

 private:
  WebServer server;
  LoRaMsgHandler& loRaMsg;
  DeviceRegistry& deviceRegistry;
  String lastStatus;
  bool routesRegistered = false;

  // HTML page content
  static const char* getHtmlPage();

  String getContentType(String filename);

  bool exists(String path);

  bool handleFileRead(String path);

  // Helper to URL decode
  static String urlDecode(const String& input);
};
