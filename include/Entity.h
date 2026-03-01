#pragma once

#include <stdint.h>

#include "BinarySensorDeviceClass.h"
#include "CoverDeviceClass.h"
#include "SensorDeviceClass.h"
#include "Types.h"
#include "Unit.h"

class Entity {
 public:
  Entity() {
    info.deviceId = 0;
    info.entityId = 0;
    info.domain = EntityDomain(EntityDomain::Domain::SENSOR);
    info.deviceClass = nullptr;
    info.format = Format();
    info.unit = Unit();
  }
  Entity(const EntityInfo& entityInfo) : info(entityInfo) {}

  const EntityInfo& getInfo() const { return info; }

 private:
  EntityInfo info;
};
