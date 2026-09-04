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

IoTPacket makePacketFrom(uint8_t type, uint8_t src, uint8_t dst,
                         uint16_t bootId, uint32_t seq, uint8_t flags) {
    IoTPacket packet{};
    packet.version = IOT_PROTOCOL_VER;
    packet.type = static_cast<MsgType>(type);
    packet.src = src;
    packet.dst = dst;
    packet.bootId = bootId;
    packet.seq = seq;
    packet.flags = flags;
    packet.clearPayload();
    return packet;
}

bool injectPacket(const IoTPacket& packet) {
    uint8_t wire[IOT_MAX_PACKET]{};
    const std::size_t length = iot_serialize(packet, wire, sizeof(wire));
    if (length == 0) return false;
    HostUdp::inject(wire, length);
    return true;
}

bool lastSentPacket(IoTPacket& packet) {
    if (HostUdp::sentCount() == 0) return false;
    uint8_t wire[IOT_MAX_PACKET]{};
    std::size_t length = 0;
    if (!HostUdp::copySent(HostUdp::sentCount() - 1, wire, sizeof(wire), length)) {
        return false;
    }
    return iot_deserialize(wire, length, packet);
}

bool seedRemote(IoTNode& node, uint16_t bootId) {
    IoTPacket hello = makePacketFrom(static_cast<uint8_t>(MsgType::HELLO),
                                     0x02, node.getDeviceId(), bootId, 1, 0);
    if (!injectPacket(hello)) return false;
    node.loop();
    return node.getRemote(0x02) != nullptr &&
           node.getRemote(0x02)->bootId == bootId;
}

bool startReliable(IoTNode& node, IoTPacket& transmitted) {
    if (!node.sendEvent(0x06, IPAddress(192, 168, 0, 2), 4210, 0x02)) {
        return false;
    }
    node.loop();
    if (!node.isReliableInFlight()) return false;
    node.loop();
    return node.isReliableInFlight() && lastSentPacket(transmitted);
}

bool injectAck(IoTNode& node, const IoTPacket& transmitted, uint16_t bootId) {
    IoTPacket ack = makePacketFrom(static_cast<uint8_t>(MsgType::ACK),
                                   transmitted.dst, node.getDeviceId(), bootId,
                                   transmitted.seq, 0);
    if (!injectPacket(ack)) return false;
    node.loop();
    return true;
}

bool check_reliable_session_paths() {
    // 1. ACK normal con sesión conocida.
    HostUdp::reset();
    IoTNode normal(0x01, 4210);
    normal.begin(100);
    if (!expect(seedRemote(normal, 50), "no se pudo registrar sesión conocida")) return false;
    IoTPacket transmitted{};
    if (!expect(startReliable(normal, transmitted), "no se inició reliable normal")) return false;
    if (!expect(injectAck(normal, transmitted, 50), "no se inyectó ACK normal")) return false;
    if (!expect(!normal.isReliableInFlight() && normal.getStats().ackReceived == 1,
                "ACK normal no confirmó reliable")) return false;

    // 2. Bootstrap: el primer ACK aprende la sesión remota.
    HostUdp::reset();
    IoTNode bootstrap(0x01, 4210);
    bootstrap.begin(100);
    transmitted = {};
    if (!expect(startReliable(bootstrap, transmitted), "no se inició reliable bootstrap")) return false;
    if (!expect(injectAck(bootstrap, transmitted, 77), "no se inyectó ACK bootstrap")) return false;
    if (!expect(!bootstrap.isReliableInFlight() && bootstrap.getRemote(0x02) &&
                    bootstrap.getRemote(0x02)->bootId == 77,
                "bootstrap no estableció BOOT_ID remoto")) return false;

    // 3. Reinicio remoto mientras hay reliable: la sesión cambia y el ACK
    // nuevo confirma el mismo paquete en vuelo.
    HostUdp::reset();
    IoTNode changed(0x01, 4210);
    changed.begin(100);
    if (!expect(seedRemote(changed, 50), "no se pudo preparar sessionChanged")) return false;
    transmitted = {};
    if (!expect(startReliable(changed, transmitted), "no se inició reliable sessionChanged")) return false;
    IoTPacket afterRestart = makePacketFrom(static_cast<uint8_t>(MsgType::EVENT),
                                            0x02, changed.getDeviceId(), 51, 1, 0);
    if (!expect(injectPacket(afterRestart), "no se inyectó paquete de nueva sesión")) return false;
    changed.loop();
    if (!expect(changed.getRemote(0x02)->bootId == 51 && changed.isReliableInFlight(),
                "sessionChanged no actualizó la sesión esperada")) return false;
    if (!expect(injectAck(changed, transmitted, 51) && !changed.isReliableInFlight(),
                "ACK de nueva sesión no confirmó reliable")) return false;

    // 4. ACK de la sesión anterior después del reinicio: se rechaza y no
    // confirma el canal hasta que llegue el ACK de la nueva sesión.
    HostUdp::reset();
    IoTNode stale(0x01, 4210);
    stale.begin(100);
    if (!expect(seedRemote(stale, 50), "no se pudo preparar replay de sesión")) return false;
    transmitted = {};
    if (!expect(startReliable(stale, transmitted), "no se inició reliable replay")) return false;
    if (!expect(injectPacket(afterRestart), "no se reinyectó cambio de sesión")) return false;
    stale.loop();
    if (!expect(injectAck(stale, transmitted, 50), "no se inyectó ACK viejo")) return false;
    if (!expect(stale.isReliableInFlight() && stale.getStats().ackReceived == 0,
                "ACK de sesión vieja confirmó reliable")) return false;
    if (!expect(injectAck(stale, transmitted, 51) && !stale.isReliableInFlight(),
                "ACK nuevo no confirmó después del replay")) return false;

    return true;
}

