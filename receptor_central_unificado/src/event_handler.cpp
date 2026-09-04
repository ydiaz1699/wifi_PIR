// receptor_central_unificado/src/event_handler.cpp
/**
 * Event Handler V4.3 — Procesamiento genérico de eventos IoTProtocol
 *
 * El receptor NO necesita saber de antemano qué sensores existen.
 * Cualquier dispositivo que envíe un EVENT válido será procesado.
 * La acción depende del AlarmProfile::EventCode y del modo actual (armado/desarmado).
 * V4.3: verifica HMAC si auth está habilitado.
 */

#include "event_handler.h"
#include <AlarmProfile.h>
#include "hal.h"
#include "config.h"
#include "logger.h"
#include <PubSubClient.h>

extern Buzzer buzzer;
extern PubSubClient mqtt;
extern bool mqttDisponible;
extern String modoAlarma;

// --- Nombres para log ---
static const char* eventCodeToStr(AlarmProfile::EventCode code) {
    switch (code) {
        case AlarmProfile::EventCode::MOTION:       return "MOTION";
        case AlarmProfile::EventCode::DOOR_OPEN:    return "DOOR_OPEN";
        case AlarmProfile::EventCode::DOOR_CLOSE:   return "DOOR_CLOSE";
        case AlarmProfile::EventCode::BUTTON_PRESS: return "BUTTON_PRESS";
        case AlarmProfile::EventCode::BUTTON_RELEASE:return "BUTTON_RELEASE";
        case AlarmProfile::EventCode::TIMBRE:       return "TIMBRE";
        case AlarmProfile::EventCode::SMOKE:        return "SMOKE";
        case AlarmProfile::EventCode::FLOOD:        return "FLOOD";
        case AlarmProfile::EventCode::TAMPER:       return "TAMPER";
        case AlarmProfile::EventCode::LOW_BATTERY: return "LOW_BATTERY";
        case AlarmProfile::EventCode::WINDOW_OPEN: return "WINDOW_OPEN";
        case AlarmProfile::EventCode::WINDOW_CLOSE:return "WINDOW_CLOSE";
        case AlarmProfile::EventCode::VIBRATION:   return "VIBRATION";
        case AlarmProfile::EventCode::GAS_DETECTED:return "GAS_DETECTED";
        default:                      return "UNKNOWN";
    }
}

// --- Validación del vocabulario del perfil ---
static bool esEventCodeConocido(uint8_t rawCode) {
    switch (static_cast<AlarmProfile::EventCode>(rawCode)) {
        case AlarmProfile::EventCode::MOTION:
        case AlarmProfile::EventCode::DOOR_OPEN:
        case AlarmProfile::EventCode::DOOR_CLOSE:
        case AlarmProfile::EventCode::BUTTON_PRESS:
        case AlarmProfile::EventCode::BUTTON_RELEASE:
        case AlarmProfile::EventCode::TIMBRE:
        case AlarmProfile::EventCode::SMOKE:
        case AlarmProfile::EventCode::FLOOD:
        case AlarmProfile::EventCode::TAMPER:
        case AlarmProfile::EventCode::LOW_BATTERY:
        case AlarmProfile::EventCode::WINDOW_OPEN:
        case AlarmProfile::EventCode::WINDOW_CLOSE:
        case AlarmProfile::EventCode::VIBRATION:
        case AlarmProfile::EventCode::GAS_DETECTED:
            return true;
        default:
            return false;
    }
}

// --- Duración de bocina por tipo de evento ---
static unsigned long duracionPorEvento(AlarmProfile::EventCode code) {
    switch (code) {
        case AlarmProfile::EventCode::MOTION:       return DURACION_BOCINA_MOTION_MS;
        case AlarmProfile::EventCode::TIMBRE:       return DURACION_BOCINA_TIMBRE_MS;
        case AlarmProfile::EventCode::BUTTON_PRESS: return DURACION_BOCINA_TIMBRE_MS;
        case AlarmProfile::EventCode::DOOR_OPEN:    return DURACION_BOCINA_PUERTA_MS;
        case AlarmProfile::EventCode::SMOKE:        return 5000;
        case AlarmProfile::EventCode::FLOOD:        return 3000;
        // TAMPER queda explícitamente sin bocina hasta aprobar su contrato
        // de seguridad (modo, prioridad y duración). Se sigue publicando.
        case AlarmProfile::EventCode::TAMPER:       return 0;
        case AlarmProfile::EventCode::DOOR_CLOSE:
        case AlarmProfile::EventCode::BUTTON_RELEASE:
        case AlarmProfile::EventCode::LOW_BATTERY:
        case AlarmProfile::EventCode::WINDOW_OPEN:
        case AlarmProfile::EventCode::WINDOW_CLOSE:
        case AlarmProfile::EventCode::VIBRATION:
        case AlarmProfile::EventCode::GAS_DETECTED:
        default:                                    return 0;
    }
}

