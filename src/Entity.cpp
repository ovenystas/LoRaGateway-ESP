#include "Entity.h"

const char *EntityDomain::getName() const {
    switch (domain) {
        case Domain::BINARY_SENSOR: return "binary_sensor";
        case Domain::SENSOR: return "sensor";
        case Domain::COVER: return "cover";
        default: return "unknown";
    }
}
