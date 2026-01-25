# Home Assistant MQTT Configuration Examples

This document shows example Home Assistant configurations for LoRa gateway devices.

## MQTT Discovery

The gateway automatically publishes Home Assistant discovery messages when nodes are discovered. Discovery messages are published to:

```
homeassistant/{component}/{unique_id}/config
```

### Example Discovery Payloads

#### Temperature Sensor

**Topic**: `homeassistant/sensor/lora_1001_0/config`

```json
{
  "name": "Temperature Sensor",
  "unique_id": "lora_1001_0",
  "object_id": "lora_1001_0",
  "state_topic": "lora_gateway/node_1001/device_0/state",
  "value_template": "{{ value_json.value }}",
  "unit_of_measurement": "°C",
  "device": {
    "identifiers": ["lora_node_1001"],
    "name": "LoRa Node 1001"
  }
}
```

#### Motion Sensor

**Topic**: `homeassistant/binary_sensor/lora_1002_0/config`

```json
{
  "name": "Motion Sensor",
  "unique_id": "lora_1002_0",
  "object_id": "lora_1002_0",
  "state_topic": "lora_gateway/node_1002/device_0/state",
  "value_template": "{{ 'ON' if value_json.state == 'ON' else 'OFF' }}",
  "device_class": "motion",
  "device": {
    "identifiers": ["lora_node_1002"],
    "name": "LoRa Node 1002"
  }
}
```

#### Light Switch

**Topic**: `homeassistant/switch/lora_1003_0/config`

```json
{
  "name": "Light Switch",
  "unique_id": "lora_1003_0",
  "object_id": "lora_1003_0",
  "state_topic": "lora_gateway/node_1003/device_0/state",
  "command_topic": "lora_gateway/node_1003/device_0/command",
  "value_template": "{{ value_json.state }}",
  "payload_on": "{\"command\": \"ON\"}",
  "payload_off": "{\"command\": \"OFF\"}",
  "device": {
    "identifiers": ["lora_node_1003"],
    "name": "LoRa Node 1003"
  }
}
```

#### Garage Door Cover

**Topic**: `homeassistant/cover/lora_1004_0/config`

```json
{
  "name": "Garage Door",
  "unique_id": "lora_1004_0",
  "object_id": "lora_1004_0",
  "state_topic": "lora_gateway/node_1004/device_0/state",
  "command_topic": "lora_gateway/node_1004/device_0/command",
  "value_template": "{{ value_json.value }}",
  "device_class": "garage",
  "device": {
    "identifiers": ["lora_node_1004"],
    "name": "LoRa Node 1004"
  }
}
```

## Manual MQTT Configuration (Alternative to Auto-Discovery)

If you prefer manual configuration in Home Assistant, add these to `configuration.yaml`:

### Template Sensors

```yaml
template:
  - sensor:
      - name: "Garage Temperature"
        unique_id: garage_temp
        unit_of_measurement: "°C"
        state: "{{ state_attr('sensor.lora_1001_0', 'value') | float(0) }}"
        state_class: "measurement"
```

### MQTT Sensors

```yaml
mqtt:
  sensor:
    - name: "Living Room Temperature"
      unique_id: living_room_temp
      state_topic: "lora_gateway/node_1001/device_0/state"
      unit_of_measurement: "°C"
      device_class: "temperature"
      value_template: "{{ value_json.value }}"
      
    - name: "Humidity"
      unique_id: humidity
      state_topic: "lora_gateway/node_1001/device_1/state"
      unit_of_measurement: "%"
      device_class: "humidity"
      value_template: "{{ value_json.value }}"

  binary_sensor:
    - name: "Motion Sensor"
      unique_id: motion
      state_topic: "lora_gateway/node_1002/device_0/state"
      device_class: "motion"
      value_template: "{{ value_json.state }}"
      payload_on: "ON"
      payload_off: "OFF"

  switch:
    - name: "Light Switch"
      unique_id: light_switch
      state_topic: "lora_gateway/node_1003/device_0/state"
      command_topic: "lora_gateway/node_1003/device_0/command"
      value_template: "{{ value_json.state }}"
      payload_on: '{"command": "ON"}'
      payload_off: '{"command": "OFF"}'

  cover:
    - name: "Garage Door"
      unique_id: garage_door
      state_topic: "lora_gateway/node_1004/device_0/state"
      command_topic: "lora_gateway/node_1004/device_0/command"
      value_template: "{{ value_json.value }}"
      device_class: "garage"
```

