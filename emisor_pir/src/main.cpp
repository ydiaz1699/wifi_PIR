#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include "network_config.h"
#include "device_config.h"
#include "logger.h"

WiFiUDP udp;
char bufferRX[32];

// --- Pines ---
const int pinPIR = D2;
const int pinTimbre = D3;   // botón de timbre, INPUT_PULLUP, activo en LOW

// --- Timings ---
const unsigned long ANTIREBOTE_PIR_MS = 500;       // V3.3 fix: era 2000 → 500ms para detectar pulsos rápidos
const unsigned long ANTIREBOTE_TIMBRE_MS = 800;
const unsigned long TIMEOUT_ACK_MS = 500;
const int MAX_REINTENTOS = 5;
const unsigned long WIFI_RETRY_MS = 5000;
const char* DEVICE_ID = "PIR01";

unsigned long ultimaDeteccionPIR = 0;
unsigned long ultimaDeteccionTimbre = 0;
bool pirAnterior = LOW;
bool timbreAnterior = HIGH; // pull-up: reposo en HIGH, presionado en LOW

uint32_t eventCounter = 0;

// --- Buffer de eventos pendientes (V3.3 fix) ---
// Si txState no está IDLE cuando se detecta un evento,
// se guarda aquí y se envía en el siguiente loop libre.
enum class EventoPendiente { NINGUNO, MOTION, TIMBRE };
EventoPendiente pendiente = EventoPendiente::NINGUNO;

enum class TipoEvento { MOTION, TIMBRE };

enum class TxState { IDLE, SENDING, WAIT_ACK, DONE, FAILED };
TxState txState = TxState::IDLE;
unsigned long txTimer = 0;
int txIntento = 0;
uint32_t txEventId = 0;
TipoEvento txTipo = TipoEvento::MOTION;

unsigned long ultimoIntentoWiFi = 0;
bool wifiConectando = false;

void iniciarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.config(dispositivo_IP, redGateway(), redSubnet());
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiConectando = true;
    ultimoIntentoWiFi = millis();
    LOG_INFO("WiFi conectando...");
}

void manejarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (wifiConectando) {
            wifiConectando = false;
            LOG_INFO("WiFi OK, IP: %s", WiFi.localIP().toString().c_str());
        }
        return;
    }
    unsigned long ahora = millis();
    if (!wifiConectando) {
        iniciarWiFi();
    } else if (ahora - ultimoIntentoWiFi > WIFI_RETRY_MS) {
        LOG_WARN("WiFi caido, reintentando...");
        iniciarWiFi();
    }
}

static const char* tipoToStr(TipoEvento t) {
    return (t == TipoEvento::MOTION) ? "MOTION" : "TIMBRE";
}

void iniciarEnvio(TipoEvento tipo) {
    eventCounter++;
    txEventId = eventCounter;
    txTipo = tipo;
    txState = TxState::SENDING;
    txIntento = 1;
}

static void enviarPaquete() {
    char msg[32];
    snprintf(msg, sizeof(msg), "%s|%lu|%s", DEVICE_ID, (unsigned long)txEventId, tipoToStr(txTipo));
    udp.beginPacket(destino_IP, PUERTO_UDP);
    udp.write(msg);
    udp.endPacket();
    LOG_INFO("Enviado: %s (intento %d/%d)", msg, txIntento, MAX_REINTENTOS);
}

void manejarEnvio() {
    switch (txState) {
        case TxState::IDLE:
            break;

        case TxState::SENDING:
            if (WiFi.status() != WL_CONNECTED) {
                txState = TxState::FAILED;
                LOG_ERROR("Sin WiFi, evento %lu no enviado", (unsigned long)txEventId);
                break;
            }
            enviarPaquete();
            txTimer = millis();
            txState = TxState::WAIT_ACK;
            break;

        case TxState::WAIT_ACK: {
            int packetSize = udp.parsePacket();
            if (packetSize) {
                int len = udp.read(bufferRX, sizeof(bufferRX) - 1);
                bufferRX[len] = 0;

                char* sep = strchr(bufferRX, '|');
                if (sep && strncmp(bufferRX, "OK", 2) == 0) {
                    uint32_t ackId = strtoul(sep + 1, nullptr, 10);
                    if (ackId == txEventId) {
                        LOG_INFO("ACK confirmado, evento %lu", (unsigned long)txEventId);
                        txState = TxState::DONE;
                        break;
                    }
                }
            }
            if (millis() - txTimer > TIMEOUT_ACK_MS) {
                if (txIntento < MAX_REINTENTOS) {
                    txIntento++;
                    txState = TxState::SENDING;
                } else {
                    LOG_ERROR("Fallo: sin ACK tras %d intentos (evento %lu)", MAX_REINTENTOS, (unsigned long)txEventId);
                    txState = TxState::FAILED;
                }
            }
            break;
        }

        case TxState::DONE:
        case TxState::FAILED:
            txState = TxState::IDLE;
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Boot Emisor PIR+Timbre v3.3.1 =====");
    ESP.wdtEnable(8000);
    pinMode(pinPIR, INPUT);
    pinMode(pinTimbre, INPUT_PULLUP);
    iniciarWiFi();
    udp.begin(PUERTO_UDP);
}

void loop() {
    ESP.wdtFeed();
    manejarWiFi();
    manejarEnvio();

    // --- Enviar evento pendiente si TX está libre ---
    if (pendiente != EventoPendiente::NINGUNO && txState == TxState::IDLE) {
        if (pendiente == EventoPendiente::MOTION) {
            iniciarEnvio(TipoEvento::MOTION);
            LOG_INFO("Evento MOTION pendiente enviado");
        } else if (pendiente == EventoPendiente::TIMBRE) {
            iniciarEnvio(TipoEvento::TIMBRE);
            LOG_INFO("Evento TIMBRE pendiente enviado");
        }
        pendiente = EventoPendiente::NINGUNO;
    }

    // --- PIR: flanco de subida ---
    bool pirActual = digitalRead(pinPIR) == HIGH;
    if (pirActual && !pirAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > ANTIREBOTE_PIR_MS) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado (flanco subida)");
            if (txState == TxState::IDLE) {
                iniciarEnvio(TipoEvento::MOTION);
            } else {
                pendiente = EventoPendiente::MOTION;
                LOG_INFO("MOTION encolado (TX ocupado)");
            }
        }
    }
    pirAnterior = pirActual;

    // --- Timbre: flanco de bajada (pull-up, activo en LOW) ---
    bool timbreActual = digitalRead(pinTimbre) == LOW;
    if (timbreActual && !timbreAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionTimbre > ANTIREBOTE_TIMBRE_MS) {
            ultimaDeteccionTimbre = ahora;
            LOG_INFO("Timbre presionado");
            if (txState == TxState::IDLE) {
                iniciarEnvio(TipoEvento::TIMBRE);
            } else {
                pendiente = EventoPendiente::TIMBRE;
                LOG_INFO("TIMBRE encolado (TX ocupado)");
            }
        }
    }
    timbreAnterior = timbreActual;
}
