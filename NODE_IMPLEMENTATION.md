# LoRa Node Implementation Guide

This document describes how to implement LoRa sensor nodes that communicate with the LoRaGateway-ESP.

## Message Protocol

Nodes communicate with the gateway using the `LoRaMessage` format defined in [include/Types.h](../include/Types.h).

### Node Announcement (Discovery)

When a node powers on or wants to be discovered, it should send an announcement message:

```cpp
LoRaMessage announcement;
announcement.nodeId = 1001;              // Your unique node ID
announcement.deviceId = 0;               // Device index
announcement.deviceType = DeviceType::SENSOR;
announcement.messageType = 0;            // Announcement
announcement.valueType = ValueType::FLOAT_VALUE;
announcement.value.floatValue = 0.0f;
announcement.command = CommandType::SET_STATE;

// Send via LoRa
sendMessage(announcement);
```

### Sensor Value Updates

Send sensor readings periodically:

```cpp
LoRaMessage update;
update.nodeId = 1001;
update.deviceId = 0;
update.deviceType = DeviceType::SENSOR;
update.messageType = 1;                  // Sensor update
update.valueType = ValueType::FLOAT_VALUE;
update.value.floatValue = 23.5f;         // Your sensor reading
update.command = CommandType::SET_STATE;

sendMessage(update);
```

### Responding to Commands

Listen for commands from the gateway:

```cpp
LoRaMessage command;

// Read incoming LoRa message
if (readMessage(command)) {
  if (command.messageType == 3) {  // Command message
    switch (command.command) {
      case CommandType::SET_STATE:
        // command.commandValue[0] = 0 (OFF) or 1 (ON)
        handleSwitch(command.commandValue[0]);
        break;
        
      case CommandType::SET_POSITION:
        // command.commandValue[0] = position byte
        uint8_t position = command.commandValue[0];
        handlePosition(position);
        break;
        
      case CommandType::OPEN:
      case CommandType::CLOSE:
      case CommandType::STOP:
        handleCoverCommand(command.command);
        break;
        
      default:
        break;
    }
    
    // Optionally send acknowledgment
    LoRaMessage ack;
    ack.nodeId = command.nodeId;
    ack.deviceId = command.deviceId;
    ack.messageType = 2;  // Command response
    ack.command = CommandType::SET_STATE;
    sendMessage(ack);
  }
}
```

## Example Node Implementation

### Temperature Sensor Node

```cpp
#include <Arduino.h>
#include "LoRaHandler.h"
#include "Types.h"

LoRaHandler loRa(10, 9, 2);  // CS, RST, DIO

void setup() {
  Serial.begin(115200);
  
  // Initialize LoRa
  if (!loRa.begin(868000000)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  loRa.setOnMessageReceived(handleLoRaMessage);
  
  // Send announcement
  announceNode();
}

void loop() {
  // Read LoRa messages
  loRa.handle();
  
  // Send sensor update every 30 seconds
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 30000) {
    lastSend = millis();
    sendTemperatureUpdate();
  }
  
  delay(10);
}

void announceNode() {
  LoRaMessage msg;
  msg.nodeId = 1001;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::SENSOR;
  msg.messageType = 0;  // Announcement
  msg.valueType = ValueType::FLOAT_VALUE;
  msg.value.floatValue = 0.0f;
  msg.command = CommandType::SET_STATE;
  
  loRa.sendMessage(msg);
  Serial.println("Node announced!");
}

void sendTemperatureUpdate() {
  // Read temperature (example using dummy value)
  float temperature = 23.5f;  // Replace with actual sensor reading
  
  LoRaMessage msg;
  msg.nodeId = 1001;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::SENSOR;
  msg.messageType = 1;  // Sensor update
  msg.valueType = ValueType::FLOAT_VALUE;
  msg.value.floatValue = temperature;
  msg.command = CommandType::SET_STATE;
  
  loRa.sendMessage(msg);
  Serial.print("Temperature sent: ");
  Serial.println(temperature);
}

void handleLoRaMessage(const LoRaMessage& msg) {
  // Handle incoming commands if needed
  Serial.print("Message received: ");
  Serial.println(msg.messageType);
}
```

### Motion Sensor Node

```cpp
#include <Arduino.h>
#include "LoRaHandler.h"
#include "Types.h"

const int MOTION_PIN = A0;
LoRaHandler loRa(10, 9, 2);

void setup() {
  Serial.begin(115200);
  pinMode(MOTION_PIN, INPUT);
  
  if (!loRa.begin(868000000)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  announceNode();
}

void loop() {
  loRa.handle();
  
  // Read motion sensor
  static bool lastState = false;
  bool motionDetected = digitalRead(MOTION_PIN) == HIGH;
  
  if (motionDetected != lastState) {
    lastState = motionDetected;
    
    LoRaMessage msg;
    msg.nodeId = 1002;
    msg.deviceId = 0;
    msg.deviceType = DeviceType::BINARY_SENSOR;
    msg.messageType = 1;
    msg.valueType = ValueType::BOOLEAN;
    msg.value.boolValue = motionDetected;
    msg.command = CommandType::SET_STATE;
    
    loRa.sendMessage(msg);
    Serial.println(motionDetected ? "Motion detected!" : "Motion cleared");
  }
  
  delay(100);
}

void announceNode() {
  LoRaMessage msg;
  msg.nodeId = 1002;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::BINARY_SENSOR;
  msg.messageType = 0;
  msg.valueType = ValueType::BOOLEAN;
  msg.value.boolValue = false;
  msg.command = CommandType::SET_STATE;
  
  loRa.sendMessage(msg);
}
```

