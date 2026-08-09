/**
 * MQTT Manager V4 — Modo dual LOCAL/HA (heredado de V3.3)
 *
 * - Al boot intenta una vez. Si broker responde → MODO_HA.
 * - Si no → MODO_LOCAL, sondeo cada 5 min.
 * - Nunca intenta MQTT mientras bocina activa.
 */

#include <ESP8266WiFi.h>
#include "mqtt_manager.h"
#include "hal.h"
#include "config.h"
#include "logger.h"

extern Buzzer buzzer;
extern String modoAlarma;

WiFiClient espClient;
PubSubClient mqtt(espClient);
bool mqttDisponible = false;
ModoMQTT modoMQTT = ModoMQTT::MODO_LOCAL;

static unsigned long ultimoIntentoMQTT = 0;
static unsigned long ultimoUptime = 0;
static unsigned long ultimoSondeo = 0;
static uint8_t fallosConsecutivos = 0;

const char* modoMQTTStr() {
    return (modoMQTT == ModoMQTT::MODO_HA) ? "HA" : "LOCAL";
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String mensaje;
    for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];

    String t = String(topic);
    LOG_INFO("MQTT [%s]: %s", topic, mensaje.c_str());

    if (t == TOPIC_BOCINA_CMD) {
        if (mensaje == "ON") buzzer.timedOn(DURACION_BOCINA_MOTION_MS);
        else if (mensaje == "OFF") { buzzer.off(); publicarEstadoBocina(); }
    } else if (t == TOPIC_MODO) {
        if (mensaje == "armado" || mensaje == "desarmado") {
            modoAlarma = mensaje;
            mqtt.publish(TOPIC_MODO_STATE, modoAlarma.c_str(), true);
        }
    }
}


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
        mqttDisponible = true;
        fallosConsecutivos = 0;
        mqtt.publish(TOPIC_ESTADO, "online", true);
        mqtt.publish(TOPIC_BOCINA_STATE, buzzer.isOn() ? "ON" : "OFF", true);
        mqtt.publish(TOPIC_MODO_STATE, modoAlarma.c_str(), true);
        mqtt.publish(TOPIC_IP, WiFi.localIP().toString().c_str(), true);
        mqtt.subscribe(TOPIC_BOCINA_CMD);
        mqtt.subscribe(TOPIC_MODO);
        return true;
    }

    LOG_WARN("MQTT fallo, rc=%d", mqtt.state());
    mqttDisponible = false;
    fallosConsecutivos++;
    return false;
}

void inicializarMQTT() {
    mqtt.setSocketTimeout(2);
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setBufferSize(512);
    mqtt.setCallback(mqttCallback);

    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("Boot: probando broker MQTT...");
        if (intentarConexionMQTT()) {
            modoMQTT = ModoMQTT::MODO_HA;
            LOG_INFO(">>> Modo HA (Home Assistant) <<<");
        } else {
            modoMQTT = ModoMQTT::MODO_LOCAL;
            LOG_INFO(">>> Modo LOCAL (broker no disponible) <<<");
        }
    } else {
        modoMQTT = ModoMQTT::MODO_LOCAL;
        LOG_INFO(">>> Modo LOCAL (sin WiFi al boot) <<<");
    }
    ultimoSondeo = millis();
}

void manejarMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        mqttDisponible = false;
        return;
    }

    // --- MODO LOCAL: solo sondeo cada 5 min ---
    if (modoMQTT == ModoMQTT::MODO_LOCAL) {
        unsigned long ahora = millis();
        if (ahora - ultimoSondeo >= MQTT_SONDEO_INTERVAL_MS) {
            ultimoSondeo = ahora;
            if (!buzzer.isOn()) {
                LOG_INFO("Sondeo: verificando broker...");
                if (intentarConexionMQTT()) {
                    modoMQTT = ModoMQTT::MODO_HA;
                    LOG_INFO(">>> Broker detectado! Modo HA <<<");
                }
            }
        }
        return;
    }

    // --- MODO HA: MQTT activo ---
    if (!mqtt.connected()) {
        mqttDisponible = false;
        unsigned long ahora = millis();
        if (ahora - ultimoIntentoMQTT > MQTT_RECONNECT_INTERVAL_MS) {
            ultimoIntentoMQTT = ahora;
            if (buzzer.isOn()) return;  // No bloquear durante alarma
            if (!intentarConexionMQTT()) {
                if (fallosConsecutivos >= 3) {
                    LOG_WARN("Broker caido, volviendo a LOCAL");
                    modoMQTT = ModoMQTT::MODO_LOCAL;
                    ultimoSondeo = millis();
                    fallosConsecutivos = 0;
                }
            }
        }
        return;
    }

    mqtt.loop();
    mqttDisponible = true;

    unsigned long ahora = millis();
    if (ahora - ultimoUptime > 60000) {
        ultimoUptime = ahora;
        mqtt.publish(TOPIC_UPTIME, String(millis() / 1000).c_str(), true);
    }
}

void publicarEstadoBocina() {
    if (mqtt.connected()) {
        mqtt.publish(TOPIC_BOCINA_STATE, buzzer.isOn() ? "ON" : "OFF", true);
    }
}
