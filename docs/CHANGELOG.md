# Changelog

## V3.1 (Versión original)
- Protocolo texto: `PIR01|eventId|MOTION` / `OK|eventId`
- Emisor: máquina de estados TX bloqueante (IDLE→SENDING→WAIT_ACK→DONE)
- Receptor: MQTT obligatorio, sin modo local
- 3 reintentos, 300ms timeout ACK
- Problema: TIMBRE se perdía cuando MQTT bloqueaba el receptor

## V3.2
- Receptor: verificación TCP antes de mqtt.connect() (intentó evitar bloqueo)
- Emisor: timeout ACK 300→500ms, reintentos 3→5
- Resultado: TCP verify en ESP8266 bloqueaba 5 segundos igualmente

## V3.2b
- Receptor: eliminada verificarTCPBroker() (bloqueaba 5s)
- Backoff progresivo en reconnect MQTT: 10s primer intento, 30s si falla 2+
- Mejora parcial pero insuficiente

## V3.3 — Modo LOCAL/HA
- **Cambio fundamental**: modo dual de operación
- MODO_LOCAL: cero intentos MQTT en el loop (loop ultra-rápido)
- MODO_HA: MQTT activo normalmente
- Auto-detección: al boot prueba MQTT una vez → decide modo
- Sondeo cada 5 min: si broker vuelve → cambia a HA automáticamente
- Si broker muere: 3 fallos → vuelve a LOCAL
- Nunca intenta MQTT mientras bocina suena
- **TIMBRE ahora funciona 100%** en modo LOCAL

## V3.3.1
- Fix: antirebote PIR 2000ms → 500ms
- Fix: buffer de evento pendiente (si TX ocupado, no se descarta)
- Problema: PIR y timbre seguían bloqueándose mutuamente

## V3.4 — Fire-and-forget
- Rediseño: eliminada máquina de estados TX completamente
- Envío inmediato × 3 (redundancia)
- Sin ACK, sin espera, sin bloqueo
- PIR y timbre 100% independientes
- Problema: sin confirmación de recepción

## V3.5 — ACK asíncrono no-bloqueante
- **Mejor de ambos mundos**: envío instantáneo + confirmación
- Cola de 4 eventos "en vuelo" simultáneos
- ACK se verifica en background (sin bloquear detección)
- Reenvío automático si no hay ACK en 500ms (máx 3 intentos)
- PIR y timbre completamente independientes
- Latencia <1ms

## V3.5.1 — Receptor actualizado
- Receptor: drain loop (procesa hasta 8 paquetes por ciclo)
- Receptor: dedup window circular de 8 (reemplaza lastEventId simple)
- Receptor: ACK se envía SIEMPRE (incluso para duplicados)
- Emisor: antirebote PIR 500ms → 200ms
- Emisor: detección PIR por flanco con log de liberación
- Sistema completo emisor+receptor sincronizado

---

## V4.0 — IoTProtocol (biblioteca binaria)
- Protocolo binario: cabecera 11B + TLV payload + CRC16
- IoTNode: cola de 8, backoff, ACK automático, heartbeat, discovery
- IDs numéricos (1 byte), SEQ 16-bit
- TLV extensible para cualquier tipo de sensor
- Emisor y receptor V4 como proyectos separados

## V4.1 — Estabilización del protocolo
- SEQ ampliado a 32 bits
- BOOT_ID 16-bit en cabecera (resuelve reinicio + SEQ=1)
- Validación estricta: len == expected, version major check
- TLV estricto: tamaño exacto para tipos fijos
- PAY_LEN reducido a 1 byte (max 64)
- Prioridades: URGENT > NORMAL > BACKGROUND
- Un solo canal reliable en vuelo
- IoTPacket.priority() derivada de flags

## V4.1.1 — Hardening
- Dedup Window: ventana circular de 8 SEQ por remoto
- ACK siempre se envía (incluso para duplicados)
- COMMAND_ID + ResultCode para futuros comandos/respuestas
- Device Registry ampliado: type, name, fwVersion desde HELLO
- ONLINE/STALE/OFFLINE state machine (90s/180s thresholds)
- Central publica status cada 30s en MQTT

## V4.2 — STATE + Stats + RTT
- STATE_REPORT / STATE_REQUEST para sincronización de estado
- Central envía STATE_REQUEST broadcast al boot (STATE_SYNC)
- IoTStats: 10 contadores (tx, rx, ack, timeouts, retries, duplicates, drops)
- IoTRtt: min/max/avg (EMA 0.75/0.25) medido en cada ACK
- Heartbeat enriquecido: uptime, RSSI, freeHeap, queueDepth, txCount, ackTimeouts
- Jitter ±5s en heartbeat para distribuir tráfico
- BootReason enum para diagnóstico de reinicios

## V4.3 — HMAC + Persistencia + Config remota
- IoTAuth: HMAC-SHA256 truncado 4 bytes (BearSSL, built-in ESP8266)
- IoTStorage: LittleFS para boot counter, config, auth key
- IoTConfigHandler: config remota sin recompilar (heartbeat, antirebote, nombre, auth)
- AUTH_KEY movida a secrets.h (no versionado)
- Broadcast con WiFi.broadcastIP() (respeta subnet)
- Auth logic unificada: un solo punto de decisión (verifyPacket)
