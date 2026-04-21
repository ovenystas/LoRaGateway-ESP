#include <unity.h>

#include <cmath>

#include "../include/Format.h"

void setUp(void) {
  // This is run before each test
}

void tearDown(void) {
  // This is run after each test
}

// Test toByte and fromByte conversions
void test_format_byte_conversion_unsigned_1byte_0decimals(void) {
  Format fmt(false, 0, 0);
  uint8_t byte = fmt.toByte();

  // Bit 4: 0 (unsigned)
  // Bits 3-2: 00 (0 = 1 byte)
  // Bits 1-0: 00 (0 decimals)
  // Expected: 0b00000000 = 0x00
  TEST_ASSERT_EQUAL_UINT8(0x00, byte);

  Format fmt2(byte);
  TEST_ASSERT_FALSE(fmt2.getIsSigned());
  TEST_ASSERT_EQUAL_UINT8(0, fmt2.getSize());
  TEST_ASSERT_EQUAL_UINT8(0, fmt2.getPrecision());
}

void test_format_byte_conversion_signed_2byte_2decimals(void) {
  Format fmt(true, 1, 2);
  uint8_t byte = fmt.toByte();

  // Bit 4: 1 (signed)
  // Bits 3-2: 01 (1 = 2 bytes)
  // Bits 1-0: 10 (2 decimals)
  // Expected: 0b00010110 = 0x16 = 22
  TEST_ASSERT_EQUAL_UINT8(0x16, byte);

  Format fmt2(byte);
  TEST_ASSERT_TRUE(fmt2.getIsSigned());
  TEST_ASSERT_EQUAL_UINT8(1, fmt2.getSize());
  TEST_ASSERT_EQUAL_UINT8(2, fmt2.getPrecision());
}

void test_format_byte_conversion_signed_4byte_3decimals(void) {
  Format fmt(true, 2, 3);
  uint8_t byte = fmt.toByte();

  // Bit 4: 1 (signed)
  // Bits 3-2: 10 (2 = 4 bytes)
  // Bits 1-0: 11 (3 decimals)
  // isSigned: 0x10, size shift: (2<<2)=0x08, precision: 0x03
  // Expected: 0x10 | 0x08 | 0x03 = 0x1B = 27
  TEST_ASSERT_EQUAL_UINT8(27, byte);

  Format fmt2(byte);
  TEST_ASSERT_TRUE(fmt2.getIsSigned());
  TEST_ASSERT_EQUAL_UINT8(2, fmt2.getSize());
  TEST_ASSERT_EQUAL_UINT8(3, fmt2.getPrecision());
}

// Test fromRawValue for unsigned values
void test_fromrawvalue_unsigned_no_precision(void) {
  Format fmt(false, 0, 0);  // unsigned, 1 byte, 0 decimals
  float result = fmt.fromRawValue(100);
  TEST_ASSERT_EQUAL_FLOAT(100.0f, result);
}

void test_fromrawvalue_unsigned_2decimals(void) {
  Format fmt(false, 0, 2);  // unsigned, 1 byte, 2 decimals
  float result = fmt.fromRawValue(1234);
  TEST_ASSERT_EQUAL_FLOAT(12.34f, result);
}

void test_fromrawvalue_unsigned_3decimals(void) {
  Format fmt(false, 1, 3);  // unsigned, 2 bytes, 3 decimals
  float result = fmt.fromRawValue(45678);
  TEST_ASSERT_EQUAL_FLOAT(45.678f, result);
}

// Test fromRawValue for signed values (1 byte)
void test_fromrawvalue_signed_1byte_positive(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte, 0 decimals
  // Positive value: 50
  float result = fmt.fromRawValue(50);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, result);
}

void test_fromrawvalue_signed_1byte_negative(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte, 0 decimals
  // Negative value: (uint32_t)-50 = 0xFFFFFFCE, but only lower 8 bits matter
  // 0xCE = 206, cast to int8_t = -50
  uint32_t negValue = (uint32_t)(int8_t)(-50);
  float result = fmt.fromRawValue(negValue);
  TEST_ASSERT_EQUAL_FLOAT(-50.0f, result);
}

void test_fromrawvalue_signed_1byte_2decimals(void) {
  Format fmt(true, 0, 2);  // signed, 1 byte, 2 decimals
  // Value: -10 represented as (uint32_t)(int8_t)(-10)
  uint32_t value = (uint32_t)(int8_t)(-10);
  float result = fmt.fromRawValue(value);
  TEST_ASSERT_EQUAL_FLOAT(-0.10f, result);
}

