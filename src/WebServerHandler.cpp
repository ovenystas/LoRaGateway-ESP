#include "WebServerHandler.h"

#include <SPIFFS.h>

// Static instance for callback handlers
static WebServerHandler* g_pWebServerHandler = nullptr;

// Static wrapper functions for WebServer callbacks
static void handleRootWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleRoot();
}

static void handlePingWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handlePing();
}

static void handleDiscoveryWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleDiscovery();
}

static void handleValueGetWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleValueGet();
}

static void handleValueSetWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleValueSet();
}

static void handleGetStatusWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleGetStatus();
}

static void handleNotFoundWrapper() {
  if (g_pWebServerHandler) g_pWebServerHandler->handleNotFound();
}

WebServerHandler::WebServerHandler(LoRaMsgHandler& loRaMsg,
                                   DeviceRegistry& registry, uint16_t port)
    : server(port),
      loRaMsg(loRaMsg),
      deviceRegistry(registry),
      lastStatus(""),
      routesRegistered(false) {
  g_pWebServerHandler = this;
}

bool WebServerHandler::begin() {
  if (routesRegistered) {
    return true;  // Already initialized
  }

  // Set up routes
  server.on("/", HTTP_GET, handleRootWrapper);
  server.on("/ping", HTTP_POST, handlePingWrapper);
  server.on("/discovery", HTTP_POST, handleDiscoveryWrapper);
  server.on("/ping", HTTP_POST, handlePingWrapper);
  server.on("/value-get", HTTP_POST, handleValueGetWrapper);
  server.on("/value-set", HTTP_POST, handleValueSetWrapper);
  server.on("/status", HTTP_GET, handleGetStatusWrapper);

  // 404 handler
  server.onNotFound(handleNotFoundWrapper);

  server.begin();
  routesRegistered = true;
  Serial.println(F("WebServer started on port 80"));
  return true;
}

void WebServerHandler::handle() { server.handleClient(); }

void WebServerHandler::stop() { server.stop(); }

String WebServerHandler::getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool WebServerHandler::exists(String path) {
  bool yes = false;
  File file = SPIFFS.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

bool WebServerHandler::handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void WebServerHandler::handleRoot() {
  if (!handleFileRead("/index.html")) {
    server.send(404, "text/plain", "FileNotFound");
  }
}

void WebServerHandler::handlePing() {
  // Get device ID from POST data
  String deviceIdStr = "";

  if (server.hasArg("deviceId")) {
    deviceIdStr = server.arg("deviceId");
  } else if (server.method() == HTTP_POST && server.args() > 0) {
    // Try to get from POST body
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "deviceId") {
        deviceIdStr = server.arg(i);
        break;
      }
    }
  }

  if (deviceIdStr.length() == 0) {
    server.send(400, "application/json", R"({"error":"Missing deviceId"})");
    return;
  }

  uint8_t deviceId = atoi(deviceIdStr.c_str());

  bool success = loRaMsg.sendPingRequest(deviceId);

  if (success) {
    lastStatus = String("Ping request sent to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(200, "application/json",
                R"({"success":true,"message":"Ping request sent"})");
  } else {
    lastStatus = String("Failed to send ping to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(500, "application/json",
                R"({"success":false,"error":"Failed to send ping request"})");
  }
}

void WebServerHandler::handleDiscovery() {
  // Get device ID from POST data
  String deviceIdStr = "";
  String entityIdStr = "";

  if (server.hasArg("deviceId")) {
    deviceIdStr = server.arg("deviceId");
  } else if (server.method() == HTTP_POST && server.args() > 0) {
    // Try to get from POST body
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "deviceId") {
        deviceIdStr = server.arg(i);
      } else if (server.argName(i) == "entityId") {
        entityIdStr = server.arg(i);
      }
    }
  }

  if (deviceIdStr.length() == 0) {
    server.send(400, "application/json", R"({"error":"Missing deviceId"})");
    return;
  }

  uint8_t deviceId = atoi(deviceIdStr.c_str());
  uint8_t entityId = 255;  // Default: all entities

  // Get entity ID if provided
  if (server.hasArg("entityId")) {
    entityId = atoi(server.arg("entityId").c_str());
  } else if (entityIdStr.length() > 0) {
    entityId = atoi(entityIdStr.c_str());
  }

  bool success = loRaMsg.sendDiscoveryRequest(deviceId, entityId);

  if (success) {
    lastStatus = String("Discovery request sent to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(200, "application/json",
                R"({"success":true,"message":"Discovery request sent"})");
  } else {
    lastStatus = String("Failed to send discovery to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(
        500, "application/json",
        R"({"success":false,"error":"Failed to send discovery request"})");
  }
}

