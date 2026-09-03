/**
 * MQTT Manager V4.1 — Modo dual LOCAL/HA
 *
 * - El primer intento se difiere al loop, después de atender IoTNode.
 * - Si el broker responde → MODO_HA; si no → MODO_LOCAL.
 * - En LOCAL sondea cada 5 min; en HA reintenta cada 15 s.
 * - Nunca intenta MQTT mientras la bocina está activa.
 */

#include <ESP8266WiFi.h>
#include "mqtt_manager.h"
#include "hal.h"
#include "config.h"
#include "logger.h"
#include "mqtt_discovery.h"

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
static unsigned long inicioMQTT = 0;
static bool primerIntentoPendiente = false;
static bool solicitarStateSync = false;
static const unsigned long MQTT_INITIAL_DELAY_MS = 1000;

const char* modoMQTTStr() {
    return (modoMQTT == ModoMQTT::MODO_HA) ? "HA" : "LOCAL";
}

static void publicarModoAlarma() {
    mqtt.publish(TOPIC_MODO_STATE, modoAlarma.c_str(), true);
    mqtt.publish(TOPIC_V3_MODO_STATE, modoAlarma.c_str(), true);
}

static void procesarComandoBocina(const String& mensaje) {
    if (mensaje == "ON") {
        buzzer.timedOn(DURACION_BOCINA_MOTION_MS);
    } else if (mensaje == "OFF") {
        buzzer.off();
        publicarEstadoBocina();
    }
}

static void procesarComandoModo(const String& mensaje) {
    if (mensaje != "armado" && mensaje != "desarmado") return;

    modoAlarma = mensaje;
    publicarModoAlarma();
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String mensaje;
    for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];

    String t = String(topic);
    LOG_INFO("MQTT [%s]: %s", topic, mensaje.c_str());

    // Los topics V3/V4 son aliases de compatibilidad. Ambos pasan por el
    // mismo handler; aún no existe CMD_ID/deduplicación MQTT contractual.
    if (t == TOPIC_BOCINA_CMD || t == TOPIC_V3_BOCINA_CMD) {
        procesarComandoBocina(mensaje);
    } else if (t == TOPIC_MODO || t == TOPIC_V3_MODO) {
        procesarComandoModo(mensaje);
    }
}

static bool intentarConexionMQTT() {
    LOG_INFO("MQTT: conectando a %s:%d...", mqtt_server, mqtt_port);

    bool ok;
    if (mqtt_user[0] != '\0') {
        ok = mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass,
                          TOPIC_V3_ESTADO, 0, true, "offline");
    } else {
        ok = mqtt.connect(mqtt_client_id, TOPIC_V3_ESTADO, 0, true, "offline");
    }

    if (ok) {
        LOG_INFO("MQTT conectado OK");
        mqttDisponible = true;
        solicitarStateSync = true;
        fallosConsecutivos = 0;

        // El topic V3 es el LWT/availability que consume Discovery.
        mqtt.publish(TOPIC_V3_ESTADO, "online", true);
        mqtt.publish(TOPIC_ESTADO, "online", true);
        publicarDiscovery();

        const char* buzzerState = buzzer.isOn() ? "ON" : "OFF";
        mqtt.publish(TOPIC_BOCINA_STATE, buzzerState, true);
        mqtt.publish(TOPIC_V3_BOCINA_STATE, buzzerState, true);
        mqtt.publish(TOPIC_MODO_STATE, modoAlarma.c_str(), true);
        mqtt.publish(TOPIC_V3_MODO_STATE, modoAlarma.c_str(), true);
        const String ip = WiFi.localIP().toString();
        mqtt.publish(TOPIC_IP, ip.c_str(), true);
        mqtt.publish(TOPIC_V3_IP, ip.c_str(), true);
        mqtt.subscribe(TOPIC_BOCINA_CMD);
        mqtt.subscribe(TOPIC_V3_BOCINA_CMD);
        mqtt.subscribe(TOPIC_MODO);
        mqtt.subscribe(TOPIC_V3_MODO);
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
    mqtt.setBufferSize(768);
    mqtt.setCallback(mqttCallback);

    // No conectar desde setup: IoTNode debe quedar atendiendo UDP primero.
    // El primer intento se agenda para manejarMQTT(), después de una iteración
    // completa de node.loop(), y conserva el fallback LOCAL si falla.
    modoMQTT = ModoMQTT::MODO_LOCAL;
    mqttDisponible = false;
    inicioMQTT = millis();
    primerIntentoPendiente = true;
    ultimoSondeo = inicioMQTT;
    LOG_INFO(">>> MQTT diferido: arranque en modo LOCAL <<<");
}

void manejarMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        mqttDisponible = false;
        return;
    }

    // El primer connect se ejecuta solo desde el loop, y nunca antes de que
    // IoTNode haya tenido su primera oportunidad de drenar UDP/ACKs.
    const unsigned long ahora = millis();
    if (primerIntentoPendiente && ahora - inicioMQTT >= MQTT_INITIAL_DELAY_MS) {
        // Igual que V3: una alarma local activa tiene prioridad sobre MQTT.
        if (buzzer.isOn()) return;
        primerIntentoPendiente = false;
        LOG_INFO("Boot: probando broker MQTT tras arranque IoT...");
        if (intentarConexionMQTT()) {
            modoMQTT = ModoMQTT::MODO_HA;
            LOG_INFO(">>> Modo HA <<<");
        } else {
            modoMQTT = ModoMQTT::MODO_LOCAL;
            ultimoSondeo = ahora;
            LOG_INFO(">>> Modo LOCAL (broker no disponible) <<<");
        }
        return;
    }

    // --- MODO LOCAL ---
    if (modoMQTT == ModoMQTT::MODO_LOCAL) {
        const unsigned long localAhora = millis();
        if (localAhora - ultimoSondeo >= MQTT_SONDEO_INTERVAL_MS) {
            ultimoSondeo = localAhora;
            if (!buzzer.isOn()) {
                LOG_INFO("Sondeo broker...");
                if (intentarConexionMQTT()) {
                    modoMQTT = ModoMQTT::MODO_HA;
                    LOG_INFO(">>> Broker detectado! Modo HA <<<");
                }
            } else {
                // Posponer sondeo 30s si hay alarma activa, como en V3.
                ultimoSondeo = localAhora - MQTT_SONDEO_INTERVAL_MS + 30000;
            }
        }
        return;
    }

    // --- MODO HA ---
    if (!mqtt.connected()) {
        if (mqttDisponible) {
            // Sellar el instante de la caída una sola vez. Así el primer
            // reintento ocurre 15s después de detectarla, no inmediatamente.
            ultimoIntentoMQTT = millis();
        }
        mqttDisponible = false;
        const unsigned long reconnectAhora = millis();
        if (reconnectAhora - ultimoIntentoMQTT > MQTT_RECONNECT_INTERVAL_MS) {
            ultimoIntentoMQTT = reconnectAhora;
            if (buzzer.isOn()) return;
            if (!intentarConexionMQTT()) {
                if (fallosConsecutivos >= 3) {
                    LOG_WARN("Broker caido, volviendo a LOCAL");
                    modoMQTT = ModoMQTT::MODO_LOCAL;
                    // Primer sondeo acelerado tras una caída; los siguientes
                    // respetan MQTT_SONDEO_INTERVAL_MS (5 minutos).
                    ultimoSondeo = millis() - MQTT_SONDEO_INTERVAL_MS +
                                   MQTT_SONDEO_DESPUES_DE_CAIDA_MS;
                    fallosConsecutivos = 0;
                }
            }
        }
        return;
    }

    const bool loopOk = mqtt.loop();
    mqttDisponible = loopOk && mqtt.connected();
    if (!mqttDisponible) return;

    const unsigned long uptimeAhora = millis();
    if (uptimeAhora - ultimoUptime > 60000) {
        ultimoUptime = uptimeAhora;
        const String uptime = String(millis() / 1000);
        mqtt.publish(TOPIC_UPTIME, uptime.c_str(), true);
        mqtt.publish(TOPIC_V3_UPTIME, uptime.c_str(), true);
    }
}

bool consumirSolicitudStateSync() {
    const bool pendiente = solicitarStateSync;
    solicitarStateSync = false;
    return pendiente;
}

void publicarEstadoBocina() {
    if (mqtt.connected()) {
        const char* state = buzzer.isOn() ? "ON" : "OFF";
        mqtt.publish(TOPIC_BOCINA_STATE, state, true);
        mqtt.publish(TOPIC_V3_BOCINA_STATE, state, true);
    }
}
