#include "WebServerHandler.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "Logger.h"

static const char* JSON_CONTENT = "application/json";

WebServerHandler::WebServerHandler(LoRaMsgHandler& loRaMsg,
                                   DeviceRegistry& registry, uint16_t port)
    : server(port),
      ws("/ws"),
      loRaMsg(loRaMsg),
      deviceRegistry(registry),
      lastStatus(""),
      started(false) {}

bool WebServerHandler::begin() {
  if (started) return true;

  // Set up WebSocket with the event handler
  ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                    AwsEventType type, void* arg, uint8_t* data, size_t len) {
    this->onWsEvent(server, client, type, arg, data, len);
  });
  server.addHandler(&ws);

  // Wire up Logger's WebSocket pointer so log messages are pushed in real time
  logger.setWebSocket(&ws);

  // Serve static files from SPIFFS. index.html is the default page.
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  // --- REST endpoints ---

  server.on("/ping", HTTP_POST, [this](AsyncWebServerRequest* request) {
    String deviceIdStr;
    if (request->hasParam("deviceId", true)) {
      deviceIdStr = request->getParam("deviceId", true)->value();
    } else if (request->hasParam("deviceId", false)) {
      deviceIdStr = request->getParam("deviceId", false)->value();
    }

    if (deviceIdStr.length() == 0) {
      request->send(400, JSON_CONTENT, R"({"error":"Missing deviceId"})");
      return;
    }

    uint8_t deviceId = atoi(deviceIdStr.c_str());
    bool success = loRaMsg.sendPingRequest(deviceId);

    if (success) {
      lastStatus = String("Ping request sent to device ") + deviceId;
      logger.printf(Logger::INF, "WebServer: %s", lastStatus.c_str());
      request->send(200, JSON_CONTENT,
                    R"({"success":true,"message":"Ping request sent"})");
    } else {
      lastStatus = String("Failed to send ping to device ") + deviceId;
      logger.printf(Logger::ERR, "WebServer: %s", lastStatus.c_str());
      request->send(
          500, JSON_CONTENT,
          R"({"success":false,"error":"Failed to send ping request"})");
    }
  });

  server.on("/discovery", HTTP_POST, [this](AsyncWebServerRequest* request) {
    String deviceIdStr;
    String entityIdStr;

    if (request->hasParam("deviceId", true)) {
      deviceIdStr = request->getParam("deviceId", true)->value();
    } else if (request->hasParam("deviceId", false)) {
      deviceIdStr = request->getParam("deviceId", false)->value();
    }

    if (request->hasParam("entityId", true)) {
      entityIdStr = request->getParam("entityId", true)->value();
    } else if (request->hasParam("entityId", false)) {
      entityIdStr = request->getParam("entityId", false)->value();
    }

    if (deviceIdStr.length() == 0) {
      request->send(400, JSON_CONTENT, R"({"error":"Missing deviceId"})");
      return;
    }

    uint8_t deviceId = atoi(deviceIdStr.c_str());
    uint8_t entityId = 255;  // Default: all entities
    if (entityIdStr.length() > 0) {
      entityId = atoi(entityIdStr.c_str());
    }

    bool success = loRaMsg.sendDiscoveryRequest(deviceId, entityId);

    if (success) {
      lastStatus = String("Discovery request sent to device ") + deviceId;
      logger.printf(Logger::INF, "WebServer: %s", lastStatus.c_str());
      request->send(200, JSON_CONTENT,
                    R"({"success":true,"message":"Discovery request sent"})");
    } else {
      lastStatus = String("Failed to send discovery to device ") + deviceId;
      logger.printf(Logger::ERR, "WebServer: %s", lastStatus.c_str());
      request->send(
          500, JSON_CONTENT,
          R"({"success":false,"error":"Failed to send discovery request"})");
    }
  });

  server.on("/value-get", HTTP_POST, [this](AsyncWebServerRequest* request) {
    String deviceIdStr;
    String entityIdStr;

    if (request->hasParam("deviceId", true)) {
      deviceIdStr = request->getParam("deviceId", true)->value();
    } else if (request->hasParam("deviceId", false)) {
      deviceIdStr = request->getParam("deviceId", false)->value();
    }
    if (request->hasParam("entityId", true)) {
      entityIdStr = request->getParam("entityId", true)->value();
    } else if (request->hasParam("entityId", false)) {
      entityIdStr = request->getParam("entityId", false)->value();
    }

    if (deviceIdStr.length() == 0 || entityIdStr.length() == 0) {
      request->send(400, JSON_CONTENT,
                    R"({"error":"Missing deviceId or entityId"})");
      return;
    }

    uint8_t deviceId = atoi(deviceIdStr.c_str());
    uint8_t entityId = atoi(entityIdStr.c_str());
    bool success = loRaMsg.sendValueGetRequest(deviceId, entityId);

    if (success) {
      lastStatus = String("Value get request for entity ") + entityId +
                   String(" sent to device ") + deviceId;
      logger.printf(Logger::INF, "WebServer: %s", lastStatus.c_str());
      request->send(200, JSON_CONTENT,
                    R"({"success":true,"message":"Value get request sent"})");
    } else {
      lastStatus = String("Failed to send value get request for entity ") +
                   entityId + String(" to device ") + deviceId;
      logger.printf(Logger::ERR, "WebServer: %s", lastStatus.c_str());
      request->send(
          500, JSON_CONTENT,
          R"({"success":false,"error":"Failed to send value get request"})");
    }
  });

  server.on("/value-set", HTTP_POST, [this](AsyncWebServerRequest* request) {
    String deviceIdStr;
    String entityIdStr;
    String valueStr;

    if (request->hasParam("deviceId", true)) {
      deviceIdStr = request->getParam("deviceId", true)->value();
    } else if (request->hasParam("deviceId", false)) {
      deviceIdStr = request->getParam("deviceId", false)->value();
    }
    if (request->hasParam("entityId", true)) {
      entityIdStr = request->getParam("entityId", true)->value();
    } else if (request->hasParam("entityId", false)) {
      entityIdStr = request->getParam("entityId", false)->value();
    }
    if (request->hasParam("value", true)) {
      valueStr = request->getParam("value", true)->value();
    } else if (request->hasParam("value", false)) {
      valueStr = request->getParam("value", false)->value();
    }

    if (deviceIdStr.length() == 0 || entityIdStr.length() == 0 ||
        valueStr.length() == 0) {
      request->send(400, JSON_CONTENT,
                    R"({"error":"Missing deviceId, entityId or value"})");
      return;
    }

    const uint8_t deviceId = atoi(deviceIdStr.c_str());
    const uint8_t entityId = atoi(entityIdStr.c_str());
    const float value = atof(valueStr.c_str());

    const EntityInfo* entity = deviceRegistry.getEntity(deviceId, entityId);
    if (!entity) {
      logger.printf(Logger::ERR, "Error: Entity %d not found on device %d",
                    entityId, deviceId);
      request->send(400, JSON_CONTENT,
                    R"({"error":"Entity not found on device"})");
      return;
    }

    const uint32_t rawValue = entity->format.toRawValue(value);
    const bool success =
        loRaMsg.sendValueSetRequest(deviceId, entityId, rawValue);

    if (success) {
      lastStatus = String("Value set request for entity ") + entityId +
                   String(" with value ") + value + String(" sent to device ") +
                   deviceId;
      logger.printf(Logger::INF, "WebServer: %s", lastStatus.c_str());
      request->send(200, JSON_CONTENT,
                    R"({"success":true,"message":"Value set request sent"})");
    } else {
      lastStatus = String("Failed to send value set request for entity ") +
                   entityId + String(" with value ") + value +
                   String(" to device ") + deviceId;
      logger.printf(Logger::ERR, "WebServer: %s", lastStatus.c_str());
      request->send(
          500, JSON_CONTENT,
          R"({"success":false,"error":"Failed to send value set request"})");
    }
  });

  server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    char json[256];
    snprintf(json, sizeof(json), R"({"status":"%s"})", lastStatus.c_str());
    request->send(200, JSON_CONTENT, json);
  });

  // 404 handler
  server.onNotFound([](AsyncWebServerRequest* request) {
    String message = "File Not Found\n\n";
    message += "URI: " + request->url() + "\n";
    message +=
        "Method: " + String(request->method() == HTTP_GET ? "GET" : "POST") +
        "\n";
    message += "Arguments: " + String(request->params()) + "\n";

    for (size_t i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      message += " " + p->name() + ": " + p->value() + "\n";
    }

    request->send(404, "text/plain", message);
  });

  server.begin();
  started = true;
  logger.println(Logger::INF, F("AsyncWebServer started on port 80"));
  return true;
}

