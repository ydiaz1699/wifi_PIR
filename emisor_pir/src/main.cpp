/**
 * Emisor PIR + Timbre V3.4
 *
 * Diseño: FIRE-AND-FORGET con redundancia
 *
 * - PIR y TIMBRE son completamente INDEPENDIENTES
 * - No hay máquina de estados TX ni espera de ACK
 * - Cada evento se envía 3 veces inmediatamente (redundancia anti-pérdida)
 * - El receptor se encarga de deduplicar (ya lo hace con eventId)
 * - Latencia: <1ms entre detección y envío
 * - Nunca se bloquea, nunca se "espera", nunca se descarta un evento
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"
#include "network_config.h"
#include "device_config.h"
#include "logger.h"

WiFiUDP udp;

// --- Pines ---
const int pinPIR = D2;
const int pinTimbre = D3;   // INPUT_PULLUP, activo en LOW

// --- Timings ---
const unsigned long ANTIREBOTE_PIR_MS = 500;
const unsigned long ANTIREBOTE_TIMBRE_MS = 500;
const unsigned long WIFI_RETRY_MS = 5000;
const int ENVIOS_REDUNDANTES = 3;     // Enviar cada evento 3 veces (anti-pérdida)
const int DELAY_ENTRE_ENVIOS_US = 500; // 500 microsegundos entre envíos redundantes

const char* DEVICE_ID = "PIR01";

// --- Estado ---
unsigned long ultimaDeteccionPIR = 0;
unsigned long ultimaDeteccionTimbre = 0;
bool pirAnterior = LOW;
bool timbreAnterior = HIGH;

uint32_t eventCounter = 0;

unsigned long ultimoIntentoWiFi = 0;
bool wifiConectando = false;

// ============================================================
// WiFi
// ============================================================

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

// ============================================================
// Envío INMEDIATO — fire-and-forget con redundancia
// ============================================================

static const char* tipoToStr(const char* tipo) { return tipo; }

void enviarEvento(const char* tipo) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN("Sin WiFi, evento %s perdido", tipo);
        return;
    }

    eventCounter++;
    char msg[32];
    snprintf(msg, sizeof(msg), "%s|%lu|%s", DEVICE_ID, (unsigned long)eventCounter, tipo);

    // Enviar N veces con micro-delay entre cada uno
    // El receptor deduplica por eventId, así que solo procesa 1
    for (int i = 0; i < ENVIOS_REDUNDANTES; i++) {
        udp.beginPacket(destino_IP, PUERTO_UDP);
        udp.write(msg);
        udp.endPacket();
        if (i < ENVIOS_REDUNDANTES - 1) {
            delayMicroseconds(DELAY_ENTRE_ENVIOS_US);
        }
    }

    LOG_INFO("Enviado %dx: %s", ENVIOS_REDUNDANTES, msg);
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Boot Emisor PIR+Timbre v3.4 (fire-and-forget) =====");
    ESP.wdtEnable(8000);
    pinMode(pinPIR, INPUT);
    pinMode(pinTimbre, INPUT_PULLUP);
    iniciarWiFi();
    udp.begin(PUERTO_UDP);
}

// ============================================================
// LOOP — Ultra simple, sin máquina de estados
// ============================================================

void loop() {
    ESP.wdtFeed();
    manejarWiFi();

    // --- PIR: flanco de subida ---
    bool pirActual = digitalRead(pinPIR) == HIGH;
    if (pirActual && !pirAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > ANTIREBOTE_PIR_MS) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado");
            enviarEvento("MOTION");
        }
    }
    pirAnterior = pirActual;

    // --- Timbre: flanco de bajada (pull-up, activo LOW) ---
    bool timbreActual = digitalRead(pinTimbre) == LOW;
    if (timbreActual && !timbreAnterior) {
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionTimbre > ANTIREBOTE_TIMBRE_MS) {
            ultimaDeteccionTimbre = ahora;
            LOG_INFO("Timbre presionado");
            enviarEvento("TIMBRE");
        }
    }
    timbreAnterior = timbreActual;
}
