# Quick Reference Card

## Configuration (Config.h)

```cpp
// WiFi
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// MQTT
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883

// LoRa
#define LORA_FREQUENCY 868000000  // 868 MHz EU, 915 MHz US
#define LORA_CS_PIN D8
#define LORA_RST_PIN D4
#define LORA_DIO_PIN D3

// Node Management
#define NODE_TIMEOUT_SECONDS 300
```

## Device Types

```cpp
DeviceType::BINARY_SENSOR  // 0 - Motion, door sensors
DeviceType::SENSOR          // 1 - Temperature, humidity
DeviceType::SWITCH          // 2 - On/off control
DeviceType::COVER           // 3 - Position control
```

## Command Types

```cpp
CommandType::SET_STATE      // Toggle switch on/off
CommandType::SET_POSITION   // Set cover position (0-255)
CommandType::OPEN           // Open cover
CommandType::CLOSE          // Close cover
CommandType::STOP           // Stop cover movement
```

## Value Types

```cpp
ValueType::BOOLEAN          // true/false
ValueType::INT_VALUE        // -2.1B to +2.1B
ValueType::FLOAT_VALUE      // IEEE 754 float
ValueType::STRING           // Text data
```

## MQTT Topics

### State Topics (Gateway → MQTT)
```
lora_gateway/node_{nodeId}/device_{deviceId}/state
```

### Command Topics (MQTT → Gateway)
```
lora_gateway/node_{nodeId}/device_{deviceId}/command
```

### Discovery Topics (Gateway → MQTT)
```
homeassistant/{component}/lora_{nodeId}_{deviceId}/config
```

## JSON Payloads

### Sensor State
```json
{
  "value": 23.5,
  "device": "Temperature Sensor",
  "timestamp": 1234567890
}
```

### Switch State
```json
{
  "value": true,
  "state": "ON",
  "device": "Light Switch"
}
```

### Switch Command
```json
{"command": "ON"}
{"command": "OFF"}
```

### Cover Command
```json
{"command": "OPEN"}
{"command": "CLOSE"}
{"command": "STOP"}
{"value": 128}  // Position 0-255
```

## Node Implementation Skeleton

```cpp
#include <Arduino.h>
#include "LoRaHandler.h"
#include "Types.h"

LoRaHandler loRa(10, 9, 2);  // CS, RST, DIO

void setup() {
  Serial.begin(115200);
  if (!loRa.begin(868000000)) while(1);
  loRa.setOnMessageReceived(handleCommand);
  announceNode();
}

void loop() {
  loRa.handle();
  
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 30000) {
    lastSend = millis();
    sendUpdate();
  }
  
  delay(100);
}

void announceNode() {
  LoRaMessage msg;
  msg.nodeId = 1001;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::SENSOR;
  msg.messageType = 0;  // Announcement
  msg.valueType = ValueType::FLOAT_VALUE;
  msg.value.floatValue = 0.0f;
  loRa.sendMessage(msg);
}

void sendUpdate() {
  LoRaMessage msg;
  msg.nodeId = 1001;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::SENSOR;
  msg.messageType = 1;  // Update
  msg.valueType = ValueType::FLOAT_VALUE;
  msg.value.floatValue = 23.5f;  // Your value
  loRa.sendMessage(msg);
}

void handleCommand(const LoRaMessage& msg) {
  if (msg.messageType == 3) {  // Command
    switch (msg.command) {
      case CommandType::SET_STATE:
        // msg.commandValue[0] = 0 (OFF) or 1 (ON)
        break;
      case CommandType::SET_POSITION:
        // msg.commandValue[0] = position (0-255)
        break;
      default:
        break;
    }
  }
}
```

## Wiring (Huzzah + RFM95)

```
RFM95 GND   → GND
RFM95 3V3   → 3V3
RFM95 NSS   → D8  (CS)
RFM95 RST   → D4  (RST)
RFM95 DIO0  → D3  (DIO)
RFM95 SCK   → D5  (GPIO14)
RFM95 MOSI  → D7  (GPIO13)
RFM95 MISO  → D6  (GPIO12)
```

## Build & Upload

```bash
# PlatformIO CLI
platformio run -e huzzah
platformio run -e huzzah --target upload
platformio device monitor

# Or use VS Code PlatformIO extension
# Click Build, Upload, Monitor buttons
```

## MQTT Testing Commands

```bash
# Subscribe to all gateway topics
mosquitto_sub -h 192.168.1.100 -t "lora_gateway/#"

# Subscribe to discovery topics
mosquitto_sub -h 192.168.1.100 -t "homeassistant/#"

# Publish a command
mosquitto_pub -h 192.168.1.100 \
  -t "lora_gateway/node_1003/device_0/command" \
  -m '{"command": "ON"}'

# Publish a position
mosquitto_pub -h 192.168.1.100 \
  -t "lora_gateway/node_1004/device_0/command" \
  -m '{"value": 200}'
```

## Serial Monitor Output

### Startup
```
====================================
LoRa Gateway starting up...
====================================
Initializing LoRa... OK
Connecting to WiFi SSID: MyWiFi
......
WiFi connected! IP address: 192.168.1.100
Connecting to MQTT broker at 192.168.1.100:1883
MQTT connected!
Gateway initialized successfully!
```

### Operation
```
LoRa message received from Node 1001, Device 0
New node discovered! Node ID: 1001
Device registered: Unknown Device
Published Home Assistant discovery for device Unknown Device
Published sensor value to MQTT: 23.5
```

## Troubleshooting Checklist

- [ ] LoRa module has power
- [ ] Antenna is connected to RFM95
- [ ] Pins in Config.h match your wiring
- [ ] WiFi SSID and password are correct
- [ ] MQTT broker IP is reachable
- [ ] Node frequency matches gateway (868 vs 915 MHz)
- [ ] Serial monitor shows startup messages
- [ ] WiFi shows IP address
- [ ] MQTT shows "connected"
- [ ] LoRa shows "message received"

## Entity IDs in Home Assistant

Automatic format for discovered devices:

```
sensor.lora_1001_0
binary_sensor.lora_1002_0
switch.lora_1003_0
cover.lora_1004_0
```

Where:
- `1001`, `1002`, etc. = Node ID
- `0`, `1`, etc. = Device ID within node

## Useful Links

- **Home Assistant MQTT**: https://www.home-assistant.io/integrations/mqtt/
- **RFM95 Datasheet**: https://cdn.sparkfun.com/assets/learn_tutorials/8/0/RFM95_datasheet.pdf
- **LoRa Arduino Library**: https://github.com/sandeepmistry/arduino-LoRa
- **PubSubClient**: https://github.com/knolleary/pubsubclient
- **ArduinoJson**: https://arduinojson.org/

## File Locations

| File | Purpose | Path |
|------|---------|------|
| Configuration | WiFi, MQTT, LoRa settings | `include/Config.h` |
| Data Types | Message structures | `include/Types.h` |
| Gateway Logic | Main event loop | `src/main.cpp` |
| LoRa Radio | RFM95 communication | `src/LoRaHandler.cpp` |
| MQTT Broker | MQTT operations | `src/MqttHandler.cpp` |
| Node Registry | Device tracking | `src/NodeRegistry.cpp` |
| Quick Start | Setup guide | `QUICKSTART.md` |
| Full Docs | Complete reference | `README.md` |

---

**For complete documentation, see INDEX.md**
