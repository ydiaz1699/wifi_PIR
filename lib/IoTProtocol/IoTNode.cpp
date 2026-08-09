/**
 * IoTNode V4 — Implementación del nodo de comunicación
 */

#include "IoTNode.h"
#include <string.h>

// ============================================================
// Constructor
// ============================================================

IoTNode::IoTNode(uint8_t deviceId, uint16_t udpPort)
    : _deviceId(deviceId), _udpPort(udpPort), _seq(0),
      _heartbeatEnabled(false), _heartbeatInterval(IOT_HEARTBEAT_INTERVAL),
      _lastHeartbeat(0), _handler(nullptr)
{
    memset(_queue, 0, sizeof(_queue));
    memset(_remotes, 0, sizeof(_remotes));
}

void IoTNode::begin() {
    _udp.begin(_udpPort);
}

// ============================================================
// Loop principal
// ============================================================

void IoTNode::loop() {
    _processIncoming();
    _processQueue();
    if (_heartbeatEnabled) _sendHeartbeat();
}


// ============================================================
// Secuencia
// ============================================================

uint16_t IoTNode::getNextSeq() {
    _seq++;
    if (_seq == 0) _seq = 1;  // Evitar SEQ=0 (reservado)
    return _seq;
}

// ============================================================
// Envío de evento simple
// ============================================================

bool IoTNode::sendEvent(EventCode event, IPAddress destIP, uint16_t destPort, uint8_t destId) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::EVENT;
    pkt.src = _deviceId;
    pkt.dst = destId;
    pkt.seq = getNextSeq();
    pkt.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    pkt.clearPayload();
    pkt.addTLV_uint8(TlvTag::EVENT_TYPE, static_cast<uint8_t>(event));
    pkt.addTLV_uint8(TlvTag::EVENT_VALUE, 1);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());

    return sendPacket(pkt, destIP, destPort);
}


// ============================================================
// Envío con cola (para paquetes reliable)
// ============================================================

bool IoTNode::sendPacket(IoTPacket &pkt, IPAddress destIP, uint16_t destPort) {
    // Buscar slot libre
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        if (!_queue[i].active) {
            _queue[i].pkt = pkt;
            _queue[i].destIP = destIP;
            _queue[i].destPort = destPort;
            _queue[i].retries = 0;
            _queue[i].maxRetries = pkt.isReliable() ? IOT_MAX_RETRIES : 1;
            _queue[i].nextRetryAt = 0;  // Enviar inmediatamente
            _queue[i].timeoutMs = IOT_ACK_TIMEOUT_BASE;
            _queue[i].active = true;
            _queue[i].waitingAck = false;
            return true;
        }
    }
    return false;  // Cola llena
}

// Envío directo sin cola
void IoTNode::sendDirect(IoTPacket &pkt, IPAddress destIP, uint16_t destPort) {
    uint8_t buf[IOT_MAX_PACKET];
    size_t len = iot_serialize(pkt, buf, sizeof(buf));
    if (len > 0) {
        _udp.beginPacket(destIP, destPort);
        _udp.write(buf, len);
        _udp.endPacket();
    }
}


// ============================================================
// Procesar cola de envío (backoff exponencial)
// ============================================================

void IoTNode::_processQueue() {
    unsigned long ahora = millis();

    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        QueueEntry &q = _queue[i];
        if (!q.active) continue;

        // Si estamos esperando ACK y no ha pasado el timeout, seguir esperando
        if (q.waitingAck && ahora < q.nextRetryAt) continue;

        // Si estamos esperando ACK y pasó el timeout
        if (q.waitingAck) {
            q.retries++;
            if (q.retries >= q.maxRetries) {
                // Fallo: no se pudo entregar
                q.active = false;
                continue;
            }
            q.waitingAck = false;
            // Siguiente intento con backoff
        }

        // Enviar
        uint8_t buf[IOT_MAX_PACKET];
        size_t len = iot_serialize(q.pkt, buf, sizeof(buf));
        if (len > 0) {
            _udp.beginPacket(q.destIP, q.destPort);
            _udp.write(buf, len);
            _udp.endPacket();
        }

        if (q.pkt.needsAck()) {
            q.waitingAck = true;
            q.nextRetryAt = ahora + _calcBackoff(q.retries);
        } else {
            // No necesita ACK, envío único
            q.active = false;
        }
    }
}

unsigned long IoTNode::_calcBackoff(uint8_t attempt) {
    // Backoff exponencial: 300, 500, 800, 1300, 2000
    unsigned long base = IOT_ACK_TIMEOUT_BASE;
    for (uint8_t i = 0; i < attempt; i++) {
        base = (base * 3) / 2;
        if (base > IOT_ACK_TIMEOUT_MAX) {
            base = IOT_ACK_TIMEOUT_MAX;
            break;
        }
    }
    return base;
}


// ============================================================
// Recepción de paquetes
// ============================================================

