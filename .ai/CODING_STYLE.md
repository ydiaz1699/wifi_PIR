# CODING_STYLE

Estilo **observado en el código real** del repo. Reglas derivadas de leer las
fuentes, no plantillas genéricas. Al escribir código nuevo, imitar lo existente.

## Idioma

- **Español** para la lógica de aplicación: nombres de funciones y variables
  (`manejarWiFi`, `iniciarWiFi`, `enviarStateRequestSiCorresponde`,
  `actualizacionEnProgreso`, `pirAnterior`, `ultimaDeteccionTimbre`), comentarios
  y mensajes de log.
- **Inglés** en el core reutilizable para la API pública y los símbolos del
  contrato wire (`IoTNode`, `sendEvent`, `BootReason`, `MsgType`, `TlvTag`). Los
  identificadores wire no se traducen.

## Headers

- `#pragma once` en todos los headers (no include guards con `#ifndef`).
- Config por módulo con `extern` en el `.h` y definición en el `.cpp`
  (p. ej. `extern const int PIN_PIR;` en `device_config.h`,
  `const int PIN_PIR = D2;` en `device_config.cpp`).
- Identidad y constantes de compilación con `#define` en mayúsculas
  (`MY_DEVICE_ID`, `EMISOR_OTA_HOSTNAME`, `IOT_QUEUE_SIZE`).

## Logging

- Macros del logger local (`include/logger.h`): `LOG_DEBUG/INFO/WARN/ERROR`,
  envoltura de `Serial.printf` con prefijo `[D]/[I]/[W]/[E]` y `\n` final.
- Mensajes concisos en español; reportar la verdad (p. ej. `LOG_WARN` cuando un
  evento NO se encoló, en vez de afirmar siempre "encolado").

## Convenciones C++

- `enum class` tipado (`uint8_t`) para tipos del protocolo y dominio.
- `static` para funciones y estado con alcance de archivo; `namespace { ... }`
  anónimo en los `.cpp` para estado interno (ver `ota.cpp`).
- `constexpr` para conversiones puras (`AlarmProfile::toWire`).
- `static_cast` explícito para conversiones de enum ↔ entero.
- Comparaciones de tiempo seguras ante rollover de `millis()` usando resta con
  `static_cast<long>(...)` (patrón en receptor y `retryStorageIfNeeded`).
- Comentarios que explican el **porqué** y los casos borde (abundan en el repo),
  no el qué obvio.

## Patrones del dominio

- WiFi/OTA/MQTT como máquinas de estado **no bloqueantes** en `loop()`; evitar
  `delay()` largos (salvo reinicio terminal).
- Watchdog: `ESP.wdtEnable(8000)` y `ESP.wdtFeed()` al inicio del `loop()`.
- Sincronizar estado lógico de sensores con el nivel físico en `setup()` para no
  emitir flancos falsos al arrancar.
- Numeración de versión de firmware de aplicación en `FW_VERSION` (string
  `"4.3.0"`), separada de la versión del core.
