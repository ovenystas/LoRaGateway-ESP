# LoRaGateway-ESP - Complete Documentation Index

Welcome to the LoRa Gateway project! This document provides an overview of all documentation and components.

## 📚 Documentation Files

### Getting Started
- **[QUICKSTART.md](QUICKSTART.md)** ⭐ **START HERE**
  - 5-minute setup guide
  - Hardware wiring diagram
  - Configuration steps
  - Troubleshooting guide
  - Your first sensor node example

### Architecture & Design
- **[ARCHITECTURE.md](ARCHITECTURE.md)**
  - System overview diagram
  - Data flow visualizations
  - Message format specifications
  - Component interaction diagrams
  - Timing and state machine diagrams

### Implementation Guides
- **[NODE_IMPLEMENTATION.md](NODE_IMPLEMENTATION.md)**
  - How to create LoRa sensor nodes
  - Complete example implementations
  - Temperature sensor example
  - Motion sensor example
  - Light switch example
  - Garage door cover example
  - Best practices for node development
  - Message type specifications

- **[README.md](README.md)**
  - Full gateway documentation
  - Feature overview
  - Hardware and software setup
  - Configuration reference
  - MQTT topic structure
  - Message protocol details
  - Home Assistant integration overview
  - Troubleshooting

### Home Assistant Integration
- **[MQTT_HA_CONFIG.md](MQTT_HA_CONFIG.md)**
  - Auto-discovery mechanism
  - Example discovery payloads
  - Manual MQTT configuration examples
  - Automation recipes
  - Dashboard card examples
  - MQTT testing commands
  - Home Assistant templates

### Project Structure
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)**
  - File organization overview
  - Component descriptions
  - Data flow explanations
  - Build and upload instructions
  - Extension points
  - Performance considerations
  - Debugging guide

## 🗂️ Source Code Organization

### Headers (`include/`)

```cpp
Config.h
├─ WiFi credentials and MQTT settings
├─ LoRa pin configuration
├─ Frequency selection (868/915 MHz)
└─ Node timeout settings

Types.h
├─ LoRaMessage structure
├─ Device types (BinarySensor, Sensor, Switch, Cover)
├─ Command types (SetState, SetPosition, Open, Close, Stop)
├─ Value types (Boolean, Int32, Float, String)
├─ DeviceInfo metadata
└─ NodeInfo metadata

LoRaHandler.h
├─ RFM95 module interface
├─ Message encoding/decoding
├─ CRC16 validation
└─ Callback registration

MqttHandler.h
├─ MQTT client wrapper
├─ Sensor value publishing
├─ Home Assistant discovery
├─ Command subscription
└─ Message callbacks

NodeRegistry.h
├─ Runtime node tracking
├─ Device registration
├─ Query by ID
├─ Timeout detection
└─ Device enumeration
```

### Implementation (`src/`)

```cpp
main.cpp (Primary Gateway Logic)
├─ WiFi connection management
├─ MQTT broker communication
├─ LoRa message routing
├─ Node discovery handler
├─ Command dispatcher
└─ Event loop orchestration

LoRaHandler.cpp (Radio Communication)
├─ RFM95 initialization
├─ Message serialization
├─ CRC16 calculation
├─ Frequency configuration
└─ Spreading factor setup

MqttHandler.cpp (MQTT Operations)
├─ PubSubClient wrapper
├─ JSON payload generation
├─ Home Assistant discovery generation
├─ Topic management
└─ Subscription handling

NodeRegistry.cpp (Device Registry)
├─ In-memory storage
├─ Node registration/removal
├─ Device metadata management
├─ Last-seen tracking
└─ Timeout management
```

## 🚀 Quick Navigation by Use Case

### "I want to set up the gateway"
1. Read: [QUICKSTART.md](QUICKSTART.md)
2. Configure: [include/Config.h](include/Config.h)
3. Build & Upload: Follow QUICKSTART steps
4. Verify: Check serial monitor

