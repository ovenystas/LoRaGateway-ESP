# LoRaGateway Project Structure

Complete implementation of a bidirectional LoRa ↔ MQTT/Home Assistant gateway.

## Project Overview

```
LoRaGateway-ESP/
├── include/
│   ├── Config.h                    # Configuration settings (WiFi, MQTT, LoRa pins)
│   ├── Types.h                     # Data structures (LoRaMessage, DeviceType, etc.)
│   ├── LoRaHandler.h              # LoRa radio communication interface
│   ├── MqttHandler.h              # MQTT client and Home Assistant discovery
│   └── NodeRegistry.h             # Runtime node and device registry
├── src/
│   ├── main.cpp                   # Gateway logic, message routing, WiFi/MQTT management
│   ├── LoRaHandler.cpp            # LoRa encoding/decoding with CRC16
│   ├── MqttHandler.cpp            # MQTT operations and HA discovery publishing
│   └── NodeRegistry.cpp           # Node/device registration and tracking
├── lib/
│   └── README                     # External libraries (added by PlatformIO)
├── test/
│   └── README                     # Test directory (can add unit tests here)
├── platformio.ini                 # PlatformIO build configuration
├── README.md                      # Main gateway documentation
├── NODE_IMPLEMENTATION.md         # Guide for creating LoRa sensor nodes
└── MQTT_HA_CONFIG.md             # Home Assistant configuration examples
```

## File Descriptions

### Header Files (`include/`)

#### `Config.h`
User-configurable settings:
- WiFi SSID and password
- MQTT broker IP, port, client ID
- LoRa pin assignments
- LoRa frequency (868 MHz EU / 915 MHz US)
- Node timeout interval

#### `Types.h`
Core data structures:
- `LoRaMessage` - Binary message format with CRC validation
- `DeviceType` enum (BinarySensor, Sensor, Switch, Cover)
- `CommandType` enum (SetState, SetPosition, Open, Close, Stop)
- `ValueType` enum (Boolean, Int32, Float, String)
- `DeviceInfo` - Metadata for individual devices
- `NodeInfo` - Metadata for sensor nodes

#### `LoRaHandler.h`
Radio communication class:
- `begin(frequency)` - Initialize RFM95 module
- `sendMessage(msg)` - Send message to LoRa network
- `readMessage(msg)` - Receive and parse message
- `encodeMessage()` / `decodeMessage()` - Binary encoding with CRC16
- Message callbacks for asynchronous reception

#### `MqttHandler.h`
MQTT communication class:
- `connect(broker, port, clientId)` - Connect to MQTT broker
- `publishSensorValue()` - Publish sensor readings as JSON
- `publishDiscovery()` - Publish Home Assistant discovery messages
- `subscribeToCommands()` - Subscribe to device command topics
- Message callbacks for command reception

#### `NodeRegistry.h`
In-memory node tracking:
- `registerNode()` - Add newly discovered node
- `registerDevice()` - Associate device with node
- `getNode()` / `getDevice()` - Query registry
- `updateNodeLastSeen()` - Heartbeat tracking for timeout detection
- `getAllNodes()` - Retrieve all registered nodes and devices

### Implementation Files (`src/`)

#### `main.cpp`
Gateway orchestration:
- **WiFi Management** - Connect/reconnect to WiFi network
- **MQTT Management** - Connect to broker, resubscribe on reconnect
- **LoRa Message Handler** - Process incoming wireless messages
- **MQTT Message Handler** - Parse and forward commands to nodes
- **Node Discovery** - Register new nodes, publish HA discovery
- **Command Routing** - Forward MQTT commands to LoRa nodes
- **Timeout Detection** - Remove nodes that haven't been seen

#### `LoRaHandler.cpp`
Radio implementation:
- RFM95 module initialization and configuration
- Message serialization to binary format
- CRC16 CCITT calculation for error detection
- Callback-based message reception
- Frequency and spreading factor configuration

#### `MqttHandler.cpp`
MQTT implementation:
- PubSubClient wrapper with WiFi client
- JSON payload generation for sensor values
- Home Assistant discovery message generation
- Topic building and subscription management
- Automatic message acknowledgment

#### `NodeRegistry.cpp`
Registry implementation:
- Dynamic array-based storage (max 50 nodes, 16 devices/node)
- Node lookup by ID
- Device lookup by node and device ID
- Timestamp tracking for availability monitoring
- Memory-efficient structure layout

## Data Flow

### 1. Node Discovery Flow

```
LoRa Node sends announcement message
           ↓
    LoRaHandler receives
           ↓
    main.cpp handleLoRaMessage()
           ↓
    NodeRegistry registers new node
           ↓
    MqttHandler.publishDiscovery()
           ↓
    HA discovers device automatically
```

### 2. Sensor Update Flow

```
LoRa Node reads sensor → sends update
           ↓
    LoRaHandler.encodeMessage() + CRC
           ↓
    RFM95 radio transmission
           ↓
    Gateway RFM95 receives
           ↓
    LoRaHandler.decodeMessage() + CRC verify
           ↓
    handleLoRaMessage() callback
           ↓
    MqttHandler.publishSensorValue()
           ↓
    MQTT broker receives JSON
           ↓
    Home Assistant updates entity state
```

