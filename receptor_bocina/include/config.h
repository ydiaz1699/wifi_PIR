#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "secrets.h"

extern const char* ssid;
extern const char* password;
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;

extern const char* mqtt_server;
extern const int mqtt_port;
extern const char* mqtt_client_id;
extern const char* mqtt_user;
extern const char* mqtt_pass;

extern const char* TOPIC_EVENTO;
extern const char* TOPIC_TIMBRE;
extern const char* TOPIC_ESTADO;
extern const char* TOPIC_COMANDO;
extern const char* TOPIC_COMANDO_STATE;
extern const char* TOPIC_MODO;
extern const char* TOPIC_MODO_STATE;
extern const char* TOPIC_UPTIME;
extern const char* TOPIC_IP;

extern const int pinBocina;
extern const int pinLed;

extern const unsigned int puertoUDP;

extern const unsigned long INTERVALO_RECONEXION_MQTT;
extern const unsigned long INTERVALO_UPTIME;
extern const unsigned long INTERVALO_REINTENTO_WIFI;
extern const unsigned long DURACION_BOCINA_MS;
extern const unsigned long DURACION_TIMBRE_MS;
extern const unsigned long VENTANA_ANTIDUP_MS;
