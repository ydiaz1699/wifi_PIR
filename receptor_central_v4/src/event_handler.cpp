/**
 * Event Handler V4.1 — Procesamiento genérico de eventos IoTProtocol
 *
 * El receptor NO necesita saber de antemano qué sensores existen.
 * Cualquier dispositivo que envíe un EVENT válido será procesado.
 * La acción depende del EventCode y del modo actual (armado/desarmado).
 */

#include "event_handler.h"
#include "hal.h"
#include "config.h"
#include "logger.h"
#include <PubSubClient.h>

extern Buzzer buzzer;
extern PubSubClient mqtt;
extern bool mqttDisponible;
extern String modoAlarma;

// --- Nombres para log ---
static const char* eventCodeToStr(EventCode code) {
    switch (code) {
        case EventCode::MOTION:       return "MOTION";
        case EventCode::DOOR_OPEN:    return "DOOR_OPEN";
        case EventCode::DOOR_CLOSE:   return "DOOR_CLOSE";
        case EventCode::BUTTON_PRESS: return "BUTTON_PRESS";
        case EventCode::TIMBRE:       return "TIMBRE";
        case EventCode::SMOKE:        return "SMOKE";
        case EventCode::FLOOD:        return "FLOOD";
        case EventCode::TAMPER:       return "TAMPER";
        case EventCode::LOW_BATTERY:  return "LOW_BATTERY";
        default:                      return "UNKNOWN";
    }
}

// --- Duración de bocina por tipo de evento ---
static unsigned long duracionPorEvento(EventCode code) {
    switch (code) {
        case EventCode::MOTION:       return DURACION_BOCINA_MOTION_MS;
        case EventCode::TIMBRE:       return DURACION_BOCINA_TIMBRE_MS;
        case EventCode::BUTTON_PRESS: return DURACION_BOCINA_TIMBRE_MS;
        case EventCode::DOOR_OPEN:    return DURACION_BOCINA_PUERTA_MS;
        case EventCode::SMOKE:        return 5000;
        case EventCode::FLOOD:        return 3000;
        default:                      return DURACION_BOCINA_TIMBRE_MS;
    }
}

// --- ¿Activar bocina? Depende del modo ---
static bool debeActivarBocina(EventCode code) {
    // Siempre suenan (no dependen del modo)
    if (code == EventCode::TIMBRE || code == EventCode::BUTTON_PRESS) return true;
    if (code == EventCode::SMOKE || code == EventCode::FLOOD) return true;

    // Solo si armado
    return (modoAlarma == "armado");
}

// --- MQTT publish helpers ---
static void publishEvent(uint8_t srcId, EventCode code, uint8_t value) {
    if (!mqttDisponible) return;
    char topic[48], payload[32];
    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/evento", srcId);
    snprintf(payload, sizeof(payload), "%s|%d", eventCodeToStr(code), value);
    mqtt.publish(topic, payload);
}

static void publishHeartbeat(uint8_t srcId, uint32_t uptime, int8_t rssi) {
    if (!mqttDisponible) return;
    char topic[48], payload[16];

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/uptime", srcId);
    snprintf(payload, sizeof(payload), "%lu", (unsigned long)uptime);
    mqtt.publish(topic, payload, true);

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/rssi", srcId);
    snprintf(payload, sizeof(payload), "%d", rssi);
    mqtt.publish(topic, payload, true);
}

// ============================================================
// Handler principal — IoTNode lo llama cuando llega un paquete válido
// ============================================================

void handleIoTPacket(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    switch (pkt.type) {
        case MsgType::EVENT: {
            uint8_t eventType = 0, eventValue = 1;
            pkt.getTLV_uint8(TlvTag::EVENT_TYPE, eventType);
            pkt.getTLV_uint8(TlvTag::EVENT_VALUE, eventValue);

            EventCode code = static_cast<EventCode>(eventType);
            LOG_INFO("EVENT 0x%02X: %s val=%d (%s boot=0x%04X seq=%lu)",
                     pkt.src, eventCodeToStr(code), eventValue,
                     remoteIP.toString().c_str(), pkt.bootId,
                     (unsigned long)pkt.seq);

            publishEvent(pkt.src, code, eventValue);

            if (eventValue > 0 && debeActivarBocina(code)) {
                unsigned long dur = duracionPorEvento(code);
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
            publishHeartbeat(pkt.src, uptime, rssi);
            break;
        }

        case MsgType::HELLO: {
            char name[20] = "";
            uint8_t devType = 0;
            pkt.getTLV_string(TlvTag::DEVICE_NAME, name, sizeof(name));
            pkt.getTLV_uint8(TlvTag::DEVICE_TYPE_TAG, devType);
            LOG_INFO("HELLO 0x%02X: \"%s\" type=%d boot=0x%04X (%s)",
                     pkt.src, name, devType, pkt.bootId,
                     remoteIP.toString().c_str());

            // Publicar discovery en MQTT
            if (mqttDisponible) {
                char topic[48], payload[40];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/name", pkt.src);
                mqtt.publish(topic, name, true);
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/status", pkt.src);
                mqtt.publish(topic, "online", true);
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