### 3. Command Flow

```
Home Assistant sends command (via frontend or automation)
           ↓
    MQTT broker publishes to node command topic
           ↓
    Gateway receives (handleMqttMessage callback)
           ↓
    Parse command and extract parameters
           ↓
    NodeRegistry lookup device metadata
           ↓
    Build LoRaMessage with command
           ↓
    LoRaHandler.sendMessage()
           ↓
    RFM95 transmits to node
           ↓
    Node receives and executes command
           ↓
    Node sends acknowledgment/status
           ↓
    Gateway publishes status to MQTT
```

## MQTT Topic Structure

### State Topics (Gateway → MQTT)
```
lora_gateway/node_{nodeId}/device_{deviceId}/state
```

Example payloads:
```json
{"value": 23.5, "device": "Temperature", "timestamp": 1234567890}
{"value": true, "state": "ON", "device": "Motion Sensor"}
{"value": 128, "device": "Door Position"}
```

### Command Topics (MQTT → Gateway)
```
lora_gateway/node_{nodeId}/device_{deviceId}/command
```

Example payloads:
```json
{"command": "ON"}
{"command": "OFF"}
{"command": "OPEN"}
{"command": "CLOSE"}
{"command": "STOP"}
{"value": 128}
```

### Home Assistant Discovery Topics
```
homeassistant/{component_type}/lora_{nodeId}_{deviceId}/config
```

Component types: `sensor`, `binary_sensor`, `switch`, `cover`

## Build and Upload

### Prerequisites
- PlatformIO IDE or CLI installed
- ESP32 board definition
- USB connection to development board

### Steps

1. **Configure Settings**
   - Edit [include/Config.h](../include/Config.h)
   - Set WiFi SSID and password
   - Set MQTT broker IP and port
   - Set LoRa pins matching your hardware

2. **Build Project**
   ```bash
   cd LoRaGateway-ESP
   platformio run -e nodemcu-32s
   ```

3. **Upload to Device**
   ```bash
   platformio run -e nodemcu-32s --target upload
   ```

4. **Monitor Serial Output**
   ```bash
   platformio device monitor
   ```

## Extending the Gateway

### Adding New Device Types

1. Add to `DeviceType` enum in [include/Types.h](../include/Types.h)
2. Update `publishDiscovery()` in [src/MqttHandler.cpp](../src/MqttHandler.cpp) to handle new type
3. Update `handleMqttMessage()` in [src/main.cpp](../src/main.cpp) for command parsing

### Implementing Home Assistant Entities

Each device type maps to HA component:
- `BinarySensor` → Home Assistant binary_sensor
- `Sensor` → Home Assistant sensor
- `Switch` → Home Assistant switch (on/off control)
- `Cover` → Home Assistant cover (position control)

### Custom Message Payloads

Modify `encodeMessage()` and `decodeMessage()` in [src/LoRaHandler.cpp](../src/LoRaHandler.cpp) to support custom data formats or compression.

## Limitations and Future Enhancements

### Current Limitations
- Max 50 concurrent nodes
- Max 16 devices per node
- No message encryption (but infrastructure present via Crypto library)
- No node authentication
- Single gateway (no mesh networking)

### Possible Enhancements
- Message encryption using AES-256
- Node authentication/pairing
- Battery voltage monitoring
- Signal strength (RSSI) reporting
- Mesh networking support
- Over-the-air firmware updates
- Data persistence to EEPROM
- Multiple gateway support with message repeating

## Performance Considerations

### Memory Usage
- Node registry: ~2KB per node
- Message buffers: ~200 bytes
- Stack: ~20KB remaining available
- Heap: Managed by Arduino runtime

### Power Consumption
- WiFi connected: ~70-80mA
- LoRa RX: ~12-15mA
- LoRa TX: ~80-100mA (brief)
- Sleep mode: <1mA (if implemented)

### Latency
- LoRa message round-trip: 100-500ms depending on spreading factor
- MQTT publish-to-HA: <100ms
- Total command latency: 200-600ms typical

## Debugging

### Enable Serial Logging
Serial output at 115200 baud shows:
- Gateway startup progress
- WiFi connection status
- MQTT connection events
- LoRa message reception
- Node discovery
- Command execution
- Timeout events

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| No LoRa messages | Pin configuration | Check Config.h pins |
| MQTT not connecting | WiFi down | Check SSID/password |
| Nodes not discovered | Wrong frequency | Set correct 868/915 MHz |
| Commands not working | Topic mismatch | Verify MQTT topic format |
| High latency | Poor RF environment | Move gateway/antenna |

## License and References

- LoRa library: https://github.com/sandeepmistry/arduino-LoRa
- PubSubClient: https://github.com/knolleary/pubsubclient
- ArduinoJson: https://arduinojson.org/
- RFM95 datasheet: https://cdn.sparkfun.com/assets/learn_tutorials/8/0/RFM95_datasheet.pdf

## Support Resources

- [Main README](README.md) - Gateway documentation
- [Node Implementation Guide](NODE_IMPLEMENTATION.md) - How to build sensor nodes
- [Home Assistant Configuration](MQTT_HA_CONFIG.md) - HA integration examples
