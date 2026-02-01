# LoRa Gateway Architecture

## System Overview Diagram

```
                          ┌─────────────────┐
                          │  Home Assistant │
                          └────────┬────────┘
                                   │
                          MQTT Discovery &
                         State Updates (JSON)
                                   │
                    ┌──────────────▼──────────────┐
                    │    MQTT Broker              │
                    │ (Mosquitto/HA Supervisor)   │
                    └──────────────┬──────────────┘
                                   │
            ┌──────────────────────┼─────────────────────────┐
            │      WiFi            │         MQTT            │
            │   (TCP/IP)           │      (TCP/IP)           │
            │                      │                         │
    ┌───────▼──────────────────────▼─────────┐               │
    │   ESP32 + RFM95                      │               │
    │   LoRa Gateway                         │               │
    │  ┌──────────────────────────────────┐  │               │
    │  │ main.cpp                         │  │               │
    │  │ - WiFi Manager                   │  │               │
    │  │ - MQTT Client Handler            │  │               │
    │  │ - LoRa Message Router            │  │               │
    │  │ - Node Discovery Engine          │  │               │
    │  │ - Command Dispatcher             │  │               │
    │  └───────┬─────────────────┬────────┘  │               │
    │          │                 │           │               │
    │          │                 │           │               │
    │  ┌───────▼──────┐   ┌──────▼──────┐    │               │
    │  │ LoRaHandler  │   │MqttHandler  │    │               │
    │  ├──────────────┤   ├─────────────┤    │               │
    │  │ RFM95        │   │PubSub       │    │               │
    │  │ Encode/Dec.  │   │Discovery    │    │               │
    │  │ CRC16        │   │Json Payload │    │               │
    │  └───────┬──────┘   └──────┬──────┘    │               │
    │          │                 │           │               │
    │  ┌───────▼─────────────────▼─────────┐ │               │
    │  │  NodeRegistry                     │ │               │
    │  │  - Node List (50 max)             │ │               │
    │  │  - Device Map (16 per node)       │ │               │
    │  │  - Last Seen Tracking             │ │               │
    │  │  - Timeout Detection              │ │               │
    │  └───────────────────────────────────┘ │               │
    │                                        │               │
    └────────────────────┬───────────────────┘               │
                         │                                   │
         ┌───────────────┼───────────────┐                   │
         │      LoRa (868/915 MHz)       │                   │
         │   Wireless (100-300m range)   │                   │
         │                               │                   │
    ┌────▼──────┐  ┌───────────┐  ┌──────▼────┐  ┌────────┐  │
    │ Node 1001 │  │ Node 1002 │  │ Node 1003 │  │Node... │  │
    │ (Sensor)  │  │(Binary)   │  │(Switch)   │  │        │  │
    └───────────┘  └───────────┘  └───────────┘  └────────┘  │
                                                             │
                          ┌──────────────────────────────────┘
                          │
    ┌─────────────────────▼──────────────────────┐
    │ Home Assistant Entities                    │
    │  - sensor.lora_1001_0 (temperature)        │
    │  - binary_sensor.lora_1002_0 (motion)      │
    │  - switch.lora_1003_0 (light control)      │
    │  - cover.lora_1004_0 (garage door)         │
    └────────────────────────────────────────────┘
```

## Data Flow Diagrams

### 1. Node Discovery & Announcement

```
LoRa Node Powers On
         │
         ├─ Read Sensors
         │
         ├─ Build LoRaMessage (messageType=0, announcement)
         │
         ├─ Encode message + CRC16
         │
    ┌────▼────────────────────────────┐
    │  Send via RFM95 Radio           │
    │  (868/915 MHz frequency)        │
    └────┬────────────────────────────┘
         │ LoRa Wireless
         │
    ┌────▼────────────────────────────┐
    │  Gateway RFM95 Receives         │
    │  (parsePacket, read bytes)      │
    └────┬────────────────────────────┘
         │
         ├─ Decode message
         │
         ├─ Verify CRC16 checksum
         │
         ├─ handleLoRaMessage() callback
         │
    ┌────▼────────────────────────────┐
    │  New Node? Check NodeRegistry   │
    │  No → onNodeDiscovered()        │
    └────┬────────────────────────────┘
         │
         ├─ Register node in NodeRegistry
         │
         ├─ Register device metadata
         │
         ├─ Build Home Assistant discovery
         │
    ┌────▼────────────────────────────┐
    │  MqttHandler.publishDiscovery() │
    │  HA will auto-create entity     │
    └─────────────────────────────────┘
```

### 2. Sensor Value Publishing