// --- ¿Activar bocina? Es una allowlist deliberada, no un fallback por modo. ---
static bool debeActivarBocina(AlarmProfile::EventCode code) {
    switch (code) {
        // Conserva la política V3/V4 actual: estos eventos suenan siempre.
        case AlarmProfile::EventCode::TIMBRE:
        case AlarmProfile::EventCode::BUTTON_PRESS:
        case AlarmProfile::EventCode::SMOKE:
        case AlarmProfile::EventCode::FLOOD:
            return true;

        // MOTION y apertura solo activan con la alarma armada.
        case AlarmProfile::EventCode::MOTION:
        case AlarmProfile::EventCode::DOOR_OPEN:
            return modoAlarma == "armado";

        // Eventos de estado/diagnóstico y códigos aún sin contrato acústico
        // se publican, pero no activan la bocina accidentalmente.
        case AlarmProfile::EventCode::DOOR_CLOSE:
        case AlarmProfile::EventCode::BUTTON_RELEASE:
        case AlarmProfile::EventCode::TAMPER:
        case AlarmProfile::EventCode::LOW_BATTERY:
        case AlarmProfile::EventCode::WINDOW_OPEN:
        case AlarmProfile::EventCode::WINDOW_CLOSE:
        case AlarmProfile::EventCode::VIBRATION:
        case AlarmProfile::EventCode::GAS_DETECTED:
        default:
            return false;
    }
}

// --- MQTT publish helpers ---
static void publishEvent(uint8_t srcId, AlarmProfile::EventCode code, uint8_t value) {
    if (!mqttDisponible) return;
    char topic[48], payload[32];
    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/evento", srcId);
    snprintf(payload, sizeof(payload), "%s|%d", eventCodeToStr(code), value);
    mqtt.publish(topic, payload);

    // Topics V3 para conservar la publicación consumida por HA.
    if (value > 0 && code == AlarmProfile::EventCode::MOTION) {
        mqtt.publish(TOPIC_V3_EVENTO, "detectado");
    } else if (value > 0 && code == AlarmProfile::EventCode::TIMBRE) {
        mqtt.publish(TOPIC_V3_TIMBRE, "presionado");
    }
}

static void publishHeartbeat(uint8_t srcId, const IoTPacket &pkt) {
    if (!mqttDisponible) return;
    char topic[48], payload[16];

    uint32_t uptime = 0;
    int8_t rssi = 0;
    uint32_t freeHeap = 0;
    uint8_t queueDepth = 0;
    uint32_t txCount = 0;
    uint32_t ackTimeouts = 0;
    char fwVersion[12] = "";
    uint8_t bootReason = static_cast<uint8_t>(BootReason::UNKNOWN);

    pkt.getTLV_uint32(TlvTag::UPTIME_SEC, uptime);
    pkt.getTLV_int8(TlvTag::RSSI_VAL, rssi);
    const bool hasFreeHeap = pkt.getTLV_uint32(TlvTag::FREE_HEAP, freeHeap);
    const bool hasQueueDepth = pkt.getTLV_uint8(TlvTag::QUEUE_DEPTH, queueDepth);
    const bool hasTxCount = pkt.getTLV_uint32(TlvTag::TX_COUNT, txCount);
    const bool hasAckTimeouts = pkt.getTLV_uint32(TlvTag::ACK_TIMEOUTS, ackTimeouts);
    const bool hasFwVersion = pkt.getTLV_string(TlvTag::FW_VERSION, fwVersion,
                                                 sizeof(fwVersion));
    const bool hasBootReason = pkt.getTLV_uint8(TlvTag::BOOT_REASON, bootReason);

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/uptime", srcId);
    snprintf(payload, sizeof(payload), "%lu", (unsigned long)uptime);
    mqtt.publish(topic, payload, true);

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/rssi", srcId);
    snprintf(payload, sizeof(payload), "%d", rssi);
    mqtt.publish(topic, payload, true);

    if (hasFreeHeap) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/heap", srcId);
        snprintf(payload, sizeof(payload), "%lu", (unsigned long)freeHeap);
        mqtt.publish(topic, payload, true);
    }

    if (hasTxCount) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/tx_count", srcId);
        snprintf(payload, sizeof(payload), "%lu", (unsigned long)txCount);
        mqtt.publish(topic, payload, true);
    }

    if (hasAckTimeouts) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/ack_timeouts", srcId);
        snprintf(payload, sizeof(payload), "%lu", (unsigned long)ackTimeouts);
        mqtt.publish(topic, payload, true);
    }

    if (hasQueueDepth) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/queue_depth", srcId);
        snprintf(payload, sizeof(payload), "%u", static_cast<unsigned>(queueDepth));
        mqtt.publish(topic, payload, true);
    }

    if (hasFwVersion) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/fw_version", srcId);
        mqtt.publish(topic, fwVersion, true);
    }

    if (hasBootReason) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/boot_reason", srcId);
        snprintf(payload, sizeof(payload), "%u", static_cast<unsigned>(bootReason));
        mqtt.publish(topic, payload, true);
    }
}

