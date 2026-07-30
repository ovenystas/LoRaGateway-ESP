#pragma once

#include <Arduino.h>
#include <Print.h>

#include <vector>

// Forward declaration
class AsyncWebSocket;

class Logger : public Print {
 public:
  enum Level : uint8_t { ERR = 0, WRN = 1, INF = 2, DBG = 3 };

  static const char* levelToString(Level level);

  Logger();
  ~Logger();

  // Set the web socket pointer for pushing log messages to web clients.
  void setWebSocket(AsyncWebSocket* ws);

  // Enable or disable web logging.
  void setWebEnabled(bool enabled);
  bool isWebEnabled() const;

  // Set/get the maximum log level. Messages above this level are suppressed.
  void setMaxLevel(Level level);
  Level getMaxLevel() const;

  // Get buffered log entries (for new WebSocket clients).
  struct LogEntry {
    Level level;
    unsigned long timestamp;
    String message;
  };
  const std::vector<LogEntry>& getBufferedEntries() const;

  // --- Print interface ---
  // Each write() call buffers characters. Flush happens on println().
  size_t write(uint8_t c) override;
  void flush();

  // Convenience methods that set the level and then act like normal print.
  // Usage: logger.println(Logger::INF, F("Device %d"), id);
  void print(Level level, char c);
  void print(Level level, const String& str);
  void print(Level level, const char* str);
  void println(Level level, const String& str);
  void println(Level level, const char* str);
  void println(Level level);
  void printf(Level level, const char* format, ...)
      __attribute__((format(printf, 3, 4)));

 private:
  AsyncWebSocket* webSocket;
  bool webEnabled;
  Level maxLevel;
  Level currentMessageLevel;
  bool messageActive;
  String buffer;
  std::vector<LogEntry> ringBuffer;

  static const size_t MAX_BUFFER_SIZE = 200;

  void finalizeMessage();
  void addToRingBuffer(const LogEntry& entry);
  void pushToWebSocket(const LogEntry& entry);
};

// Global logger instance
extern Logger logger;