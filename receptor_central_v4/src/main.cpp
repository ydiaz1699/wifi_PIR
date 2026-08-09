/**
 * Receptor Central IoT — V4.1
 *
 * Recibe paquetes IoTProtocol de cualquier sensor/botón/dispositivo.
 * No tiene lógica hardcodeada por sensor — procesa genéricamente.
 * La biblioteca IoTNode maneja ACK, deduplicación, y registro automático.
 *
 * Loop:
 *   1. WiFi
 *   2. IoTNode.loop() → recibe UDP, ACK, dedup, despacha
 *   3. Buzzer timer
 *   4. MQTT (modo LOCAL/HA)
 *   5. OTA
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <IoTNode.h>
#include "config.h"
#include "hal.h"
#include "logger.h"
#include "mqtt_manager.h"
#include "event_handler.h"

// --- Hardware ---
Led led(pinLed);
Buzzer buzzer(pinBocina);

// --- IoTProtocol Node ---
IoTNode node(MY_DEVICE_ID, IOT_UDP_PORT);

// --- Estado ---
String modoAlarma = "armado";

// --- WiFi ---
static bool wifiConectando = false;
static unsigned long ultimoIntentoWiFi = 0;

static void iniciarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.config(local_IP, gateway_IP, subnet_mask);
    WiFi.begin(ssid, password);
    wifiConectando = true;
    ultimoIntentoWiFi = millis();
    LOG_INFO("WiFi conectando...");
}

static void manejarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (wifiConectando) {
            wifiConectando = false;
            LOG_INFO("WiFi OK: %s", WiFi.localIP().toString().c_str());
        }
        return;
    }
    unsigned long ahora = millis();
    if (!wifiConectando) {
        iniciarWiFi();
    } else if (ahora - ultimoIntentoWiFi > 5000) {
        LOG_WARN("WiFi reintentando...");
        iniciarWiFi();
    }
}

// --- OTA ---
static void setupOTA() {
    ArduinoOTA.setHostname("central-iot");
    ArduinoOTA.onStart([]() { LOG_INFO("OTA Start"); });
    ArduinoOTA.onEnd([]() { LOG_INFO("OTA End"); });
    ArduinoOTA.onError([](ota_error_t err) { LOG_ERROR("OTA Error %u", err); });
    ArduinoOTA.begin();
    LOG_INFO("OTA listo");
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Central IoT V4.1 =====");

    ESP.wdtEnable(8000);

    led.begin();
    buzzer.begin();
    buzzer.setLed(&led);

    iniciarWiFi();

    // Esperar WiFi (máximo 10s)
    LOG_INFO("Esperando WiFi...");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
        ESP.wdtFeed();
        delay(100);
    }

    // IoTNode
    node.begin();
    node.onPacketReceived(handleIoTPacket);
    LOG_INFO("IoTNode (ID=0x%02X, bootId=0x%04X, puerto=%d)",
             MY_DEVICE_ID, node.getBootId(), IOT_UDP_PORT);

    // MQTT (dual LOCAL/HA)
    inicializarMQTT();

    setupOTA();

    LOG_INFO("Modo MQTT: %s | Alarma: %s", modoMQTTStr(), modoAlarma.c_str());
    LOG_INFO("Setup completo — esperando eventos...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    ESP.wdtFeed();

    manejarWiFi();
    buzzer.loop();

    // PRIORIDAD #1: IoTProtocol (nunca bloquea)
    node.loop();

    // PRIORIDAD #2: MQTT
    if (WiFi.status() == WL_CONNECTED) {
        manejarMQTT();
    }

    // Publicar cambios de bocina
    static bool lastBuzzer = false;
    if (buzzer.isOn() != lastBuzzer) {
        lastBuzzer = buzzer.isOn();
        publicarEstadoBocina();
    }

    // Publicar estado ONLINE/STALE/OFFLINE cada 30s
    static unsigned long lastStatusPub = 0;
    if (millis() - lastStatusPub >= 30000) {
        lastStatusPub = millis();
        if (mqttDisponible) {
            // Buscar todos los IDs posibles de sensores (0x02–0x7F)
            for (uint8_t id = 0x02; id <= 0x7F; id++) {
                RemoteDevice* dev = node.getRemote(id);
                if (!dev) continue;
                char topic[48];
                snprintf(topic, sizeof(topic), "casa/iot/device_%02X/status", dev->id);
                const char* stateStr = "unknown";
                switch (dev->state) {
                    case DeviceState::ONLINE:  stateStr = "online"; break;
                    case DeviceState::STALE:   stateStr = "stale"; break;
                    case DeviceState::OFFLINE: stateStr = "offline"; break;
                    default: break;
                }
                mqtt.publish(topic, stateStr, true);
            }
        }
    }

    ArduinoOTA.handle();
}
