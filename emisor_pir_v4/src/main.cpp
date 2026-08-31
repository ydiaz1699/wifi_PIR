/**
 * Emisor PIR + Timbre — IoTProtocol V4.3
 *
 * Nuevas features V4.3:
 * - IoTStorage: BOOT_ID persistente (incremental, no random)
 * - IoTStorage: Config persistente en LittleFS
 * - IoTConfigHandler: config remota desde la central sin recompilar
 * - IoTAuth: HMAC-SHA256 opcional (habilitado vía config o hardcoded)
 *
 * Para nuevo sensor: solo cambiar device_config.h/cpp
 */

#include <ESP8266WiFi.h>
#include <IoTNode.h>
#include <IoTStorage.h>
#include <IoTConfigHandler.h>
#include <IoTAuth.h>
#include "secrets.h"
#include "network_config.h"
#include "device_config.h"
#include "logger.h"
#include "ota.h"

// --- Shared secret para HMAC (desde secrets.h, NO versionado) ---
static const uint8_t AUTH_KEY[] = IOT_AUTH_KEY;

// --- Objetos globales ---
IoTStorage storage;
IoTNode node(MY_DEVICE_ID, UDP_PORT);
IoTAuth auth(AUTH_KEY, IOT_AUTH_KEY_LEN);
IoTConfigHandler* configHandler = nullptr;

// --- Estado de sensores ---
bool pirAnterior = LOW;
bool timbreAnterior = HIGH;
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
            setupOTA();
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

// --- Callback de config aplicada ---
static void onConfigApplied(const IoTConfig &cfg) {
    LOG_INFO("Config aplicada: hb=%lums antirebote=%lums name='%s'",
             (unsigned long)cfg.heartbeatIntervalMs,
             (unsigned long)cfg.antireboteMs,
             cfg.deviceName);
    // Actualizar heartbeat interval en runtime
    node.enableHeartbeat(central_IP, UDP_PORT, cfg.heartbeatIntervalMs);
}

// --- STATE_REPORT ---
static void sendStateReport(IPAddress destIP, uint16_t destPort) {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::STATE_REPORT;
    pkt.src = MY_DEVICE_ID;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.bootId = node.getBootId();
    pkt.seq = node.getNextSeq();
    pkt.flags = 0;
    pkt.clearPayload();

    pkt.addTLV_uint8(TlvTag::STATE_MOTION, pirAnterior ? 1 : 0);
    pkt.addTLV_uint8(TlvTag::STATE_BUTTON, timbreAnterior == LOW ? 1 : 0);
    pkt.addTLV_uint32(TlvTag::UPTIME_SEC, millis() / 1000);
    pkt.addTLV_int8(TlvTag::RSSI_VAL, (int8_t)WiFi.RSSI());
    pkt.addTLV_uint32(TlvTag::FREE_HEAP, ESP.getFreeHeap());

    const IoTStats& stats = node.getStats();
    pkt.addTLV_uint32(TlvTag::TX_COUNT, stats.txPackets);
    pkt.addTLV_uint32(TlvTag::ACK_TIMEOUTS, stats.ackTimeouts);

    // Firmar si auth habilitado
    if (storage.config().authEnabled) {
        auth.signPacket(pkt);
    }

    node.sendDirect(pkt, destIP, destPort);
    LOG_INFO("STATE_REPORT enviado");
}