void WebServerHandler::stop() {
  server.end();
  started = false;
  logger.setWebSocket(nullptr);
}

void WebServerHandler::onWsEvent(AsyncWebSocket* server,
                                 AsyncWebSocketClient* client,
                                 AwsEventType type, void* arg, uint8_t* data,
                                 size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      logger.printf(Logger::INF, "WebSocket client #%u connected",
                    client->id());
      sendInitToClient(client);
      break;

    case WS_EVT_DISCONNECT:
      logger.printf(Logger::INF, "WebSocket client #%u disconnected",
                    client->id());
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len &&
          info->opcode == WS_TEXT) {
        data[len] = '\0';
        handleWsMessage(client, String((char*)data));
      }
      break;
    }

    case WS_EVT_ERROR:
      logger.printf(Logger::ERR, "WebSocket error on client #%u", client->id());
      break;

    default:
      break;
  }
}

void WebServerHandler::sendInitToClient(AsyncWebSocketClient* client) {
  if (!client) return;

  // Send settings
  char initBuf[256];
  snprintf(initBuf, sizeof(initBuf),
           R"({"type":"init","webEnabled":%s,"level":"%s"})",
           logger.isWebEnabled() ? "true" : "false",
           Logger::levelToString(logger.getMaxLevel()));
  client->text(initBuf);

  // Send buffered log history (one message per entry)
  const auto& history = logger.getBufferedEntries();
  for (const auto& entry : history) {
    char logBuf[512];
    int hdr =
        snprintf(logBuf, sizeof(logBuf),
                 R"({"type":"log","level":"%s","timestamp":%lu,"message":")",
                 Logger::levelToString(entry.level), entry.timestamp);

    size_t pos = hdr;
    for (size_t i = 0; i < entry.message.length() && pos < sizeof(logBuf) - 4;
         i++) {
      char c = entry.message[i];
      if (c == '"') {
        if (pos + 1 < sizeof(logBuf) - 4) {
          logBuf[pos++] = '\\';
          logBuf[pos++] = '"';
        }
      } else if (c == '\\') {
        if (pos + 1 < sizeof(logBuf) - 4) {
          logBuf[pos++] = '\\';
          logBuf[pos++] = '\\';
        }
      } else if (c == '\n') {
        if (pos + 1 < sizeof(logBuf) - 4) {
          logBuf[pos++] = '\\';
          logBuf[pos++] = 'n';
        }
      } else if (c == '\r') {
        if (pos + 1 < sizeof(logBuf) - 4) {
          logBuf[pos++] = '\\';
          logBuf[pos++] = 'r';
        }
      } else if (c == '\t') {
        if (pos + 1 < sizeof(logBuf) - 4) {
          logBuf[pos++] = '\\';
          logBuf[pos++] = 't';
        }
      } else if (c < 0x20) {
        // Skip control characters
      } else {
        logBuf[pos++] = c;
      }
    }
    logBuf[pos++] = '"';
    logBuf[pos++] = '}';
    logBuf[pos] = '\0';

    client->text(logBuf);
  }
}

