#pragma once
#include <PubSubClient.h>

extern PubSubClient mqtt;
extern bool haDisponible;

void inicializarMQTT();
void manejarMQTT();
void publicarEstadoBocina();
