#include <ESP8266WiFi.h>
#include "mqtt_cliente.h"
#include "mqtt_discovery.h"
#include "hal.h"
#include "config.h"
#include "logger.h"
#include "state_machine.h"
#include "red_wifi.h"

extern Buzzer buzzer;
extern String modoActual;

WiFiClient espClient;
PubSubClient mqtt(espClient);
bool haDisponible = false;

static unsigned long ultimoIntentoMQTT = 0;
static unsigned long ultimoUptime = 0;

// --- Guardia anti-bloqueo ---
// Controla cuánto tiempo puede pasar intentando conectar TCP al broker.
// Si la conexión TCP no se establece rápido, se aborta para no bloquear
// la recepción UDP (que es la función primaria del dispositivo).
static const unsigned long TCP_CONNECT_TIMEOUT_MS = 1000;

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String mensaje;
    for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];

    String t = String(topic);
    LOG_INFO("MQTT recibido [%s]: %s", topic, mensaje.c_str());

    if (t == TOPIC_COMANDO) {
        if (mensaje == "ON") {
            buzzer.timedOn(DURACION_BOCINA_MS);
        } else if (mensaje == "OFF") {
            buzzer.off();
            publicarEstadoBocina();
        }
    } else if (t == TOPIC_MODO) {
        if (mensaje == "armado" || mensaje == "desarmado") {
            modoActual = mensaje;
            mqtt.publish(TOPIC_MODO_STATE, modoActual.c_str(), true);
        }
    }
}

// --- Conexión TCP no-bloqueante antes de MQTT ---
// Primero verificamos que el broker responde TCP en <1s.
// Si no responde, salimos sin bloquear. Así el loop sigue
// procesando paquetes UDP normalmente.
static bool verificarTCPBroker() {
    if (espClient.connected()) return true;  // Ya conectado

    LOG_DEBUG("MQTT: verificando TCP hacia %s:%d...", mqtt_server, mqtt_port);

    unsigned long inicio = millis();
    int resultado = espClient.connect(mqtt_server, mqtt_port);

    unsigned long duracion = millis() - inicio;

    if (resultado) {
        LOG_DEBUG("MQTT: TCP OK en %lums", duracion);
        espClient.stop();  // Cerramos; PubSubClient abrirá su propia conexión
        return true;
    }

    LOG_WARN("MQTT: TCP fallo tras %lums, broker no disponible", duracion);
    return false;
}

static void conectarMQTT() {
    // Paso 1: Verificar que el broker responde TCP rápido
    if (!verificarTCPBroker()) {
        reportarFalloMQTT();
        return;
    }

    // Paso 2: Si TCP respondió, ahora sí intentar handshake MQTT
    LOG_INFO("Conectando MQTT...");
    bool ok;
    if (mqtt_user[0] != '\0') {
        ok = mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass, TOPIC_ESTADO, 0, true, "offline");
    } else {
        ok = mqtt.connect(mqtt_client_id, TOPIC_ESTADO, 0, true, "offline");
    }

    if (ok) {
        LOG_INFO("MQTT OK");
        haDisponible = true;
        reportarExitoConexion();
        mqtt.publish(TOPIC_ESTADO, "online", true);

        publicarDiscovery();

        mqtt.publish(TOPIC_COMANDO_STATE, buzzer.isOn() ? "ON" : "OFF", true);
        mqtt.publish(TOPIC_MODO_STATE, modoActual.c_str(), true);
        mqtt.publish(TOPIC_IP, WiFi.localIP().toString().c_str(), true);

        mqtt.subscribe(TOPIC_COMANDO);
        mqtt.subscribe(TOPIC_MODO);

        transitionTo(SystemState::READY);
    } else {
        LOG_ERROR("MQTT fallo, rc=%d", mqtt.state());
        haDisponible = false;
        reportarFalloMQTT();
    }
}

void inicializarMQTT() {
    mqtt.setSocketTimeout(1);       // Timeout TCP de 1 segundo (mínimo de PubSubClient)
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setBufferSize(768);
    mqtt.setCallback(mqttCallback);
}

void manejarMQTT() {
    if (!wifiConectado()) {
        haDisponible = false;
        return;
    }

    if (!mqtt.connected()) {
        haDisponible = false;
        unsigned long ahora = millis();
        if (ahora - ultimoIntentoMQTT > INTERVALO_RECONEXION_MQTT) {
            ultimoIntentoMQTT = ahora;
            if (!inState(SystemState::CONNECT_MQTT) && !inState(SystemState::RECOVER)) {
                transitionTo(SystemState::CONNECT_MQTT);
            }
            conectarMQTT();
        }
        return;
    }

    mqtt.loop();
    haDisponible = true;

    unsigned long ahora = millis();
    if (ahora - ultimoUptime > INTERVALO_UPTIME) {
        ultimoUptime = ahora;
        mqtt.publish(TOPIC_UPTIME, String(millis() / 1000).c_str(), true);
    }
}

void publicarEstadoBocina() {
    if (mqtt.connected()) {
        mqtt.publish(TOPIC_COMANDO_STATE, buzzer.isOn() ? "ON" : "OFF", true);
    }
}
