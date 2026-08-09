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
ModoConexion modoConexion = ModoConexion::LOCAL;  // Arranca en LOCAL hasta detectar broker

// --- Timers ---
static unsigned long ultimoIntentoMQTT = 0;
static unsigned long ultimoUptime = 0;
static unsigned long ultimoSondeo = 0;

// --- Sondeo del broker ---
// En modo LOCAL, cada INTERVALO_SONDEO_BROKER intenta conectar UNA vez.
// Si conecta → pasa a INTELIGENTE. Si falla → sigue en LOCAL.
// El sondeo se hace SOLO si la bocina NO está sonando.
static const unsigned long INTERVALO_SONDEO_BROKER = 300000;  // 5 minutos

// --- Estado del intento de conexión bloqueante ---
// Usamos un flag para que el sondeo bloqueante ocurra máximo UNA vez
// por ciclo de sondeo, y nunca durante una alarma activa.
static bool sondeoEnCurso = false;

const char* modoConexionStr() {
    return (modoConexion == ModoConexion::INTELIGENTE) ? "INTELIGENTE" : "LOCAL";
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String mensaje;
    for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];

    String t = String(topic);
    LOG_INFO("MQTT [%s]: %s", topic, mensaje.c_str());

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

// Intenta la conexión MQTT real (puede bloquear ~5s si broker no responde)
static bool intentarConexionMQTT() {
    LOG_INFO("MQTT: conectando a %s:%d...", mqtt_server, mqtt_port);

    bool ok;
    if (mqtt_user[0] != '\0') {
        ok = mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass, TOPIC_ESTADO, 0, true, "offline");
    } else {
        ok = mqtt.connect(mqtt_client_id, TOPIC_ESTADO, 0, true, "offline");
    }

    if (ok) {
        LOG_INFO("MQTT conectado OK");
        haDisponible = true;
        reportarExitoConexion();
        mqtt.publish(TOPIC_ESTADO, "online", true);
        publicarDiscovery();
        mqtt.publish(TOPIC_COMANDO_STATE, buzzer.isOn() ? "ON" : "OFF", true);
        mqtt.publish(TOPIC_MODO_STATE, modoActual.c_str(), true);
        mqtt.publish(TOPIC_IP, WiFi.localIP().toString().c_str(), true);
        mqtt.subscribe(TOPIC_COMANDO);
        mqtt.subscribe(TOPIC_MODO);
        return true;
    }

    LOG_WARN("MQTT fallo, rc=%d", mqtt.state());
    haDisponible = false;
    return false;
}

void inicializarMQTT() {
    mqtt.setSocketTimeout(2);  // Mínimo práctico para ESP8266
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setBufferSize(768);
    mqtt.setCallback(mqttCallback);

    // --- Intento inicial al boot ---
    // Si el broker responde, arranca en modo INTELIGENTE.
    // Si no, arranca en LOCAL (sin más intentos hasta el próximo sondeo).
    if (wifiConectado()) {
        LOG_INFO("Boot: probando broker MQTT...");
        if (intentarConexionMQTT()) {
            modoConexion = ModoConexion::INTELIGENTE;
            transitionTo(SystemState::READY);
            LOG_INFO(">>> Modo INTELIGENTE (Home Assistant) <<<");
        } else {
            modoConexion = ModoConexion::LOCAL;
            LOG_INFO(">>> Modo LOCAL (broker no disponible) <<<");
        }
    } else {
        modoConexion = ModoConexion::LOCAL;
        LOG_INFO(">>> Modo LOCAL (sin WiFi al boot) <<<");
    }
    ultimoSondeo = millis();
}

void manejarMQTT() {
    if (!wifiConectado()) {
        haDisponible = false;
        return;
    }

    // ==========================================
    // MODO LOCAL: no hace nada de MQTT
    // Solo sondea cada 5 minutos para ver si el broker volvió.
    // El sondeo SOLO ocurre si la bocina NO está sonando.
    // ==========================================
    if (modoConexion == ModoConexion::LOCAL) {
        unsigned long ahora = millis();
        if (ahora - ultimoSondeo >= INTERVALO_SONDEO_BROKER) {
            ultimoSondeo = ahora;
            if (!buzzer.isOn()) {
                LOG_INFO("Sondeo: verificando si broker MQTT volvio...");
                if (intentarConexionMQTT()) {
                    modoConexion = ModoConexion::INTELIGENTE;
                    LOG_INFO(">>> Broker detectado! Cambiando a modo INTELIGENTE <<<");
                    transitionTo(SystemState::READY);
                } else {
                    LOG_INFO("Sondeo: broker sigue ausente, continuo en LOCAL");
                }
            } else {
                // Posponer sondeo 30s si hay alarma activa
                ultimoSondeo = ahora - INTERVALO_SONDEO_BROKER + 30000;
            }
        }
        return;  // En modo LOCAL, no hacemos nada más de MQTT
    }

    // ==========================================
    // MODO INTELIGENTE: MQTT activo
    // ==========================================
    if (!mqtt.connected()) {
        haDisponible = false;
        unsigned long ahora = millis();

        if (ahora - ultimoIntentoMQTT > INTERVALO_RECONEXION_MQTT) {
            ultimoIntentoMQTT = ahora;

            // NO intentar reconectar si la bocina está sonando
            if (buzzer.isOn()) return;

            if (!inState(SystemState::CONNECT_MQTT) && !inState(SystemState::RECOVER)) {
                transitionTo(SystemState::CONNECT_MQTT);
            }

            if (!intentarConexionMQTT()) {
                reportarFalloMQTT();

                // Si falla muchas veces consecutivas, volver a LOCAL
                // (el broker se apagó para mantenimiento)
                static uint8_t fallosDesdeInteligente = 0;
                fallosDesdeInteligente++;
                if (fallosDesdeInteligente >= 3) {
                    LOG_WARN("Broker caido, volviendo a modo LOCAL");
                    modoConexion = ModoConexion::LOCAL;
                    ultimoSondeo = millis();
                    fallosDesdeInteligente = 0;
                    transitionTo(SystemState::READY);
                }
            } else {
                static uint8_t fallosDesdeInteligente = 0;
                fallosDesdeInteligente = 0;
            }
        }
        return;
    }

    // --- Conectado: loop normal ---
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
