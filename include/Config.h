#pragma once

// Include secrets from secrets.h (keep this file private, not in git)
#include "secrets.h"

// LoRa Configuration
// Pins for RFM95 (GPIO numbers for NodeMCU-ESP32)
#define LORA_CS_PIN 5    // D5  - Chip Select
#define LORA_RST_PIN 14  // D14 - Reset
#define LORA_DIO_PIN 2   // D2  - DIO0 (interrupt)

// LoRa Frequency (868 MHz for EU, 915 MHz for US)
#define LORA_FREQUENCY 868e6

// Gateway Configuration
#define GATEWAY_NAME "LoRa Gateway"
#define NODE_TIMEOUT_SECONDS 300  // Timeout for node considered offline