static void publishStateReport(uint8_t srcId, const IoTPacket &pkt) {
    if (!mqttDisponible) return;
    char topic[48];

    uint8_t motionState = 0, buttonState = 0;

    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_MOTION), motionState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/motion", srcId);
        mqtt.publish(topic, motionState ? "active" : "idle", true);
    }
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_BUTTON), buttonState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/button", srcId);
        mqtt.publish(topic, buttonState ? "pressed" : "released", true);
    }

    // Todos los StateTag actualmente definidos por AlarmProfile tienen una
    // traducción MQTT explícita. No existe aún un StateTag de batería: LOW_BATTERY
    // es EventCode y requiere un contrato separado si se necesita estado retained.
    uint8_t doorState = 0, relayState = 0, smokeState = 0;
    uint8_t alarmState = 0, floodState = 0;
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_DOOR), doorState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/door", srcId);
        mqtt.publish(topic, doorState ? "open" : "closed", true);
    }
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_RELAY), relayState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/relay", srcId);
        mqtt.publish(topic, relayState ? "on" : "off", true);
    }
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_SMOKE), smokeState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/smoke", srcId);
        mqtt.publish(topic, smokeState ? "detected" : "clear", true);
    }
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_ALARM), alarmState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/alarm", srcId);
        mqtt.publish(topic, alarmState ? "active" : "clear", true);
    }
    if (pkt.getTLV_uint8(AlarmProfile::toCoreTlvTag(AlarmProfile::StateTag::STATE_FLOOD), floodState)) {
        snprintf(topic, sizeof(topic), "casa/iot/device_%02X/state/flood", srcId);
        mqtt.publish(topic, floodState ? "detected" : "clear", true);
    }
}

// ============================================================
// Handler principal — IoTNode lo llama cuando llega un paquete válido
// ============================================================

