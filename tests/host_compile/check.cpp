#include "AlarmProfile.h"
#include "IoTNode.h"
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

bool check_profile_conversions() {
    return expect(
               AlarmProfile::toWire(AlarmProfile::EventCode::MOTION) == 0x01,
               "MOTION debe conservar el valor wire 0x01") &&
           expect(
               AlarmProfile::toWire(AlarmProfile::EventCode::TIMBRE) == 0x06,
               "TIMBRE debe conservar el valor wire 0x06") &&
           expect(
               AlarmProfile::toWire(AlarmProfile::DeviceType::PIR_SENSOR) == 0x02,
               "PIR_SENSOR debe conservar el valor wire 0x02") &&
           expect(
               static_cast<uint8_t>(AlarmProfile::toCoreTlvTag(
                   AlarmProfile::StateTag::STATE_MOTION)) == 0xA0,
               "STATE_MOTION debe conservar el tag wire 0xA0");
}

bool check_packet_round_trip() {
    IoTPacket packet{};
    packet.version = IOT_PROTOCOL_VER;
    packet.type = MsgType::EVENT;
    packet.src = 0x02;
    packet.dst = IOT_DEVICE_CENTRAL;
    packet.bootId = 0x1234;
    packet.seq = 0x01020304;
    packet.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    packet.clearPayload();

    if (!expect(packet.addTLV_uint8(
                    TlvTag::EVENT_TYPE,
                    AlarmProfile::toWire(AlarmProfile::EventCode::MOTION)),
                "no se pudo agregar EVENT_TYPE")) {
        return false;
    }
    if (!expect(packet.addTLV_uint8(
                    AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_MOTION),
                    1),
                "no se pudo agregar STATE_MOTION")) {
        return false;
    }

    uint8_t wire[IOT_MAX_PACKET]{};
    const std::size_t wireLength = iot_serialize(packet, wire, sizeof(wire));
    if (!expect(wireLength == static_cast<std::size_t>(IOT_HEADER_SIZE +
                                                         packet.payloadLen +
                                                         IOT_CRC_SIZE),
                "longitud serializada inesperada")) {
        return false;
    }

    IoTPacket decoded{};
    if (!expect(iot_deserialize(wire, wireLength, decoded),
                "el paquete válido no se pudo deserializar")) {
        return false;
    }

    if (!expect(decoded.version == packet.version && decoded.type == packet.type &&
                    decoded.src == packet.src && decoded.dst == packet.dst &&
                    decoded.bootId == packet.bootId && decoded.seq == packet.seq &&
                    decoded.flags == packet.flags,
                "la cabecera no sobrevivió al round-trip")) {
        return false;
    }

    uint8_t eventCode = 0;
    if (!expect(decoded.getTLV_uint8(TlvTag::EVENT_TYPE, eventCode),
                "no se pudo leer EVENT_TYPE")) {
        return false;
    }
    if (!expect(eventCode == AlarmProfile::toWire(AlarmProfile::EventCode::MOTION),
                "EVENT_TYPE perdió su valor wire")) {
        return false;
    }

    uint8_t motionState = 0;
    if (!expect(decoded.getTLV_uint8(
                    AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_MOTION),
                    motionState),
                "no se pudo leer STATE_MOTION")) {
        return false;
    }
    if (!expect(motionState == 1, "STATE_MOTION perdió su valor")) {
        return false;
    }

    uint8_t corrupted[IOT_MAX_PACKET]{};
    std::memcpy(corrupted, wire, wireLength);
    corrupted[wireLength - 1] ^= 0x01;
    if (!expect(!iot_deserialize(corrupted, wireLength, decoded),
                "un CRC alterado fue aceptado")) {
        return false;
    }

    return expect(!iot_deserialize(wire, wireLength - 1, decoded),
                   "un paquete truncado fue aceptado");
}

bool check_node_api() {
    IoTNode node(0x02, 4210);
    node.begin(1234);

    if (!expect(node.getBootId() == 1234, "IoTNode no conservó BOOT_ID")) {
        return false;
    }

    IPAddress central(192, 168, 0, 201);

    node.sendHello(central, 4210,
                   AlarmProfile::toWire(AlarmProfile::DeviceType::PIR_SENSOR),
                   "PIR Entrada");
    if (!expect(node.queuedCount() == 1,
                "sendHello no encoló el paquete reliable")) {
        return false;
    }

    if (!expect(node.sendEvent(
                    AlarmProfile::toWire(AlarmProfile::EventCode::MOTION),
                    central, 4210),
                "sendEvent(MOTION) rechazó el evento")) {
        return false;
    }
    if (!expect(node.sendEvent(
                    AlarmProfile::toWire(AlarmProfile::EventCode::TIMBRE),
                    central, 4210),
                "sendEvent(TIMBRE) rechazó el evento")) {
        return false;
    }
    if (!expect(node.queuedCount() == 3,
                "la cola no contiene HELLO y los dos eventos")) {
        return false;
    }

    // El primer loop toma el primer reliable de la cola; el segundo ejercita
    // la transmisión a través del stub WiFi/UDP y deja el ACK pendiente.
    node.loop();
    if (!expect(node.isReliableInFlight(),
                "IoTNode no activó el canal reliable")) {
        return false;
    }
    node.loop();

    return expect(node.getStats().txReliable == 1,
                  "IoTNode no registró el reliable en vuelo");
}

}  // namespace

int main() {
    if (!check_profile_conversions()) return 1;
    if (!check_packet_round_trip()) return 2;
    if (!check_node_api()) return 3;

    std::puts("OK: host compile check wifi_PIR");
    return 0;
}
