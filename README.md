# LoRaGateway-ESP

An ESP-based LoRa gateway bridging a local LoRa sensor network with MQTT/Home Assistant integration.

## Overview

This gateway enables bidirectional communication between:
- **LoRa Devices**: Sensor devices communicating wirelessly via LoRa (868 MHz / 915 MHz)
- **MQTT Server**: Receives sensor updates and sends commands
- **Home Assistant**: Auto-discovers devices and enables real-time control

### Key Features

- **Runtime Device Discovery**: Devices are discovered automatically when they first communicate
- **Home Assistant Auto-Discovery**: Devices appear automatically in Home Assistant
- **Bidirectional Communication**: Sensor values → MQTT, Commands from HA → LoRa
- **Device Types**: BinarySensor, Sensor, Switch, Cover
- **Device Timeout Management**: Automatically removes offline devices
- **CRC16 Validation**: Reliable message integrity checking
- **Credentials Support**: Optional MQTT authentication

## Quick Start

### Requirements

**Hardware:**
- ESP32 board (Joy-it NodeMCU-ESP32 recommended)
- RFM95 LoRa module (868 MHz or 915 MHz)
- USB cable for programming
- LoRa antenna

**Software:**
- PlatformIO (VS Code extension or CLI)
- MQTT broker (Mosquitto, Home Assistant, etc.)
- WiFi network access

### Setup (5 minutes)

1. **Copy secrets template:**
   ```bash
   cp include/secrets_example.h include/secrets.h
   ```

2. **Edit `include/secrets.h` with your credentials:**
   ```cpp
   #define WIFI_SSID "YourWiFiName"
   #define WIFI_PASSWORD "YourPassword"
   #define MQTT_BROKER "192.168.1.100"
   #define MQTT_PORT 1883
   #define MQTT_USERNAME "mqtt_user"      // Optional
   #define MQTT_PASSWORD "mqtt_password"  // Optional
   ```

3. **Hardware Wiring**

   | RFM95 | NodeMCU-ESP32 |
   | ----- | ------------- |
   | 3.3V  | 3V3           |
   | GND   | GND           |
   | NSS   | D5            |
   | MISO  | D19           |
   | MOSI  | D23           |
   | SCK   | D18           |
   | RST   | D14           |
   | DIO0  | D2            |
   | ANA   | Antenna wire  |

   **Antenna wire length:**

   - 868 MHz: 86,3 mm
   - 915 MHz: 81,9 mm
   - 433 MHz: 173,1 mm

4. **Build and Upload:**
   ```bash
   platformio run -e nodemcu-32s --target upload
   platformio device monitor
   ```

5. **Verify startup:**
   ```
   LoRa Gateway starting up...
   WiFi connected! IP: 192.168.1.x
   MQTT connected!
   ```

### Adjust Pin Configuration

Edit `include/secrets.h` to change LoRa pins (if using different GPIO):
```cpp
#define LORA_CS_PIN 5       // Chip Select (NSS)
#define LORA_RST_PIN 14     // Reset
#define LORA_DIO_PIN 2      // DIO0 interrupt
```

## Architecture

### Core Components

1. **Types.h** - Data structures and message formats
   - `LoRaMessage`: Wireless message format with CRC validation
   - Entity types: BinarySensor, Sensor, Switch, Cover
   - Value types: Boolean, Int, Float, String

2. **LoRaHandler** - LoRa radio communication
   - Message encoding/decoding with CRC16 checksums
   - Sends messages to nodes
   - Receives and parses messages from nodes

3. **MqttHandler** - MQTT broker communication
   - Connects to MQTT server
   - Publishes sensor values and discovery messages
   - Subscribes to command topics

4. **DeviceRegistry** - Tracks discovered devices and entities
   - Maintains list of registered devices
   - Stores entity metadata (type, name, unit, etc.)
   - Updates last-seen timestamps for timeout detection

5. **main.cpp** - Gateway logic and message routing
   - WiFi/MQTT connection management
   - Routes messages between LoRa and MQTT
   - Handles node discovery
   - Manages command execution

