#pragma once
#include <IoTProtocol.h>

/**
 * Procesa un paquete IoTProtocol recibido.
 * Es el "cerebro" del receptor: decide qué hacer según el tipo de evento.
 * No tiene lógica hardcodeada por sensor — usa el EVENT_TYPE del TLV.
 */
void handleIoTPacket(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort);

/**
 * Publica un evento IoT en MQTT (si conectado).
 * Genera topic automáticamente: casa/iot/{deviceId}/{eventType}
 */
void publishEventToMQTT(uint8_t srcId, EventCode eventCode, uint8_t value);

/**
 * Publica heartbeat info en MQTT.
 */
void publishHeartbeatToMQTT(uint8_t srcId, uint32_t uptime, int8_t rssi);
