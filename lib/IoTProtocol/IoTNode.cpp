/**
 * IoTNode V4.1.1 — Implementación con hardening
 *
 * Fixes vs V4.1:
 * - Dedup Window: ventana circular de últimos 8 SEQ por dispositivo
 * - ACK siempre se envía (incluso para duplicados)
 * - Device Registry ampliado (type, name, fw, state)
 * - ONLINE/STALE/OFFLINE state machine
 * - _fillRemoteFromHello() para registrar info de HELLO
 */

#include "IoTNode.h"
#include <string.h>

// ============================================================
// Constructor + begin
// ============================================================

IoTNode::IoTNode(uint8_t deviceId, uint16_t udpPort)
    : _deviceId(deviceId), _udpPort(udpPort), _bootId(0), _seq(0),
      _queueCount(0), _overflowPolicy(QueueOverflow::DROP_OLDEST_BG),
      _hbEnabled(false), _hbInterval(IOT_HEARTBEAT_INTERVAL),
      _lastHb(0), _lastStateCheck(0), _handler(nullptr)
{
    memset(_queue, 0, sizeof(_queue));
    memset(_remotes, 0, sizeof(_remotes));
    memset(&_reliable, 0, sizeof(_reliable));
}

void IoTNode::begin() {
    randomSeed(analogRead(A0) ^ micros());
    _bootId = (uint16_t)(random(1, 65535));
    _seq = 0;
    _lastStateCheck = millis();
    _udp.begin(_udpPort);
}

// ============================================================
// Loop
// ============================================================

void IoTNode::loop() {
    _processIncoming();
    _processReliable();
    _processQueue();
    if (_hbEnabled) _sendHeartbeat();

    // Actualizar estados cada 10 segundos
    unsigned long ahora = millis();
    if (ahora - _lastStateCheck >= 10000) {
        _lastStateCheck = ahora;
        _updateDeviceStates();
    }
}

// ============================================================
// Transmisión UDP
// ============================================================

void IoTNode::_transmitPacket(const IoTPacket &pkt, IPAddress destIP, uint16_t destPort) {
    uint8_t buf[IOT_MAX_PACKET];
    size_t len = iot_serialize(pkt, buf, sizeof(buf));
    if (len > 0) {
        _udp.beginPacket(destIP, destPort);
        _udp.write(buf, len);
        _udp.endPacket();
    }
}

void IoTNode::sendDirect(const IoTPacket &pkt, IPAddress destIP, uint16_t destPort) {
    if (WiFi.status() != WL_CONNECTED) return;
    _transmitPacket(pkt, destIP, destPort);
}

// ============================================================
// Canal reliable
// ============================================================

void IoTNode::_processReliable() {
    if (!_reliable.active) return;
    unsigned long ahora = millis();

    if (_reliable.waitingAck && ahora < _reliable.nextRetryAt) return;

    if (_reliable.waitingAck) {
        _reliable.attempt++;
        if (_reliable.attempt >= _reliable.maxAttempts) {
            _reliable.active = false;
            return;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        _transmitPacket(_reliable.pkt, _reliable.destIP, _reliable.destPort);
        _reliable.waitingAck = true;
        _reliable.nextRetryAt = ahora + _calcBackoff(_reliable.attempt);
    } else {
        _reliable.nextRetryAt = ahora + 1000;
    }
}

unsigned long IoTNode::_calcBackoff(uint8_t attempt) {
    unsigned long ms = IOT_ACK_TIMEOUT_BASE;
    for (uint8_t i = 0; i < attempt; i++) {
        ms = (ms * 3) / 2;
        if (ms > IOT_ACK_TIMEOUT_MAX) { ms = IOT_ACK_TIMEOUT_MAX; break; }
    }
    return ms;
}

// ============================================================
// Cola FIFO con prioridades
// ============================================================

void IoTNode::_processQueue() {
    if (_reliable.active) return;
    if (_queueCount == 0) return;

    int idx = _findHighestPriorityEntry();
    if (idx < 0) return;

    QueueEntry &entry = _queue[idx];

    if (entry.pkt.isReliable()) {
        _reliable.pkt = entry.pkt;
        _reliable.destIP = entry.destIP;
        _reliable.destPort = entry.destPort;
        _reliable.attempt = 0;
        _reliable.maxAttempts = IOT_MAX_RETRIES;
        _reliable.nextRetryAt = 0;
        _reliable.active = true;
        _reliable.waitingAck = false;
    } else {
        if (WiFi.status() == WL_CONNECTED) {
            _transmitPacket(entry.pkt, entry.destIP, entry.destPort);
        }
    }

    entry.occupied = false;
    _queueCount--;
}

uint32_t IoTNode::getNextSeq() {
    _seq++;
    if (_seq == 0) _seq = 1;
    return _seq;
}

uint8_t IoTNode::queuedCount() const { return _queueCount; }
bool IoTNode::isQueueEmpty() const { return _queueCount == 0; }
bool IoTNode::isQueueFull() const { return _queueCount >= IOT_QUEUE_SIZE; }

int IoTNode::_findHighestPriorityEntry() const {
    int best = -1;
    Priority bestPri = Priority::BACKGROUND;
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        if (!_queue[i].occupied) continue;
        if (best == -1 || _queue[i].priority < bestPri) {
            best = i; bestPri = _queue[i].priority;
        }
    }
    return best;
}

