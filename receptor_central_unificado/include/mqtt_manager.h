#pragma once
#include <PubSubClient.h>

extern PubSubClient mqtt;
extern bool mqttDisponible;

// Modos de conexión MQTT (igual que V3.3)
enum class ModoMQTT { MODO_LOCAL, MODO_HA };
extern ModoMQTT modoMQTT;

void inicializarMQTT();
void manejarMQTT();
void publicarEstadoBocina();
const char* modoMQTTStr();
