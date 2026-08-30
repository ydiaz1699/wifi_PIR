#pragma once
#include <Arduino.h>

// ============================================================================
// CONFIGURACIÓN DE COMPILACIÓN
// ============================================================================
// Descomenta para habilitar logging detallado (aumenta tamaño de binario)
// #define DEBUG_LOG

#ifdef DEBUG_LOG
    #define LOG(x)    Serial.print(x)
    #define LOGLN(x)  Serial.println(x)
    #define LOGF(...) Serial.printf(__VA_ARGS__)
#else
    #define LOG(x)    ((void)0)
    #define LOGLN(x)  ((void)0)
    #define LOGF(...) ((void)0)
#endif