void WebServerHandler::handleWsMessage(AsyncWebSocketClient* client,
                                       const String& message) {
  // Parse incoming JSON command from the web UI.
  // Expected formats:
  //   {"type":"setLogLevel","level":2}
  //   {"type":"toggleWebLog","enabled":true}

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    logger.printf(Logger::WRN, "Invalid WebSocket message: %s",
                  message.c_str());
    return;
  }

  const char* msgType = doc["type"];
  if (!msgType) return;

  if (strcmp(msgType, "setLogLevel") == 0) {
    int level = doc["level"] | -1;
    if (level >= Logger::ERR && level <= Logger::DBG) {
      logger.setMaxLevel(static_cast<Logger::Level>(level));
      logger.printf(Logger::INF, "Log level set to %s",
                    Logger::levelToString(static_cast<Logger::Level>(level)));

      // Notify all clients of the updated level
      char buf[128];
      snprintf(buf, sizeof(buf),
               R"({"type":"logSettings","webEnabled":%s,"level":"%s"})",
               logger.isWebEnabled() ? "true" : "false",
               Logger::levelToString(logger.getMaxLevel()));
      ws.textAll(buf);
    }
  } else if (strcmp(msgType, "toggleWebLog") == 0) {
    bool enabled = doc["enabled"] | false;
    logger.setWebEnabled(enabled);
    logger.printf(Logger::INF, "Web logging %s",
                  enabled ? "enabled" : "disabled");

    // Notify all clients
    char buf[128];
    snprintf(buf, sizeof(buf),
             R"({"type":"logSettings","webEnabled":%s,"level":"%s"})",
             logger.isWebEnabled() ? "true" : "false",
             Logger::levelToString(logger.getMaxLevel()));
    ws.textAll(buf);
  }
}

String WebServerHandler::urlDecode(const String& input) {
  String result;
  for (size_t i = 0; i < input.length(); i++) {
    if (input[i] == '%' && i + 2 < input.length()) {
      String hex = input.substring(i + 1, i + 3);
      result += (char)strtol(hex.c_str(), nullptr, 16);
      i += 2;
    } else if (input[i] == '+') {
      result += ' ';
    } else {
      result += input[i];
    }
  }
  return result;
}