void WebServerHandler::handleValueGet() {
  // Get device ID from POST data
  String deviceIdStr = "";
  String entityIdStr = "";

  if (server.hasArg("deviceId") && server.hasArg("entityId")) {
    deviceIdStr = server.arg("deviceId");
    entityIdStr = server.arg("entityId");
  } else {
    server.send(400, "application/json",
                R"({"error":"Missing deviceId or entityId"})");
    return;
  }

  uint8_t deviceId = atoi(deviceIdStr.c_str());
  uint8_t entityId = atoi(entityIdStr.c_str());

  bool success = loRaMsg.sendValueGetRequest(deviceId, entityId);

  if (success) {
    lastStatus = String("Value get request for entity ") + entityId +
                 String(" sent to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(200, "application/json",
                R"({"success":true,"message":"Value get request sent"})");
  } else {
    lastStatus = String("Failed to send value get request for entity ") +
                 entityId + String(" to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(
        500, "application/json",
        R"({"success":false,"error":"Failed to send value get request"})");
  }
}

void WebServerHandler::handleValueSet() {
  // Get device ID from POST data
  String deviceIdStr = "";
  String entityIdStr = "";
  String valueStr = "";

  if (server.hasArg("deviceId") && server.hasArg("entityId") &&
      server.hasArg("value")) {
    deviceIdStr = server.arg("deviceId");
    entityIdStr = server.arg("entityId");
    valueStr = server.arg("value");
  } else {
    server.send(400, "application/json",
                R"({"error":"Missing deviceId, entityId or value"})");
    return;
  }

  const uint8_t deviceId = atoi(deviceIdStr.c_str());
  const uint8_t entityId = atoi(entityIdStr.c_str());
  const float value = atof(valueStr.c_str());

  const EntityInfo* entity = deviceRegistry.getEntity(deviceId, entityId);
  if (!entity) {
    Serial.print("Error: Entity ");
    Serial.print(entityId);
    Serial.print(" not found on device ");
    Serial.println(deviceId);
    return;
  }

  const uint32_t rawValue = entity->format.toRawValue(value);

  const bool success =
      loRaMsg.sendValueSetRequest(deviceId, entityId, rawValue);

  if (success) {
    lastStatus = String("Value set request for entity ") + entityId +
                 String(" with value ") + value + String(" sent to device ") +
                 deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(200, "application/json",
                R"({"success":true,"message":"Value set request sent"})");
  } else {
    lastStatus = String("Failed to send value set request for entity ") +
                 entityId + String(" with value ") + value +
                 String(" to device ") + deviceId;
    Serial.print(F("WebServer: "));
    Serial.println(lastStatus);
    server.send(
        500, "application/json",
        R"({"success":false,"error":"Failed to send value set request"})");
  }
}

void WebServerHandler::handleGetStatus() {
  String json = R"({"status":")" + lastStatus + R"("})";
  server.send(200, "application/json", json);
}

