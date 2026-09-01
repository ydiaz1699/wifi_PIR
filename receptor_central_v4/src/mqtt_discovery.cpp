#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "config.h"
#include "logger.h"
#include "mqtt_discovery.h"

extern PubSubClient mqtt;

namespace {

void agregarDevice(JsonObject doc) {
    JsonObject device = doc.createNestedObject("device");
    JsonArray ids = device.createNestedArray("identifiers");
    ids.add("central_iot");
    device["name"] = "Central Alarma IoT";
    device["manufacturer"] = "Casero";
    device["model"] = "ESP8266 NodeMCU";
    device["sw_version"] = "V4";
}

void publicarEntidad(const char* topicConfig, void (*llenar)(JsonObject)) {
    StaticJsonDocument<768> doc;
    JsonObject entity = doc.to<JsonObject>();
    llenar(entity);
    agregarDevice(entity);

    char payload[768];
    const size_t length = serializeJson(doc, payload, sizeof(payload));
    if (length == 0 || !mqtt.publish(topicConfig, payload, true)) {
        LOG_WARN("Discovery MQTT fallo: %s", topicConfig);
    }
}

}  // namespace

void publicarDiscovery() {
    // Los state/command topics son los de V3 para conservar automatizaciones.
    publicarEntidad("homeassistant/binary_sensor/central_alarma_pir/config", [](JsonObject o) {
        o["name"] = "Alarma - Movimiento PIR";
        o["unique_id"] = "central_alarma_pir_evento";
        o["state_topic"] = TOPIC_V3_EVENTO;
        o["payload_on"] = "detectado";
        o["off_delay"] = 5;
        o["device_class"] = "motion";
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/binary_sensor/central_alarma_timbre/config", [](JsonObject o) {
        o["name"] = "Alarma - Timbre";
        o["unique_id"] = "central_alarma_timbre_evento";
        o["state_topic"] = TOPIC_V3_TIMBRE;
        o["payload_on"] = "presionado";
        o["off_delay"] = 3;
        o["device_class"] = "occupancy";
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/binary_sensor/central_alarma_online/config", [](JsonObject o) {
        o["name"] = "Central Alarma - Online";
        o["unique_id"] = "central_alarma_online";
        o["state_topic"] = TOPIC_V3_ESTADO;
        o["payload_on"] = "online";
        o["payload_off"] = "offline";
        o["device_class"] = "connectivity";
    });

    publicarEntidad("homeassistant/switch/central_alarma_manual/config", [](JsonObject o) {
        o["name"] = "Alarma - Forzar Bocina";
        o["unique_id"] = "central_alarma_manual";
        o["command_topic"] = TOPIC_V3_BOCINA_CMD;
        o["state_topic"] = TOPIC_V3_BOCINA_STATE;
        o["payload_on"] = "ON";
        o["payload_off"] = "OFF";
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/select/central_alarma_modo/config", [](JsonObject o) {
        o["name"] = "Alarma - Modo";
        o["unique_id"] = "central_alarma_modo";
        o["command_topic"] = TOPIC_V3_MODO;
        o["state_topic"] = TOPIC_V3_MODO_STATE;
        JsonArray options = o.createNestedArray("options");
        options.add("armado");
        options.add("desarmado");
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/sensor/central_alarma_uptime/config", [](JsonObject o) {
        o["name"] = "Central Alarma - Uptime";
        o["unique_id"] = "central_alarma_uptime";
        o["state_topic"] = TOPIC_V3_UPTIME;
        o["unit_of_measurement"] = "s";
        o["icon"] = "mdi:clock-outline";
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
        o["entity_category"] = "diagnostic";
    });

    publicarEntidad("homeassistant/sensor/central_alarma_ip/config", [](JsonObject o) {
        o["name"] = "Central Alarma - IP";
        o["unique_id"] = "central_alarma_ip";
        o["state_topic"] = TOPIC_V3_IP;
        o["icon"] = "mdi:ip-network";
        o["availability_topic"] = TOPIC_V3_ESTADO;
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
        o["entity_category"] = "diagnostic";
    });

    LOG_INFO("Discovery MQTT V4 publicado (7 entidades, topics V3 compatibles)");
}
