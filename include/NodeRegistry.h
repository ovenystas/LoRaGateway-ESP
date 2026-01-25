#pragma once

#include "Types.h"
#include <Arduino.h>

class NodeRegistry {
public:
  static const uint8_t MAX_NODES = 50;
  static const uint8_t MAX_DEVICES_PER_NODE = 16;
  
  NodeRegistry();
  
  // Register a new node (called when node is first discovered)
  bool registerNode(uint16_t nodeId, const char* nodeName);
  
  // Unregister a node
  void unregisterNode(uint16_t nodeId);
  
  // Register a device within a node
  bool registerDevice(uint16_t nodeId, const DeviceInfo& device);
  
  // Get node info
  NodeInfo* getNode(uint16_t nodeId);
  
  // Get device info
  DeviceInfo* getDevice(uint16_t nodeId, uint8_t deviceId);
  
  // Check if node exists
  bool hasNode(uint16_t nodeId);
  
  // Check if device exists
  bool hasDevice(uint16_t nodeId, uint8_t deviceId);
  
  // Update last seen time for a node
  void updateNodeLastSeen(uint16_t nodeId);
  
  // Get all registered nodes
  NodeInfo** getAllNodes(uint8_t& count);
  
  // Clear all registrations
  void clear();
  
  // Get node count
  uint8_t getNodeCount();
  
private:
  struct InternalNodeInfo {
    NodeInfo info;
    DeviceInfo devices[MAX_DEVICES_PER_NODE];
    uint8_t registeredDeviceCount;
  };
  
  InternalNodeInfo nodes[MAX_NODES];
  uint8_t nodeCount;
  
  int findNodeIndex(uint16_t nodeId);
};
