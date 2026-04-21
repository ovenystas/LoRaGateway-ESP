#pragma once

#include <stddef.h>
#include <stdint.h>

// Mock Print class for native testing
// This provides a minimal interface compatible with Arduino's Print class
class Print {
 public:
  virtual ~Print() = default;

  // Mock implementation of print that does nothing
  virtual size_t write(uint8_t) { return 1; }

  size_t print(const char* str) { return str ? strlen(str) : 0; }
  size_t print(unsigned char c) { return 1; }
  size_t print(int i) { return 1; }
  size_t print(unsigned int i) { return 1; }
  size_t print(long l) { return 1; }
  size_t print(unsigned long l) { return 1; }
  size_t print(double f, int precision = 2) { return 1; }
  size_t print(const void* p) { return 1; }

  size_t println(const char* str) { return str ? strlen(str) + 1 : 1; }
  size_t println(unsigned char c) { return 2; }
  size_t println(int i) { return 1; }
  size_t println(unsigned int i) { return 1; }
  size_t println(long l) { return 1; }
  size_t println(unsigned long l) { return 1; }
  size_t println(double f, int precision = 2) { return 1; }
  size_t println(const void* p) { return 1; }

 private:
  size_t strlen(const char* str) const {
    size_t len = 0;
    while (str[len]) len++;
    return len;
  }
};
