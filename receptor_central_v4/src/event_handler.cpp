/**
 * Event Handler V4 — Procesamiento genérico de eventos IoTProtocol
 *
 * El receptor NO necesita saber de antemano qué sensores existen.
 * Cualquier dispositivo que envíe un EVENT válido será procesado.
 *
 * La acción depende del EventCode (MOTION, TIMBRE, DOOR_OPEN, etc.)
 * y del modo actual del sistema (armado/desarmado).
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

// --- Tabla de nombres para log ---
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

// --- Determinar duración de bocina según evento ---
static unsigned long duracionPorEvento(EventCode code) {
    switch (code) {
        case EventCode::MOTION:       return DURACION_BOCINA_MOTION_MS;
        case EventCode::TIMBRE:       return DURACION_BOCINA_TIMBRE_MS;
        case EventCode::BUTTON_PRESS: return DURACION_BOCINA_TIMBRE_MS;
        case EventCode::DOOR_OPEN:    return DURACION_BOCINA_PUERTA_MS;
        case EventCode::SMOKE:        return 5000;  // Alarma larga
        case EventCode::FLOOD:        return 3000;
        default:                      return DURACION_BOCINA_TIMBRE_MS;
    }
}

// --- ¿El evento activa bocina según el modo actual? ---
static bool debeActivarBocina(EventCode code) {
    // TIMBRE siempre suena (es un aviso, no alarma)
    if (code == EventCode::TIMBRE || code == EventCode::BUTTON_PRESS) {
        return true;
    }
    // SMOKE y FLOOD siempre suenan (emergencia)
    if (code == EventCode::SMOKE || code == EventCode::FLOOD) {
        return true;
    }
    // MOTION, DOOR, TAMPER: solo si está armado
    if (modoAlarma == "armado") {
        return true;
    }
    return false;
}


// ============================================================
// Publicar evento en MQTT
// Topic automático: casa/iot/device_XX/evento
// ============================================================

void publishEventToMQTT(uint8_t srcId, EventCode eventCode, uint8_t value) {
    if (!mqttDisponible) return;

    char topic[48];
    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/evento", srcId);

    char payload[32];
    snprintf(payload, sizeof(payload), "%s|%d", eventCodeToStr(eventCode), value);

    mqtt.publish(topic, payload);
    LOG_DEBUG("MQTT pub: %s = %s", topic, payload);
}

void publishHeartbeatToMQTT(uint8_t srcId, uint32_t uptime, int8_t rssi) {
    if (!mqttDisponible) return;

    char topic[48];
    char payload[32];

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/uptime", srcId);
    snprintf(payload, sizeof(payload), "%lu", (unsigned long)uptime);
    mqtt.publish(topic, payload, true);

    snprintf(topic, sizeof(topic), "casa/iot/device_%02X/rssi", srcId);
    snprintf(payload, sizeof(payload), "%d", rssi);
    mqtt.publish(topic, payload, true);
}

// ============================================================
// Handler principal — llamado por IoTNode cuando llega un paquete
// ============================================================

void handleIoTPacket(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    switch (pkt.type) {
        case MsgType::EVENT: {
            uint8_t eventType = 0;
            uint8_t eventValue = 1;
            pkt.getTLV_uint8(TlvTag::EVENT_TYPE, eventType);
            pkt.getTLV_uint8(TlvTag::EVENT_VALUE, eventValue);

            EventCode code = static_cast<EventCode>(eventType);
            LOG_INFO("EVENT de 0x%02X: %s val=%d (%s)",
                     pkt.src, eventCodeToStr(code), eventValue,
                     remoteIP.toString().c_str());

            // Publicar en MQTT
            publishEventToMQTT(pkt.src, code, eventValue);

            // Activar bocina si corresponde
            if (eventValue > 0 && debeActivarBocina(code)) {
                unsigned long duracion = duracionPorEvento(code);
                buzzer.timedOn(duracion);
                LOG_INFO("Bocina ON %lums (evento %s)", duracion, eventCodeToStr(code));
            }
            break;
        }

        case MsgType::HEARTBEAT: {
            uint32_t uptime = 0;
            int8_t rssi = 0;
            pkt.getTLV_uint32(TlvTag::UPTIME_SEC, uptime);
            pkt.getTLV_int8(TlvTag::RSSI_VAL, rssi);

            LOG_DEBUG("HEARTBEAT de 0x%02X: uptime=%lus rssi=%ddBm",
                      pkt.src, (unsigned long)uptime, rssi);

            publishHeartbeatToMQTT(pkt.src, uptime, rssi);
            break;
        }

        case MsgType::HELLO: {
            LOG_INFO("HELLO de 0x%02X (%s) — nuevo dispositivo!",
                     pkt.src, remoteIP.toString().c_str());
            // Podríamos publicar discovery en MQTT aquí
            break;
        }

        case MsgType::DATA: {
            // Datos de sensores (temperatura, humedad, etc.)
            int16_t temp = 0;
            uint16_t hum = 0;
            bool hasTemp = pkt.getTLV_int16(TlvTag::TEMPERATURE, temp);
            bool hasHum = pkt.getTLV_uint16(TlvTag::HUMIDITY, hum);

            if (hasTemp) {
                char topic[48], payload[16];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/temperatura", pkt.src);
                snprintf(payload, sizeof(payload), "%.1f", temp / 10.0);
                if (mqttDisponible) mqtt.publish(topic, payload, true);
                LOG_DEBUG("DATA temp de 0x%02X: %.1f°C", pkt.src, temp / 10.0);
            }
            if (hasHum) {
                char topic[48], payload[16];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/humedad", pkt.src);
                snprintf(payload, sizeof(payload), "%.1f", hum / 10.0);
                if (mqttDisponible) mqtt.publish(topic, payload, true);
                LOG_DEBUG("DATA hum de 0x%02X: %.1f%%", pkt.src, hum / 10.0);
            }
            break;
        }

        case MsgType::STATUS: {
            uint32_t freeHeap = 0;
            int8_t wifiRssi = 0;
            pkt.getTLV_uint32(TlvTag::FREE_HEAP, freeHeap);
            pkt.getTLV_int8(TlvTag::WIFI_RSSI, wifiRssi);
            LOG_DEBUG("STATUS de 0x%02X: heap=%lu rssi=%d",
                      pkt.src, (unsigned long)freeHeap, wifiRssi);
            break;
        }

        default:
            LOG_DEBUG("Paquete tipo 0x%02X de 0x%02X (no procesado)",
                      static_cast<uint8_t>(pkt.type), pkt.src);
            break;
    }
}