## Automations

### Example: Close Garage Door at Night

```yaml
automation:
  - alias: "Close Garage Door at Night"
    trigger:
      platform: time
      at: "22:00:00"
    action:
      - service: mqtt.publish
        data:
          topic: "lora_gateway/node_1004/device_0/command"
          payload: '{"command": "CLOSE"}'
```

### Example: Alert on Motion

```yaml
automation:
  - alias: "Motion Detected Alert"
    trigger:
      platform: mqtt
      topic: "lora_gateway/node_1002/device_0/state"
    condition:
      condition: template
      value_template: "{{ trigger.payload_json.state == 'ON' }}"
    action:
      - service: persistent_notification.create
        data:
          title: "Motion Alert"
          message: "Motion detected by living room sensor"
          notification_id: "motion_alert"
```

### Example: Conditional Light Control

```yaml
automation:
  - alias: "Control Light by Temperature"
    trigger:
      platform: mqtt
      topic: "lora_gateway/node_1001/device_0/state"
    condition:
      condition: template
      value_template: "{{ trigger.payload_json.value | float(0) > 25 }}"
    action:
      - service: mqtt.publish
        data:
          topic: "lora_gateway/node_1003/device_0/command"
          payload: '{"command": "ON"}'
```

## Dashboard Cards

### History Stats Card

```yaml
type: history-stats
title: Garage Door Open Time
entities:
  - binary_sensor.garage_door
state_type: "time"
period:
  type: day
```

### Gauge Card

```yaml
type: gauge
entity: sensor.living_room_temperature
min: 15
max: 30
unit: "°C"
```

### Entity Button Card

```yaml
type: button
entity: switch.light_switch
tap_action:
  action: call-service
  service: switch.toggle
  target:
    entity_id: switch.light_switch
```

## Troubleshooting

### Devices Not Appearing

1. Check that the gateway is connected to MQTT broker
2. Verify nodes are sending announcement messages
3. Check Home Assistant MQTT integration is enabled
4. Look at Home Assistant logs for discovery errors

### Commands Not Working

1. Verify command topic matches published discovery message
2. Check payload format matches node implementation
3. Ensure node is in the node registry
4. Monitor serial output of gateway for command routing

### Stale Values

1. Increase node update frequency
2. Check node is not timing out (300 second default)
3. Verify WiFi and MQTT connectivity of gateway
4. Monitor gateway serial output for communication errors

## Topics Reference

| Component | State Topic | Command Topic |
|-----------|------------|---------------|
| Sensor | `lora_gateway/node_{id}/device_{id}/state` | N/A |
| BinarySensor | `lora_gateway/node_{id}/device_{id}/state` | N/A |
| Switch | `lora_gateway/node_{id}/device_{id}/state` | `lora_gateway/node_{id}/device_{id}/command` |
| Cover | `lora_gateway/node_{id}/device_{id}/state` | `lora_gateway/node_{id}/device_{id}/command` |

## Testing with MQTT Client

Test communication with `mosquitto_pub` / `mosquitto_sub`:

```bash
# Subscribe to all gateway messages
mosquitto_sub -h localhost -t "lora_gateway/#"

# Publish a command to turn on a light
mosquitto_pub -h localhost -t "lora_gateway/node_1003/device_0/command" -m '{"command": "ON"}'

# Publish a position command to a cover
mosquitto_pub -h localhost -t "lora_gateway/node_1004/device_0/command" -m '{"command": "SET_POSITION", "value": 128}'
```
