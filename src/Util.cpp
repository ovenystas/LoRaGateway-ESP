#include "Util.h"

#include <Arduino.h>

void printUptime(Print& printer) {
  auto t = millis();

  uint16_t hours = t / 3600000;
  t %= 3600000;

  uint8_t minutes = t / 60000;
  t %= 60000;

  uint8_t seconds = t / 1000;
  t %= 1000;

  uint16_t milliseconds = t;

  printer.print('[');
  printer.print(hours);
  printer.print(':');
  if (minutes < 10) {
    printer.print('0');
  }
  printer.print(minutes);
  printer.print(':');
  if (seconds < 10) {
    printer.print('0');
  }
  printer.print(seconds);
  printer.print('.');
  if (milliseconds < 100) {
    printer.print('0');
  }
  if (milliseconds < 10) {
    printer.print('0');
  }
  printer.print(milliseconds);
  printer.print("] ");
}

void printMillis(Print& printer) {
  auto t = millis();

  printer.print("\n[");
  printer.print(t);
  printer.print("] ");
}

void printVersion(Print& printer, uint8_t major, uint8_t minor, uint8_t patch) {
  printer.print(major);
  printer.print('.');
  printer.print(minor);
  printer.print('.');
  printer.print(patch);
}

void printHex(Print& printer, uint8_t value, bool prefix) {
  if (prefix) {
    printer.print("0x");
  }
  if (value < 0x10) {
    printer.print('0');
  }
  printer.print(value, HEX);
}

void printHex(Print& printer, uint16_t value, bool prefix) {
  if (prefix) {
    printer.print("0x");
  }
  if (value < 0x1000) {
    printer.print('0');
  }
  if (value < 0x100) {
    printer.print('0');
  }
  if (value < 0x10) {
    printer.print('0');
  }
  printer.print(value, HEX);
}

void printHex(Print& printer, uint32_t value, bool prefix) {
  if (prefix) {
    printer.print("0x");
  }
  if (value < 0x10000000) {
    printer.print('0');
  }
  if (value < 0x1000000) {
    printer.print('0');
  }
  if (value < 0x100000) {
    printer.print('0');
  }
  if (value < 0x10000) {
    printer.print('0');
  }
  if (value < 0x1000) {
    printer.print('0');
  }
  if (value < 0x100) {
    printer.print('0');
  }
  if (value < 0x10) {
    printer.print('0');
  }
  printer.print(value, HEX);
}

void printArray(Print& printer, const uint8_t* buf, size_t len, int base,
                bool prefix) {
  for (size_t i = 0; i < len; i++) {
    if (base == HEX) {
      printHex(printer, buf[i], prefix);
    } else {
      if (prefix) {
        if (base == BIN) {
          printer.print('b');
        } else if (base == OCT) {
          printer.print('o');
        } else if (base == DEC) {
          printer.print('d');
        }
      }
      printer.print(buf[i], base);
    }

    if (i < len - 1) {
      printer.print(' ');
    }
  }
}