void IoTNode::_processIncoming() {
    int packetSize = _udp.parsePacket();
    if (!packetSize) return;

    if (packetSize > (int)sizeof(_rxBuf)) {
        // Paquete demasiado grande, descartar
        _udp.flush();
        return;
    }

    int len = _udp.read(_rxBuf, sizeof(_rxBuf));
    if (len <= 0) return;

    IPAddress remoteIP = _udp.remoteIP();
    uint16_t remotePort = _udp.remotePort();

    IoTPacket pkt;
    if (!iot_deserialize(_rxBuf, len, pkt)) {
        return;  // Paquete inválido (magic, CRC, etc.)
    }

    // Verificar que es para nosotros (o broadcast)
    if (pkt.dst != _deviceId && pkt.dst != IOT_DEVICE_BROADCAST) {
        return;
    }

    // Actualizar tabla de remotos
    _updateRemote(pkt.src, remoteIP, remotePort, pkt.seq);

    // Si es un ACK, procesarlo contra la cola
    if (pkt.type == MsgType::ACK) {
        _handleAck(pkt);
        return;
    }

    // Si el paquete requiere ACK, enviarlo automáticamente
    if (pkt.needsAck()) {
        _sendAutoAck(pkt, remoteIP, remotePort);
    }

    // Verificar duplicado (solo para eventos/datos reliable)
    if (pkt.isReliable() && _isDuplicate(pkt.src, pkt.seq)) {
        return;  // Ya procesado, ACK ya se reenvió
    }

    // Despachar al handler del usuario
    if (_handler) {
        _handler(pkt, remoteIP, remotePort);
    }
}


// ============================================================
// ACK automático
// ============================================================

void IoTNode::_sendAutoAck(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    IoTPacket ack;
    ack.version = IOT_PROTOCOL_VER;
    ack.type = MsgType::ACK;
    ack.src = _deviceId;
    ack.dst = pkt.src;
    ack.seq = pkt.seq;   // Echo del SEQ original
    ack.flags = 0;
    ack.clearPayload();

    sendDirect(ack, remoteIP, remotePort);
}

// Procesar ACK recibido: marcar como entregado en la cola
void IoTNode::_handleAck(const IoTPacket &pkt) {
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        QueueEntry &q = _queue[i];
        if (q.active && q.waitingAck && q.pkt.seq == pkt.seq && q.pkt.dst == pkt.src) {
            q.active = false;  // Entregado exitosamente
            return;
        }
    }
}

// ============================================================
// Deduplicación
// ============================================================

bool IoTNode::_isDuplicate(uint8_t srcId, uint16_t seq) {
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == srcId) {
            if (_remotes[i].lastSeq == seq) return true;
            _remotes[i].lastSeq = seq;
            return false;
        }
    }
    return false;  // Dispositivo nuevo, no es duplicado
}

// ============================================================
// Tabla de dispositivos remotos
// ============================================================

void IoTNode::_updateRemote(uint8_t srcId, IPAddress ip, uint16_t port, uint16_t seq) {
    // Buscar existente
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == srcId) {
            _remotes[i].ip = ip;
            _remotes[i].port = port;
            _remotes[i].lastSeq = seq;
            _remotes[i].lastSeen = millis();
            return;
        }
    }
    // Slot nuevo
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (!_remotes[i].active) {
            _remotes[i] = { srcId, ip, port, seq, millis(), true };
            return;
        }
    }
    // Tabla llena: reciclar el más viejo
    int oldest = 0;
    for (int i = 1; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].lastSeen < _remotes[oldest].lastSeen) oldest = i;
    }
    _remotes[oldest] = { srcId, ip, port, seq, millis(), true };
}

void IoTNode::registerRemote(uint8_t id, IPAddress ip, uint16_t port) {
    _updateRemote(id, ip, port, 0);
}

RemoteDevice* IoTNode::getRemote(uint8_t id) {
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == id) return &_remotes[i];
    }
    return nullptr;
}


// ============================================================
// Heartbeat
// ============================================================

void IoTNode::enableHeartbeat(IPAddress destIP, uint16_t destPort, unsigned long intervalMs) {
    _heartbeatEnabled = true;
    _heartbeatIP = destIP;
    _heartbeatPort = destPort;
    _heartbeatInterval = intervalMs;
    _lastHeartbeat = millis();
}

void IoTNode::disableHeartbeat() {
    _heartbeatEnabled = false;
}

void IoTNode::_sendHeartbeat() {
    unsigned long ahora = millis();
    if (ahora - _lastHeartbeat < _heartbeatInterval) return;
    _lastHeartbeat = ahora;

    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::HEARTBEAT;
    pkt.src = _deviceId;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.seq = getNextSeq();
    pkt.flags = 0;  // Heartbeat no requiere ACK
    pkt.clearPayload();
    pkt.addTLV_uint32(TlvTag::UPTIME_SEC, millis() / 1000);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());

    sendDirect(pkt, _heartbeatIP, _heartbeatPort);
}

// ============================================================
// Discovery (HELLO)
// ============================================================

void IoTNode::sendHello(IPAddress destIP, uint16_t destPort) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::HELLO;
    pkt.src = _deviceId;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.seq = getNextSeq();
    pkt.flags = IOT_FLAG_ACK_REQUIRED;
    pkt.clearPayload();
    // El emisor puede agregar su tipo y nombre aquí

    sendPacket(pkt, destIP, destPort);
}

// ============================================================
// Callbacks
// ============================================================

void IoTNode::onPacketReceived(IoTPacketHandler handler) {
    _handler = handler;
}

// ============================================================
// Info
// ============================================================

bool IoTNode::isQueueEmpty() const {
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        if (_queue[i].active) return false;
    }
    return true;
}

uint8_t IoTNode::queuedCount() const {
    uint8_t count = 0;
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        if (_queue[i].active) count++;
    }
    return count;
}
