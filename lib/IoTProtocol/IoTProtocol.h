/**
 * IoTProtocol V4 — Protocolo binario universal para redes de sensores ESP8266/ESP32
 *
 * Características:
 * - Cabecera binaria compacta (14 bytes fijos)
 * - Payload TLV (Type-Length-Value) extensible
 * - CRC16 para integridad
 * - IDs numéricos para dispositivos (1 byte origen, 1 byte destino)
 * - Número de secuencia (16 bits)
 * - Flags: ACK_REQUIRED, URGENT, HAS_TIMESTAMP, RELIABLE
 * - Tipos de mensaje: EVENT, DATA, COMMAND, ACK, HEARTBEAT, HELLO, HELLO_ACK, ERROR
 * - ACK selectivo (solo cuando FLAG lo indica)
 * - Descubrimiento automático (HELLO/HELLO_ACK)
 * - Heartbeat para monitoreo de dispositivos vivos
 *
 * Formato del paquete:
 * ┌───────┬─────┬──────┬─────┬─────┬──────┬───────┬─────────┬───────┐
 * │ MAGIC │ VER │ TYPE │ SRC │ DST │ SEQ  │ FLAGS │ PAY_LEN │ [TLV] │
 * │ 2B    │ 1B  │ 1B   │ 1B  │ 1B  │ 2B   │ 1B    │ 2B      │ NB    │
 * └───────┴─────┴──────┴─────┴─────┴──────┴───────┴─────────┴───────┘
 *                                                              │
 *                                                         ┌────┴────┐
 *                                                         │  CRC16  │
 *                                                         │  2B     │
 *                                                         └─────────┘
 *
 * TLV format:
 * ┌──────┬────────┬───────┐
 * │ TAG  │ LENGTH │ VALUE │
 * │ 1B   │ 1B     │ N B   │
 * └──────┴────────┴───────┘
 */

#pragma once
#include <Arduino.h>

// ============================================================
// Constantes del protocolo
// ============================================================

#define IOT_MAGIC_0       0xA5
#define IOT_MAGIC_1       0x5A
#define IOT_PROTOCOL_VER  4

#define IOT_HEADER_SIZE   11   // MAGIC(2) + VER(1) + TYPE(1) + SRC(1) + DST(1) + SEQ(2) + FLAGS(1) + PAY_LEN(2)
#define IOT_CRC_SIZE      2
#define IOT_MAX_PAYLOAD   64   // Máximo payload TLV (suficiente para la mayoría de sensores)
#define IOT_MAX_PACKET    (IOT_HEADER_SIZE + IOT_MAX_PAYLOAD + IOT_CRC_SIZE)

// ============================================================
// Device IDs (1 byte: 0-255 dispositivos)
// ============================================================

#define IOT_DEVICE_BROADCAST  0x00   // Broadcast a todos
#define IOT_DEVICE_CENTRAL    0x01   // Receptor central

// Rangos sugeridos:
// 0x02 - 0x1F: PIR sensors
// 0x20 - 0x3F: Botones/timbres
// 0x40 - 0x5F: Temperatura/humedad
// 0x60 - 0x7F: Relés/actuadores
// 0x80 - 0x9F: Displays
// 0xA0 - 0xFE: Reservado para expansión
// 0xFF: ID no asignado

// ============================================================
// Tipos de mensaje (1 byte)
// ============================================================

enum class MsgType : uint8_t {
    EVENT       = 0x01,   // Sensor → Central (PIR, botón, puerta, etc.)
    DATA        = 0x02,   // Sensor → Central (temperatura, humedad, etc.)
    COMMAND     = 0x03,   // Central → Actuador (relé ON, LED toggle, etc.)
    RESPONSE    = 0x04,   // Actuador → Central (respuesta a comando)
    ACK         = 0x05,   // Confirmación genérica
    HEARTBEAT   = 0x06,   // Cualquiera → Central (estoy vivo)
    HELLO       = 0x07,   // Nuevo dispositivo anuncia presencia
    HELLO_ACK   = 0x08,   // Central responde al HELLO
    CONFIG      = 0x09,   // Central → Dispositivo (configuración remota)
    ERROR_MSG   = 0x0A,   // Error genérico
    DISPLAY     = 0x0B,   // Central → Display (texto/datos para LCD)
    STATUS      = 0x0C,   // Cualquiera → Central (estado completo)
};

// ============================================================
// Flags (1 byte, bitmap)
// ============================================================

#define IOT_FLAG_ACK_REQUIRED   0x01   // El emisor espera ACK
#define IOT_FLAG_URGENT         0x02   // Prioridad alta
#define IOT_FLAG_RELIABLE       0x04   // Reintentar si no hay ACK
#define IOT_FLAG_HAS_TIMESTAMP  0x08   // El payload incluye timestamp

// ============================================================
// TLV Tags (1 byte cada uno)
// ============================================================

enum class TlvTag : uint8_t {
    // Eventos (0x01 - 0x1F)
    EVENT_TYPE      = 0x01,   // uint8_t: tipo de evento (EventCode)
    EVENT_VALUE     = 0x02,   // uint8_t: valor del evento (1=activo, 0=inactivo)

