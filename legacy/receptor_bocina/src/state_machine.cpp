#include "state_machine.h"
#include "logger.h"

static SystemState _state = SystemState::BOOT;
static uint8_t _fallosWifiConsecutivos = 0;
static uint8_t _fallosMqttConsecutivos = 0;
static unsigned long _recoverDesde = 0;

static const uint8_t MAX_FALLOS_ANTES_DE_ERROR = 6;
static const unsigned long RECOVER_TIMEOUT_MS = 15000;

const char* stateToString(SystemState s) {
    switch(s) {
        case SystemState::BOOT:             return "BOOT";
        case SystemState::CONNECT_WIFI:     return "CONNECT_WIFI";
        case SystemState::CONNECT_MQTT:     return "CONNECT_MQTT";
        case SystemState::READY:            return "READY";
        case SystemState::ALARM_TRIGGERED:  return "ALARM_TRIGGERED";
        case SystemState::ERROR:            return "ERROR";
        case SystemState::RECOVER:          return "RECOVER";
    }
    return "UNKNOWN";
}

void transitionTo(SystemState newState) {
    if (_state != newState) {
        LOG_INFO("STATE: %s -> %s", stateToString(_state), stateToString(newState));
        _state = newState;
        if (newState == SystemState::RECOVER) {
            _recoverDesde = millis();
        }
    }
}

SystemState currentState() { return _state; }
bool inState(SystemState s) { return _state == s; }

void reportarFalloWiFi() {
    _fallosWifiConsecutivos++;
    if (_fallosWifiConsecutivos >= MAX_FALLOS_ANTES_DE_ERROR && _state != SystemState::ERROR) {
        LOG_ERROR("Demasiados fallos de WiFi consecutivos (%d), pasando a ERROR", _fallosWifiConsecutivos);
        transitionTo(SystemState::ERROR);
    }
}

void reportarFalloMQTT() {
    _fallosMqttConsecutivos++;
    if (_fallosMqttConsecutivos >= MAX_FALLOS_ANTES_DE_ERROR && _state != SystemState::ERROR) {
        LOG_ERROR("Demasiados fallos de MQTT consecutivos (%d), pasando a ERROR", _fallosMqttConsecutivos);
        transitionTo(SystemState::ERROR);
    }
}

void reportarExitoConexion() {
    _fallosWifiConsecutivos = 0;
    _fallosMqttConsecutivos = 0;
}
