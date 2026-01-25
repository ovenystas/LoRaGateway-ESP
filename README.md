# LoRaGateway-ESP

An ESP-based LoRa gateway bridging a local LoRa sensor network with MQTT/Home Assistant.

## Overview

This gateway enables bidirectional communication between:
- **LoRa Nodes**: Sensor devices communicating wirelessly via LoRa (868 MHz)
- **MQTT Server**: Receives sensor updates and sends commands
- **Home Assistant**: Auto-discovers devices and integrates with HA automations

### Key Features

- **Runtime Node Discovery**: Nodes are discovered when they first communicate
- **Home Assistant Auto-Discovery**: Automatically announces discovered devices to HA
- **Bidirectional Communication**: Sensor values → MQTT, Commands from HA → LoRa
- **Device Types Supported**:
  - BinarySensor (motion sensors, door sensors, etc.)
  - Sensor (temperature, humidity, distance, etc.)
  - Switch (on/off control)
  - Cover (blinds, garage doors, etc.)
- **Node Timeout Management**: Automatically removes nodes that haven't communicated

## Hardware

- **ESP8266 Development Board**: Adafruit Feather Huzzah
- **LoRa Module**: RFM95 (868 MHz / 915 MHz configurable)

### Pin Configuration (Configurable)

By default in [include/Config.h](include/Config.h):
```
LORA_CS_PIN  = D8  (Chip Select)
LORA_RST_PIN = D4  (Reset)
LORA_DIO_PIN = D3  (DIO0 interrupt)
```

Adjust these pins based on your wiring.

## Software Architecture

### Core Components

1. **Types.h** - Data structures and message formats
   - `LoRaMessage`: Wireless message format with CRC validation
   - Device types: BinarySensor, Sensor, Switch, Cover
   - Value types: Boolean, Int, Float, String

2. **LoRaHandler** - LoRa radio communication
   - Message encoding/decoding with CRC16 checksums
   - Sends messages to nodes
   - Receives and parses messages from nodes

3. **MqttHandler** - MQTT broker communication
   - Connects to MQTT server
   - Publishes sensor values and discovery messages
   - Subscribes to command topics

4. **NodeRegistry** - Tracks discovered nodes and devices
   - Maintains list of registered nodes
   - Stores device metadata (type, name, unit, etc.)
   - Updates last-seen timestamps for timeout detection

5. **main.cpp** - Gateway logic and message routing
   - WiFi/MQTT connection management
   - Routes messages between LoRa and MQTT
   - Handles node discovery
   - Manages command execution

## Configuration

Edit [include/Config.h](include/Config.h) with your settings:

```cpp
// WiFi
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// MQTT
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora_gateway"

// LoRa
#define LORA_CS_PIN D8
#define LORA_RST_PIN D4
#define LORA_DIO_PIN D3
#define LORA_FREQUENCY 868000000  // 868 MHz for EU, 915 MHz for US

// Gateway
#define NODE_TIMEOUT_SECONDS 300  // Remove nodes offline for 5 minutes
```

## Message Topics

### State Publishing (Gateway → MQTT)
```
lora_gateway/node_{nodeId}/device_{deviceId}/state
```

Payload example:
```json
{
  "value": 23.5,
  "device": "Temperature Sensor",
  "timestamp": 1234567890
}
```

### Command Subscription (MQTT → Gateway → LoRa)
```
lora_gateway/node_{nodeId}/device_{deviceId}/command
```

Payload examples:
```json
{"command": "ON"}
{"command": "OFF"}
{"command": "OPEN"}
{"command": "CLOSE"}
{"command": "STOP"}
{"value": 128}
```

### Home Assistant Discovery
```
homeassistant/{component_type}/lora_{nodeId}_{deviceId}/config
```

Discovery payloads include device metadata, state topics, command topics, and integration info for Home Assistant.

## LoRa Message Format

Binary format with CRC16 validation:
```
[SYNC] [NodeID-H] [NodeID-L] [DeviceID] [Type] [MsgType] [ValueType] 
[Value...] [Command] [CmdData0-3] [CRC-H] [CRC-L]
```

- **SYNC**: 0xAA
- **NodeID**: 16-bit node identifier
- **DeviceID**: 8-bit device ID within node
- **Type**: Device type (0=BinarySensor, 1=Sensor, 2=Switch, 3=Cover)
- **MsgType**: Message type (0=announcement, 1=sensor update, 3=command)
- **ValueType**: Value encoding (0=bool, 1=int32, 2=float, 3=string)
- **Value**: Variable length based on ValueType
- **CRC16**: CCITT polynomial validation

## Home Assistant Integration

Once a node is discovered, the gateway automatically publishes discovery messages. In Home Assistant, devices appear under the "LoRa Gateway" organization with the ability to:

- Read sensor values in real-time
- Control switches and covers via commands
- Create automations based on sensor state changes
- Monitor node availability

### Discovery Topics Structure

**Binary Sensors** → `homeassistant/binary_sensor/lora_*_{nodeId}_{deviceId}/config`
**Sensors** → `homeassistant/sensor/lora_{nodeId}_{deviceId}/config`
**Switches** → `homeassistant/switch/lora_{nodeId}_{deviceId}/config`
**Covers** → `homeassistant/cover/lora_{nodeId}_{deviceId}/config`

## Libraries

- **LoRa** by Sandeep Mistry v0.8.0 - LoRa radio communication
- **CRC** by Rob Tillaart v1.0.3 - CRC16 message validation
- **Crypto** by Rhys Weatherley v0.4.0 - Encryption support (future)
- **PubSubClient** by Nick O'Leary v2.8 - MQTT client
- **ArduinoJson** by Benoit Blanchon v7.4.2 - JSON payload handling

## Building and Uploading

Using PlatformIO:

```bash
# Build for ESP8266/Huzzah
platformio run -e huzzah

# Upload to device
platformio run -e huzzah --target upload

# Monitor serial output
platformio device monitor
```

## Extending the Gateway

### Supporting New Device Types

1. Add new type to `DeviceType` enum in [include/Types.h](include/Types.h)
2. Update `publishDiscovery()` in [src/MqttHandler.cpp](src/MqttHandler.cpp)
3. Update command parsing in `handleMqttMessage()` in [src/main.cpp](src/main.cpp)

### Custom Message Payloads

Modify `encodeMessage()` and `decodeMessage()` in [src/LoRaHandler.cpp](src/LoRaHandler.cpp) to support custom data formats.

### Node Authentication (Future)

The Crypto library is included for potential AES-256 encryption support. Implement encryption in the message encoding/decoding functions.

## Troubleshooting

### No LoRa Messages Received
- Verify pin connections match Config.h
- Check LoRa frequency matches nodes (868 MHz vs 915 MHz)
- Ensure RFM95 module has proper power supply
- Check antenna connection

### MQTT Not Connecting
- Verify MQTT broker IP and port in Config.h
- Check WiFi connection (Serial monitor should show IP)
- Verify WiFi SSID and password
- Check MQTT broker allows anonymous connections (if no credentials set)

### Nodes Not Discovered
- Ensure node sends first message with announcement (messageType = 0)
- Check LoRa messages are received (Serial monitor)
- Verify node ID and device ID are not out of range (uint16_t and uint8_t)
