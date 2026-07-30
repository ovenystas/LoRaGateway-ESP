#include "Logger.h"

#include <ESPAsyncWebServer.h>
#include <stdarg.h>
#include <stdio.h>

#define RING_BUFFER_MAX 200

Logger logger;

const char* Logger::levelToString(Level level) {
  switch (level) {
    case ERR:
      return "ERR";
    case WRN:
      return "WRN";
    case INF:
      return "INF";
    case DBG:
      return "DBG";
    default:
      return "???";
  }
}

Logger::Logger()
    : webSocket(nullptr),
      webEnabled(false),
      maxLevel(DBG),
      currentMessageLevel(INF),
      messageActive(false),
      buffer("") {
  ringBuffer.reserve(RING_BUFFER_MAX);
}

Logger::~Logger() {}

void Logger::setWebSocket(AsyncWebSocket* ws) { webSocket = ws; }

void Logger::setWebEnabled(bool enabled) { webEnabled = enabled; }

bool Logger::isWebEnabled() const { return webEnabled; }

void Logger::setMaxLevel(Level level) { maxLevel = level; }

Logger::Level Logger::getMaxLevel() const { return maxLevel; }

const std::vector<Logger::LogEntry>& Logger::getBufferedEntries() const {
  return ringBuffer;
}

size_t Logger::write(uint8_t c) {
  // Always write to Serial
  if (c == '\n') {
    Serial.write('\r');
  }
  Serial.write(c);

  // Buffer for potential web push
  if (messageActive && currentMessageLevel <= maxLevel) {
    buffer += (char)c;
  }
  return 1;
}

void Logger::flush() {
  if (messageActive && currentMessageLevel <= maxLevel) {
    finalizeMessage();
  }
  messageActive = false;
  buffer = "";
}

void Logger::finalizeMessage() {
  if (buffer.length() == 0) return;

  // Remove trailing newline
  String msg = buffer;
  if (msg.endsWith("\r\n")) {
    msg.remove(msg.length() - 2);
  } else if (msg.endsWith("\n")) {
    msg.remove(msg.length() - 1);
  } else if (msg.endsWith("\r")) {
    msg.remove(msg.length() - 1);
  }

  // Strip leading whitespace and newlines
  while (msg.length() > 0) {
    char c = msg.charAt(0);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      msg.remove(0, 1);
    } else {
      break;
    }
  }

  if (msg.length() == 0) return;

  LogEntry entry;
  entry.level = currentMessageLevel;
  entry.timestamp = millis();
  entry.message = msg;

  addToRingBuffer(entry);
  pushToWebSocket(entry);
}

void Logger::addToRingBuffer(const LogEntry& entry) {
  if (ringBuffer.size() >= RING_BUFFER_MAX) {
    ringBuffer.erase(ringBuffer.begin());
  }
  ringBuffer.push_back(entry);
}

void Logger::pushToWebSocket(const LogEntry& entry) {
  if (!webSocket || !webEnabled) return;

  // Build JSON: {"type":"log","level":"INF","timestamp":12345,"message":"..."}
  // Use a static buffer to avoid dynamic allocation on every log line
  static const size_t JSON_BUF_SIZE = 512;
  char jsonBuf[JSON_BUF_SIZE];
  int len = snprintf(
      jsonBuf, JSON_BUF_SIZE,
      "{\"type\":\"log\",\"level\":\"%s\",\"timestamp\":%lu,\"message\":\"",
      levelToString(entry.level), entry.timestamp);

  // Append message (escape simple chars)
  size_t pos = len;
  for (size_t i = 0; i < entry.message.length() && pos < JSON_BUF_SIZE - 4;
       i++) {
    char c = entry.message[i];
    if (c == '"') {
      if (pos + 1 < JSON_BUF_SIZE - 4) {
        jsonBuf[pos++] = '\\';
        jsonBuf[pos++] = '"';
      }
    } else if (c == '\\') {
      if (pos + 1 < JSON_BUF_SIZE - 4) {
        jsonBuf[pos++] = '\\';
        jsonBuf[pos++] = '\\';
      }
    } else if (c == '\n') {
      if (pos + 1 < JSON_BUF_SIZE - 4) {
        jsonBuf[pos++] = '\\';
        jsonBuf[pos++] = 'n';
      }
    } else if (c == '\r') {
      if (pos + 1 < JSON_BUF_SIZE - 4) {
        jsonBuf[pos++] = '\\';
        jsonBuf[pos++] = 'r';
      }
    } else if (c == '\t') {
      if (pos + 1 < JSON_BUF_SIZE - 4) {
        jsonBuf[pos++] = '\\';
        jsonBuf[pos++] = 't';
      }
    } else if (c < 0x20) {
      // Skip control characters
    } else {
      jsonBuf[pos++] = c;
    }
  }
  jsonBuf[pos++] = '"';
  jsonBuf[pos++] = '}';
  jsonBuf[pos] = '\0';

  webSocket->textAll(jsonBuf);
}

// --- Convenience methods ---

void Logger::print(Level level, char c) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::print(c);
}

void Logger::print(Level level, const String& str) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::print(str);
}

void Logger::print(Level level, const char* str) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::print(str);
}

void Logger::println(Level level, const String& str) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::println(str);
  flush();
}

void Logger::println(Level level, const char* str) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::println(str);
  flush();
}

void Logger::println(Level level) {
  if (level > maxLevel) return;
  if (!messageActive) {
    messageActive = true;
    currentMessageLevel = level;
    buffer = "";
  }
  Print::println();
  flush();
}

void Logger::printf(Level level, const char* format, ...) {
  if (level > maxLevel) return;

  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  messageActive = true;
  currentMessageLevel = level;
  buffer = "";

  Print::print(buf);
  flush();
}