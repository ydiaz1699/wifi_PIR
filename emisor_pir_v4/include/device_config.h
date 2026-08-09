/**
 * Configuración específica de este emisor.
 *
 * Para crear un nuevo emisor (PIR02, TIMBRE01, PUERTA01, etc.):
 * 1. Cambiar MY_DEVICE_ID a un ID único (ver rangos en IoTProtocol.h)
 * 2. Cambiar MY_DEVICE_TYPE según corresponda
 * 3. Cambiar MY_DEVICE_NAME para identificación
 * 4. Ajustar IP estática (dispositivo_IP)
 * 5. Configurar pines según hardware
 */

#pragma once
#include <ESP8266WiFi.h>
#include <IoTProtocol.h>

// --- Identidad del dispositivo ---
#define MY_DEVICE_ID    0x02              // ID único en la red (PIR01 = 0x02)
#define MY_DEVICE_TYPE  DeviceType::PIR_SENSOR
#define MY_DEVICE_NAME  "PIR Entrada"

// --- Red ---
extern IPAddress dispositivo_IP;
extern IPAddress central_IP;
extern const uint16_t UDP_PORT;

// --- Pines ---
extern const int PIN_PIR;
extern const int PIN_TIMBRE;

// --- Timings ---
extern const unsigned long ANTIREBOTE_PIR_MS;
extern const unsigned long ANTIREBOTE_TIMBRE_MS;
extern const unsigned long HEARTBEAT_INTERVAL_MS;
