/**
 * Alarma V3.5 — Recepción multi-paquete + ACK inmediato
 *
 * Actualizado para V3.5 del emisor (async ACK, envíos simultáneos):
 * - Procesa TODOS los paquetes disponibles en cada llamada (drain loop)
 * - ACK enviado inmediatamente para cada paquete
 * - Deduplicación por eventId (ventana de últimos 8 por emisor)
 * - PIR y TIMBRE activan acciones independientes
 * - Nunca se pierde un paquete por "solo proceso 1 por loop"
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "alarma.h"
#include "hal.h"
#include "mqtt_cliente.h"
#include "config.h"
#include "logger.h"
#include "state_machine.h"

extern Buzzer buzzer;

static WiFiUDP udp;
static char buffer[32];

String modoActual = "armado";

// ============================================================
// Deduplicación con ventana (misma idea que V4.1.1)
// ============================================================

#define DEDUP_WINDOW 8
#define MAX_EMISORES 4

struct EmisorInfo {
    IPAddress ip;
    uint32_t seqWindow[DEDUP_WINDOW];
    uint8_t windowCount;
    uint8_t windowHead;
    bool activo;
};

static EmisorInfo emisores[MAX_EMISORES];

static bool esEventoNuevo(IPAddress origen, uint32_t eventId) {
    // Buscar emisor existente
    for (int i = 0; i < MAX_EMISORES; i++) {
        if (emisores[i].activo && emisores[i].ip == origen) {
            // Buscar en ventana
            for (uint8_t j = 0; j < emisores[i].windowCount; j++) {
                if (emisores[i].seqWindow[j] == eventId) return false;  // DUPLICADO
            }
            // Nuevo: insertar en ventana circular
            emisores[i].seqWindow[emisores[i].windowHead] = eventId;
            emisores[i].windowHead = (emisores[i].windowHead + 1) % DEDUP_WINDOW;
            if (emisores[i].windowCount < DEDUP_WINDOW) emisores[i].windowCount++;
            return true;
        }
    }
    // Emisor nuevo: buscar slot libre
    for (int i = 0; i < MAX_EMISORES; i++) {
        if (!emisores[i].activo) {
            emisores[i].ip = origen;
            emisores[i].seqWindow[0] = eventId;
            emisores[i].windowCount = 1;
            emisores[i].windowHead = 1;
            emisores[i].activo = true;
            return true;
        }
    }
    // Tabla llena: reciclar slot 0
    emisores[0].ip = origen;
    emisores[0].seqWindow[0] = eventId;
    emisores[0].windowCount = 1;
    emisores[0].windowHead = 1;
    LOG_WARN("Tabla de emisores llena, slot 0 reciclado");
    return true;
}

// ============================================================
// ACK
// ============================================================

static void enviarACK(IPAddress destino, unsigned int puerto, uint32_t eventId) {
    char msg[16];
    snprintf(msg, sizeof(msg), "OK|%lu", (unsigned long)eventId);
    udp.beginPacket(destino, puerto);
    udp.write(msg);
    udp.endPacket();
}

// ============================================================
// Acciones por tipo de evento (independientes)
// ============================================================

static void activarAlarmaMotion() {
    LOG_INFO("MOTION: activando alarma");
    if (haDisponible) {
        mqtt.publish(TOPIC_EVENTO, "detectado");
    }
    if (modoActual == "armado") {
        buzzer.timedOn(DURACION_BOCINA_MS);
    }
}

static void activarTimbre() {
    LOG_INFO("TIMBRE: sonido corto de aviso");
    if (haDisponible) {
        mqtt.publish(TOPIC_TIMBRE, "presionado");
    }
    // Timbre SIEMPRE suena (independiente del modo armado/desarmado)
    buzzer.timedOn(DURACION_TIMBRE_MS);
}

// ============================================================
// Procesar UN paquete recibido
// ============================================================

static void procesarPaquete() {
    IPAddress remoteIP = udp.remoteIP();
    unsigned int remotePort = udp.remotePort();

    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len <= 0) return;
    buffer[len] = 0;

    // Parsear: "DEVICE|eventId|TIPO"
    char* deviceId = strtok(buffer, "|");
    char* idStr = strtok(nullptr, "|");
    char* tipo = strtok(nullptr, "|");

    if (!deviceId || !idStr || !tipo) {
        LOG_WARN("Paquete UDP malformado");
        return;
    }

    uint32_t eventId = strtoul(idStr, nullptr, 10);

    // ACK SIEMPRE (incluso para duplicados — el emisor necesita confirmación)
    enviarACK(remoteIP, remotePort, eventId);

    // Deduplicación con ventana
    if (!esEventoNuevo(remoteIP, eventId)) {
        LOG_DEBUG("Duplicado #%lu de %s (ACK reenviado)", 
                  (unsigned long)eventId, remoteIP.toString().c_str());
        return;
    }

    // Evento nuevo → procesar
    LOG_INFO("Evento nuevo: %s #%lu tipo=%s (%s)", 
             deviceId, (unsigned long)eventId, tipo, remoteIP.toString().c_str());

    if (strcmp(tipo, "MOTION") == 0) {
        transitionTo(SystemState::ALARM_TRIGGERED);
        activarAlarmaMotion();
    } else if (strcmp(tipo, "TIMBRE") == 0) {
        activarTimbre();
    } else {
        LOG_WARN("Tipo desconocido: %s", tipo);
    }
}

// ============================================================
// Inicialización
// ============================================================

void inicializarAlarma() {
    udp.begin(puertoUDP);
    for (int i = 0; i < MAX_EMISORES; i++) {
        emisores[i].activo = false;
        emisores[i].windowCount = 0;
        emisores[i].windowHead = 0;
    }
}

// ============================================================
// manejarAlarma — DRAIN LOOP: procesa TODOS los paquetes disponibles
// ============================================================

void manejarAlarma() {
    // Procesar todos los paquetes en el buffer UDP (no solo 1)
    // Esto es crítico cuando el emisor V3.5 envía múltiples eventos
    // casi simultáneamente (PIR + TIMBRE en el mismo milisegundo)
    int procesados = 0;
    const int MAX_POR_CICLO = 8;  // Límite para no monopolizar el loop

    while (procesados < MAX_POR_CICLO) {
        int packetSize = udp.parsePacket();
        if (!packetSize) break;  // No hay más paquetes

        procesarPaquete();
        procesados++;
    }
}
