# TASKS — Pendientes reales

Derivadas de los informes en `.agents/` y de `docs/ROADMAP.md`. Los bugs
críticos/altos B1–B15 ya se arreglaron en la rama `fix/emisor-pir-criticos`
(ver `CHANGELOG.md`); aquí quedan los **no bloqueantes** y el roadmap.

## Hallazgos no bloqueantes pendientes

Fuente: `.agents/tasks/analisis-profundo-emisor-pir.md` (bugs POSIBLE/PROBABLE
que no se consideraron críticos).

- **OTA de un disparo (B2/B3, residual).** La reinicialización de OTA depende de
  la transición a `WL_CONNECTED`; `setupOTA()` es idempotente. Verificar en
  campo que tras reconexiones repetidas OTA sigue disponible.
- **BOOT_ID volátil probabilístico (B11).** Si LittleFS falla,
  `isBootIdPersistent() == false` y un reinicio con BOOT_ID repetido puede hacer
  que la central descarte los primeros eventos como replay. Mitigación aplicada:
  más entropía en el ID volátil; falta un "resync" explícito o garantizar
  LittleFS formateado en primera instalación.
- **B13 — superficie de bootstrap-ACK con auth OFF.** Mantener auth habilitado
  en producción; documentado como aceptable en LAN de confianza.
- **B14 — PIR/timbre por polling.** Evaluar `attachInterrupt` si se requieren
  pulsos de timbre muy cortos y fiables; por ahora, mantener el `loop()` corto.

## Roadmap activo

Fuente: `docs/ROADMAP.md` y `docs/PLAN_EJECUCION_FUTURA.md`.

- **V4.4 — Replay protection con timestamp** (prioridad alta): TLV `UPTIME_SEC`
  comparado contra el último uptime conocido por remoto (con tolerancia de
  jitter), además de la ventana de dedup actual.
- **V4.4 — Tests unitarios de `IoTProtocol`**: ampliar el entorno `native` de
  `tests/` (ver `TESTING.md`).
- **Evaluación tecnológica V5**: pendiente; no modifica V4.

## Mantenimiento

- Instalar g++ en la máquina de CI/desarrollo para poder correr los checks host
  (hoy no está disponible; ver `TESTING.md`).
- Esta rama (`fix/emisor-pir-criticos`) está sin commitear según el último
  informe de verificación; confirmar el estado antes de hacer merge.

> Antes de dar por cerrada una tarea de bugfix, revalidar con `pio run` de ambos
> entornos y, si hay hardware, los tests incrementales de `TESTING.md`.
