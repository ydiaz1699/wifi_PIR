#pragma once
#include <PubSubClient.h>

extern PubSubClient mqtt;
extern bool haDisponible;

// --- Modos de operación ---
enum class ModoConexion { LOCAL, INTELIGENTE };
extern ModoConexion modoConexion;

const char* modoConexionStr();

void inicializarMQTT();
void manejarMQTT();
void publicarEstadoBocina();
