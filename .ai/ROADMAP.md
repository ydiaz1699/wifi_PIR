# ROADMAP

Resumen del rumbo del proyecto. Detalle en
[`docs/ROADMAP.md`](../docs/ROADMAP.md) y en el plan extenso
[`docs/PLAN_EJECUCION_FUTURA.md`](../docs/PLAN_EJECUCION_FUTURA.md) — no
duplicar, referenciar.

## Estado de versiones

- **V3.5.1** — producción histórica, protocolo de texto (`legacy/`).
- **V4.3** — desarrollo actual, protocolo binario `IoTProtocol` con lib
  reutilizable (`emisor_pir_unificado/`, `receptor_central_unificado/`,
  `lib/`). Rama de trabajo `fix/emisor-pir-criticos`.

## Rutas activas

Según la actualización canónica de `docs/ROADMAP.md`: los diez drafts fueron
auditados y retirados (ver `docs/DRAFTS_AUDIT.md`). Las rutas vivas son
`emisor_pir_unificado/`, `receptor_central_unificado/` y `legacy/`. El
vocabulario de dominio nuevo va en `lib/AlarmProfile/AlarmProfile.h`;
`IoTProtocol.h` conserva solo el core.

## V4.4 — Seguridad y testing (prioridad alta)

1. **Replay protection con timestamp**: TLV `UPTIME_SEC` en heartbeat/eventos; la
   central rechaza un paquete cuyo uptime sea menor que el último conocido del
   remoto sin cambio de BOOT_ID (tolerancia ~5 s por jitter). Complementa la
   ventana de dedup. Archivos: `lib/IoTProtocol/IoTNode.{h,cpp}` (campo
   `lastUptime` en `RemoteDevice`).
2. **Tests unitarios de `IoTProtocol`**: ampliar el entorno `native` de `tests/`.

## V5 — pendiente

Evaluación tecnológica V5 abierta; no modifica V4. Sin planificación cerrada.

## Reglas críticas para evolucionar (de `docs/ROADMAP.md`)

1. No bloquear el loop del receptor > ~100 ms.
2. Sensores del emisor 100 % independientes.
3. No usar `LOCAL` como identificador (macro del SDK).
4. `WiFiClient::connect()` bloquea ~5 s: no usarlo como test rápido.
5. Credenciales en `secrets.h` (no versionado), nunca en `.cpp`.
6. El PIR se mantiene en HIGH por segundos: no asumir flancos limpios.
7. UDP tiene prioridad absoluta sobre MQTT en el loop del receptor.
8. La deduplicación necesita BOOT_ID además de SEQ.
