#include <unity.h>

#include <cstring>
#include <stdint.h>

#include "IoTProtocol.h"

namespace {

IoTPacket makePacket() {
    IoTPacket packet{};
    packet.version = IOT_PROTOCOL_VER;
    packet.type = MsgType::HELLO;
    packet.src = 0x12;
    packet.dst = IOT_DEVICE_CENTRAL;
    packet.bootId = 0xABCD;
    packet.seq = 0x10203040;
    packet.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    packet.clearPayload();
    packet.addTLV_uint8(TlvTag::DEVICE_TYPE_TAG, 0x02);
    packet.addTLV_string(TlvTag::DEVICE_NAME, "PIR-SALA");
    packet.addTLV_int16(TlvTag::TEMPERATURE, -123);
    packet.addTLV_uint32(TlvTag::UPTIME_SEC, 0x01020304);
    return packet;
}

void assertPacketHeaderEqual(const IoTPacket& expected, const IoTPacket& actual) {
    TEST_ASSERT_EQUAL_UINT8(expected.version, actual.version);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected.type), static_cast<uint8_t>(actual.type));
    TEST_ASSERT_EQUAL_UINT8(expected.src, actual.src);
    TEST_ASSERT_EQUAL_UINT8(expected.dst, actual.dst);
    TEST_ASSERT_EQUAL_UINT16(expected.bootId, actual.bootId);
    TEST_ASSERT_EQUAL_UINT32(expected.seq, actual.seq);
    TEST_ASSERT_EQUAL_UINT8(expected.flags, actual.flags);
    TEST_ASSERT_EQUAL_UINT8(expected.payloadLen, actual.payloadLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.payload, actual.payload, expected.payloadLen);
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_crc16_known_vector() {
    const uint8_t input[] = "123456789";
    TEST_ASSERT_EQUAL_UINT16(0x29B1, iot_crc16(input, 9));
}

void test_tlv_roundtrip_and_big_endian_values() {
    IoTPacket packet{};
    packet.clearPayload();

    TEST_ASSERT_TRUE(packet.addTLV_uint8(TlvTag::EVENT_VALUE, 0x7F));
    TEST_ASSERT_TRUE(packet.addTLV_int8(TlvTag::RSSI_VAL, -58));
    TEST_ASSERT_TRUE(packet.addTLV_uint16(TlvTag::BATTERY_MV, 0x1234));
    TEST_ASSERT_TRUE(packet.addTLV_int16(TlvTag::TEMPERATURE, -123));
    TEST_ASSERT_TRUE(packet.addTLV_uint32(TlvTag::UPTIME_SEC, 0x01020304));
    TEST_ASSERT_TRUE(packet.addTLV_string(TlvTag::DEVICE_NAME, "PIR-SALA"));

    uint8_t u8 = 0;
    int8_t i8 = 0;
    uint16_t u16 = 0;
    int16_t i16 = 0;
    uint32_t u32 = 0;
    char name[16] = {};

    TEST_ASSERT_TRUE(packet.hasTLV(TlvTag::EVENT_VALUE));
    TEST_ASSERT_TRUE(packet.getTLV_uint8(TlvTag::EVENT_VALUE, u8));
    TEST_ASSERT_EQUAL_UINT8(0x7F, u8);
    TEST_ASSERT_TRUE(packet.getTLV_int8(TlvTag::RSSI_VAL, i8));
    TEST_ASSERT_EQUAL_INT8(-58, i8);
    TEST_ASSERT_TRUE(packet.getTLV_uint16(TlvTag::BATTERY_MV, u16));
    TEST_ASSERT_EQUAL_UINT16(0x1234, u16);
    TEST_ASSERT_TRUE(packet.getTLV_int16(TlvTag::TEMPERATURE, i16));
    TEST_ASSERT_EQUAL_INT16(-123, i16);
    TEST_ASSERT_TRUE(packet.getTLV_uint32(TlvTag::UPTIME_SEC, u32));
    TEST_ASSERT_EQUAL_UINT32(0x01020304, u32);
    TEST_ASSERT_TRUE(packet.getTLV_string(TlvTag::DEVICE_NAME, name, sizeof(name)));
    TEST_ASSERT_EQUAL_STRING("PIR-SALA", name);
}

void test_tlv_validation_rejects_truncated_values() {
    const uint8_t truncated[] = {static_cast<uint8_t>(TlvTag::TEMPERATURE), 2, 0x01};
    TEST_ASSERT_FALSE(iot_validate_tlv(truncated, sizeof(truncated)));

    IoTPacket packet{};
    packet.payloadLen = sizeof(truncated);
    memcpy(packet.payload, truncated, sizeof(truncated));

    uint16_t value = 0;
    TEST_ASSERT_FALSE(packet.getTLV_uint16(TlvTag::TEMPERATURE, value));
}

void test_tlv_getters_require_exact_lengths() {
    IoTPacket packet{};
    packet.payloadLen = 3;
    packet.payload[0] = static_cast<uint8_t>(TlvTag::EVENT_VALUE);
    packet.payload[1] = 1;
    packet.payload[2] = 0x01;

    uint8_t u8 = 0;
    uint16_t u16 = 0;
    TEST_ASSERT_TRUE(packet.getTLV_uint8(TlvTag::EVENT_VALUE, u8));
    TEST_ASSERT_FALSE(packet.getTLV_uint16(TlvTag::EVENT_VALUE, u16));
}