    // Datos de sensores (0x20 - 0x3F)
    TEMPERATURE     = 0x20,   // int16_t: temp × 10 (ej: 237 = 23.7°C)
    HUMIDITY        = 0x21,   // uint16_t: hum × 10 (ej: 482 = 48.2%)
    PRESSURE        = 0x22,   // uint16_t: hPa
    LIGHT           = 0x23,   // uint16_t: lux
    BATTERY_PCT     = 0x24,   // uint8_t: 0-100%
    BATTERY_MV      = 0x25,   // uint16_t: mV
    RSSI_VAL        = 0x26,   // int8_t: dBm

    // Comandos (0x40 - 0x5F)
    CMD_STATE       = 0x40,   // uint8_t: 0=OFF, 1=ON, 2=TOGGLE
    CMD_DURATION    = 0x41,   // uint16_t: ms
    CMD_CHANNEL     = 0x42,   // uint8_t: canal del relé (0-7)

    // Display (0x60 - 0x7F)
    DISPLAY_LINE    = 0x60,   // uint8_t: número de línea (0-3)
    DISPLAY_TEXT    = 0x61,   // string: texto a mostrar
    DISPLAY_CLEAR   = 0x62,   // uint8_t: 1=limpiar pantalla

    // Discovery/Config (0x80 - 0x9F)
    DEVICE_NAME     = 0x80,   // string: nombre legible (ej: "PIR Entrada")
    DEVICE_TYPE     = 0x81,   // uint8_t: DeviceType enum
    CAPABILITY      = 0x82,   // uint8_t: capacidad que reporta
    FW_VERSION      = 0x83,   // string: versión firmware

    // Timestamps (0xA0 - 0xAF)
    TIMESTAMP_MS    = 0xA0,   // uint32_t: millis() del emisor
    UPTIME_SEC      = 0xA1,   // uint32_t: segundos desde boot

    // Status (0xC0 - 0xDF)
    FREE_HEAP       = 0xC0,   // uint32_t: bytes libres
    WIFI_RSSI       = 0xC1,   // int8_t: RSSI actual
    ERROR_CODE      = 0xC2,   // uint8_t: código de error
};

// ============================================================
// Códigos de eventos (van dentro de TLV EVENT_TYPE)
// ============================================================

enum class EventCode : uint8_t {
    MOTION          = 0x01,
    DOOR_OPEN       = 0x02,
    DOOR_CLOSE      = 0x03,
    BUTTON_PRESS    = 0x04,
    BUTTON_RELEASE  = 0x05,
    TIMBRE          = 0x06,
    SMOKE           = 0x07,
    FLOOD           = 0x08,
    TAMPER          = 0x09,
    LOW_BATTERY     = 0x0A,
};

// ============================================================
// Tipos de dispositivo (para HELLO/discovery)
// ============================================================

enum class DeviceType : uint8_t {
    CENTRAL     = 0x01,
    PIR_SENSOR  = 0x02,
    BUTTON      = 0x03,
    TEMP_SENSOR = 0x04,
    RELAY       = 0x05,
    DISPLAY     = 0x06,
    DOOR_SENSOR = 0x07,
    SMOKE_SENSOR = 0x08,
    MULTI_SENSOR = 0x09,
};

// ============================================================
// Estructura del paquete
// ============================================================

struct IoTPacket {
    // Cabecera
    uint8_t  version;
    MsgType  type;
    uint8_t  src;
    uint8_t  dst;
    uint16_t seq;
    uint8_t  flags;

    // Payload TLV
    uint8_t  payload[IOT_MAX_PAYLOAD];
    uint16_t payloadLen;

    // Métodos para construir payload TLV
    void clearPayload();
    bool addTLV_uint8(TlvTag tag, uint8_t value);
    bool addTLV_int8(TlvTag tag, int8_t value);
    bool addTLV_uint16(TlvTag tag, uint16_t value);
    bool addTLV_int16(TlvTag tag, int16_t value);
    bool addTLV_uint32(TlvTag tag, uint32_t value);
    bool addTLV_string(TlvTag tag, const char* str);

    // Métodos para leer payload TLV
    bool getTLV_uint8(TlvTag tag, uint8_t &value) const;
    bool getTLV_int8(TlvTag tag, int8_t &value) const;
    bool getTLV_uint16(TlvTag tag, uint16_t &value) const;
    bool getTLV_int16(TlvTag tag, int16_t &value) const;
    bool getTLV_uint32(TlvTag tag, uint32_t &value) const;
    bool getTLV_string(TlvTag tag, char* buf, uint8_t maxLen) const;

    // Helpers
    bool needsAck() const { return (flags & IOT_FLAG_ACK_REQUIRED) != 0; }
    bool isUrgent() const { return (flags & IOT_FLAG_URGENT) != 0; }
    bool isReliable() const { return (flags & IOT_FLAG_RELIABLE) != 0; }
};

// ============================================================
// CRC16
// ============================================================

uint16_t iot_crc16(const uint8_t* data, size_t len);

// ============================================================
// Serialización / Deserialización
// ============================================================

// Serializa paquete → buffer. Retorna tamaño total o 0 si error.
size_t iot_serialize(const IoTPacket &pkt, uint8_t* buf, size_t bufSize);

// Deserializa buffer → paquete. Retorna true si válido (magic + CRC ok).
bool iot_deserialize(const uint8_t* buf, size_t len, IoTPacket &pkt);