// Test fromRawValue for signed values (2 bytes)
void test_fromrawvalue_signed_2byte_positive(void) {
  Format fmt(true, 1, 0);  // signed, 2 bytes, 0 decimals
  float result = fmt.fromRawValue(10000);
  TEST_ASSERT_EQUAL_FLOAT(10000.0f, result);
}

void test_fromrawvalue_signed_2byte_negative(void) {
  Format fmt(true, 1, 0);  // signed, 2 bytes, 0 decimals
  // -32768 as uint32_t
  uint32_t value = (uint32_t)(int16_t)(-32768);
  float result = fmt.fromRawValue(value);
  TEST_ASSERT_EQUAL_FLOAT(-32768.0f, result);
}

void test_fromrawvalue_signed_2byte_3decimals(void) {
  Format fmt(true, 1, 3);  // signed, 2 bytes, 3 decimals
  // 12345 / 1000 = 12.345
  float result = fmt.fromRawValue(12345);
  TEST_ASSERT_EQUAL_FLOAT(12.345f, result);
}

// Test fromRawValue for signed values (4 bytes)
void test_fromrawvalue_signed_4byte_positive(void) {
  Format fmt(true, 2, 0);  // signed, 4 bytes, 0 decimals
  float result = fmt.fromRawValue(1000000);
  TEST_ASSERT_EQUAL_FLOAT(1000000.0f, result);
}

void test_fromrawvalue_signed_4byte_negative(void) {
  Format fmt(true, 2, 0);  // signed, 4 bytes, 0 decimals
  // -1000000 as uint32_t
  uint32_t value = (uint32_t)(int32_t)(-1000000);
  float result = fmt.fromRawValue(value);
  TEST_ASSERT_EQUAL_FLOAT(-1000000.0f, result);
}

// Test toRawValue (inverse of scaleValue)
void test_torawvalue_unsigned_no_precision(void) {
  Format fmt(false, 0, 0);  // unsigned, 1 byte, 0 decimals
  uint32_t result = fmt.toRawValue(100.0f);
  TEST_ASSERT_EQUAL_UINT32(100, result);
}

void test_torawvalue_unsigned_2decimals(void) {
  Format fmt(false, 0, 2);  // unsigned, 1 byte, 2 decimals
  uint32_t result = fmt.toRawValue(12.34f);
  TEST_ASSERT_EQUAL_UINT32(1234, result);
}

void test_torawvalue_unsigned_3decimals(void) {
  Format fmt(false, 1, 3);  // unsigned, 2 bytes, 3 decimals
  uint32_t result = fmt.toRawValue(45.678f);
  TEST_ASSERT_EQUAL_UINT32(45678, result);
}

void test_torawvalue_signed_1byte(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte, 0 decimals
  uint32_t result = fmt.toRawValue(-50.0f);
  // -50 as uint32_t (lower 8 bits)
  uint32_t expected = (uint32_t)(int8_t)(-50);
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

void test_torawvalue_signed_2byte_3decimals(void) {
  Format fmt(true, 1, 3);  // signed, 2 bytes, 3 decimals
  uint32_t result = fmt.toRawValue(12.345f);
  TEST_ASSERT_EQUAL_UINT32(12345, result);
}

// Test round-trip conversion (toRawValue -> fromRawValue)
void test_roundtrip_unsigned_1byte_2decimals(void) {
  Format fmt(false, 0, 2);
  float original = 2.55f;
  uint32_t raw = fmt.toRawValue(original);
  float recovered = fmt.fromRawValue(raw);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, original, recovered);
}

void test_roundtrip_signed_1byte(void) {
  Format fmt(true, 0, 1);  // signed, 1 byte, 1 decimal
  float original = -1.2f;
  uint32_t raw = fmt.toRawValue(original);
  float recovered = fmt.fromRawValue(raw);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, original, recovered);
}

void test_roundtrip_signed_4byte_2decimals(void) {
  Format fmt(true, 2, 2);  // signed, 4 bytes, 2 decimals
  float original = -12345.67f;
  uint32_t raw = fmt.toRawValue(original);
  float recovered = fmt.fromRawValue(raw);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, original, recovered);
}

// Test isValidRawValue - unsigned
void test_isvalidrawvalue_unsigned_1byte_valid_min(void) {
  Format fmt(false, 0, 0);  // unsigned, 1 byte
  TEST_ASSERT_TRUE(fmt.isValidRawValue(0));
}

