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
   - `LoRaHeader`: Message header with destination, source, ID, and flags
   - `LoRaMsgType`: 10 message types (ping, discovery, value, config, service)
   - Entity types: BinarySensor, Sensor, Switch, Cover
   - Device classes for each entity type
   - Payload structures: ValueItem, DiscoveryItem, ConfigItem, ServiceItem

2. **LoRaHandler** - LoRa radio communication
   - Header-based protocol with ACK request/response capability
   - Message encoding/decoding with CRC16 checksums
   - Variable-length payload support
   - Sends and receives messages from devices

3. **MqttHandler** - MQTT broker communication
   - Connects to MQTT server (with optional authentication)
   - Publishes sensor values and discovery messages
   - Subscribes to command topics

4. **DeviceRegistry** - Tracks discovered devices and entities
   - Maintains list of registered devices
   - Stores entity metadata (type, name, unit, device class, etc.)
   - Updates last-seen timestamps for timeout detection

5. **main.cpp** - Gateway logic and message routing
   - WiFi/MQTT connection management
   - Routes messages between LoRa and MQTT
   - Handles device discovery messages
   - Processes sensor value updates
   - Executes commands from MQTT

## Configuration

All settings are in `include/secrets.h`:

```cpp
// WiFi credentials
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourPassword"

// MQTT broker
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora-gw"
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
Topic: `lora-gw/device_{deviceId}/entity_{entityId}/state`
```json
{
  "value": 23.5,
  "entity": "Temperature Sensor",
  "timestamp": 1234567890
}
```

### Commands (MQTT → Gateway → LoRa)
Topic: `lora-gw/device_{deviceId}/entity_{entityId}/command`
```json
{"command": "ON"}     // BinarySensor/Switch
{"command": "OFF"}    // BinarySensor/Switch
{"command": "OPEN"}   // Cover
{"command": "CLOSE"}  // Cover
{"command": "STOP"}   // Cover
{"value": 128}        // Any numeric device
```

## LoRa Protocol

The gateway uses a structured header-based protocol inspired by professional IoT systems, with support for multiple message types and reliable communication.

### Message Structure

```
[Header (4 bytes)] [Payload (0-92 bytes)] [CRC16 (2 bytes)]
```

**Header Format:**
```
Byte 0: Destination Address
Byte 1: Source Address
Byte 2: Message ID
Byte 3: Flags
  - Bit 7: ACK Response
  - Bit 6: ACK Request
  - Bits 3-0: Message Type (0-9)
```

### Message Types

| Type | Name            | Direction        | Purpose                       |
| ---- | --------------- | ---------------- | ----------------------------- |
| 0    | `ping_req`      | Device → Gateway | Request RSSI measurement      |
| 1    | `ping_msg`      | Gateway → Device | RSSI response                 |
| 2    | `discovery_req` | Gateway → Device | Request entity discovery      |
| 3    | `discovery_msg` | Device → Gateway | Announce entity with metadata |
| 4    | `value_req`     | Gateway → Device | Request sensor values         |
| 5    | `value_msg`     | Device → Gateway | Send sensor values            |
| 6    | `config_req`    | Gateway → Device | Request configuration         |
| 7    | `config_msg`    | Device → Gateway | Send configuration            |
| 8    | `configSet_req` | Gateway → Device | Set configuration value       |
| 9    | `service_req`   | Gateway → Device | Invoke service                |

### Payload Structures

**ValueItem** (5 bytes) - Sensor value
```
Byte 0: Entity ID
Bytes 1-4: Value (int32, big-endian)
```

**DiscoveryItem** (5 bytes) - Entity announcement
```
Byte 0: Entity ID
Byte 1: Entity Type (0=BinarySensor, 1=Sensor, 2=Switch, 3=Cover)
Byte 2: Device Class (type-specific)
Byte 3: Unit (measurement unit)
Byte 4: Format (size, signedness, precision)
```

**ConfigItem** (11 bytes) - Configuration metadata
```
Byte 0: Config ID
Byte 1: Unit
Byte 2: Format
Bytes 3-6: Min Value (int32, big-endian)
Bytes 7-10: Max Value (int32, big-endian)
```

**ServiceItem** (2 bytes) - Service request
```
Byte 0: Entity ID
Byte 1: Service Command
```

### CRC16 Validation

- **Polynomial**: 0x1021 (CCITT)
- **Init**: 0xFFFF
- **Reflected Input/Output**: Yes
- **Covers**: All bytes except the CRC itself

## Message Format (Legacy Reference)

Binary format with CRC16 validation:
```
[Header (4)] [Payload (0-92)] [CRC16 (2)]
```

- **Header**: Destination, Source, Message ID, Flags
- **Message Type**: Determines payload interpretation (ping, discovery, value, config, service)
- **Payload**: Variable length based on message type
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

### Adding a New Entity Type

1. Add type to `EntityDomain` enum in [include/Types.h](include/Types.h)
2. Add device class enum if needed (e.g., `NewDeviceClass`)
3. Update discovery parsing in [src/main.cpp](src/main.cpp) `handleLoRaMessage()`
4. Update MQTT discovery topic in [src/MqttHandler.cpp](src/MqttHandler.cpp)

### Adding a New Message Type

1. Add type to `LoRaMsgType` enum in [include/Types.h](include/Types.h)
2. Create new payload struct (e.g., `CustomItem`) with `toByteArray()` and `fromByteArray()` methods
3. Add parsing logic in [src/main.cpp](src/main.cpp) `handleLoRaMessage()`
4. Update `LoRaHeader` constructor calls to use new message type in [src/LoRaMsgHandler.cpp](src/LoRaMsgHandler.cpp)

### Custom LoRa Message Payloads

Modify encoding/decoding in [src/LoRaHandler.cpp](src/LoRaHandler.cpp):
- `encodeMessage()` - Serialize header + payload + CRC16
- `decodeMessage()` - Parse header, validate CRC16, extract payload

### Adding MQTT Authentication

Already supported! Use the overloaded `connect()` function:
```cpp
mqtt.connect(broker, port, clientId, username, password);
```

### Understanding Device Classes

Device classes provide semantic information to Home Assistant:
- **BinarySensorDeviceClass**: battery, motion, door, window, presence, etc.
- **SensorDeviceClass**: humidity, distance, temperature
- **CoverDeviceClass**: garage, blind, curtain
