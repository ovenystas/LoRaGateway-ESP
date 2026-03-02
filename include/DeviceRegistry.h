#pragma once

#include "Types.h"
#include <Arduino.h>

class DeviceRegistry {
public:
  static const uint8_t MAX_DEVICES = 50;
  static const uint8_t MAX_ENTITIES_PER_DEVICE = 16;

  DeviceRegistry();

  // Register a new device (called when device is first discovered)
  bool registerDevice(uint16_t deviceId, const char* deviceName);

  // Unregister a device
  void unregisterDevice(uint16_t deviceId);

  // Register an entity within a device
  bool registerEntity(uint16_t deviceId, const EntityInfo& entity);

  // Get device info
  DeviceInfo* getDevice(uint16_t deviceId);

  // Get entity info
  EntityInfo* getEntity(uint16_t deviceId, uint8_t entityId);

  // Check if device exists
  bool hasDevice(uint16_t deviceId);

  // Check if entity exists
  bool hasEntity(uint16_t deviceId, uint8_t entityId);

  // Update last seen time for a device
  void updateDeviceLastSeen(uint16_t deviceId);

  // Get all registered devices
  DeviceInfo** getAllDevices(uint8_t& count);

  // Clear all registrations
  void clear();

  // Get device count
  uint8_t getDeviceCount();

private:
  struct InternalDeviceInfo {
    DeviceInfo info;
    EntityInfo entities[MAX_ENTITIES_PER_DEVICE];
    uint8_t registeredEntityCount;
  };

  InternalDeviceInfo devices[MAX_DEVICES];
  uint8_t deviceCount;

  int findDeviceIndex(uint16_t deviceId);
};