```
Node Reads Temperature Sensor
         │
         ├─ Convert to float (23.5°C)
         │
         ├─ Build LoRaMessage
         │  - messageType=1 (sensor update)
         │  - valueType=FLOAT_VALUE
         │  - value.floatValue=23.5
         │
    ┌────▼────────────────────────────┐
    │  Send via RFM95                 │
    │  Packet: [SYNC][ID][TYPE][VAL]  │
    │         [CRC_H][CRC_L]          │
    └────┬────────────────────────────┘
         │ LoRa Wireless (100-300m)
         │
    ┌────▼────────────────────────────┐
    │  Gateway RFM95 Receives         │
    │  loRa.handle() in main loop     │
    └────┬────────────────────────────┘
         │
         ├─ Decode + CRC validate
         │
         ├─ handleLoRaMessage() trigger
         │
         ├─ Check NodeRegistry
         │
         ├─ Match to device metadata
         │
    ┌────▼────────────────────────────┐
    │  Build MQTT JSON Payload:       │
    │  {"value": 23.5,                │
    │   "device": "Temperature",      │
    │   "timestamp": 1234567890}      │
    └────┬────────────────────────────┘
         │
         ├─ Topic: lora_gateway/node_1001/
         │         device_0/state
         │
    ┌────▼────────────────────────────┐
    │  MQTT Publish (WiFi)            │
    │  PubSubClient.publish()         │
    └────┬────────────────────────────┘
         │ WiFi/TCP IP to broker
         │
    ┌────▼────────────────────────────┐
    │  MQTT Broker                    │
    │  Stores message                 │
    └────┬────────────────────────────┘
         │
         ├─ HA MQTT integration receives
         │
    ┌────▼────────────────────────────┐
    │  sensor.lora_1001_0             │
    │  state = 23.5 °C                │
    │  last_updated = now             │
    └─────────────────────────────────┘
```

### 3. Command Execution

```
Home Assistant User Action (Toggle Light)
         │
    ┌────▼─────────────────────────┐
    │  HA Switch Entity triggers   │
    │  switch.lora_1003_0.toggle() │
    └────┬─────────────────────────┘
         │
         ├─ Create MQTT message
         │
    ┌────▼──────────────────────────────────┐
    │  Publish to:                          │
    │  lora_gateway/node_1003/device_0/     │
    │  command                              │
    │                                       │
    │  Payload:                             │
    │  {"command": "ON"}                    │
    └────┬──────────────────────────────────┘
         │ WiFi/MQTT to broker
         │
    ┌────▼──────────────────────────────────┐
    │  MQTT Broker                          │
    │  Routes to subscribed client          │
    └────┬──────────────────────────────────┘
         │ WiFi/MQTT delivery
         │
    ┌────▼──────────────────────────────────┐
    │  Gateway receives MQTT message        │
    │  PubSubClient callback activated      │
    │  handleMqttMessage(topic, payload)    │
    └────┬──────────────────────────────────┘
         │
         ├─ Parse topic for node_id, device_id
         │
         ├─ Parse JSON payload
         │
         ├─ Extract command ("ON")
         │
         ├─ NodeRegistry lookup device
         │
    ┌────▼──────────────────────────────────┐
    │  Build LoRaMessage:                   │
    │  - messageType=3 (command)            │
    │  - command=SET_STATE                  │
    │  - commandValue[0]=1 (ON)             │
    └────┬──────────────────────────────────┘
         │
         ├─ Encode message + CRC
         │
         ├─ sendMessage() via RFM95
         │
    ┌────▼──────────────────────────────────┐
    │  LoRa Wireless Transmission           │
    │  (868/915 MHz, 100-500ms latency)     │
    └────┬──────────────────────────────────┘
         │
    ┌────▼──────────────────────────────────┐
    │  LoRa Node Receives                   │
    │  handleLoRaMessage() callback         │
    │  messageType=3 detected               │
    └────┬──────────────────────────────────┘
         │
         ├─ Extract command
         │
         ├─ Validate device ownership
         │
    ┌────▼──────────────────────────────────┐
    │  Execute Command                      │
    │  digitalWrite(RELAY_PIN, HIGH)        │
    │  Light turns ON                       │
    └────┬──────────────────────────────────┘
         │
         ├─ Send acknowledgment (optional)
         │
    ┌────▼──────────────────────────────────┐
    │  Build status update message          │
    │  Send to gateway via LoRa             │
    └────┬──────────────────────────────────┘
         │
         ├─ Gateway receives status
         │
    ┌────▼──────────────────────────────────┐
    │  Publish to state topic:              │
    │  {"value": true, "state": "ON"}       │
    └────┬──────────────────────────────────┘
         │
         ├─ HA updates entity state
         │
    ┌────▼──────────────────────────────────┐
    │  UI shows Light as ON                 │
    │  All systems synchronized             │
    └───────────────────────────────────────┘
```

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      main.cpp                               │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ WiFi Manager        MQTT Manager      LoRa Listener    │ │
│  │ - connect()         - connect()       - handle()       │ │
│  │ - status check      - loop()          - isConnected()  │ │
│  │ - reconnect         - publish()       - setCallback()  │ │
│  └────────────────────────────────────────────────────────┘ │
│           ▲                    ▲                  ▲         │
│           │                    │                  │         │
│    ┌──────▼──────┐    ┌────────▼────────┐  ┌──────▼──────┐  │
│    │ WiFi Stack  │    │ MQTT Handler    │  │LoRa Handler │  │
│    │ (ESP32)   │    │ - PubSubClient  │  │ - RFM95 SPI │  │
│    │             │    │ - Discovery Gen │  │ - Encoding  │  │
│    │             │    │ - JSON Builder  │  │ - CRC16     │  │
│    └─────────────┘    └────────┬────────┘  └──────┬──────┘  │
│                                │                  │         │
│                                │                  │         │
│                        ┌───────▼──────────────────▼──────┐  │
│                        │   NodeRegistry                  │  │
│                        │ (Runtime node tracking)         │  │
│                        │ - Register nodes                │  │
│                        │ - Store device metadata         │  │
│                        │ - Track last-seen times         │  │
│                        │ - Query by ID                   │  │
│                        └─────────────────────────────────┘  │
│                                                             │
│                     ┌─────────────────────────────┐         │
│                     │   Types.h                   │         │
│                     │ (Data structures)           │         │
│                     │ - LoRaMessage               │         │
│                     │ - DeviceType enum           │         │
│                     │ - CommandType enum          │         │
│                     │ - ValueType enum            │         │
│                     │ - Device metadata structs   │         │
│                     └─────────────────────────────┘         │
│                                                             │
│                     ┌─────────────────────────────┐         │
│                     │   Config.h                  │         │
│                     │ (User configuration)        │         │
│                     │ - WiFi SSID/password        │         │
│                     │ - MQTT broker settings      │         │
│                     │ - LoRa frequency            │         │
│                     │ - Pin assignments           │         │
│                     └─────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