### "I want to create a sensor node"
1. Read: [NODE_IMPLEMENTATION.md](NODE_IMPLEMENTATION.md)
2. Choose example: Temperature, Motion, Switch, or Cover
3. Copy and modify example code
4. Test with gateway
5. Integrate with Home Assistant

### "I want to integrate with Home Assistant"
1. Read: [MQTT_HA_CONFIG.md](MQTT_HA_CONFIG.md)
2. Enable MQTT integration in HA
3. Devices auto-discover when nodes are added
4. Set up automations and dashboards

### "I want to understand the architecture"
1. Read: [ARCHITECTURE.md](ARCHITECTURE.md)
2. Review: [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)
3. Study diagrams in ARCHITECTURE.md
4. Trace data flows through [src/main.cpp](src/main.cpp)

### "I want to extend functionality"
1. Read: [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) section "Extending the Gateway"
2. Review relevant source files
3. See [Types.h](include/Types.h) for data structures
4. Check examples in [NODE_IMPLEMENTATION.md](NODE_IMPLEMENTATION.md)

## 📦 Component Overview

### Core Components

| Component | Purpose | Location |
|-----------|---------|----------|
| **LoRaHandler** | Wireless communication with RFM95 module | [LoRaHandler.h](include/LoRaHandler.h) / [.cpp](src/LoRaHandler.cpp) |
| **MqttHandler** | WiFi-based MQTT communication | [MqttHandler.h](include/MqttHandler.h) / [.cpp](src/MqttHandler.cpp) |
| **NodeRegistry** | Runtime registry of discovered nodes | [NodeRegistry.h](include/NodeRegistry.h) / [.cpp](src/NodeRegistry.cpp) |
| **Types** | Message and data structures | [Types.h](include/Types.h) |
| **Config** | User configuration settings | [Config.h](include/Config.h) |
| **main** | Gateway logic and event loop | [main.cpp](src/main.cpp) |

### Dependencies

| Library | Version | Purpose | Provider |
|---------|---------|---------|----------|
| LoRa | 0.8.0 | RFM95 module communication | Sandeep Mistry |
| PubSubClient | 2.8 | MQTT client | Nick O'Leary |
| ArduinoJson | 7.4.2 | JSON payload handling | Benoit Blanchon |
| CRC | 1.0.3 | CRC16 message validation | Rob Tillaart |
| Crypto | 0.4.0 | Encryption support (future) | Rhys Weatherley |
| ESP8266 Arduino Core | 3.1.x | ESP8266 SDK | ESP8266 Community |

## 🔧 Configuration Checklist

Before deploying, configure these settings in [include/Config.h](include/Config.h):

- [ ] WiFi SSID
- [ ] WiFi Password
- [ ] MQTT Broker IP Address
- [ ] MQTT Port (1883 default)
- [ ] MQTT Client ID
- [ ] LoRa Frequency (868000000 EU, 915000000 US)
- [ ] LoRa CS Pin
- [ ] LoRa RST Pin
- [ ] LoRa DIO0 Pin

## 📊 Message Flow Summary

```
Node sends announcement
    ↓
Gateway receives & registers
    ↓
HA discovery published
    ↓
HA creates entities
    ↓
Node sends sensor value
    ↓
Gateway publishes to MQTT
    ↓
HA updates entity state
    ↓
User creates automation/trigger
    ↓
HA sends command via MQTT
    ↓
Gateway routes to node
    ↓
Node executes command
```

## 🎯 Key Features

✅ **Runtime Node Discovery** - Nodes automatically discovered on first message
✅ **Home Assistant Auto-Discovery** - Devices appear in HA without manual config
✅ **Bidirectional Communication** - Sensor values and commands flow both ways
✅ **Multiple Device Types** - Binary Sensors, Sensors, Switches, and Covers
✅ **Message Validation** - CRC16 checksums on all LoRa messages
✅ **Node Timeout Management** - Automatic removal of offline nodes
✅ **Flexible Message Format** - Support for boolean, integer, float, and string values
✅ **Command Execution** - Real-time command routing from MQTT to LoRa nodes

## 🔍 Debugging Tips

