#include "config.h"

// --- Red ---
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
IPAddress local_IP(192, 168, 0, 201);
IPAddress gateway_IP(192, 168, 0, 1);
IPAddress subnet_mask(255, 255, 255, 0);

// --- MQTT ---
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_client_id = "central_iot";
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASSWORD;

// --- MQTT Topics ---
const char* TOPIC_ESTADO       = "casa/iot/central/estado";
const char* TOPIC_UPTIME       = "casa/iot/central/uptime";
const char* TOPIC_IP           = "casa/iot/central/ip";
const char* TOPIC_MODO         = "casa/iot/alarma/modo/set";
const char* TOPIC_MODO_STATE   = "casa/iot/alarma/modo/state";
const char* TOPIC_BOCINA_CMD   = "casa/iot/alarma/bocina/set";
const char* TOPIC_BOCINA_STATE = "casa/iot/alarma/bocina/state";

// --- Hardware ---
const int pinBocina = D5;
const int pinLed = D6;

// --- IoTProtocol ---
const uint16_t IOT_UDP_PORT = 4210;
const uint8_t MY_DEVICE_ID = IOT_DEVICE_CENTRAL;

// --- Timings ---
const unsigned long DURACION_BOCINA_MOTION_MS = 1000;
const unsigned long DURACION_BOCINA_TIMBRE_MS = 500;
const unsigned long DURACION_BOCINA_PUERTA_MS = 2000;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 15000;
const unsigned long MQTT_SONDEO_INTERVAL_MS = 300000;
const unsigned long HEARTBEAT_TIMEOUT_MS = 180000;
