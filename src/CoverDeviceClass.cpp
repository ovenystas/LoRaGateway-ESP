#include "CoverDeviceClass.h"

const char* CoverDeviceClass::getName() const {
  static const char* names[] = {"none",   "awning",  "blind",  "curtain",
                                "damper", "door",    "garage", "gate",
                                "shade",  "shutter", "window"};

  if (deviceClass < sizeof(names) / sizeof(names[0])) {
    return names[deviceClass];
  }
  return "unknown";
}