int IoTNode::_findLowestPriorityEntry() const {
    int worst = -1;
    Priority worstPri = Priority::URGENT;
    for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
        if (!_queue[i].occupied) continue;
        if (worst == -1 || _queue[i].priority > worstPri) {
            worst = i; worstPri = _queue[i].priority;
        }
    }
    return worst;
}

bool IoTNode::enqueue(IoTPacket &pkt, IPAddress destIP, uint16_t destPort) {
    Priority pri = pkt.priority();

    if (!isQueueFull()) {
        for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
            if (!_queue[i].occupied) {
                _queue[i].pkt = pkt;
                _queue[i].destIP = destIP;
                _queue[i].destPort = destPort;
                _queue[i].priority = pri;
                _queue[i].occupied = true;
                _queueCount++;
                return true;
            }
        }
    }

    switch (_overflowPolicy) {
        case QueueOverflow::DROP_NEWEST:
            return false;
        case QueueOverflow::DROP_OLDEST_BG: {
            if (pri == Priority::BACKGROUND) return false;
            int victim = -1;
            for (int i = 0; i < IOT_QUEUE_SIZE; i++) {
                if (_queue[i].occupied && _queue[i].priority == Priority::BACKGROUND) {
                    victim = i; break;
                }
            }
            if (victim < 0) return false;
            _queue[victim].pkt = pkt;
            _queue[victim].destIP = destIP;
            _queue[victim].destPort = destPort;
            _queue[victim].priority = pri;
            return true;
        }
        case QueueOverflow::DROP_OLDEST: {
            int victim = _findLowestPriorityEntry();
            if (victim < 0) return false;
            if (_queue[victim].priority < pri) return false;
            _queue[victim].pkt = pkt;
            _queue[victim].destIP = destIP;
            _queue[victim].destPort = destPort;
            _queue[victim].priority = pri;
            return true;
        }
    }
    return false;
}

void IoTNode::setOverflowPolicy(QueueOverflow policy) {
    _overflowPolicy = policy;
}

// ============================================================
// Recepción — FIX: ACK se envía SIEMPRE (antes de dedup check)
// ============================================================

void IoTNode::_processIncoming() {
    int packetSize = _udp.parsePacket();
    if (!packetSize) return;
    if (packetSize > (int)sizeof(_rxBuf)) { _udp.flush(); return; }

    int len = _udp.read(_rxBuf, sizeof(_rxBuf));
    if (len <= 0) return;

    IPAddress remoteIP = _udp.remoteIP();
    uint16_t remotePort = _udp.remotePort();

    IoTPacket pkt;
    if (!iot_deserialize(_rxBuf, len, pkt)) return;

    if (pkt.dst != _deviceId && pkt.dst != IOT_DEVICE_BROADCAST) return;

    // Actualizar tabla de remotos (IP, port, lastSeen)
    _updateRemote(pkt.src, remoteIP, remotePort, pkt.bootId, pkt.seq);

    // ACK propio → procesar contra canal reliable
    if (pkt.type == MsgType::ACK) {
        _handleAck(pkt);
        return;
    }

    // *** FIX V4.1.1: ACK se envía SIEMPRE, incluso para duplicados ***
    // Si el emisor retransmite porque no recibió nuestro ACK anterior,
    // debemos responder ACK de nuevo aunque ya hayamos procesado el evento.
    if (pkt.needsAck()) {
        _sendAutoAck(pkt, remoteIP, remotePort);
    }

    // Deduplicación con ventana (solo para reliable)
    if (pkt.isReliable() && _isDuplicate(pkt.src, pkt.bootId, pkt.seq)) {
        // ACK ya se envió arriba, pero NO volvemos a procesar el evento
        return;
    }

    // Si es HELLO, llenar el registry con la info del dispositivo
    if (pkt.type == MsgType::HELLO) {
        RemoteDevice* dev = getRemote(pkt.src);
        if (dev) _fillRemoteFromHello(*dev, pkt);
    }

    // Despachar al handler del usuario
    if (_handler) {
        _handler(pkt, remoteIP, remotePort);
    }
}

