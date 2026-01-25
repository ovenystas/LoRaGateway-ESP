#pragma once

// WiFi Configuration
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora_gateway"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// LoRa Configuration
// Pins for RFM95 (adjust based on your wiring)
#define LORA_CS_PIN D8      // Chip Select
#define LORA_RST_PIN D4     // Reset
#define LORA_DIO_PIN D3     // DIO0 (interrupt)

// LoRa Frequency (868 MHz for EU, 915 MHz for US)
#define LORA_FREQUENCY 868000000

// Gateway Configuration
#define GATEWAY_NAME "LoRa Gateway"
#define NODE_TIMEOUT_SECONDS 300  // Timeout for node considered offline
