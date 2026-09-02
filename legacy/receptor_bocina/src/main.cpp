#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "red_wifi.h"
#include "mqtt_cliente.h"
#include "alarma.h"
#include "hal.h"
#include "ota.h"
#include "logger.h"
#include "state_machine.h"
#include "config.h"

Led led(pinLed);
Buzzer buzzer(pinBocina);

void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_INFO("===== Booting Alarma Receptor v3.3 =====");

    ESP.wdtEnable(8000);

    led.begin();
    buzzer.begin();
    buzzer.setLed(&led);

    transitionTo(SystemState::BOOT);
    iniciarConexionWiFi();

    // Esperar WiFi antes de decidir modo (máximo 10 segundos)
    LOG_INFO("Esperando WiFi para decidir modo...");
    unsigned long t0 = millis();
    while (!wifiConectado() && millis() - t0 < 10000) {
        ESP.wdtFeed();
        delay(100);
    }

    inicializarAlarma();
    inicializarMQTT();  // Aquí se decide LOCAL o INTELIGENTE
    setupOTA();

    LOG_INFO("Modo de operacion: %s", modoConexionStr());
    LOG_INFO("Setup completo — loop activo");
}

void loop() {
    ESP.wdtFeed();

    manejarWiFi();
    buzzer.loop();
    handleOTA();

    // ============================================================
    // PRIORIDAD #1: Procesar eventos UDP (PIR/timbre) — SIEMPRE
    // Esto corre en cada iteración del loop sin importar nada.
    // Es la función primaria del dispositivo.
    // ============================================================
    manejarAlarma();

    // ============================================================
    // PRIORIDAD #2: MQTT
    // En modo LOCAL: manejarMQTT() solo hace el sondeo cada 5 min.
    // En modo INTELIGENTE: mqtt.loop() + reconexión si se pierde.
    // Internamente, manejarMQTT() ya verifica que no intente
    // connect() mientras la bocina esté sonando.
    // ============================================================
    if (wifiConectado()) {
        manejarMQTT();
    }

    // --- Publicar cambios de estado de la bocina ---
    static bool lastBuzzerState = false;
    bool currentBuzzerState = buzzer.isOn();
    if (currentBuzzerState != lastBuzzerState) {
        lastBuzzerState = currentBuzzerState;
        publicarEstadoBocina();
    }

    // --- Transición de vuelta a READY cuando termina la alarma ---
    if (inState(SystemState::ALARM_TRIGGERED) && !buzzer.isOn()) {
        transitionTo(SystemState::READY);
    }
}
