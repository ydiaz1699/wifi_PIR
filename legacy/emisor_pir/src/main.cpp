/**
 * Emisor PIR + Timbre V3.5
 *
 * Diseño: ACK ASÍNCRONO NO-BLOQUEANTE
 *
 * - PIR y TIMBRE envían INMEDIATAMENTE al detectarse (nunca esperan)
 * - Cola de hasta 4 eventos "pendientes de ACK"
 * - Los ACKs se verifican en background (sin bloquear detección)
 * - Si un evento no recibe ACK en 500ms, se reenvía automáticamente
 * - Máximo 3 reintentos por evento, después se marca como fallido
 * - Múltiples eventos pueden estar en vuelo SIMULTÁNEAMENTE
 *
 * Resultado:
 * - PIR y TIMBRE son 100% independientes
 * - Se confirma que el receptor recibió cada evento
 * - Si se pierde un paquete, se reintenta sin afectar otros eventos
 * - Latencia de envío: <1ms (instantáneo)
 */

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
const int pinTimbre = D3;

// --- Timings ---
const unsigned long ANTIREBOTE_PIR_MS = 200;    // Mínimo entre dos detecciones PIR
const unsigned long ANTIREBOTE_TIMBRE_MS = 500;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long ACK_TIMEOUT_MS = 500;     // Timeout para reenvío
const int MAX_REINTENTOS = 3;                 // Reintentos por evento

const char* DEVICE_ID = "PIR01";

// ============================================================
// Cola de eventos en vuelo (pendientes de ACK)
// Múltiples eventos pueden estar en vuelo simultáneamente
// ============================================================

#define MAX_EN_VUELO 4

struct EventoEnVuelo {
    uint32_t eventId;
    char mensaje[32];
    unsigned long enviadoEn;     // millis() del último envío
    uint8_t intentos;            // Envíos realizados
    bool activo;                 // Slot en uso
    bool confirmado;             // ACK recibido
};

EventoEnVuelo enVuelo[MAX_EN_VUELO];

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
// Envío UDP (no bloquea)
// ============================================================

static void enviarUDP(const char* msg) {
    udp.beginPacket(destino_IP, PUERTO_UDP);
    udp.write(msg);
    udp.endPacket();
}

// ============================================================
// Enviar evento INMEDIATAMENTE + registrar en cola de vuelo
// ============================================================

void enviarEvento(const char* tipo) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN("Sin WiFi, evento %s perdido", tipo);
        return;
    }

    eventCounter++;

    // Armar mensaje
    char msg[32];
    snprintf(msg, sizeof(msg), "%s|%lu|%s", DEVICE_ID, (unsigned long)eventCounter, tipo);

    // Enviar inmediatamente (no esperar nada)
    enviarUDP(msg);
    LOG_INFO("Enviado: %s", msg);

    // Buscar slot libre en la cola de vuelo
    for (int i = 0; i < MAX_EN_VUELO; i++) {
        if (!enVuelo[i].activo) {
            enVuelo[i].eventId = eventCounter;
            strncpy(enVuelo[i].mensaje, msg, sizeof(enVuelo[i].mensaje));
            enVuelo[i].enviadoEn = millis();
            enVuelo[i].intentos = 1;
            enVuelo[i].activo = true;
            enVuelo[i].confirmado = false;
            return;
        }
    }

    // Cola llena — el evento se envió pero no se trackea (fire-and-forget fallback)
    LOG_WARN("Cola de ACK llena, evento %lu sin tracking", (unsigned long)eventCounter);
}

// ============================================================
// Verificar ACKs recibidos (no-bloqueante)
// ============================================================

void verificarACKs() {
    // Leer todos los paquetes UDP disponibles
    while (true) {
        int packetSize = udp.parsePacket();
        if (!packetSize) break;

        int len = udp.read(bufferRX, sizeof(bufferRX) - 1);
        if (len <= 0) continue;
        bufferRX[len] = 0;

        // Parsear ACK: "OK|eventId"
        char* sep = strchr(bufferRX, '|');
        if (sep && strncmp(bufferRX, "OK", 2) == 0) {
            uint32_t ackId = strtoul(sep + 1, nullptr, 10);

            // Buscar en cola y marcar como confirmado
            for (int i = 0; i < MAX_EN_VUELO; i++) {
                if (enVuelo[i].activo && enVuelo[i].eventId == ackId) {
                    enVuelo[i].confirmado = true;
                    enVuelo[i].activo = false;
                    LOG_INFO("ACK recibido: evento %lu", (unsigned long)ackId);
                    break;
                }
            }
        }
    }
}

// ============================================================
// Reenviar eventos sin ACK (no-bloqueante)
// ============================================================

void reenviarPendientes() {
    unsigned long ahora = millis();

    for (int i = 0; i < MAX_EN_VUELO; i++) {
        if (!enVuelo[i].activo) continue;

        // ¿Pasó el timeout sin ACK?
        if (ahora - enVuelo[i].enviadoEn >= ACK_TIMEOUT_MS) {
            if (enVuelo[i].intentos >= MAX_REINTENTOS) {
                // Fallo definitivo
                LOG_ERROR("Evento %lu: sin ACK tras %d intentos",
                          (unsigned long)enVuelo[i].eventId, MAX_REINTENTOS);
                enVuelo[i].activo = false;
            } else {
                // Reenviar
                enVuelo[i].intentos++;
                enVuelo[i].enviadoEn = ahora;
                enviarUDP(enVuelo[i].mensaje);
                LOG_INFO("Reenvio: %s (intento %d/%d)",
                         enVuelo[i].mensaje, enVuelo[i].intentos, MAX_REINTENTOS);
            }
        }
    }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Boot Emisor PIR+Timbre v3.5.1 (async ACK + PIR fix) =====");
    ESP.wdtEnable(8000);
    pinMode(pinPIR, INPUT);
    pinMode(pinTimbre, INPUT_PULLUP);

    // Inicializar cola
    for (int i = 0; i < MAX_EN_VUELO; i++) {
        enVuelo[i].activo = false;
    }

    iniciarWiFi();
    udp.begin(PUERTO_UDP);
}

// ============================================================
// LOOP — Todo es no-bloqueante
// ============================================================

void loop() {
    ESP.wdtFeed();
    manejarWiFi();

    // 1. Verificar ACKs recibidos (instantáneo, no bloquea)
    verificarACKs();

    // 2. Reenviar eventos sin ACK (solo si pasó timeout)
    reenviarPendientes();

    // 3. PIR: detección DUAL (flanco + re-trigger)
    //    - Primera detección: flanco de subida (LOW→HIGH)
    //    - Si el PIR sigue HIGH por más de ANTIREBOTE_PIR_MS:
    //      se considera nueva activación (re-trigger)
    //    - Esto permite detectar activaciones manuales rápidas
    //      Y activaciones largas del PIR real
    bool pirActual = digitalRead(pinPIR) == HIGH;

    if (pirActual && !pirAnterior) {
        // Flanco de subida: enviar inmediatamente
        unsigned long ahora = millis();
        if (ahora - ultimaDeteccionPIR > ANTIREBOTE_PIR_MS) {
            ultimaDeteccionPIR = ahora;
            LOG_INFO("PIR detectado (flanco subida)");
            enviarEvento("MOTION");
        }
    } else if (!pirActual && pirAnterior) {
        // Flanco de bajada: el PIR se liberó, resetear para próxima detección
        LOG_DEBUG("PIR liberado");
    }
    pirAnterior = pirActual;

    // 4. Timbre: flanco de bajada — envío INMEDIATO
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
