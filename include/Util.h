#pragma once

#include <Print.h>

#define lowWord(ul) (static_cast<uint16_t>((ul) & 0xffff))
#define highWord(ul) (static_cast<uint16_t>((ul) >> 16))

#define arrayLength(array) (sizeof(array) / sizeof(array[0]))

#if 0
#define htons(x) ((((x) << 8) & 0xFF00) | (((x) >> 8) & 0xFF))
#define ntohs(x) htons(x)

#define htonl(x)                                            \
  (((x) << 24 & 0xFF000000UL) | ((x) << 8 & 0x00FF0000UL) | \
   ((x) >> 8 & 0x0000FF00UL) | ((x) >> 24 & 0x000000FFUL))
#define ntohl(x) htonl(x)

inline int8_t hton(int8_t v) { return v; }
inline int16_t hton(int16_t v) { return htons(v); }
inline int32_t hton(int32_t v) { return htonl(v); }

inline uint8_t hton(uint8_t v) { return v; }
inline uint16_t hton(uint16_t v) { return htons(v); }
inline uint32_t hton(uint32_t v) { return htonl(v); }

inline int8_t ntoh(int8_t v) { return v; }
inline int16_t ntoh(int16_t v) { return ntohs(v); }
inline int32_t ntoh(int32_t v) { return ntohl(v); }

inline uint8_t ntoh(uint8_t v) { return v; }
inline uint16_t ntoh(uint16_t v) { return ntohs(v); }
inline uint32_t ntoh(uint32_t v) { return ntohl(v); }
#endif

void printMillis(Print& printer);
void printUptime(Print& printer);
void printVersion(Print& printer, uint8_t major, uint8_t minor, uint8_t patch);
void printHex(Print& printer, uint8_t value, bool prefix = false);
void printHex(Print& printer, uint16_t value, bool prefix = false);
void printHex(Print& printer, uint32_t value, bool prefix = false);

void printArray(Print& printer, const uint8_t* buf, size_t len, int base = DEC,
                bool prefix = false);

namespace Util {

constexpr uint16_t ONE_HOUR_IN_SECONDS = 1u * 60u * 60u;
constexpr uint16_t ONE_MINUTE_IN_MILLISECONDS = 60u * 1000u;
constexpr uint16_t TWELVE_HOURS_IN_SECONDS = 12u * 60u * 60u;

}  // namespace Util