uint8_t callbackCount = 0;

void countPackets(const IoTPacket&, IPAddress, uint16_t) {
    ++callbackCount;
}

bool makeValidEvent(IoTPacket& packet, uint16_t bootId, uint32_t seq) {
    packet = makePacketFrom(static_cast<uint8_t>(MsgType::EVENT),
                            0x02, 0x01, bootId, seq,
                            IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE);
    return packet.addTLV_uint8(TlvTag::EVENT_TYPE, 0x06) &&
           packet.addTLV_uint8(TlvTag::EVENT_VALUE, 1) &&
           packet.addTLV_int8(TlvTag::RSSI_VAL, -50);
}

bool check_diagnostic_hello_tlvs() {
    HostUdp::reset();
    IoTNode node(0x02, 4210);
    node.begin(1234);
    node.setFirmwareVersion("4.3.0");
    node.setBootReason(BootReason::WATCHDOG);
    node.sendHello(IPAddress(192, 168, 0, 201), 4210, 0x02, "PIR Entrada");
    node.loop();
    node.loop();

    IoTPacket sent{};
    if (!expect(lastSentPacket(sent), "HELLO diagnóstico no fue transmitido")) return false;
    char version[12] = "";
    uint8_t reason = 0;
    if (!expect(sent.getTLV_string(TlvTag::FW_VERSION, version, sizeof(version)),
                "HELLO no contiene FW_VERSION configurada por la aplicación")) return false;
    if (!expect(sent.getTLV_uint8(TlvTag::BOOT_REASON, reason),
                "HELLO no contiene BOOT_REASON")) return false;
    return expect(std::strcmp(version, "4.3.0") == 0 &&
                      reason == static_cast<uint8_t>(BootReason::WATCHDOG),
                  "TLV diagnóstico HELLO tiene valores incorrectos");
}

bool check_dedup_seq_wraparound() {
    HostUdp::reset();
    callbackCount = 0;
    IoTNode node(0x01, 4210);
    node.begin(100);
    node.onPacketReceived(countPackets);

    IoTPacket beforeWrap{};
    IoTPacket afterWrap{};
    if (!expect(makeValidEvent(beforeWrap, 50, 0xFFFFFFFEUL),
                "no se pudo construir EVENT antes de wrap")) return false;
    if (!expect(makeValidEvent(afterWrap, 50, 1),
                "no se pudo construir EVENT después de wrap")) return false;

    if (!expect(injectPacket(beforeWrap), "no se inyectó EVENT antes de wrap")) return false;
    node.loop();
    if (!expect(callbackCount == 1, "EVENT inicial no llegó al callback")) return false;

    if (!expect(injectPacket(afterWrap), "no se inyectó EVENT después de wrap")) return false;
    node.loop();
    if (!expect(callbackCount == 2,
                "SEQ 1 después de 0xFFFFFFFF fue tratado como replay")) return false;

    if (!expect(injectPacket(beforeWrap), "no se reinyectó EVENT antiguo")) return false;
    node.loop();
    return expect(callbackCount == 2 && node.getStats().duplicates > 0,
                  "EVENT antiguo volvió a producir efecto después de wrap");
}

bool check_name_limit() {
    HostUdp::reset();
    IoTNode receiver(0x01, 4210);
    receiver.begin(100);
    IoTPacket hello = makePacketFrom(static_cast<uint8_t>(MsgType::HELLO),
                                     0x02, receiver.getDeviceId(), 1, 1, 0);
    const char* name = "12345678901234567890123";
    if (!expect(hello.addTLV_string(TlvTag::DEVICE_NAME, name),
                "no se pudo construir nombre de 23 caracteres")) return false;
    if (!expect(injectPacket(hello), "no se pudo inyectar HELLO con nombre largo")) return false;
    receiver.loop();
    RemoteDevice* remote = receiver.getRemote(0x02);
    return expect(remote != nullptr && std::strcmp(remote->name, name) == 0,
                  "nombre de 23 caracteres no se preservó en registry");
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

    // HELLO solo tolera perder FW_VERSION; si falta un campo de identidad
    // crítico, debe descartarse en lugar de encolar discovery incompleto.
    IoTNode invalidHello(0x02, 4210);
    invalidHello.begin(1234);
    invalidHello.sendHello(central, 4210,
                           AlarmProfile::toWire(AlarmProfile::DeviceType::PIR_SENSOR),
                           "nombre demasiado largo para el payload de discovery con suficientes caracteres adicionales");
    if (!expect(invalidHello.queuedCount() == 0,
                "HELLO sin DEVICE_NAME no fue descartado")) {
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
    if (!check_reliable_session_paths()) return 4;
    if (!check_name_limit()) return 5;
    if (!check_dedup_seq_wraparound()) return 6;
    if (!check_diagnostic_hello_tlvs()) return 7;

    std::puts("OK: host compile check wifi_PIR");
    return 0;
}
