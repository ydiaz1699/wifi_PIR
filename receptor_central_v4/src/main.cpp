/**
 * Receptor Central IoT — V4.3
 *
 * Nuevas features:
 * - IoTStorage: BOOT_ID persistente + config LittleFS
 * - IoTAuth: verificación HMAC opcional para paquetes entrantes
 * - Puede enviar CONFIG a nodos remotos vía MQTT command
 * - STATE_REQUEST broadcast al boot para STATE_SYNC
 *
 * Loop (prioridades):
 *   1. WiFi
 *   2. IoTNode.loop() → UDP, ACK, dedup, despacho
 *   3. Buzzer timer
 *   4. MQTT (modo LOCAL/HA)
 *   5. Device state publish (30s)
 *   6. OTA
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <IoTNode.h>
#include <IoTStorage.h>
#include <IoTAuth.h>
#include "config.h"
#include "hal.h"
#include "logger.h"
#include "mqtt_manager.h"
#include "event_handler.h"

// --- Shared secret (mismo que en emisores) ---
static const uint8_t AUTH_KEY[] = {
    0x4D, 0x79, 0x49, 0x6F, 0x54, 0x4B, 0x65, 0x79,
    0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x21, 0x21
};

// --- Hardware ---
Led led(pinLed);
Buzzer buzzer(pinBocina);

// --- IoTProtocol ---
IoTStorage storage;
IoTNode node(MY_DEVICE_ID, IOT_UDP_PORT);
IoTAuth auth(AUTH_KEY, sizeof(AUTH_KEY));

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
    LOG_INFO("===== Central IoT V4.3 =====");

    ESP.wdtEnable(8000);

    led.begin();
    buzzer.begin();
    buzzer.setLed(&led);

    // --- Storage ---
    if (storage.begin()) {
        storage.loadConfig();
        LOG_INFO("Storage OK: boot#%lu", (unsigned long)storage.getBootCount());
    } else {
        LOG_ERROR("Storage FAIL");
    }

    // Auth: verificar paquetes si habilitado en config
    if (storage.config().authEnabled) {
        auth.setRequired(true);
        LOG_INFO("Auth HMAC: REQUERIDO");
    } else {
        auth.setRequired(false);
        LOG_INFO("Auth HMAC: opcional (acepta todo)");
    }

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

    // STATE_SYNC: pedir estado actual a todos los nodos
    {
        IoTPacket req;
        req.version = IOT_PROTOCOL_VER;
        req.type = MsgType::STATE_REQUEST;
        req.src = MY_DEVICE_ID;
        req.dst = IOT_DEVICE_BROADCAST;
        req.bootId = node.getBootId();
        req.seq = node.getNextSeq();
        req.flags = 0;
        req.clearPayload();
        node.sendDirect(req, IPAddress(192, 168, 0, 255), IOT_UDP_PORT);
        LOG_INFO("STATE_REQUEST broadcast enviado");
    }
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

    // Publicar ONLINE/STALE/OFFLINE cada 30s
    static unsigned long lastStatusPub = 0;
    if (millis() - lastStatusPub >= 30000) {
        lastStatusPub = millis();
        if (mqttDisponible) {
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
