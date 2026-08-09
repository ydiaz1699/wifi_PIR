#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "mqtt_discovery.h"
#include "logger.h"

extern PubSubClient mqtt;

static void agregarDevice(JsonObject doc) {
    JsonObject device = doc.createNestedObject("device");
    JsonArray ids = device.createNestedArray("identifiers");
    ids.add("bocina_esp_alarma");
    device["name"] = "Alarma Bocina";
    device["manufacturer"] = "Casero";
    device["model"] = "ESP8266 D1 Mini";
    device["sw_version"] = "3.2";
}

static void publicarEntidad(const char* topicConfig, void (*llenar)(JsonObject)) {
    StaticJsonDocument<768> doc;
    llenar(doc.as<JsonObject>());
    agregarDevice(doc.as<JsonObject>());

    char payload[768];
    size_t n = serializeJson(doc, payload);
    mqtt.publish(topicConfig, (const uint8_t*)payload, n, true);
}

void publicarDiscovery() {
    publicarEntidad("homeassistant/binary_sensor/alarma_pir/config", [](JsonObject o) {
        o["name"] = "Alarma - Movimiento PIR";
        o["unique_id"] = "alarma_pir_evento";
        o["state_topic"] = "casa/alarma/evento";
        o["payload_on"] = "detectado";
        o["off_delay"] = 5;
        o["device_class"] = "motion";
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/binary_sensor/alarma_timbre/config", [](JsonObject o) {
        o["name"] = "Alarma - Timbre";
        o["unique_id"] = "alarma_timbre_evento";
        o["state_topic"] = "casa/alarma/timbre";
        o["payload_on"] = "presionado";
        o["off_delay"] = 3;
        o["device_class"] = "occupancy";
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/binary_sensor/alarma_bocina_online/config", [](JsonObject o) {
        o["name"] = "Bocina Alarma - Online";
        o["unique_id"] = "alarma_bocina_online";
        o["state_topic"] = "casa/alarma/estado";
        o["payload_on"] = "online";
        o["payload_off"] = "offline";
        o["device_class"] = "connectivity";
    });

    publicarEntidad("homeassistant/switch/alarma_bocina_manual/config", [](JsonObject o) {
        o["name"] = "Alarma - Forzar Bocina";
        o["unique_id"] = "alarma_bocina_manual";
        o["command_topic"] = "casa/alarma/bocina/set";
        o["state_topic"] = "casa/alarma/bocina/state";
        o["payload_on"] = "ON";
        o["payload_off"] = "OFF";
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/select/alarma_modo/config", [](JsonObject o) {
        o["name"] = "Alarma - Modo";
        o["unique_id"] = "alarma_modo";
        o["command_topic"] = "casa/alarma/modo/set";
        o["state_topic"] = "casa/alarma/modo/state";
        JsonArray opts = o.createNestedArray("options");
        opts.add("armado");
        opts.add("desarmado");
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
    });

    publicarEntidad("homeassistant/sensor/alarma_bocina_uptime/config", [](JsonObject o) {
        o["name"] = "Alarma Bocina - Uptime";
        o["unique_id"] = "alarma_bocina_uptime";
        o["state_topic"] = "casa/alarma/uptime";
        o["unit_of_measurement"] = "s";
        o["icon"] = "mdi:clock-outline";
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
        o["entity_category"] = "diagnostic";
    });

    publicarEntidad("homeassistant/sensor/alarma_bocina_ip/config", [](JsonObject o) {
        o["name"] = "Alarma Bocina - IP";
        o["unique_id"] = "alarma_bocina_ip";
        o["state_topic"] = "casa/alarma/ip";
        o["icon"] = "mdi:ip-network";
        o["availability_topic"] = "casa/alarma/estado";
        o["payload_available"] = "online";
        o["payload_not_available"] = "offline";
        o["entity_category"] = "diagnostic";
    });

    LOG_INFO("Discovery MQTT publicado (7 entidades, 1 dispositivo)");
}
