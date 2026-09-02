#pragma once

enum class SystemState {
    BOOT,
    CONNECT_WIFI,
    CONNECT_MQTT,
    READY,
    ALARM_TRIGGERED,
    ERROR,
    RECOVER
};

const char* stateToString(SystemState s);
void transitionTo(SystemState newState);
SystemState currentState();
bool inState(SystemState s);

void reportarFalloWiFi();
void reportarFalloMQTT();
void reportarExitoConexion();