## Configuration

All settings are in `include/secrets.h`:

```cpp
// WiFi credentials
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourPassword"

// MQTT broker
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora_gateway"
#define MQTT_USERNAME "user"      // Optional
#define MQTT_PASSWORD "password"  // Optional

// LoRa pins and frequency
#define LORA_CS_PIN 5           // GPIO5 (NSS)
#define LORA_RST_PIN 14         // GPIO14 (RESET)
#define LORA_DIO_PIN 2          // GPIO2 (DIO0)
#define LORA_FREQUENCY 868000000  // 868 MHz EU, 915 MHz US

// Gateway timeout for inactive devices
#define DEVICE_TIMEOUT_SECONDS 300
```

## Message Topics and Payloads

### State Publishing (Gateway → MQTT)
Topic: `lora_gateway/device_{deviceId}/entity_{entityId}/state`
```json
{
  "value": 23.5,
  "entity": "Temperature Sensor",
  "timestamp": 1234567890
}
```

### Commands (MQTT → Gateway → LoRa)
Topic: `lora_gateway/device_{deviceId}/entity_{entityId}/command`
```json
{"command": "ON"}     // BinarySensor/Switch
{"command": "OFF"}    // BinarySensor/Switch
{"command": "OPEN"}   // Cover
{"command": "CLOSE"}  // Cover
{"command": "STOP"}   // Cover
{"value": 128}        // Any numeric device
```

## LoRa Message Format

Binary format with CRC16 validation:
```
[SYNC] [DeviceID-H] [DeviceID-L] [EntityID] [Type] [MsgType] [ValueType] 
[Value...] [Command] [CmdData0-3] [CRC-H] [CRC-L]
```

- **SYNC**: 0xAA
- **DeviceID**: 16-bit device identifier
- **EntityID**: 8-bit entity ID within device
- **Type**: Entity type (0=BinarySensor, 1=Sensor, 2=Switch, 3=Cover)
- **MsgType**: Message type (0=announcement, 1=sensor update, 3=command)
- **ValueType**: Value encoding (0=bool, 1=int32, 2=float, 3=string)
- **Value**: Variable length based on ValueType
- **CRC16**: CCITT polynomial validation

## Home Assistant Integration

Once devices are discovered, they appear automatically in Home Assistant. To enable:

**In Home Assistant `configuration.yaml`:**
```yaml
mqtt:
  broker: 192.168.1.100
  username: mqtt_user
  password: mqtt_password
```

Restart Home Assistant. Devices appear under **Settings → Devices & Services → MQTT**.

**Capabilities:**
- Read sensor values in real-time
- Control switches and covers via commands
- Create automations based on sensor state changes
- Monitor device availability

**Discovery Topics:**
- Binary Sensors: `homeassistant/binary_sensor/lora_{deviceId}_{entityId}/config`
- Sensors: `homeassistant/sensor/lora_{deviceId}_{entityId}/config`
- Switches: `homeassistant/switch/lora_{deviceId}_{entityId}/config`
- Covers: `homeassistant/cover/lora_{deviceId}_{entityId}/config`

## Libraries

- **LoRa** - LoRa radio communication
- **PubSubClient** - MQTT client
- **ArduinoJson** - JSON handling
- **CRC** - Message validation

## Extending the Gateway

### Adding a New Device Type

1. Add type to `EntityType` enum in [include/Types.h](include/Types.h)
2. Update `publishDiscovery()` in [src/MqttHandler.cpp](src/MqttHandler.cpp)
3. Update command handling in [src/main.cpp](src/main.cpp)

### Custom LoRa Message Format

Modify encoding/decoding in [src/LoRaHandler.cpp](src/LoRaHandler.cpp) to support custom payloads.

### Adding MQTT Authentication

Use the overloaded `connect()` function in [MqttHandler](include/MqttHandler.h):
```cpp
mqttHandler.connect(broker, port, clientId, username, password);
```