void WebServerHandler::handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (int i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

const char* WebServerHandler::getHtmlPage() {
  return R"EOF(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LoRa Gateway Control</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
            padding: 40px;
            max-width: 600px;
            width: 100%;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 25px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
            font-size: 14px;
        }
        input[type="text"],
        input[type="number"],
        select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input[type="text"]:focus,
        input[type="number"]:focus,
        select:focus {
            outline: none;
            border-color: #667eea;
        }
        .button-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 30px;
        }
        button {
            padding: 12px 24px;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .btn-ping {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .btn-value {
            background: linear-gradient(135deg, #66e6ea 0%, #4b67a2 100%);
            color: white;
        }
        .btn-ping:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        .btn-ping:active {
            transform: translateY(0);
        }
        .btn-discovery {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
        }
        .btn-discovery:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(245, 87, 108, 0.4);
        }
        .btn-discovery:active {
            transform: translateY(0);
        }
        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            transform: none;
        }
        .status-message {
            margin-top: 30px;
            padding: 15px;
            border-radius: 6px;
            display: none;
            font-size: 14px;
            font-weight: 500;
        }
        .status-message.show {
            display: block;
        }
        .status-message.success {
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status-message.error {
            background-color: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .spinner {
            display: inline-block;
            width: 14px;
            height: 14px;
            border: 2px solid rgba(0, 0, 0, 0.1);
            border-radius: 50%;
            border-top-color: #667eea;
            animation: spin 0.6s linear infinite;
            margin-right: 8px;
            vertical-align: middle;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .info-box {
            background-color: #e7f3ff;
            border-left: 4px solid #667eea;
            padding: 12px;
            margin-bottom: 25px;
            border-radius: 4px;
            font-size: 13px;
            color: #004085;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>LoRa Gateway Control</h1>
        <p class="subtitle">Send commands to LoRa devices</p>

        <div class="info-box">
            💡 Enter a device ID and select an action to control remote LoRa devices.
        </div>

        <form id="commandForm">
            <div class="form-group">
                <label for="deviceId">Device ID</label>
                <input type="number" id="deviceId" name="deviceId" min="0" max="255" placeholder="Enter device ID (0-255)" required>
            </div>

            <div class="form-group">
                <label for="entityId">Entity ID (Optional)</label>
                <input type="number" id="entityId" name="entityId" min="0" max="255" placeholder="Leave empty for all entities">
            </div>

            <div class="button-group">
                <button type="button" class="btn-ping" id="pingBtn" onclick="sendPing()">📡 Send Ping</button>
                <button type="button" class="btn-discovery" id="discoveryBtn" onclick="sendDiscovery()">🔍 Send Discovery</button>
                <button type="button" class="btn-value" id="valueBtn" onclick="sendValueRequest()">🫴 Get Value</button>
            </div>
        </form>

        <div id="statusMessage" class="status-message"></div>
    </div>

    <script>
        const statusMsg = document.getElementById('statusMessage');
        const pingBtn = document.getElementById('pingBtn');
        const discoveryBtn = document.getElementById('discoveryBtn');
        const deviceIdInput = document.getElementById('deviceId');
        const entityIdInput = document.getElementById('entityId');

        function showStatus(message, isSuccess, isLoading = false) {
            statusMsg.textContent = '';
            if (isLoading) {
                const spinner = document.createElement('span');
                spinner.className = 'spinner';
                statusMsg.appendChild(spinner);
                statusMsg.appendChild(document.createTextNode(message));
            } else {
                statusMsg.textContent = message;
            }
            statusMsg.className = 'status-message show ' + (isSuccess ? 'success' : 'error');
        }

        function setButtonsDisabled(disabled) {
            pingBtn.disabled = disabled;
            discoveryBtn.disabled = disabled;
            deviceIdInput.disabled = disabled;
            entityIdInput.disabled = disabled;
        }

        async function sendPing() {
            const deviceId = document.getElementById('deviceId').value;
            if (!deviceId) {
                showStatus('Please enter a device ID', false);
                return;
            }

            setButtonsDisabled(true);
            showStatus('Sending ping request...', true, true);

            try {
                const response = await fetch('/ping', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: 'deviceId=' + encodeURIComponent(deviceId)
                });

                const data = await response.json();

                if (response.ok && data.success) {
                    showStatus('✓ Ping sent to device ' + deviceId, true);
                } else {
                    showStatus('✗ Failed to send ping: ' + (data.error || 'Unknown error'), false);
                }
            } catch (error) {
                showStatus('✗ Network error: ' + error.message, false);
            } finally {
                setButtonsDisabled(false);
            }
        }

        async function sendDiscovery() {
            const deviceId = document.getElementById('deviceId').value;
            const entityId = document.getElementById('entityId').value;

            if (!deviceId) {
                showStatus('Please enter a device ID', false);
                return;
            }

            setButtonsDisabled(true);
            showStatus('Sending discovery request...', true, true);

            try {
                let body = 'deviceId=' + encodeURIComponent(deviceId);
                if (entityId) {
                    body += '&entityId=' + encodeURIComponent(entityId);
                }

                const response = await fetch('/discovery', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: body
                });

                const data = await response.json();

                if (response.ok && data.success) {
                    showStatus('✓ Discovery sent to device ' + deviceId, true);
                } else {
                    showStatus('✗ Failed to send discovery: ' + (data.error || 'Unknown error'), false);
                }
            } catch (error) {
                showStatus('✗ Network error: ' + error.message, false);
            } finally {
                setButtonsDisabled(false);
            }
        }

        async function sendValueRequest() {
            const deviceId = document.getElementById('deviceId').value;
            const entityId = document.getElementById('entityId').value;

            if (!deviceId) {
                showStatus('Please enter a device ID', false);
                return;
            }

            setButtonsDisabled(true);
            showStatus('Sending value request...', true, true);

            try {
                let body = 'deviceId=' + encodeURIComponent(deviceId);
                if (entityId) {
                    body += '&entityId=' + encodeURIComponent(entityId);
                }

                const response = await fetch('/value', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: body
                });

                const data = await response.json();

                if (response.ok && data.success) {
                    showStatus('✓ Value request sent to device ' + deviceId, true);
                } else {
                    showStatus('✗ Failed to send value request: ' + (data.error || 'Unknown error'), false);
                }
            } catch (error) {
                showStatus('✗ Network error: ' + error.message, false);
            } finally {
                setButtonsDisabled(false);
            }
        }

        // Allow Enter key to trigger value request
        document.getElementById('entityId').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                e.preventDefault();
                sendValueRequest();
            }
        });
    </script>
</body>
</html>
)EOF";
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
