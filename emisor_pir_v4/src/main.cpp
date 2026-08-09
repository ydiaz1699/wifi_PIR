/**
 * Emisor PIR + Timbre — IoTProtocol V4.1
 *
 * Características:
 * - Cola de 8 eventos (PIR + timbre nunca se pisan)
 * - Backoff exponencial automático
 * - BOOT_ID resuelve reinicio + SEQ vuelve a 1
 * - Heartbeat cada 60s (central sabe que está vivo)
 * - HELLO al boot (discovery automático)
 * - CRC16 en cada paquete
 * - Encola sin WiFi (envía cuando reconecta)
 *
 * Para nuevo sensor: solo cambiar device_config.h/cpp
 */

#include <ESP8266WiFi.h>
#include <IoTNode.h>
#include "secrets.h"
#include "network_config.h"
#include "device_config.h"
#include "logger.h"

// --- IoTProtocol Node ---
IoTNode node(MY_DEVICE_ID, UDP_PORT);

// --- Estado de sensores ---
bool pirAnterior = LOW;
bool timbreAnterior = HIGH;  // Pull-up: reposo HIGH
unsigned long ultimaDeteccionPIR = 0;
unsigned long ultimaDeteccionTimbre = 0;

// --- WiFi ---
static bool wifiConectando = false;
static unsigned long ultimoIntentoWiFi = 0;

static void iniciarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.config(dispositivo_IP, redGateway(), redSubnet());
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiConectando = true;
    ultimoIntentoWiFi = millis();
    LOG_INFO("WiFi conectando...");
}

static void manejarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (wifiConectando) {
            wifiConectando = false;
            LOG_INFO("WiFi OK: %s", WiFi.localIP().toString().c_str());
        }
        return;
    }
    unsigned long ahora = millis();
    if (!wifiConectando) {
        iniciarWiFi();
    } else if (ahora - ultimoIntentoWiFi > 5000) {
        LOG_WARN("WiFi reintentando...");
        iniciarWiFi();
    }
}

// --- Callback para paquetes recibidos (comandos de la central) ---
static void sendStateReport(IPAddress destIP, uint16_t destPort) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::STATE_REPORT;
    pkt.src = MY_DEVICE_ID;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.bootId = node.getBootId();
    pkt.seq = node.getNextSeq();
    pkt.flags = 0;  // Fire-and-forget (la central lo pidió, no necesita ACK)
    pkt.clearPayload();

    // Estado actual de los sensores
    pkt.addTLV_uint8(TlvTag::STATE_MOTION, pirAnterior ? 1 : 0);
    pkt.addTLV_uint8(TlvTag::STATE_BUTTON, timbreAnterior == LOW ? 1 : 0);

    // Telemetría
    pkt.addTLV_uint32(TlvTag::UPTIME_SEC, millis() / 1000);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());
    pkt.addTLV_uint32(TlvTag::FREE_HEAP, ESP.getFreeHeap());

    // Stats resumidas
    const IoTStats& stats = node.getStats();
    pkt.addTLV_uint32(TlvTag::TX_COUNT, stats.txPackets);
    pkt.addTLV_uint32(TlvTag::ACK_TIMEOUTS, stats.ackTimeouts);

    node.sendDirect(pkt, destIP, destPort);
    LOG_INFO("STATE_REPORT enviado");
}

static void onPacketReceived(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    switch (pkt.type) {
        case MsgType::HELLO_ACK:
            LOG_INFO("Central respondio HELLO_ACK — registrado");
            break;

        case MsgType::STATE_REQUEST:
            LOG_INFO("STATE_REQUEST recibido — respondiendo");
            sendStateReport(remoteIP, remotePort);
            break;

        case MsgType::COMMAND: {
            uint8_t state = 0;
            if (pkt.getTLV_uint8(TlvTag::CMD_STATE, state)) {
                LOG_INFO("Comando recibido: state=%d", state);
            }
            break;
        }

        default:
            LOG_DEBUG("Paquete tipo 0x%02X ignorado", static_cast<uint8_t>(pkt.type));
            break;
    }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Emisor IoT V4.2 [%s] ID=0x%02X =====", MY_DEVICE_NAME, MY_DEVICE_ID);

    ESP.wdtEnable(8000);
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_TIMBRE, INPUT_PULLUP);

    iniciarWiFi();

    // Esperar WiFi (máximo 10s)
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
        ESP.wdtFeed();
        delay(100);
    }

    // Inicializar IoTNode
    node.begin();
    node.onPacketReceived(onPacketReceived);
    node.enableHeartbeat(central_IP, UDP_PORT, HEARTBEAT_INTERVAL_MS);

    LOG_INFO("IoTNode iniciado (bootId=0x%04X, puerto=%d)", node.getBootId(), UDP_PORT);
    LOG_INFO("Central: %s:%d", central_IP.toString().c_str(), UDP_PORT);

    // HELLO (discovery) — se encola, se envía cuando haya WiFi
    node.sendHello(central_IP, UDP_PORT, MY_DEVICE_TYPE, MY_DEVICE_NAME);

    LOG_INFO("Setup completo — monitoreando sensores...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    ESP.wdtFeed();
    manejarWiFi();

    // IoTNode: procesa cola, reliable, ACKs, heartbeat
    node.loop();

    // --- PIR: flanco de subida ---
    bool pirActual = digitalRead(PIN_PIR) == HIGH;
    if (pirActual && !pirAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > ANTIREBOTE_PIR_MS) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado");
            node.sendEvent(EventCode::MOTION, central_IP, UDP_PORT);
            LOG_INFO("MOTION encolado (queue=%d, reliable=%s)",
                     node.queuedCount(),
                     node.isReliableInFlight() ? "si" : "no");
        }
    }
    pirAnterior = pirActual;

    // --- Timbre: flanco de bajada (pull-up, activo LOW) ---
    bool timbreActual = digitalRead(PIN_TIMBRE) == LOW;
    if (timbreActual && !timbreAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionTimbre > ANTIREBOTE_TIMBRE_MS) {
            ultimaDeteccionTimbre = ahora;
            LOG_INFO("Timbre presionado");
            node.sendEvent(EventCode::TIMBRE, central_IP, UDP_PORT);
            LOG_INFO("TIMBRE encolado (queue=%d)", node.queuedCount());
        }
    }
    timbreAnterior = timbreActual;
}
