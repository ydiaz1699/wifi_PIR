#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
IPAddress local_IP(192, 168, 0, 201);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_client_id = "bocina_esp";
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASSWORD;

const char* TOPIC_EVENTO        = "casa/alarma/evento";
const char* TOPIC_TIMBRE        = "casa/alarma/timbre";
const char* TOPIC_ESTADO        = "casa/alarma/estado";
const char* TOPIC_COMANDO       = "casa/alarma/bocina/set";
const char* TOPIC_COMANDO_STATE = "casa/alarma/bocina/state";
const char* TOPIC_MODO          = "casa/alarma/modo/set";
const char* TOPIC_MODO_STATE    = "casa/alarma/modo/state";
const char* TOPIC_UPTIME        = "casa/alarma/uptime";
const char* TOPIC_IP            = "casa/alarma/ip";

const int pinBocina = D5;
const int pinLed = D6;

const unsigned int puertoUDP = 4210;

const unsigned long INTERVALO_RECONEXION_MQTT = 15000;   // 15s entre reconexiones en modo INTELIGENTE
const unsigned long INTERVALO_UPTIME = 60000;            // Publica uptime cada 60s
const unsigned long INTERVALO_REINTENTO_WIFI = 5000;     // 5s entre reintentos WiFi
const unsigned long DURACION_BOCINA_MS = 1000;           // Alarma motion: 1 segundo
const unsigned long DURACION_TIMBRE_MS = 500;            // Timbre: 500ms
const unsigned long VENTANA_ANTIDUP_MS = 1500;           // Anti-duplicado UDP
