/**
 * Receptor Central IoT V4
 *
 * Recibe paquetes IoTProtocol de cualquier sensor/botón/dispositivo.
 * No tiene lógica hardcodeada por sensor — procesa genéricamente.
 *
 * Flujo del loop:
 *   1. WiFi management
 *   2. IoTNode.loop() — recibe UDP, despacha, envía ACKs
 *   3. Buzzer timer
 *   4. MQTT (si WiFi ok y modo HA)
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
    LOG_INFO("===== Central IoT Receptor V4.0 =====");

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

    // Inicializar IoTProtocol node
    node.begin();
    node.onPacketReceived(handleIoTPacket);
    LOG_INFO("IoTNode iniciado (ID=0x%02X, puerto=%d)", MY_DEVICE_ID, IOT_UDP_PORT);

    // MQTT (modo dual LOCAL/HA)
    inicializarMQTT();

    setupOTA();

    LOG_INFO("Modo MQTT: %s | Modo alarma: %s", modoMQTTStr(), modoAlarma.c_str());
    LOG_INFO("Setup completo — esperando eventos...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    ESP.wdtFeed();

    manejarWiFi();
    buzzer.loop();

    // PRIORIDAD #1: IoTProtocol — recibir, ACK, despachar eventos
    node.loop();

    // PRIORIDAD #2: MQTT (no bloquea en modo LOCAL)
    if (WiFi.status() == WL_CONNECTED) {
        manejarMQTT();
    }

    // Publicar cambios de bocina
    static bool lastBuzzer = false;
    if (buzzer.isOn() != lastBuzzer) {
        lastBuzzer = buzzer.isOn();
        publicarEstadoBocina();
    }

    ArduinoOTA.handle();
}
