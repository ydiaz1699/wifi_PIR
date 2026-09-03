#include "IoTAuth.h"
#include "IoTProtocol.h"

#include <cstdio>
#include <cstring>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        return false;
    }
    return true;
}

bool check_known_hmac_vector() {
    const uint8_t key[] = "known-vector-key";
    IoTPacket packet{};
    packet.version = IOT_PROTOCOL_VER;
    packet.type = MsgType::EVENT;
    packet.src = 0x02;
    packet.dst = IOT_DEVICE_CENTRAL;
    packet.bootId = 0x1234;
    packet.seq = 0x01020304;
    packet.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    packet.clearPayload();
    packet.addTLV_uint8(TlvTag::EVENT_TYPE, 0x06);
    packet.addTLV_uint8(TlvTag::EVENT_VALUE, 1);

    const uint8_t payloadLength = packet.payloadLen;
    // Vector independiente calculado con Python:
    // HMAC-SHA256("known-vector-key", header || payload)[:4] = 7b5f5cee.
    const uint8_t expected[IOT_HMAC_TRUNC_SIZE] = {
        0x7B, 0x5F, 0x5C, 0xEE,
    };

    IoTAuth auth(key, sizeof(key) - 1);
    if (!expect(auth.signPacket(packet), "IoTAuth no pudo firmar vector conocido")) {
        return false;
    }
    if (!expect(packet.payloadLen == payloadLength + 2 + IOT_HMAC_TRUNC_SIZE,
                "el vector firmado tiene longitud inesperada")) {
        return false;
    }

    const uint8_t* received = packet.payload + payloadLength + 2;
    if (!expect(std::memcmp(received, expected, IOT_HMAC_TRUNC_SIZE) == 0,
                "_computeHmac no coincide con HMAC-SHA256 independiente")) {
        return false;
    }

    auth.setRequired(true);
    return expect(auth.verifyPacket(packet),
                  "el vector HMAC firmado no pudo verificarse");
}

}  // namespace

int main() {
    if (!check_known_hmac_vector()) return 1;
    std::puts("OK: HMAC vector check wifi_PIR");
    return 0;
}
