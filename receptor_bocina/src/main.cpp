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
    LOG_INFO("===== Booting Alarma Receptor v3.2 =====");

    ESP.wdtEnable(8000);

    led.begin();
    buzzer.begin();
    buzzer.setLed(&led);

    transitionTo(SystemState::BOOT);
    iniciarConexionWiFi();
    inicializarAlarma();
    inicializarMQTT();
    setupOTA();

    transitionTo(SystemState::CONNECT_WIFI);
}

void loop() {
    ESP.wdtFeed();

    manejarWiFi();
    buzzer.loop();
    handleOTA();

    // ============================================================
    // PRIORIDAD #1: Procesar eventos UDP (PIR/timbre)
    // Esto SIEMPRE corre primero, sin importar el estado de MQTT.
    // El receptor debe responder ACK al emisor en <300ms.
    // ============================================================
    manejarAlarma();

    // ============================================================
    // PRIORIDAD #2: MQTT (solo si WiFi conectado Y no en alarma activa)
    // Si la bocina está sonando, no intentar reconexión MQTT porque
    // connect() puede bloquear ~1s y perderíamos paquetes UDP de
    // un segundo evento que llegue inmediatamente después.
    // ============================================================
    if (wifiConectado()) {
        if (mqtt.connected()) {
            // Ya conectado: solo hacer loop() que es no-bloqueante
            manejarMQTT();
        } else if (!buzzer.isOn()) {
            // Desconectado y sin alarma activa: intentar reconectar
            // (con la guardia TCP anti-bloqueo en mqtt_cliente.cpp)
            manejarMQTT();
        }
        // Si desconectado Y buzzer activo: no intentar reconexión,
        // se reintentará cuando la bocina se apague.
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

    // --- Manejo de ERROR/RECOVER ---
    if (inState(SystemState::ERROR)) {
        static unsigned long entradaError = 0;
        static bool primeraVezEnError = true;
        if (primeraVezEnError) {
            entradaError = millis();
            primeraVezEnError = false;
            LOG_WARN("Sistema en ERROR: WiFi/MQTT fallando, alarma sigue en modo local");
        }
        if (millis() - entradaError > 5000) {
            transitionTo(SystemState::RECOVER);
            primeraVezEnError = true;
        }
    }

    if (inState(SystemState::RECOVER)) {
        static bool intentoHecho = false;
        if (!intentoHecho) {
            LOG_WARN("RECOVER: forzando reconexion WiFi/MQTT");
            WiFi.disconnect();
            mqtt.disconnect();
            iniciarConexionWiFi();
            intentoHecho = true;
        }
        if (wifiConectado() && mqtt.connected()) {
            intentoHecho = false;
            transitionTo(SystemState::READY);
        }
    }
}
