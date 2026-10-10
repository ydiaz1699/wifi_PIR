#include "device_config.h"
#include "network_config.h"

// --- Red ---
IPAddress dispositivo_IP(192, 168, 0, 200);
IPAddress central_IP(192, 168, 0, 201);
// Puerto UDP del emisor unificado. Fuente única de verdad en network_config.h
// (IOT_UDP_PORT_DEFAULT = 4210), compartida con PUERTO_UDP de legacy.
const uint16_t UDP_PORT = IOT_UDP_PORT_DEFAULT;

// --- Pines ---
const int PIN_PIR = D2;
const int PIN_TIMBRE = D3;   // INPUT_PULLUP, activo en LOW

// --- Timings ---
const unsigned long ANTIREBOTE_PIR_MS = 200;
const unsigned long ANTIREBOTE_TIMBRE_MS = 800;
const unsigned long HEARTBEAT_INTERVAL_MS = 60000;  // 60s