void handleIoTPacket(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    // IoTNode ya verificó auth antes de tocar registry, ACK, dedup o llamar
    // este handler. Aquí solo se procesan paquetes aceptados por la frontera.
    switch (pkt.type) {
        case MsgType::EVENT: {
            uint8_t eventType = 0;
            uint8_t eventValue = 1;
            const bool hasEventType = pkt.getTLV_uint8(TlvTag::EVENT_TYPE, eventType);
            pkt.getTLV_uint8(TlvTag::EVENT_VALUE, eventValue);

            if (!hasEventType) {
                LOG_WARN("EVENT 0x%02X descartado: falta EVENT_TYPE", pkt.src);
                break;
            }

            AlarmProfile::EventCode code = static_cast<AlarmProfile::EventCode>(eventType);
            const bool knownEvent = esEventCodeConocido(eventType);
            LOG_INFO("EVENT 0x%02X: %s val=%d (%s boot=0x%04X seq=%lu)%s",
                     pkt.src, eventCodeToStr(code), eventValue,
                     remoteIP.toString().c_str(), pkt.bootId,
                     (unsigned long)pkt.seq,
                     knownEvent ? "" : " [desconocido]");

            // Se conserva la telemetría del evento, pero un código futuro o
            // corrupto nunca puede activar la bocina por una regla por defecto.
            publishEvent(pkt.src, code, eventValue);
            if (!knownEvent) break;

            if (eventValue > 0 && debeActivarBocina(code)) {
                unsigned long dur = duracionPorEvento(code);
                if (dur == 0) break;
                buzzer.timedOn(dur);
                LOG_INFO("Bocina ON %lums (%s)", dur, eventCodeToStr(code));
            }
            break;
        }

        case MsgType::HEARTBEAT: {
            uint32_t uptime = 0;
            int8_t rssi = 0;
            pkt.getTLV_uint32(TlvTag::UPTIME_SEC, uptime);
            pkt.getTLV_int8(TlvTag::RSSI_VAL, rssi);
            LOG_DEBUG("HB 0x%02X: up=%lus rssi=%d boot=0x%04X",
                      pkt.src, (unsigned long)uptime, rssi, pkt.bootId);
            publishHeartbeat(pkt.src, pkt);
            break;
        }

        case MsgType::HELLO: {
            char name[IOT_DEVICE_NAME_MAX] = "";
            char fwVersion[12] = "";
            uint8_t devType = 0;
            uint8_t bootReason = static_cast<uint8_t>(BootReason::UNKNOWN);
            const bool hasFwVersion = pkt.getTLV_string(TlvTag::FW_VERSION,
                                                        fwVersion,
                                                        sizeof(fwVersion));
            const bool hasBootReason = pkt.getTLV_uint8(TlvTag::BOOT_REASON,
                                                        bootReason);
            pkt.getTLV_string(TlvTag::DEVICE_NAME, name, sizeof(name));
            pkt.getTLV_uint8(TlvTag::DEVICE_TYPE_TAG, devType);
            LOG_INFO("HELLO 0x%02X: \"%s\" type=%d boot=0x%04X (%s) fw=%s reset=%u",
                     pkt.src, name, devType, pkt.bootId,
                     remoteIP.toString().c_str(),
                     hasFwVersion ? fwVersion : "unknown",
                     hasBootReason ? static_cast<unsigned>(bootReason) : 255U);

            // Publicar discovery en MQTT
            if (mqttDisponible) {
                char topic[48];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/name", pkt.src);
                mqtt.publish(topic, name, true);
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/status", pkt.src);
                mqtt.publish(topic, "online", true);
                if (hasFwVersion) {
                    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/fw_version", pkt.src);
                    mqtt.publish(topic, fwVersion, true);
                }
                if (hasBootReason) {
                    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/boot_reason", pkt.src);
                    char payload[8];
                    snprintf(payload, sizeof(payload), "%u",
                             static_cast<unsigned>(bootReason));
                    mqtt.publish(topic, payload, true);
                }
            }
            break;
        }

        case MsgType::DATA: {
            int16_t temp = 0;
            uint16_t hum = 0;
            if (pkt.getTLV_int16(TlvTag::TEMPERATURE, temp)) {
                char topic[48], payload[16];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/temperatura", pkt.src);
                snprintf(payload, sizeof(payload), "%.1f", temp / 10.0);
                if (mqttDisponible) mqtt.publish(topic, payload, true);
                LOG_DEBUG("DATA 0x%02X: temp=%.1f", pkt.src, temp / 10.0);
            }
            if (pkt.getTLV_uint16(TlvTag::HUMIDITY, hum)) {
                char topic[48], payload[16];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/humedad", pkt.src);
                snprintf(payload, sizeof(payload), "%.1f", hum / 10.0);
                if (mqttDisponible) mqtt.publish(topic, payload, true);
            }
            break;
        }

        case MsgType::STATE_REPORT: {
            LOG_INFO("STATE_REPORT 0x%02X recibido", pkt.src);
            publishStateReport(pkt.src, pkt);
            break;
        }

        case MsgType::STATUS: {
            LOG_DEBUG("STATUS 0x%02X recibido", pkt.src);
            break;
        }

        default:
            LOG_DEBUG("Tipo 0x%02X de 0x%02X (no procesado)",
                      static_cast<uint8_t>(pkt.type), pkt.src);
            break;
    }
}
