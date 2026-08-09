/**
 * IoTNode — Nodo de red IoTProtocol V4
 *
 * Encapsula toda la lógica de comunicación UDP:
 * - Envío con reintentos (backoff exponencial)
 * - Recepción y despacho de paquetes
 * - Cola de eventos salientes
 * - Heartbeat automático
 * - ACK automático
 * - Discovery (HELLO/HELLO_ACK)
 *
 * Uso típico:
 *   IoTNode node(myDeviceId, udpPort);
 *   node.begin();
 *   node.sendEvent(EventCode::MOTION);
 *   node.loop();  // en cada iteración
 */

#pragma once
#include "IoTProtocol.h"
#include <WiFiUdp.h>

// ============================================================
// Configuración
// ============================================================

#define IOT_QUEUE_SIZE        8     // Cola de paquetes salientes
#define IOT_MAX_REMOTES       8     // Dispositivos remotos conocidos
#define IOT_MAX_RETRIES       5     // Reintentos para paquetes reliable

// Intervalos por defecto (ms)
#define IOT_HEARTBEAT_INTERVAL  60000    // 60 segundos
#define IOT_ACK_TIMEOUT_BASE    300      // Timeout base para ACK
#define IOT_ACK_TIMEOUT_MAX     2000     // Timeout máximo (backoff)

// ============================================================
// Callback para paquetes recibidos
// ============================================================

typedef void (*IoTPacketHandler)(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort);

// ============================================================
// Estado de un dispositivo remoto conocido
// ============================================================

struct RemoteDevice {
    uint8_t   id;
    IPAddress ip;
    uint16_t  port;
    uint16_t  lastSeq;
    unsigned long lastSeen;
    bool      active;
};

// ============================================================
// Entrada en la cola de envío
// ============================================================

struct QueueEntry {
    IoTPacket pkt;
    IPAddress destIP;
    uint16_t  destPort;
    uint8_t   retries;
    uint8_t   maxRetries;
    unsigned long nextRetryAt;
    unsigned long timeoutMs;
    bool      active;
    bool      waitingAck;
};

// ============================================================
// IoTNode — Clase principal
// ============================================================

class IoTNode {
public:
    IoTNode(uint8_t deviceId, uint16_t udpPort);

    // Inicializar UDP
    void begin();

    // Loop principal — llamar en cada iteración
    void loop();

    // --- Envío de mensajes ---

    // Enviar evento simple (ACK required, reliable, con backoff)
    bool sendEvent(EventCode event, IPAddress destIP, uint16_t destPort, uint8_t destId = IOT_DEVICE_CENTRAL);

    // Enviar paquete genérico (lo encola)
    bool sendPacket(IoTPacket &pkt, IPAddress destIP, uint16_t destPort);

    // Enviar paquete directo sin cola (para ACKs, heartbeats)
    void sendDirect(IoTPacket &pkt, IPAddress destIP, uint16_t destPort);

    // --- Heartbeat ---
    void enableHeartbeat(IPAddress destIP, uint16_t destPort, unsigned long intervalMs = IOT_HEARTBEAT_INTERVAL);
    void disableHeartbeat();

    // --- Discovery ---
    void sendHello(IPAddress destIP, uint16_t destPort);

    // --- Callbacks ---
    void onPacketReceived(IoTPacketHandler handler);

    // --- Info ---
    uint8_t getDeviceId() const { return _deviceId; }
    uint16_t getNextSeq();
    bool isQueueEmpty() const;
    uint8_t queuedCount() const;

    // --- Dispositivos remotos ---
    RemoteDevice* getRemote(uint8_t id);
    void registerRemote(uint8_t id, IPAddress ip, uint16_t port);

private:
    uint8_t  _deviceId;
    uint16_t _udpPort;
    uint16_t _seq;
    WiFiUDP  _udp;

    // Cola de envío
    QueueEntry _queue[IOT_QUEUE_SIZE];

    // Dispositivos conocidos
    RemoteDevice _remotes[IOT_MAX_REMOTES];

    // Heartbeat
    bool _heartbeatEnabled;
    IPAddress _heartbeatIP;
    uint16_t _heartbeatPort;
    unsigned long _heartbeatInterval;
    unsigned long _lastHeartbeat;

    // Callback
    IoTPacketHandler _handler;

    // Recepción
    uint8_t _rxBuf[IOT_MAX_PACKET];

    // Internos
    void _processIncoming();
    void _processQueue();
    void _sendHeartbeat();
    void _handleAck(const IoTPacket &pkt);
    void _sendAutoAck(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort);
    bool _isDuplicate(uint8_t srcId, uint16_t seq);
    void _updateRemote(uint8_t srcId, IPAddress ip, uint16_t port, uint16_t seq);
    unsigned long _calcBackoff(uint8_t attempt);
};
