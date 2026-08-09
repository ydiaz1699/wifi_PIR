#pragma once
#include <PubSubClient.h>

extern PubSubClient mqtt;
extern bool haDisponible;

// --- Modos de operación ---
// NOTA: No usar "LOCAL" como nombre — el SDK ESP8266 lo define como macro (#define LOCAL static)
enum class ModoConexion { MODO_LOCAL, MODO_HA };
extern ModoConexion modoConexion;

const char* modoConexionStr();

void inicializarMQTT();
void manejarMQTT();
void publicarEstadoBocina();