void test_isvalidrawvalue_unsigned_1byte_valid_max(void) {
  Format fmt(false, 0, 0);  // unsigned, 1 byte
  TEST_ASSERT_TRUE(fmt.isValidRawValue(255));
}

void test_isvalidrawvalue_unsigned_1byte_invalid(void) {
  Format fmt(false, 0, 0);  // unsigned, 1 byte
  TEST_ASSERT_FALSE(fmt.isValidRawValue(256));
}

void test_isvalidrawvalue_unsigned_2byte_valid_max(void) {
  Format fmt(false, 1, 0);  // unsigned, 2 bytes
  TEST_ASSERT_TRUE(fmt.isValidRawValue(65535));
}

void test_isvalidrawvalue_unsigned_2byte_invalid(void) {
  Format fmt(false, 1, 0);  // unsigned, 2 bytes
  TEST_ASSERT_FALSE(fmt.isValidRawValue(65536));
}

void test_isvalidrawvalue_unsigned_4byte_valid(void) {
  Format fmt(false, 2, 0);  // unsigned, 4 bytes
  TEST_ASSERT_TRUE(fmt.isValidRawValue(4294967295U));
}

// Test isValidRawValue - signed
void test_isvalidrawvalue_signed_1byte_valid_positive_min(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte
  TEST_ASSERT_TRUE(fmt.isValidRawValue(0));
}

void test_isvalidrawvalue_signed_1byte_valid_positive_max(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte
  TEST_ASSERT_TRUE(fmt.isValidRawValue(127));
}

void test_isvalidrawvalue_signed_1byte_valid_negative_min(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte (-128 to 127)
  uint32_t value = (uint32_t)(int8_t)(-128);
  TEST_ASSERT_TRUE(fmt.isValidRawValue(value));
}

void test_isvalidrawvalue_signed_1byte_invalid_too_positive(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte
  TEST_ASSERT_FALSE(fmt.isValidRawValue(128));
}

void test_isvalidrawvalue_signed_1byte_invalid_too_negative(void) {
  Format fmt(true, 0, 0);  // signed, 1 byte
  // For signed 1 byte, only -128 to 127 are valid
  // 256 as uint32_t won't map back to the same value
  TEST_ASSERT_FALSE(fmt.isValidRawValue(256));
}

void test_isvalidrawvalue_signed_2byte_valid_positive_max(void) {
  Format fmt(true, 1, 0);  // signed, 2 bytes
  TEST_ASSERT_TRUE(fmt.isValidRawValue(32767));
}

void test_isvalidrawvalue_signed_2byte_valid_negative_min(void) {
  Format fmt(true, 1, 0);  // signed, 2 bytes
  uint32_t value = (uint32_t)(int16_t)(-32768);
  TEST_ASSERT_TRUE(fmt.isValidRawValue(value));
}

void test_isvalidrawvalue_signed_2byte_invalid_too_positive(void) {
  Format fmt(true, 1, 0);  // signed, 2 bytes
  TEST_ASSERT_FALSE(fmt.isValidRawValue(32768));
}

void test_isvalidrawvalue_signed_4byte_valid_positive(void) {
  Format fmt(true, 2, 0);                             // signed, 4 bytes
  TEST_ASSERT_TRUE(fmt.isValidRawValue(2147483647));  // INT32_MAX
}

void test_isvalidrawvalue_signed_4byte_valid_negative(void) {
  Format fmt(true, 2, 0);                                 // signed, 4 bytes
  uint32_t value = (uint32_t)(int32_t)(-2147483647 - 1);  // INT32_MIN
  TEST_ASSERT_TRUE(fmt.isValidRawValue(value));
}

// Integration tests
void test_format_mqtt_string_to_lora_raw_positive_unsigned(void) {
  // MQTT sends "2.35", Format has 2 decimals, unsigned 1 byte (max 255)
  Format fmt(false, 0, 2);

  // Parse as float
  float mqttValue = 2.35f;

  // Convert to raw
  uint32_t rawValue = fmt.toRawValue(mqttValue);
  TEST_ASSERT_EQUAL_UINT32(235, rawValue);

  // Validate (235 fits in 1 byte)
  TEST_ASSERT_TRUE(fmt.isValidRawValue(rawValue));

  // Round-trip
  float recovered = fmt.fromRawValue(rawValue);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, mqttValue, recovered);
}

