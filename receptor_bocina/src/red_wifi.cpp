#include <ESP8266WiFi.h>
#include "red_wifi.h"
#include "config.h"
#include "logger.h"
#include "state_machine.h"

static bool wifiConectando = false;
static unsigned long ultimoIntentoWiFi = 0;

void iniciarConexionWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);
    wifiConectando = true;
    ultimoIntentoWiFi = millis();
    LOG_INFO("Iniciando conexion WiFi...");
}

void manejarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (wifiConectando) {
            wifiConectando = false;
            LOG_INFO("WiFi OK, IP: %s", WiFi.localIP().toString().c_str());
            reportarExitoConexion();
            if (inState(SystemState::CONNECT_WIFI) || inState(SystemState::RECOVER)) {
                transitionTo(SystemState::CONNECT_MQTT);
            }
        }
        return;
    }
    unsigned long ahora = millis();
    if (!wifiConectando) {
        iniciarConexionWiFi();
    } else if (ahora - ultimoIntentoWiFi > INTERVALO_REINTENTO_WIFI) {
        LOG_WARN("WiFi no conectado, reintentando...");
        reportarFalloWiFi();
        iniciarConexionWiFi();
    }
}

bool wifiConectado() {
    return WiFi.status() == WL_CONNECTED;
}
