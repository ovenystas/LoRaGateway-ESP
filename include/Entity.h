#pragma once

#include <Arduino.h>

#include <vector>

#include "BinarySensorDeviceClass.h"
#include "ConfigItem.h"
#include "CoverDeviceClass.h"
#include "Format.h"
#include "NumberDeviceClass.h"
#include "SensorDeviceClass.h"
#include "Unit.h"

// Entity Domain definition
class EntityDomain {
 public:
  enum class Domain : uint8_t {
    BINARY_SENSOR,
    BUTTON,
    CAMERA,
    CLIMATE,
    COVER,
    DEVICE_TRACKER,
    FAN,
    HUMIDIFIER,
    IMAGE,
    LAWN_MOWER,
    LIGHT,
    LOCK,
    NOTIFY,
    NUMBER,
    ROOM_PRESENCE,
    SELECT,
    SENSOR,
    SIREN,
    SWITCH,
    TEXT,
    VACUUM,
    VALVE,
    WATER_HEATER
  };

  EntityDomain() : domain(Domain::SENSOR) {}
  EntityDomain(Domain d) : domain(d) {}
  EntityDomain(uint8_t d) : domain(static_cast<Domain>(d)) {}

  const char* getName() const;
  Domain getDomain() const { return domain; }
  uint8_t toByte() const { return static_cast<uint8_t>(domain); }

 private:
  Domain domain;
};

class EntityCategory {
 public:
  enum class Category : uint8_t { NONE, CONFIG, DIAGNOSTIC };

  EntityCategory() : category(Category::NONE) {}
  EntityCategory(Category t) : category(t) {}
  EntityCategory(uint8_t t) : category(static_cast<Category>(t)) {}

  Category getType() const { return category; }
  const char* getName() const;
  uint8_t toByte() const { return static_cast<uint8_t>(category); }

 private:
  Category category;
};

// Entity metadata
struct EntityInfo {
  uint16_t deviceId;
  uint8_t entityId;
  EntityDomain domain;
  std::unique_ptr<DeviceClass> deviceClass;
  EntityCategory category;
  Unit unit;
  Format format;
  uint32_t minValue;
  uint32_t maxValue;
  String name;

  EntityInfo() = default;

  // Copy constructor for cloning deviceClass
  EntityInfo(const EntityInfo& other)
      : deviceId(other.deviceId),
        entityId(other.entityId),
        domain(other.domain),
        deviceClass(copyDeviceClass(other.deviceClass)),
        category(other.category),
        unit(other.unit),
        format(other.format),
        minValue(other.minValue),
        maxValue(other.maxValue),
        name(other.name) {}

  // Copy assignment operator
  EntityInfo& operator=(const EntityInfo& other) {
    if (this != &other) {
      deviceId = other.deviceId;
      entityId = other.entityId;
      domain = other.domain;
      deviceClass = copyDeviceClass(other.deviceClass);
      category = other.category;
      unit = other.unit;
      format = other.format;
      minValue = other.minValue;
      maxValue = other.maxValue;
      name = other.name;
    }
    return *this;
  }

  void setDeviceClass(const std::unique_ptr<DeviceClass>& source) {
    deviceClass = copyDeviceClass(source);
  }

  size_t print(Print& printer, size_t indent = 0) const {
    size_t n = 0;
    for (size_t i = 0; i < indent; i++) {
      n += printer.print(" ");
    }
    n += printer.print("Entity ID: ");
    n += printer.print(entityId);
    n += printer.print(", Name: ");
    n += printer.print(name);
    n += printer.print(", Domain: ");
    n += printer.print(domain.getName());
    n += printer.print(", Device Class: ");
    n += printer.print(deviceClass ? deviceClass->getName() : "unknown");
    n += printer.print(", Category: ");
    n += printer.print(category.getName());
    n += printer.print(", Unit: ");
    n += printer.println(unit.getName());
    n += printer.print(", Format: (");
    n += format.print(printer);
    n += printer.print("), Min=");
    n += printer.print(format.fromRawValue(minValue), format.getPrecision());
    n += printer.print(", Max=");
    n += printer.println(format.fromRawValue(maxValue), format.getPrecision());
    return n;
  }

 private:
  static std::unique_ptr<DeviceClass> copyDeviceClass(
      const std::unique_ptr<DeviceClass>& source) {
    if (!source) {
      return nullptr;
    }
    return std::unique_ptr<DeviceClass>(source->clone());
  }
};

class Entity {
 public:
  Entity() {
    info.deviceId = 0;
    info.entityId = 0;
    info.domain = EntityDomain(EntityDomain::Domain::SENSOR);
    info.deviceClass = nullptr;
    info.category = EntityCategory(EntityCategory::Category::NONE);
    info.unit = Unit();
    info.format = Format();
    info.minValue = 0;
    info.maxValue = 0;
    info.name = "";
  }
  Entity(const EntityInfo& entityInfo) : info(entityInfo) {}

  const EntityInfo& getInfo() const { return info; }

 private:
  EntityInfo info;
};