void test_format_mqtt_string_to_lora_raw_negative_signed(void) {
  // MQTT sends "-23.45", Format has 2 decimals, signed 2 bytes
  Format fmt(true, 1, 2);

  // Parse as float
  float mqttValue = -23.45f;

  // Convert to raw
  uint32_t rawValue = fmt.toRawValue(mqttValue);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)(int16_t)(-2345), rawValue);

  // Validate
  TEST_ASSERT_TRUE(fmt.isValidRawValue(rawValue));

  // Round-trip
  float recovered = fmt.fromRawValue(rawValue);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, mqttValue, recovered);
}

void setup() { UNITY_BEGIN(); }

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Byte conversion tests
  RUN_TEST(test_format_byte_conversion_unsigned_1byte_0decimals);
  RUN_TEST(test_format_byte_conversion_signed_2byte_2decimals);
  RUN_TEST(test_format_byte_conversion_signed_4byte_3decimals);

  // fromRawValue tests - unsigned
  RUN_TEST(test_fromrawvalue_unsigned_no_precision);
  RUN_TEST(test_fromrawvalue_unsigned_2decimals);
  RUN_TEST(test_fromrawvalue_unsigned_3decimals);

  // fromRawValue tests - signed 1 byte
  RUN_TEST(test_fromrawvalue_signed_1byte_positive);
  RUN_TEST(test_fromrawvalue_signed_1byte_negative);
  RUN_TEST(test_fromrawvalue_signed_1byte_2decimals);

  // fromRawValue tests - signed 2 bytes
  RUN_TEST(test_fromrawvalue_signed_2byte_positive);
  RUN_TEST(test_fromrawvalue_signed_2byte_negative);
  RUN_TEST(test_fromrawvalue_signed_2byte_3decimals);

  // fromRawValue tests - signed 4 bytes
  RUN_TEST(test_fromrawvalue_signed_4byte_positive);
  RUN_TEST(test_fromrawvalue_signed_4byte_negative);

  // toRawValue tests
  RUN_TEST(test_torawvalue_unsigned_no_precision);
  RUN_TEST(test_torawvalue_unsigned_2decimals);
  RUN_TEST(test_torawvalue_unsigned_3decimals);
  RUN_TEST(test_torawvalue_signed_1byte);
  RUN_TEST(test_torawvalue_signed_2byte_3decimals);

  // Round-trip tests
  RUN_TEST(test_roundtrip_unsigned_1byte_2decimals);
  RUN_TEST(test_roundtrip_signed_1byte);
  RUN_TEST(test_roundtrip_signed_4byte_2decimals);

  // isValidRawValue tests - unsigned
  RUN_TEST(test_isvalidrawvalue_unsigned_1byte_valid_min);
  RUN_TEST(test_isvalidrawvalue_unsigned_1byte_valid_max);
  RUN_TEST(test_isvalidrawvalue_unsigned_1byte_invalid);
  RUN_TEST(test_isvalidrawvalue_unsigned_2byte_valid_max);
  RUN_TEST(test_isvalidrawvalue_unsigned_2byte_invalid);
  RUN_TEST(test_isvalidrawvalue_unsigned_4byte_valid);

  // isValidRawValue tests - signed
  RUN_TEST(test_isvalidrawvalue_signed_1byte_valid_positive_min);
  RUN_TEST(test_isvalidrawvalue_signed_1byte_valid_positive_max);
  RUN_TEST(test_isvalidrawvalue_signed_1byte_valid_negative_min);
  RUN_TEST(test_isvalidrawvalue_signed_1byte_invalid_too_positive);
  RUN_TEST(test_isvalidrawvalue_signed_1byte_invalid_too_negative);
  RUN_TEST(test_isvalidrawvalue_signed_2byte_valid_positive_max);
  RUN_TEST(test_isvalidrawvalue_signed_2byte_valid_negative_min);
  RUN_TEST(test_isvalidrawvalue_signed_2byte_invalid_too_positive);
  RUN_TEST(test_isvalidrawvalue_signed_4byte_valid_positive);
  RUN_TEST(test_isvalidrawvalue_signed_4byte_valid_negative);

  // Integration tests
  RUN_TEST(test_format_mqtt_string_to_lora_raw_positive_unsigned);
  RUN_TEST(test_format_mqtt_string_to_lora_raw_negative_signed);

  return UNITY_END();
}
