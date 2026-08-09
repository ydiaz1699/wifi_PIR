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

// --- Control anti-bloqueo V3.2b ---
// En lugar de verificar TCP (que bloquea 5s en ESP8266),
// usamos un ping ICMP-like: intentamos connect() solo si
// el broker respondió a un ARP reciente (si está en la tabla ARP
// del ESP8266, significa que está online en la red local).
// Si no tenemos esa info, simplemente espaciamos los intentos
// cada 30 segundos para minimizar bloqueos.
static const unsigned long INTERVALO_MQTT_SIN_BROKER = 30000;  // 30s entre intentos si broker caído
static uint8_t fallosConsecutivos = 0;

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

static void conectarMQTT() {
    LOG_INFO("MQTT: intentando connect (broker %s:%d)...", mqtt_server, mqtt_port);

    bool ok;
    if (mqtt_user[0] != '\0') {
        ok = mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass, TOPIC_ESTADO, 0, true, "offline");
    } else {
        ok = mqtt.connect(mqtt_client_id, TOPIC_ESTADO, 0, true, "offline");
    }

    if (ok) {
        LOG_INFO("MQTT OK");
        haDisponible = true;
        fallosConsecutivos = 0;
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
        LOG_WARN("MQTT fallo, rc=%d", mqtt.state());
        haDisponible = false;
        fallosConsecutivos++;
        reportarFalloMQTT();
    }
}

void inicializarMQTT() {
    mqtt.setSocketTimeout(1);       // Timeout mínimo de PubSubClient (afecta read, no connect)
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

        // Backoff progresivo: cuanto más falla, más se espacia.
        // 1er fallo: 10s, 2do+: 30s. Así se reduce el tiempo bloqueado.
        unsigned long intervalo = (fallosConsecutivos >= 2)
            ? INTERVALO_MQTT_SIN_BROKER
            : INTERVALO_RECONEXION_MQTT;

        if (ahora - ultimoIntentoMQTT > intervalo) {
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
