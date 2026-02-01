#include "DeviceRegistry.h"
#include <string.h>

DeviceRegistry::DeviceRegistry() : deviceCount(0) {
  memset(devices, 0, sizeof(devices));
}

bool DeviceRegistry::registerDevice(uint16_t deviceId, const char* deviceName) {
  // Check if device already exists
  if (hasDevice(deviceId)) {
    return true;  // Already registered
  }
  
  // Check if we have space
  if (deviceCount >= MAX_DEVICES) {
    return false;
  }
  
  InternalDeviceInfo& device = devices[deviceCount];
  device.info.deviceId = deviceId;
  device.info.name = deviceName;
  device.info.lastSeen = millis();
  device.info.entityCount = 0;
  device.info.entities = device.entities;
  device.registeredEntityCount = 0;
  
  deviceCount++;
  return true;
}

void DeviceRegistry::unregisterDevice(uint16_t deviceId) {
  int idx = findDeviceIndex(deviceId);
  if (idx == -1) {
    return;
  }
  
  // Shift remaining devices
  for (int i = idx; i < deviceCount - 1; i++) {
    memcpy(&devices[i], &devices[i + 1], sizeof(InternalDeviceInfo));
  }
  
  deviceCount--;
  memset(&devices[deviceCount], 0, sizeof(InternalDeviceInfo));
}

bool DeviceRegistry::registerEntity(uint16_t deviceId, const EntityInfo& entity) {
  int idx = findDeviceIndex(deviceId);
  if (idx == -1) {
    return false;  // Device not registered
  }
  
  InternalDeviceInfo& device = devices[idx];
  
  // Check if entity already exists
  if (device.registeredEntityCount > 0) {
    for (uint8_t i = 0; i < device.registeredEntityCount; i++) {
      if (device.entities[i].entityId == entity.entityId) {
        return true;  // Already registered
      }
    }
  }
  
  // Check if we have space
  if (device.registeredEntityCount >= MAX_ENTITIES_PER_DEVICE) {
    return false;
  }
  
  device.entities[device.registeredEntityCount] = entity;
  device.registeredEntityCount++;
  device.info.entityCount = device.registeredEntityCount;
  
  return true;
}

DeviceInfo* DeviceRegistry::getDevice(uint16_t deviceId) {
  int idx = findDeviceIndex(deviceId);
  if (idx == -1) {
    return nullptr;
  }
  return &devices[idx].info;
}

EntityInfo* DeviceRegistry::getEntity(uint16_t deviceId, uint8_t entityId) {
  int idx = findDeviceIndex(deviceId);
  if (idx == -1) {
    return nullptr;
  }
  
  InternalDeviceInfo& device = devices[idx];
  for (uint8_t i = 0; i < device.registeredEntityCount; i++) {
    if (device.entities[i].entityId == entityId) {
      return &device.entities[i];
    }
  }
  
  return nullptr;
}

bool DeviceRegistry::hasDevice(uint16_t deviceId) {
  return findDeviceIndex(deviceId) != -1;
}

bool DeviceRegistry::hasEntity(uint16_t deviceId, uint8_t entityId) {
  return getEntity(deviceId, entityId) != nullptr;
}

void DeviceRegistry::updateDeviceLastSeen(uint16_t deviceId) {
  int idx = findDeviceIndex(deviceId);
  if (idx != -1) {
    devices[idx].info.lastSeen = millis();
  }
}

DeviceInfo** DeviceRegistry::getAllDevices(uint8_t& count) {
  count = deviceCount;
  // Return pointer to first device's info
  if (deviceCount == 0) {
    return nullptr;
  }
  
  static DeviceInfo* deviceArray[MAX_DEVICES];
  for (uint8_t i = 0; i < deviceCount; i++) {
    deviceArray[i] = &devices[i].info;
  }
  
  return deviceArray;
}

void DeviceRegistry::clear() {
  deviceCount = 0;
  memset(devices, 0, sizeof(devices));
}

uint8_t DeviceRegistry::getDeviceCount() {
  return deviceCount;
}

int DeviceRegistry::findDeviceIndex(uint16_t deviceId) {
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].info.deviceId == deviceId) {
      return i;
    }
  }
  return -1;
}