// ============================================================
// ACK
// ============================================================

void IoTNode::_sendAutoAck(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    IoTPacket ack;
    ack.version = IOT_PROTOCOL_VER;
    ack.type = MsgType::ACK;
    ack.src = _deviceId;
    ack.dst = pkt.src;
    ack.bootId = _bootId;
    ack.seq = pkt.seq;
    ack.flags = 0;
    ack.clearPayload();
    sendDirect(ack, remoteIP, remotePort);
}

void IoTNode::_handleAck(const IoTPacket &pkt) {
    if (!_reliable.active || !_reliable.waitingAck) return;
    if (pkt.seq == _reliable.pkt.seq && pkt.src == _reliable.pkt.dst) {
        _reliable.active = false;
    }
}

// ============================================================
// Deduplicación con VENTANA (Fix V4.1.1)
//
// Mantiene una ventana circular de los últimos IOT_DEDUP_WINDOW SEQ
// procesados por cada remoto. Si el SEQ entrante está en la ventana,
// es duplicado. Si BOOT_ID cambió, se resetea la ventana.
// ============================================================

bool IoTNode::_isDuplicate(uint8_t srcId, uint16_t bootId, uint32_t seq) {
    RemoteDevice* dev = nullptr;
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == srcId) {
            dev = &_remotes[i];
            break;
        }
    }
    if (!dev) return false;  // Dispositivo desconocido → no es duplicado

    // Si BOOT_ID cambió → dispositivo reinició → resetear ventana
    if (dev->bootId != bootId) {
        dev->bootId = bootId;
        dev->seqWindowCount = 0;
        dev->seqWindowHead = 0;
        // Insertar este SEQ como el primero
        dev->seqWindow[0] = seq;
        dev->seqWindowCount = 1;
        dev->seqWindowHead = 1;
        return false;
    }

    // Buscar en la ventana
    uint8_t count = dev->seqWindowCount;
    for (uint8_t i = 0; i < count; i++) {
        if (dev->seqWindow[i] == seq) return true;  // DUPLICADO
    }

    // No encontrado → nuevo, insertar en ventana circular
    dev->seqWindow[dev->seqWindowHead] = seq;
    dev->seqWindowHead = (dev->seqWindowHead + 1) % IOT_DEDUP_WINDOW;
    if (dev->seqWindowCount < IOT_DEDUP_WINDOW) dev->seqWindowCount++;

    return false;
}

// ============================================================
// Tabla de remotos — Registry ampliado (Fix V4.1.1)
// ============================================================

void IoTNode::_updateRemote(uint8_t srcId, IPAddress ip, uint16_t port,
                            uint16_t bootId, uint32_t seq) {
    // Buscar existente
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == srcId) {
            _remotes[i].ip = ip;
            _remotes[i].port = port;
            _remotes[i].lastSeen = millis();
            _remotes[i].state = DeviceState::ONLINE;
            return;
        }
    }
    // Slot libre → crear nuevo
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (!_remotes[i].active) {
            memset(&_remotes[i], 0, sizeof(RemoteDevice));
            _remotes[i].id = srcId;
            _remotes[i].ip = ip;
            _remotes[i].port = port;
            _remotes[i].bootId = bootId;
            _remotes[i].seqWindowCount = 0;
            _remotes[i].seqWindowHead = 0;
            _remotes[i].lastSeen = millis();
            _remotes[i].state = DeviceState::ONLINE;
            _remotes[i].active = true;
            return;
        }
    }
    // Tabla llena: reciclar el más viejo
    int oldest = 0;
    for (int i = 1; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].lastSeen < _remotes[oldest].lastSeen) oldest = i;
    }
    memset(&_remotes[oldest], 0, sizeof(RemoteDevice));
    _remotes[oldest].id = srcId;
    _remotes[oldest].ip = ip;
    _remotes[oldest].port = port;
    _remotes[oldest].bootId = bootId;
    _remotes[oldest].lastSeen = millis();
    _remotes[oldest].state = DeviceState::ONLINE;
    _remotes[oldest].active = true;
}