1. **Monitor Serial Output** - 115200 baud shows detailed logs
2. **Check WiFi Status** - Verify IP address after connection
3. **Monitor MQTT Topics** - Use `mosquitto_sub -h <broker> -t "lora_gateway/#"`
4. **Verify Node Messages** - Serial output shows incoming LoRa messages
5. **Check HA Discovery** - Subscribe to `homeassistant/+/lora_+/config`
6. **Test Commands** - Use `mosquitto_pub` to test command publishing

## 📈 Performance Characteristics

- **Node Capacity**: 50 concurrent nodes
- **Devices per Node**: 16 devices
- **Message Latency**: 100-500ms (LoRa) + <100ms (MQTT)
- **Loop Time**: ~15ms per iteration
- **Memory Usage**: ~2KB per node in registry
- **LoRa Range**: 100-300m line-of-sight
- **Update Frequency**: 1-60 seconds recommended per sensor

## 🐛 Common Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| No LoRa messages | Wrong pins or frequency | Check Config.h and wiring |
| MQTT not connecting | Broker unreachable or WiFi down | Verify WiFi and MQTT settings |
| Nodes don't appear in HA | Discovery not received | Check MQTT topics in broker |
| Commands not working | Command topic format wrong | Verify topic in MQTT client |
| High latency | Poor RF environment | Move gateway or antenna |

## 📖 File Cross-References

### Understanding Message Flow
1. Start: [ARCHITECTURE.md - Data Flow Diagrams](ARCHITECTURE.md)
2. Implementation: [src/main.cpp](src/main.cpp) lines 200-300 (message handlers)
3. Message Format: [include/Types.h](include/Types.h)
4. Encoding: [src/LoRaHandler.cpp](src/LoRaHandler.cpp) (encodeMessage/decodeMessage)

### Understanding Node Discovery
1. Trigger: Node sends messageType=0 (announcement)
2. Gateway Handler: [src/main.cpp](src/main.cpp) - handleLoRaMessage()
3. Registration: [src/NodeRegistry.cpp](src/NodeRegistry.cpp) - registerNode()
4. Discovery: [src/MqttHandler.cpp](src/MqttHandler.cpp) - publishDiscovery()

### Understanding Command Execution
1. User Action: Home Assistant UI toggle/click
2. MQTT Publish: Home Assistant publishes to command topic
3. Gateway Receive: [src/main.cpp](src/main.cpp) - handleMqttMessage()
4. Parsing: Extract command from JSON and topic
5. Sending: [src/LoRaHandler.cpp](src/LoRaHandler.cpp) - sendMessage()
6. Node Execution: Node implementation responds

## 🎓 Learning Path

**Beginner**
1. QUICKSTART.md - Get the gateway running
2. NODE_IMPLEMENTATION.md - Build a simple temperature sensor
3. MQTT_HA_CONFIG.md - Integrate with Home Assistant

**Intermediate**
1. ARCHITECTURE.md - Understand system design
2. PROJECT_STRUCTURE.md - Learn code organization
3. Modify node implementations for your sensors

**Advanced**
1. Implement encryption in message encoding/decoding
2. Add mesh networking capabilities
3. Implement battery monitoring with deep sleep
4. Create firmware update mechanism

## 📞 Support Resources

- **Serial Monitor** - Detailed logging at 115200 baud
- **MQTT Client Tools** - mosquitto_sub/pub for debugging
- **Home Assistant Logs** - Check for discovery and MQTT errors
- **Documentation** - All files in this repository

## 🚀 Next Steps

1. ✅ Read [QUICKSTART.md](QUICKSTART.md)
2. ✅ Configure [Config.h](include/Config.h)
3. ✅ Build and upload to ESP8266
4. ✅ Monitor serial output to verify operation
5. ✅ Create your first sensor node
6. ✅ Watch gateway discover and register it
7. ✅ Integrate with Home Assistant
8. ✅ Create automations and scenarios

---

**Welcome to your LoRa smart home gateway!** 🎉

For specific topics, use the navigation links above or search within individual documentation files.
