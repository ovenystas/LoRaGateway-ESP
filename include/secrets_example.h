#pragma once

// This file shows the structure needed for secrets.h
// Copy this file to secrets.h and fill in your actual values
// secrets.h will be gitignored so your credentials stay private

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora_gateway"
#define MQTT_USERNAME ""          // Leave empty if no auth required
#define MQTT_PASSWORD ""          // Leave empty if no auth required
