/**
 * IoTNode V4.1 — Nodo de comunicación IoTProtocol
 *
 * Cambios vs V4.0:
 * - FIFO real con head/tail/count (no tabla de slots)
 * - 3 prioridades: URGENT > NORMAL > BACKGROUND
 * - Un solo paquete reliable en vuelo a la vez (simplifica ACK tracking)
 * - Buffering sin WiFi: encolar aunque no haya conexión, enviar cuando vuelva
 * - Deduplicación con BOOT_ID (resuelve reinicio + SEQ vuelve a 1)
 * - Política de overflow: URGENT nunca se descarta, BACKGROUND sí
 * - BOOT_ID generado al boot (random)
 *
 * Uso:
 *   IoTNode node(MY_DEVICE_ID, UDP_PORT);
 *   node.begin();
 *   node.sendEvent(EventCode::MOTION, destIP, destPort);
 *   node.loop();  // cada iteración
 */

#pragma once
#include "IoTProtocol.h"
#include <WiFiUdp.h>

// ============================================================
// Configuración
// ============================================================

#define IOT_QUEUE_SIZE        8     // Capacidad de la cola FIFO
#define IOT_MAX_REMOTES       8     // Tabla de dispositivos remotos
#define IOT_MAX_RETRIES       5     // Reintentos para reliable

// Timeouts (ms)
#define IOT_ACK_TIMEOUT_BASE    300
#define IOT_ACK_TIMEOUT_MAX     2000
#define IOT_HEARTBEAT_INTERVAL  60000   // 60s default

// ============================================================
// Política de overflow de cola
// ============================================================

enum class QueueOverflow : uint8_t {
    DROP_NEWEST,     // Rechazar el nuevo paquete
    DROP_OLDEST_BG,  // Descartar el BACKGROUND más viejo (default)
    DROP_OLDEST,     // Descartar el más viejo sin importar prioridad
};

// ============================================================
// Callback
// ============================================================

typedef void (*IoTPacketHandler)(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort);

// ============================================================
// Dispositivo remoto conocido (para deduplicación y registry)
// ============================================================

struct RemoteDevice {
    uint8_t   id;
    IPAddress ip;
    uint16_t  port;

    // Deduplicación (con BOOT_ID)
    uint16_t  lastBootId;
    uint32_t  lastSeq;

    // Estado
    unsigned long lastSeen;       // millis() del último paquete recibido
    bool      active;
};

// ============================================================
// Entrada en la cola FIFO
// ============================================================

struct QueueEntry {
    IoTPacket pkt;
    IPAddress destIP;
    uint16_t  destPort;
    Priority  priority;
    bool      occupied;    // Slot está en uso
};

// ============================================================
// Estado del canal reliable (1 solo paquete en vuelo)
// ============================================================

struct ReliableChannel {
    IoTPacket pkt;
    IPAddress destIP;
    uint16_t  destPort;
    uint8_t   attempt;        // Intento actual (0-based)
    uint8_t   maxAttempts;
    unsigned long nextRetryAt; // millis() del próximo envío/reintento
    bool      active;          // Hay algo en vuelo
    bool      waitingAck;      // Esperando ACK
};

// ============================================================
// IoTNode — Clase principal V4.1
// ============================================================

class IoTNode {
public:
    IoTNode(uint8_t deviceId, uint16_t udpPort);

    // --- Inicialización ---
    void begin();

    // --- Loop (llamar cada iteración) ---
    void loop();

    // --- Envío de alto nivel ---

    /**
     * Encolar un evento simple. ACK_REQUIRED + RELIABLE por defecto.
     * Funciona aunque no haya WiFi — se envía cuando conecte.
     */
    bool sendEvent(EventCode event, IPAddress destIP, uint16_t destPort,
                   uint8_t destId = IOT_DEVICE_CENTRAL);

    /**
     * Encolar un paquete genérico pre-armado.
     * La prioridad se deduce de pkt.flags.
     */
    bool enqueue(IoTPacket &pkt, IPAddress destIP, uint16_t destPort);

    /**
     * Envío directo UDP (sin cola, sin ACK). Para ACKs y heartbeats.
     */
    void sendDirect(const IoTPacket &pkt, IPAddress destIP, uint16_t destPort);

    // --- Heartbeat ---
    void enableHeartbeat(IPAddress destIP, uint16_t destPort,
                         unsigned long intervalMs = IOT_HEARTBEAT_INTERVAL);
    void disableHeartbeat();

    // --- Discovery ---
    void sendHello(IPAddress destIP, uint16_t destPort, DeviceType devType, const char* devName);

    // --- Callback ---
    void onPacketReceived(IoTPacketHandler handler);

    // --- Configuración ---
    void setOverflowPolicy(QueueOverflow policy);

    // --- Info ---
    uint8_t  getDeviceId() const { return _deviceId; }
    uint16_t getBootId() const   { return _bootId; }
    uint32_t getNextSeq();
    uint8_t  queuedCount() const;
    bool     isQueueEmpty() const;
    bool     isQueueFull() const;
    bool     isReliableInFlight() const { return _reliable.active; }

    // --- Remotos ---
    RemoteDevice* getRemote(uint8_t id);
    void registerRemote(uint8_t id, IPAddress ip, uint16_t port);

private:
    uint8_t  _deviceId;
    uint16_t _udpPort;
    uint16_t _bootId;      // Random al begin()
    uint32_t _seq;
    WiFiUDP  _udp;

    // Cola FIFO
    QueueEntry _queue[IOT_QUEUE_SIZE];
    uint8_t    _queueCount;
    QueueOverflow _overflowPolicy;

    // Canal reliable (1 paquete en vuelo)
    ReliableChannel _reliable;

    // Dispositivos remotos
    RemoteDevice _remotes[IOT_MAX_REMOTES];

    // Heartbeat
    bool _hbEnabled;
    IPAddress _hbIP;
    uint16_t _hbPort;
    unsigned long _hbInterval;
    unsigned long _lastHb;

    // Callback
    IoTPacketHandler _handler;

    // Buffer de recepción
    uint8_t _rxBuf[IOT_MAX_PACKET];

    // --- Internos ---
    void _processIncoming();
    void _processQueue();
    void _processReliable();
    void _sendHeartbeat();
    void _handleAck(const IoTPacket &pkt);
    void _sendAutoAck(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort);
    bool _isDuplicate(uint8_t srcId, uint16_t bootId, uint32_t seq);
    void _updateRemote(uint8_t srcId, IPAddress ip, uint16_t port, uint16_t bootId, uint32_t seq);
    void _transmitPacket(const IoTPacket &pkt, IPAddress destIP, uint16_t destPort);
    int  _findHighestPriorityEntry() const;
    int  _findLowestPriorityEntry() const;
    unsigned long _calcBackoff(uint8_t attempt);
};
