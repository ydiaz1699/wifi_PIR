/**
 * Emisor PIR + Timbre V4
 *
 * Usa IoTProtocol para comunicarse con la central.
 * Características:
 * - Cola de eventos (PIR + timbre no se pisan)
 * - Backoff exponencial automático (manejado por IoTNode)
 * - Heartbeat cada 60s (la central sabe que está vivo)
 * - Discovery al boot (HELLO con tipo y nombre)
 * - CRC16 en cada paquete
 * - ACK automático desde la central
 *
 * Para crear un nuevo sensor, solo cambiá device_config.h/cpp
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
bool timbreAnterior = HIGH;  // Pull-up: reposo HIGH, presionado LOW
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


// ============================================================
// Callback para paquetes recibidos (comandos de la central)
// ============================================================

static void onPacketReceived(const IoTPacket &pkt, IPAddress remoteIP, uint16_t remotePort) {
    switch (pkt.type) {
        case MsgType::HELLO_ACK:
            LOG_INFO("Central respondio HELLO_ACK — registrado OK");
            break;

        case MsgType::COMMAND: {
            uint8_t state = 0;
            if (pkt.getTLV_uint8(TlvTag::CMD_STATE, state)) {
                LOG_INFO("Comando recibido: state=%d", state);
                // Aquí se podrían procesar comandos remotos
                // (ej: LED de confirmación, cambiar sensibilidad, etc.)
            }
            break;
        }

        case MsgType::CONFIG: {
            LOG_INFO("Config recibida de central (no implementado aun)");
            break;
        }

        default:
            LOG_DEBUG("Paquete tipo 0x%02X de 0x%02X (ignorado)",
                      static_cast<uint8_t>(pkt.type), pkt.src);
            break;
    }
}

// ============================================================
// Envío de HELLO al boot (Discovery)
// ============================================================

static void enviarHello() {
    IoTPacket pkt;
    pkt.version = IOT_PROTOCOL_VER;
    pkt.type = MsgType::HELLO;
    pkt.src = MY_DEVICE_ID;
    pkt.dst = IOT_DEVICE_CENTRAL;
    pkt.seq = node.getNextSeq();
    pkt.flags = IOT_FLAG_ACK_REQUIRED;
    pkt.clearPayload();
    pkt.addTLV_uint8(TlvTag::DEVICE_TYPE, static_cast<uint8_t>(MY_DEVICE_TYPE));
    pkt.addTLV_string(TlvTag::DEVICE_NAME, MY_DEVICE_NAME);
    pkt.addTLV_string(TlvTag::FW_VERSION, "4.0");

    node.sendPacket(pkt, central_IP, UDP_PORT);
    LOG_INFO("HELLO enviado a central");
}


// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Emisor IoT V4.0 [%s] ID=0x%02X =====", MY_DEVICE_NAME, MY_DEVICE_ID);

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

    LOG_INFO("IoTNode iniciado (puerto %d)", UDP_PORT);
    LOG_INFO("Central: %s:%d", central_IP.toString().c_str(), UDP_PORT);

    // Enviar HELLO (discovery)
    if (WiFi.status() == WL_CONNECTED) {
        enviarHello();
    }

    LOG_INFO("Setup completo — monitoreando sensores...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    ESP.wdtFeed();
    manejarWiFi();

    // IoTNode: procesa cola de envío + recibe ACKs + heartbeat
    node.loop();

    // --- PIR: flanco de subida ---
    bool pirActual = digitalRead(PIN_PIR) == HIGH;
    if (pirActual && !pirAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > ANTIREBOTE_PIR_MS) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado");

            if (WiFi.status() == WL_CONNECTED) {
                node.sendEvent(EventCode::MOTION, central_IP, UDP_PORT);
                LOG_INFO("Evento MOTION encolado (queue: %d)", node.queuedCount());
            } else {
                LOG_WARN("Sin WiFi, evento MOTION perdido");
            }
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

            if (WiFi.status() == WL_CONNECTED) {
                node.sendEvent(EventCode::TIMBRE, central_IP, UDP_PORT);
                LOG_INFO("Evento TIMBRE encolado (queue: %d)", node.queuedCount());
            } else {
                LOG_WARN("Sin WiFi, evento TIMBRE perdido");
            }
        }
    }
    timbreAnterior = timbreActual;
}