void IoTNode::_fillRemoteFromHello(RemoteDevice &dev, const IoTPacket &pkt) {
    uint8_t devType = 0;
    if (pkt.getTLV_uint8(TlvTag::DEVICE_TYPE_TAG, devType)) {
        dev.deviceType = static_cast<DeviceType>(devType);
    }
    pkt.getTLV_string(TlvTag::DEVICE_NAME, dev.name, sizeof(dev.name));
    pkt.getTLV_string(TlvTag::FW_VERSION, dev.fwVersion, sizeof(dev.fwVersion));
}

void IoTNode::registerRemote(uint8_t id, IPAddress ip, uint16_t port) {
    _updateRemote(id, ip, port, 0, 0);
}

RemoteDevice* IoTNode::getRemote(uint8_t id) {
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active && _remotes[i].id == id) return &_remotes[i];
    }
    return nullptr;
}

uint8_t IoTNode::getRemoteCount() const {
    uint8_t count = 0;
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (_remotes[i].active) count++;
    }
    return count;
}

// ============================================================
// ONLINE / STALE / OFFLINE state machine (Fix V4.1.1)
// ============================================================

void IoTNode::_updateDeviceStates() {
    unsigned long ahora = millis();
    for (int i = 0; i < IOT_MAX_REMOTES; i++) {
        if (!_remotes[i].active) continue;
        unsigned long elapsed = ahora - _remotes[i].lastSeen;

        DeviceState newState;
        if (elapsed < IOT_STALE_TIMEOUT_MS) {
            newState = DeviceState::ONLINE;
        } else if (elapsed < IOT_OFFLINE_TIMEOUT_MS) {
            newState = DeviceState::STALE;
        } else {
            newState = DeviceState::OFFLINE;
        }
        _remotes[i].state = newState;
    }
}

void IoTNode::updateDeviceStates() {
    _updateDeviceStates();
}

// ============================================================
// Heartbeat
// ============================================================

void IoTNode::enableHeartbeat(IPAddress destIP, uint16_t destPort, unsigned long intervalMs) {
    _hbEnabled = true;
    _hbIP = destIP;
    _hbPort = destPort;
    _hbInterval = intervalMs;
    _lastHb = millis();
}

void IoTNode::disableHeartbeat() { _hbEnabled = false; }

void IoTNode::_sendHeartbeat() {
    unsigned long ahora = millis();
    if (ahora - _lastHb < _hbInterval) return;
    _lastHb = ahora;

    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::HEARTBEAT;
    pkt.src = _deviceId;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.bootId = _bootId;
    pkt.seq = getNextSeq();
    pkt.flags = IOT_FLAG_BACKGROUND;
    pkt.clearPayload();
    pkt.addTLV_uint32(TlvTag::UPTIME_SEC, millis() / 1000);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());

    sendDirect(pkt, _hbIP, _hbPort);
}

// ============================================================
// Discovery (HELLO)
// ============================================================

void IoTNode::sendHello(IPAddress destIP, uint16_t destPort,
                        DeviceType devType, const char* devName) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::HELLO;
    pkt.src = _deviceId;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.bootId = _bootId;
    pkt.seq = getNextSeq();
    pkt.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    pkt.clearPayload();
    pkt.addTLV_uint8(TlvTag::DEVICE_TYPE_TAG, static_cast<uint8_t>(devType));
    pkt.addTLV_string(TlvTag::DEVICE_NAME, devName);
    pkt.addTLV_string(TlvTag::FW_VERSION, "4.1.1");
    pkt.addTLV_uint16(TlvTag::BOOT_ID_TAG, _bootId);

    enqueue(pkt, destIP, destPort);
}

// ============================================================
// Callback + sendEvent
// ============================================================

void IoTNode::onPacketReceived(IoTPacketHandler handler) {
    _handler = handler;
}

bool IoTNode::sendEvent(EventCode event, IPAddress destIP, uint16_t destPort, uint8_t destId) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::EVENT;
    pkt.src = _deviceId;
    pkt.dst = destId;
    pkt.bootId = _bootId;
    pkt.seq = getNextSeq();
    pkt.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE;
    pkt.clearPayload();
    pkt.addTLV_uint8(TlvTag::EVENT_TYPE, static_cast<uint8_t>(event));
    pkt.addTLV_uint8(TlvTag::EVENT_VALUE, 1);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());

    return enqueue(pkt, destIP, destPort);
}