## Message Format Structure

```
LoRa Message Binary Format:

Byte Index:   0     1    2     3      4        5        6-X        Y-Z           Last-1  Last
            [SYNC] [NID_H] [NID_L] [DID] [DTYPE] [MTYPE] [VTYPE] [VALUE...] [CMD] [CDATA] [CRC]

SYNC:     0xAA (1 byte sync byte)
NID:      nodeId (2 bytes, 16-bit uint)
DID:      deviceId (1 byte)
DTYPE:    DeviceType (1 byte, 0-3)
MTYPE:    MessageType (1 byte, 0-3)
VTYPE:    ValueType (1 byte, 0-3)
VALUE:    Variable length based on ValueType
          - BOOLEAN: 1 byte (0 or 1)
          - INT: 4 bytes (int32)
          - FLOAT: 4 bytes (IEEE 754)
          - STRING: 1 byte length + data
CMD:      CommandType (1 byte)
CDATA:    Command data (4 bytes)
CRC:      CRC16 CCITT (2 bytes, 0xFFFF preset)

Total minimum: 18 bytes
Total maximum: 64 bytes (buffer limit)
```

## Timing Diagram

```
Main Loop Execution Timing:

Loop Start
    │
    ├─ WiFi Check (every loop, <10ms)
    │  └─ If disconnected, attempt reconnect
    │
    ├─ MQTT Check (every 5 seconds)
    │  └─ If disconnected, attempt connect
    │  └─ If connected, loop() for pending messages
    │
    ├─ Node Timeout Check (every 60 seconds)
    │  └─ Remove nodes offline > 300 seconds
    │
    ├─ LoRa Handler (every loop, <1ms)
    │  └─ parsePacket() - non-blocking check
    │  └─ If available, decode and callback
    │
    ├─ MQTT Handler (every loop, <1ms)
    │  └─ client.loop() - non-blocking receive
    │
    └─ Sleep 10ms (watchdog protection)
       │
       └─ Total loop time: 11-15ms
          (66-90 loops per second)
```

## State Machine Diagram (Node Lifecycle)

```
                    ┌────────────────────┐
                    │  Node Unknown      │
                    │  (Not in registry) │
                    └────────┬───────────┘
                             │
                    (Announcement received)
                             │
                    ┌────────▼────────┐
                    │ Node Registered │
                    │ (In registry)   │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
    ┌────▼──────┐      ┌─────▼────┐       ┌──────▼──────┐
    │ Updating  │  <──►│ Idle     │    ◄──│ Executing   │
    │ (Messages)|      │ (Normal) │       │ (Commands)  │
    └────┬──────┘      └─────┬────┘       └──────┬──────┘
         │                   │                   │
         │              (No heartbeat)           │
         │              (timeout expires)        │
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                    (Timeout > 300 seconds)
                             │
                    ┌────────▼────────┐
                    │ Node Timed Out  │
                    │ (Removed)       │
                    └─────────────────┘
```
