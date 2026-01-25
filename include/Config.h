#pragma once

// Include secrets from secrets.h (keep this file private, not in git)
#include "secrets.h"

// LoRa Configuration
// Pins for RFM95 (GPIO numbers for ESP8266 Huzzah)
#define LORA_CS_PIN 15      // D8 - Chip Select
#define LORA_RST_PIN 2      // D4 - Reset
#define LORA_DIO_PIN 0      // D3 - DIO0 (interrupt)

// LoRa Frequency (868 MHz for EU, 915 MHz for US)
#define LORA_FREQUENCY 868000000

// Gateway Configuration
#define GATEWAY_NAME "LoRa Gateway"
#define NODE_TIMEOUT_SECONDS 300  // Timeout for node considered offline
