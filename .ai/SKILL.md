# SKILL — Análisis Profundo de Código

Referencia al skill usado para auditar este repo:
`github.com/ydiaz1699/analisis_de_codigo`.

## Cuándo aplicarlo

- Bugs de **timing** (antirrebote, heartbeat, backoff, watchdog) donde el
  comportamiento depende de fórmulas y casos borde (overflow de `unsigned`,
  rollover de `millis()`).
- Bugs de **protocolo y estado oculto**: cola/reliable, dedup/replay, ACK,
  firma HMAC, orden de verificación, pérdida silenciosa de eventos.
- Cuando el nombre de una función no coincide con lo que hace (p. ej.
  `sendEvent()` **encola**, no envía; `sendDirect()` no hace nada sin WiFi).
- Antes de dar por bueno código embebido que "parece" correcto en lectura
  superficial.

## Las 3 capas

1. **Capa 1 — Estático.** Mapa de componentes, flujo de datos, símbolos
   indefinidos/engañosos. (Aquí se detectó B1: `manejarWiFi()` llamada pero no
   definida → no enlazaba.)
2. **Capa 2 — Dinámico (lectura del source de la lib).** Qué hace *realmente*
   cada función usada, estado mutable oculto, cálculo de **timings reales** con
   fórmulas, interacción cola↔reliable↔WiFi, orden de la firma/verificación.
3. **Capa 3 — Tests incrementales (recomendados).** Tests que **aíslan una
   variable** cada uno (p. ej. B5 jitter de heartbeat, B7 saturación de cola,
   B9 rangos en `loadConfig`, B3 OTA `onEnd`). No ejecutados por no haber
   hardware/g++; descritos para ejecutarse después.

## Salida esperada

Un informe de solo lectura con: resumen ejecutivo, tabla consolidada de bugs
(severidad + confianza CONFIRMADO/PROBABLE/POSIBLE), cadenas causales, timings
con fórmulas, y fix propuesto por bug sin aplicarlo. Ejemplo real en
`.agents/tasks/analisis-profundo-emisor-pir.md`.

> Nota de cumplimiento: en el informe no se transcriben valores de credenciales
> (p. ej. la comparación de la clave HMAC se hizo sin volcar su valor).
