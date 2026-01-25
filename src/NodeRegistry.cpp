#include "NodeRegistry.h"
#include <string.h>

NodeRegistry::NodeRegistry() : nodeCount(0) {
  memset(nodes, 0, sizeof(nodes));
}

bool NodeRegistry::registerNode(uint16_t nodeId, const char* nodeName) {
  // Check if node already exists
  if (hasNode(nodeId)) {
    return true;  // Already registered
  }
  
  // Check if we have space
  if (nodeCount >= MAX_NODES) {
    return false;
  }
  
  InternalNodeInfo& node = nodes[nodeCount];
  node.info.nodeId = nodeId;
  node.info.name = nodeName;
  node.info.lastSeen = millis();
  node.info.deviceCount = 0;
  node.info.devices = node.devices;
  node.registeredDeviceCount = 0;
  
  nodeCount++;
  return true;
}

void NodeRegistry::unregisterNode(uint16_t nodeId) {
  int idx = findNodeIndex(nodeId);
  if (idx == -1) {
    return;
  }
  
  // Shift remaining nodes
  for (int i = idx; i < nodeCount - 1; i++) {
    memcpy(&nodes[i], &nodes[i + 1], sizeof(InternalNodeInfo));
  }
  
  nodeCount--;
  memset(&nodes[nodeCount], 0, sizeof(InternalNodeInfo));
}

bool NodeRegistry::registerDevice(uint16_t nodeId, const DeviceInfo& device) {
  int idx = findNodeIndex(nodeId);
  if (idx == -1) {
    return false;  // Node not registered
  }
  
  InternalNodeInfo& node = nodes[idx];
  
  // Check if device already exists
  if (node.registeredDeviceCount > 0) {
    for (uint8_t i = 0; i < node.registeredDeviceCount; i++) {
      if (node.devices[i].deviceId == device.deviceId) {
        return true;  // Already registered
      }
    }
  }
  
  // Check if we have space
  if (node.registeredDeviceCount >= MAX_DEVICES_PER_NODE) {
    return false;
  }
  
  node.devices[node.registeredDeviceCount] = device;
  node.registeredDeviceCount++;
  node.info.deviceCount = node.registeredDeviceCount;
  
  return true;
}

NodeInfo* NodeRegistry::getNode(uint16_t nodeId) {
  int idx = findNodeIndex(nodeId);
  if (idx == -1) {
    return nullptr;
  }
  return &nodes[idx].info;
}

DeviceInfo* NodeRegistry::getDevice(uint16_t nodeId, uint8_t deviceId) {
  int idx = findNodeIndex(nodeId);
  if (idx == -1) {
    return nullptr;
  }
  
  InternalNodeInfo& node = nodes[idx];
  for (uint8_t i = 0; i < node.registeredDeviceCount; i++) {
    if (node.devices[i].deviceId == deviceId) {
      return &node.devices[i];
    }
  }
  
  return nullptr;
}

bool NodeRegistry::hasNode(uint16_t nodeId) {
  return findNodeIndex(nodeId) != -1;
}

bool NodeRegistry::hasDevice(uint16_t nodeId, uint8_t deviceId) {
  return getDevice(nodeId, deviceId) != nullptr;
}

void NodeRegistry::updateNodeLastSeen(uint16_t nodeId) {
  int idx = findNodeIndex(nodeId);
  if (idx != -1) {
    nodes[idx].info.lastSeen = millis();
  }
}

NodeInfo** NodeRegistry::getAllNodes(uint8_t& count) {
  count = nodeCount;
  // Return pointer to first node's info
  if (nodeCount == 0) {
    return nullptr;
  }
  
  static NodeInfo* nodeArray[MAX_NODES];
  for (uint8_t i = 0; i < nodeCount; i++) {
    nodeArray[i] = &nodes[i].info;
  }
  
  return nodeArray;
}

void NodeRegistry::clear() {
  nodeCount = 0;
  memset(nodes, 0, sizeof(nodes));
}

uint8_t NodeRegistry::getNodeCount() {
  return nodeCount;
}

int NodeRegistry::findNodeIndex(uint16_t nodeId) {
  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].info.nodeId == nodeId) {
      return i;
    }
  }
  return -1;
}