void test_packet_serialize_deserialize_roundtrip() {
    const IoTPacket original = makePacket();
    uint8_t wire[IOT_MAX_PACKET] = {};
    IoTPacket decoded{};

    const size_t length = iot_serialize(original, wire, sizeof(wire));
    TEST_ASSERT_EQUAL_UINT16(IOT_HEADER_SIZE + original.payloadLen + IOT_CRC_SIZE, length);
    TEST_ASSERT_EQUAL_UINT8(IOT_MAGIC_0, wire[0]);
    TEST_ASSERT_EQUAL_UINT8(IOT_MAGIC_1, wire[1]);
    TEST_ASSERT_EQUAL_UINT16(original.bootId, (static_cast<uint16_t>(wire[6]) << 8) | wire[7]);
    TEST_ASSERT_EQUAL_UINT32(original.seq,
                             (static_cast<uint32_t>(wire[8]) << 24) |
                             (static_cast<uint32_t>(wire[9]) << 16) |
                             (static_cast<uint32_t>(wire[10]) << 8) |
                             wire[11]);

    TEST_ASSERT_TRUE(iot_deserialize(wire, length, decoded));
    assertPacketHeaderEqual(original, decoded);
}

void test_deserialize_rejects_corrupt_wire() {
    const IoTPacket original = makePacket();
    uint8_t wire[IOT_MAX_PACKET] = {};
    const size_t length = iot_serialize(original, wire, sizeof(wire));

    uint8_t corrupted[IOT_MAX_PACKET] = {};
    IoTPacket decoded{};

    memcpy(corrupted, wire, length);
    corrupted[0] ^= 0x01;
    TEST_ASSERT_FALSE(iot_deserialize(corrupted, length, decoded));

    memcpy(corrupted, wire, length);
    corrupted[length - 1] ^= 0x01;
    TEST_ASSERT_FALSE(iot_deserialize(corrupted, length, decoded));

    memcpy(corrupted, wire, length);
    corrupted[2] = 0x51;
    TEST_ASSERT_FALSE(iot_deserialize(corrupted, length, decoded));

    TEST_ASSERT_FALSE(iot_deserialize(wire, length + 1, decoded));

    memcpy(corrupted, wire, length);
    corrupted[13] = original.payloadLen + 1;
    TEST_ASSERT_FALSE(iot_deserialize(corrupted, length, decoded));
}

void test_serialize_rejects_small_output_buffer() {
    const IoTPacket packet = makePacket();
    uint8_t wire[IOT_MAX_PACKET] = {};
    const size_t expectedLength = IOT_HEADER_SIZE + packet.payloadLen + IOT_CRC_SIZE;

    TEST_ASSERT_TRUE(expectedLength < sizeof(wire));
    TEST_ASSERT_EQUAL_UINT32(0, iot_serialize(packet, wire, expectedLength - 1));
}

void test_payload_limit_is_64_bytes_without_silent_truncation() {
    IoTPacket packet{};
    packet.version = IOT_PROTOCOL_VER;
    packet.clearPayload();

    char text[63] = {};
    for (size_t i = 0; i < 62; ++i) text[i] = 'x';

    TEST_ASSERT_TRUE(packet.addTLV_string(TlvTag::ERROR_DETAIL, text));
    TEST_ASSERT_EQUAL_UINT8(IOT_MAX_PAYLOAD, packet.payloadLen);
    TEST_ASSERT_FALSE(packet.addTLV_uint8(TlvTag::EVENT_VALUE, 1));

    uint8_t wire[IOT_MAX_PACKET] = {};
    IoTPacket decoded{};
    const size_t length = iot_serialize(packet, wire, sizeof(wire));
    TEST_ASSERT_EQUAL_UINT16(IOT_MAX_PACKET, length);
    TEST_ASSERT_TRUE(iot_deserialize(wire, length, decoded));
}

void test_priority_helpers() {
    IoTPacket packet{};

    packet.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    TEST_ASSERT_TRUE(packet.needsAck());
    TEST_ASSERT_TRUE(packet.isReliable());
    TEST_ASSERT_FALSE(packet.isUrgent());
    TEST_ASSERT_FALSE(packet.isBackground());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Priority::NORMAL), static_cast<uint8_t>(packet.priority()));

    packet.flags = IOT_FLAG_URGENT | IOT_FLAG_BACKGROUND;
    TEST_ASSERT_TRUE(packet.isUrgent());
    TEST_ASSERT_TRUE(packet.isBackground());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Priority::URGENT), static_cast<uint8_t>(packet.priority()));
}

void test_minor_version_is_accepted_when_major_matches() {
    IoTPacket packet = makePacket();
    packet.version = 0x42;
    uint8_t wire[IOT_MAX_PACKET] = {};
    IoTPacket decoded{};

    const size_t length = iot_serialize(packet, wire, sizeof(wire));
    TEST_ASSERT_TRUE(iot_deserialize(wire, length, decoded));
    TEST_ASSERT_EQUAL_UINT8(0x42, decoded.version);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_crc16_known_vector);
    RUN_TEST(test_tlv_roundtrip_and_big_endian_values);
    RUN_TEST(test_tlv_validation_rejects_truncated_values);
    RUN_TEST(test_tlv_getters_require_exact_lengths);
    RUN_TEST(test_packet_serialize_deserialize_roundtrip);
    RUN_TEST(test_deserialize_rejects_corrupt_wire);
    RUN_TEST(test_serialize_rejects_small_output_buffer);
    RUN_TEST(test_payload_limit_is_64_bytes_without_silent_truncation);
    RUN_TEST(test_priority_helpers);
    RUN_TEST(test_minor_version_is_accepted_when_major_matches);
    return UNITY_END();
}