### Light Switch Node

```cpp
#include <Arduino.h>
#include "LoRaHandler.h"
#include "Types.h"

const int RELAY_PIN = 5;
LoRaHandler loRa(10, 9, 2);

bool relayState = false;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  if (!loRa.begin(868000000)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  loRa.setOnMessageReceived(handleCommand);
  announceNode();
}

void loop() {
  loRa.handle();
  delay(10);
}

void announceNode() {
  LoRaMessage msg;
  msg.nodeId = 1003;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::SWITCH;
  msg.messageType = 0;
  msg.valueType = ValueType::BOOLEAN;
  msg.value.boolValue = relayState;
  msg.command = CommandType::SET_STATE;
  
  loRa.sendMessage(msg);
}

void handleCommand(const LoRaMessage& msg) {
  if (msg.messageType == 3 && msg.command == CommandType::SET_STATE) {
    relayState = msg.commandValue[0] != 0;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    
    // Send status update
    LoRaMessage response;
    response.nodeId = 1003;
    response.deviceId = 0;
    response.deviceType = DeviceType::SWITCH;
    response.messageType = 1;
    response.valueType = ValueType::BOOLEAN;
    response.value.boolValue = relayState;
    response.command = CommandType::SET_STATE;
    
    loRa.sendMessage(response);
    Serial.println(relayState ? "Relay ON" : "Relay OFF");
  }
}
```

### Garage Door Cover Node

```cpp
#include <Arduino.h>
#include "LoRaHandler.h"
#include "Types.h"

const int MOTOR_PIN = 5;
const int POSITION_SENSOR = A0;

LoRaHandler loRa(10, 9, 2);
uint8_t doorPosition = 0;  // 0 = closed, 255 = open

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT);
  
  if (!loRa.begin(868000000)) {
    while (1);
  }
  
  loRa.setOnMessageReceived(handleCommand);
  announceNode();
}

void loop() {
  loRa.handle();
  
  // Update position from sensor
  doorPosition = map(analogRead(POSITION_SENSOR), 0, 1023, 0, 255);
  
  delay(10);
}

void announceNode() {
  LoRaMessage msg;
  msg.nodeId = 1004;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::COVER;
  msg.messageType = 0;
  msg.valueType = ValueType::INT_VALUE;
  msg.value.intValue = doorPosition;
  
  loRa.sendMessage(msg);
}

void handleCommand(const LoRaMessage& msg) {
  if (msg.messageType == 3) {
    switch (msg.command) {
      case CommandType::OPEN:
        digitalWrite(MOTOR_PIN, HIGH);
        delay(5000);  // Run motor for 5 seconds
        digitalWrite(MOTOR_PIN, LOW);
        doorPosition = 255;
        break;
        
      case CommandType::CLOSE:
        digitalWrite(MOTOR_PIN, LOW);
        delay(5000);
        doorPosition = 0;
        break;
        
      case CommandType::STOP:
        digitalWrite(MOTOR_PIN, LOW);
        break;
        
      case CommandType::SET_POSITION:
        // Position: msg.commandValue[0] = 0-255
        {
          uint8_t targetPos = msg.commandValue[0];
          // Move towards target position
        }
        break;
        
      default:
        break;
    }
    
    // Send status
    sendPositionUpdate();
  }
}

void sendPositionUpdate() {
  LoRaMessage msg;
  msg.nodeId = 1004;
  msg.deviceId = 0;
  msg.deviceType = DeviceType::COVER;
  msg.messageType = 1;
  msg.valueType = ValueType::INT_VALUE;
  msg.value.intValue = doorPosition;
  
  loRa.sendMessage(msg);
}
```

## Best Practices

1. **Node IDs**: Use unique IDs (> 1000 recommended to avoid conflicts)
2. **Device IDs**: Keep index within device to identify multiple sensors per node
3. **Update Frequency**: 
   - Events (motion, door): Send immediately
   - Periodic (temperature): Every 30-60 seconds
   - Continuous (position): On change or every 100ms
4. **Acknowledgments**: Send command responses to confirm execution
5. **Announcements**: Resend periodically (every 5 minutes) for redundancy
6. **Power Management**: Use deep sleep between updates for battery nodes

## Message Type Specifications

| MessageType | Direction | Purpose |
|------------|-----------|---------|
| 0 | Node → Gateway | Node announcement/discovery |
| 1 | Node → Gateway | Sensor value update |
| 2 | Node → Gateway | Command acknowledgment/response |
| 3 | Gateway → Node | Command to execute |

## Value Type Specifications

| ValueType | Size | Range |
|-----------|------|-------|
| BOOLEAN | 1 byte | true/false |
| INT_VALUE | 4 bytes | -2,147,483,648 to 2,147,483,647 |
| FLOAT_VALUE | 4 bytes | IEEE 754 single precision |
| STRING | Variable | Max 60 characters (with overhead) |
