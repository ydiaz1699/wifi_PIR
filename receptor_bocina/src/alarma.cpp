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

struct UltimoEvento {
    IPAddress origen;
    uint32_t eventId;
    bool valido;
};
static UltimoEvento ultimos[4];
static const int MAX_EMISORES = 4;

static bool esEventoNuevo(IPAddress origen, uint32_t eventId) {
    for (int i = 0; i < MAX_EMISORES; i++) {
        if (ultimos[i].valido && ultimos[i].origen == origen) {
            if (ultimos[i].eventId == eventId) return false;
            ultimos[i].eventId = eventId;
            return true;
        }
    }
    for (int i = 0; i < MAX_EMISORES; i++) {
        if (!ultimos[i].valido) {
            ultimos[i] = { origen, eventId, true };
            return true;
        }
    }
    ultimos[0] = { origen, eventId, true };
    LOG_WARN("Tabla de emisores llena, se reciclo el slot 0");
    return true;
}

static void enviarACK(IPAddress destino, unsigned int puerto, uint32_t eventId) {
    char msg[16];
    snprintf(msg, sizeof(msg), "OK|%lu", (unsigned long)eventId);
    udp.beginPacket(destino, puerto);
    udp.write(msg);
    udp.endPacket();
}

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
    buzzer.timedOn(DURACION_TIMBRE_MS);
}

void inicializarAlarma() {
    udp.begin(puertoUDP);
    for (int i = 0; i < MAX_EMISORES; i++) ultimos[i].valido = false;
}

void manejarAlarma() {
    int packetSize = udp.parsePacket();
    if (!packetSize) return;

    IPAddress remoteIP = udp.remoteIP();
    unsigned int remotePort = udp.remotePort();

    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = 0;

    char* deviceId = strtok(buffer, "|");
    char* idStr = strtok(nullptr, "|");
    char* tipo = strtok(nullptr, "|");

    if (!deviceId || !idStr || !tipo) {
        LOG_WARN("Paquete UDP malformado, descartado");
        return;
    }

    uint32_t eventId = strtoul(idStr, nullptr, 10);
    enviarACK(remoteIP, remotePort, eventId);

    if (!esEventoNuevo(remoteIP, eventId)) {
        LOG_DEBUG("Evento %lu de %s duplicado, ACK reenviado, no se re-dispara",
                   (unsigned long)eventId, remoteIP.toString().c_str());
        return;
    }

    LOG_INFO("Evento nuevo: %s #%lu tipo=%s (%s)", deviceId, (unsigned long)eventId, tipo, remoteIP.toString().c_str());

    if (strcmp(tipo, "MOTION") == 0) {
        transitionTo(SystemState::ALARM_TRIGGERED);
        activarAlarmaMotion();
    } else if (strcmp(tipo, "TIMBRE") == 0) {
        activarTimbre();
    } else {
        LOG_WARN("Tipo de evento desconocido: %s", tipo);
    }
}