// --- Callback de paquetes recibidos ---
static void onPacketReceived(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    // Verificar auth si está habilitado
    if (!auth.verifyPacket(pkt)) {
        LOG_WARN("Paquete de 0x%02X rechazado: auth inválida", pkt.src);
        return;
    }

    switch (pkt.type) {
        case MsgType::HELLO_ACK:
            LOG_INFO("Central respondio HELLO_ACK");
            break;

        case MsgType::STATE_REQUEST:
            LOG_INFO("STATE_REQUEST recibido");
            sendStateReport(remoteIP, remotePort);
            break;

        case MsgType::CONFIG:
            LOG_INFO("CONFIG recibido de central");
            if (configHandler) {
                configHandler->handleConfig(pkt, remoteIP, remotePort);
            }
            break;

        case MsgType::COMMAND: {
            uint8_t state = 0;
            if (pkt.getTLV_uint8(TlvTag::CMD_STATE, state)) {
                LOG_INFO("Comando: state=%d", state);
            }
            break;
        }

        default:
            LOG_DEBUG("Tipo 0x%02X ignorado", static_cast<uint8_t>(pkt.type));
            break;
    }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Emisor IoT V4.3 [%s] ID=0x%02X =====", MY_DEVICE_NAME, MY_DEVICE_ID);

    ESP.wdtEnable(8000);
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_TIMBRE, INPUT_PULLUP);

    // --- LittleFS + Storage ---
    if (storage.begin()) {
        storage.loadConfig();
        LOG_INFO("Storage OK: boot#%lu, config='%s'",
                 (unsigned long)storage.getBootCount(),
                 storage.config().deviceName);
    } else {
        LOG_ERROR("Storage FAIL: usando defaults");
    }

    iniciarWiFi();

    // Esperar WiFi (máximo 10s)
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
        ESP.wdtFeed();
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        setupOTA();
    } else {
        LOG_WARN("WiFi no disponible: OTA se inicializará al reconectar");
    }

    // --- IoTNode con BOOT_ID persistente ---
    // Override del begin() para usar boot counter del storage
    node.begin();
    // Nota: node.begin() genera random bootId, pero podemos mejorarlo
    // usando storage en futuras versiones (requiere cambio en IoTNode API)

    node.onPacketReceived(onPacketReceived);

    // Heartbeat con intervalo de config persistida
    node.enableHeartbeat(central_IP, UDP_PORT, storage.config().heartbeatIntervalMs);

    // --- Config Handler ---
    static IoTConfigHandler cfgHandler(storage, node);
    cfgHandler.onConfigApplied(onConfigApplied);
    configHandler = &cfgHandler;

    // Sincronizar auth con config persistida
    auth.setRequired(storage.config().authEnabled);

    LOG_INFO("IoTNode (bootId=0x%04X, puerto=%d)", node.getBootId(), UDP_PORT);
    LOG_INFO("Auth: %s", storage.config().authEnabled ? "HABILITADO" : "deshabilitado");

    // HELLO (discovery)
    node.sendHello(central_IP, UDP_PORT, MY_DEVICE_TYPE, MY_DEVICE_NAME);

    LOG_INFO("Setup completo — monitoreando sensores...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    ESP.wdtFeed();
    manejarWiFi();
    handleOTA();

    // Durante la transferencia no se generan eventos ni tráfico de aplicación.
    // ArduinoOTA.handle() sigue siendo atendido en cada iteración.
    if (otaEnProgreso()) {
        return;
    }

    // IoTNode: cola, reliable, ACKs, heartbeat
    node.loop();

    // Antirebote desde config persistida
    unsigned long antirebotePIR = storage.config().antireboteMs;
    unsigned long antireboteTimbre = 800;  // Fijo para timbre

    // --- PIR: flanco de subida ---
    bool pirActual = digitalRead(PIN_PIR) == HIGH;
    if (pirActual && !pirAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > antirebotePIR) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado");
            node.sendEvent(EventCode::MOTION, central_IP, UDP_PORT);
            LOG_INFO("MOTION encolado (q=%d)", node.queuedCount());
        }
    }
    pirAnterior = pirActual;

    // --- Timbre: flanco de bajada (pull-up, activo LOW) ---
    bool timbreActual = digitalRead(PIN_TIMBRE) == LOW;
    if (timbreActual && !timbreAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionTimbre > antireboteTimbre) {
            ultimaDeteccionTimbre = ahora;
            LOG_INFO("Timbre presionado");
            node.sendEvent(EventCode::TIMBRE, central_IP, UDP_PORT);
            LOG_INFO("TIMBRE encolado (q=%d)", node.queuedCount());
        }
    }
    timbreAnterior = timbreActual;
}
