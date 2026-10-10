# CHANGELOG — Resumen para asistentes

Historial completo en [`docs/CHANGELOG.md`](../docs/CHANGELOG.md). Aquí solo lo
reciente y relevante para trabajar hoy.

## Rama actual: `fix/emisor-pir-criticos`

Arreglos de los 15 bugs (B1–B15) encontrados por el análisis profundo de 3
capas (`.agents/tasks/analisis-profundo-emisor-pir.md`), aplicados según
`.agents/tasks/plan-fix-emisor.md` y verificados en
`.agents/tasks/verificacion-fix-emisor.md`. **Sin commit/push** según el último
informe de verificación: cambios en el working tree.

| Bug | Arreglo |
|---|---|
| B1/B2/B4 | `manejarWiFi()` implementado como máquina de estados no bloqueante con backoff ~5 s; llama a `setupOTA()` al pasar a `WL_CONNECTED`; eliminada la espera bloqueante de hasta 10 s en `setup()`. |
| B3 | `onEnd` de OTA baja `actualizacionEnProgreso` antes del log, evitando loop congelado. |
| B5 | Jitter de heartbeat aplicado al umbral (`_hbJitter`), sin sesgo de período ni underflow; `_lastHb = ahora`. |
| B6 | Clave HMAC rotada en el `secrets.h` real; el `.template` conserva su placeholder. |
| B7/B8 | `sendEvent()` propaga el fallo de `enqueue`, no consume SEQ si falla, y marca EVENT como `URGENT` (no descartable); `main.cpp` loguea evento perdido. |
| B9 | `loadConfig()` revalida rangos (antirebote 100–10000, heartbeat 5000–600000) tras el checksum. |
| B10 | `handleConfig()` responde `INVALID_VALUE` si llegó config pero nada fue válido por rango. |
| B11 | Más entropía en el BOOT_ID volátil; documentado el modo degradado. |
| B12 | `detectBootReason()` usa `ESP.getResetInfoPtr()->reason` como fuente primaria, texto solo como fallback. |
| B15 | Versión corregida a V4.3 en `device_config.h`; puerto UDP consolidado en `IOT_UDP_PORT_DEFAULT`. |
| B-GIT | `network_config.h` destrackeado; `network_config.h.template` versionado; `secrets.h` sigue sin versionarse. |

Verificación: `pio run -e emisor_unificado` y `-e receptor_central_unificado`
compilan y enlazan (PlatformIO 6.1.18). Los checks host no se corrieron por
falta de `g++`.

## Evolución del protocolo (resumen)

De `docs/CHANGELOG.md`: V3.x usaba protocolo de texto (`PIR01|eventId|MOTION`)
con historia de bloqueos MQTT; V4.0 introdujo el protocolo binario con
`IoTNode`; V4.1 estabilizó el wire (SEQ 32 bits, BOOT_ID, prioridades, 1
reliable en vuelo); V4.2 añadió STATE/stats/RTT y jitter de heartbeat; V4.3
añadió HMAC, persistencia LittleFS y config remota.
