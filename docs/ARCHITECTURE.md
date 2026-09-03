# Arquitectura del Sistema wifi_PIR

> **Actualización canónica 2026-09-02:** el árbol actual usa `emisor_pir_unificado/` y `receptor_central_unificado/`; la línea V3 se conserva en `legacy/emisor_pir/` y `legacy/receptor_bocina/`. Las rutas unificadas son las activas de desarrollo y no deben sustituirse por los nombres históricos. El estado de red/IP documentado aquí es una descripción histórica y debe verificarse en la red real. La auditoría de drafts está en [`DRAFTS_AUDIT.md`](DRAFTS_AUDIT.md).
>
> ## Resumen

Sistema de alarma doméstica IoT basado en ESP8266 que comunica sensores (PIR, timbre, puertas) con un receptor central mediante UDP sobre WiFi LAN. El receptor activa una bocina local y opcionalmente publica eventos en Home Assistant vía MQTT.

## Diagrama General

```
     ┌── PIR01 (0x02) ──── D1 Mini, IP 192.168.0.200
     ├── PIR02 (0x03) ──── (futuro)
     ├── TIMBRE01 (0x20) ── (futuro, ahora integrado en PIR01)
     ├── PUERTA01 (0x40) ── (futuro)
     └── TEMP01 (0x60) ──── (futuro)
           │
           │  WiFi LAN / UDP puerto 4210
           │  Protocolo: IoTProtocol V4.3 (binario, TLV, CRC16)
           ▼
  ┌────────────────────────┐
  │   RECEPTOR CENTRAL     │
  │   NodeMCU v2           │
  │   IP 192.168.0.201     │
  │   ID 0x01              │
  └────────────┬───────────┘
               │
       ┌───────┴───────┐
       ▼               ▼
    BOCINA          MQTT (opcional)
    (D5 + LED D6)      │
                       ▼
                 Home Assistant
```

## Dos Versiones Coexistentes

### V3.5.1 (Producción — flasheada actualmente)

Protocolo **texto** simple (`PIR01|5|TIMBRE`), probado y funcionando.

- **Emisor** (`legacy/emisor_pir/`): fire-and-forget con ACK asíncrono no-bloqueante
- **Receptor** (`legacy/receptor_bocina/`): drain loop, dedup window de 8, modo LOCAL/HA

### V4.3 (Desarrollo — biblioteca IoTProtocol)

Protocolo **binario** universal con biblioteca reutilizable.

- **Biblioteca** (`lib/IoTProtocol/`): protocolo, nodo, auth, storage, config
- **Emisor** (`emisor_pir_unificado/`): usa IoTNode con cola, heartbeat, discovery
- **Receptor** (`receptor_central_unificado/`): genérico, ONLINE/STALE/OFFLINE, MQTT auto

---

## V3.5.1 — Versión de Producción

### Emisor (`legacy/emisor_pir/`)

**Hardware**: ESP8266 D1 Mini, PIR en D2, botón timbre en D3 (INPUT_PULLUP)

**Diseño: ACK asíncrono no-bloqueante**

```
Detección (PIR o timbre)
    │
    ├── enviarUDP() → INMEDIATO (no espera nada)
    ├── Registrar en cola de vuelo (4 slots)
    └── Loop sigue sin bloquear

    ... en background (cada iteración del loop) ...

    verificarACKs()
        ├── Lee paquetes UDP disponibles
        ├── Si "OK|eventId" → marcar confirmado
        └── No bloquea si no hay nada

    reenviarPendientes()
        ├── Si algún evento sin ACK por >500ms → reenviar
        ├── Máximo 3 reintentos
        └── No bloquea otros eventos
```

**Protocolo de comunicación (wire format V3.x)**:
```
Emisor → Receptor:  "PIR01|5|TIMBRE"    (deviceId|eventId|tipo)
Receptor → Emisor:  "OK|5"              (ACK|eventId)
```

**Archivos clave**:
- `src/main.cpp` — toda la lógica del emisor
- `include/device_config.h` — IP del dispositivo y del receptor
- `../secrets.h` — WiFi credentials (no versionado)
- `../network_config.h` — gateway, subnet, puerto UDP

### Receptor (`legacy/receptor_bocina/`)

**Hardware**: NodeMCU v2, buzzer en D5, LED en D6

**Diseño: Drain Loop + Modo Dual LOCAL/HA**

```
loop():
  1. manejarWiFi()          — reconexión non-blocking
  2. buzzer.loop()          — timer auto-off
  3. handleOTA()            — ArduinoOTA
  4. manejarAlarma()        — PRIORIDAD #1: procesa TODOS los UDP
  5. manejarMQTT()          — solo si WiFi OK, modo LOCAL no bloquea
```

**Modo LOCAL vs HA (MQTT)**:
```
Boot:
  ├── Intenta MQTT una vez (~5s bloqueo máximo)
  │   ├── Conectó → MODO_HA (publica en Home Assistant)
  │   └── No conectó → MODO_LOCAL (cero MQTT en loop)
  │
MODO_LOCAL:
  ├── Loop ultra-rápido (solo UDP + bocina)
  ├── Sondeo broker cada 5 minutos (solo si bocina apagada)
  └── Si broker vuelve → cambia a MODO_HA automáticamente
  
MODO_HA:
  ├── MQTT activo, publica eventos
  ├── Si broker muere (3 fallos) → vuelve a MODO_LOCAL
  └── Nunca intenta reconectar MQTT mientras bocina suena
```

**Archivos clave**:
- `src/main.cpp` — loop principal con prioridades
- `src/alarma.cpp` — recepción UDP, dedup, ACK, activación bocina
- `src/mqtt_cliente.cpp` — modo LOCAL/HA, sondeo, reconexión
- `src/hal.cpp` — abstracción Led/Buzzer con timedOn()
- `src/state_machine.cpp` — estados BOOT/CONNECT/READY/ERROR/RECOVER

---

## V4.3 — Versión de Desarrollo (Biblioteca IoTProtocol)

### Formato del paquete binario

```
┌───────┬─────┬──────┬─────┬─────┬────────┬──────┬───────┬─────────┬─────────┬───────┐
│ MAGIC │ VER │ TYPE │ SRC │ DST │BOOT_ID │ SEQ  │ FLAGS │ PAY_LEN │ PAYLOAD │ CRC16 │
│ A5 5A │ 41  │ 1B   │ 1B  │ 1B  │ 2B     │ 4B   │ 1B    │ 1B      │ 0-64B   │ 2B    │
└───────┴─────┴──────┴─────┴─────┴────────┴──────┴───────┴─────────┴─────────┴───────┘

Header: 14 bytes fijos
CRC: 2 bytes (CRC16-CCITT sobre todo menos CRC)
Total mínimo: 16 bytes (sin payload)
Total máximo: 80 bytes (64 bytes payload)
```

### TLV (Type-Length-Value) dentro del payload

```
┌──────┬────────┬───────┐
│ TAG  │ LENGTH │ VALUE │
│ 1B   │ 1B     │ N B   │
└──────┴────────┴───────┘

Validación estricta: uint8=1B exacto, uint16=2B exacto, uint32=4B exacto
```

### Tipos de mensaje

| Tipo | Código | Dirección | Uso |
|------|--------|-----------|-----|
| EVENT | 0x01 | Sensor → Central | PIR, timbre, puerta, humo |
| DATA | 0x02 | Sensor → Central | Temperatura, humedad |
| COMMAND | 0x03 | Central → Actuador | Relé ON/OFF |
| RESPONSE | 0x04 | Actuador → Central | Resultado ejecución |
| ACK | 0x10 | Cualquiera | Confirmación de recepción (NO ejecución) |
| HEARTBEAT | 0x11 | Sensor → Central | Estoy vivo + telemetría |
| STATE_REPORT | 0x13 | Sensor → Central | Estado actual |
| STATE_REQUEST | 0x14 | Central → Todos | Pedir re-publicación |
| HELLO | 0x20 | Sensor → Central | Discovery al boot |
| CONFIG | 0x30 | Central → Sensor | Configuración remota |
| ERROR | 0xE0 | Cualquiera | Error con código |

### Módulos de la biblioteca

| Módulo | Responsabilidad |
|--------|-----------------|
| `IoTProtocol.h/.cpp` | Packet struct, TLV read/write, CRC16, serialize/deserialize |
| `IoTNode.h/.cpp` | UDP, cola FIFO con prioridades, reliable channel, ACK, dedup, heartbeat |
| `IoTAuth.h/.cpp` | HMAC-SHA256 truncado 4 bytes (BearSSL), sign/verify |
| `IoTStorage.h/.cpp` | LittleFS: boot counter, config persistente, auth key |
| `IoTConfigHandler.h/.cpp` | Procesa CONFIG remotos, aplica y persiste cambios |

### IoTNode — Diseño interno

```
loop():
  1. _processIncoming()    — Lee UDP, verifica CRC/version, dedup, ACK, despacha
  2. _processReliable()    — Si hay paquete en vuelo: timeout → reenviar con backoff
  3. _processQueue()       — Si reliable libre: sacar el de mayor prioridad, enviar
  4. _sendHeartbeat()      — Si pasó intervalo+jitter: enviar HB con telemetría
  5. _updateDeviceStates() — Cada 10s: actualizar ONLINE/STALE/OFFLINE
```

**Cola FIFO con prioridades**:
- URGENT (smoke, flood) — nunca se descarta
- NORMAL (motion, timbre) — puede desplazar BACKGROUND
- BACKGROUND (heartbeat, temperatura) — descartable si cola llena

**Reliable Channel**: 1 solo paquete en vuelo a la vez. Backoff exponencial: 300, 450, 675, 1012, 1518ms (cap 2000ms). Máximo 5 intentos.

**Dedup Window**: ventana circular de 8 últimos SEQ por emisor. Si BOOT_ID cambia → resetear ventana (el emisor reinició).

### MQTT Topics (generados automáticamente)

```
casa/iot/device_02/evento        ← PIR01 events
casa/iot/device_02/status        ← online/stale/offline
casa/iot/device_02/uptime        ← segundos desde boot
casa/iot/device_02/rssi          ← señal WiFi (dBm)
casa/iot/device_02/heap          ← RAM libre
casa/iot/device_02/state/motion  ← active/idle
casa/iot/device_02/state/button  ← pressed/released
casa/iot/central/estado          ← online/offline
casa/iot/alarma/modo/set         ← armado/desarmado
casa/iot/alarma/bocina/set       ← ON/OFF manual
```

---

## Estructura de archivos

```
wifi_PIR/
├── docs/
│   ├── ARCHITECTURE.md      ← Este archivo
│   ├── CHANGELOG.md         ← Historial de versiones
│   ├── BUGS_FIXED.md        ← Bugs resueltos (para no repetir)
│   └── ROADMAP.md           ← Mejoras futuras con instrucciones
│
├── secrets.h.template       ← Copiar a secrets.h (WiFi + MQTT + AUTH_KEY)
├── network_config.h         ← Gateway, subnet, puerto UDP compartido
├── .gitignore               ← secrets.h excluido
│
├── lib/IoTProtocol/         ← BIBLIOTECA REUTILIZABLE (V4.3)
│   ├── IoTProtocol.h/.cpp   ← Protocolo binario
│   ├── IoTNode.h/.cpp       ← Nodo de red
│   ├── IoTAuth.h/.cpp       ← HMAC autenticación
│   ├── IoTStorage.h/.cpp    ← Persistencia LittleFS
│   ├── IoTConfigHandler.h/.cpp ← Config remota
│   └── library.json
│
├── legacy/emisor_pir/              ← EMISOR V3.5.1 (PRODUCCIÓN)
│   ├── platformio.ini
│   ├── include/device_config.h
│   ├── include/logger.h
│   └── src/
│       ├── device_config.cpp
│       └── main.cpp
│
├── legacy/receptor_bocina/         ← RECEPTOR V3.5.1 (PRODUCCIÓN)
│   ├── platformio.ini
│   ├── include/ (alarma, config, hal, logger, mqtt_cliente,
│   │            mqtt_discovery, ota, red_wifi, state_machine)
│   └── src/ (alarma, config, hal, main, mqtt_cliente,
│             mqtt_discovery, ota, red_wifi, state_machine)
│
├── emisor_pir_unificado/           ← EMISOR V4.3 (DESARROLLO)
│   ├── platformio.ini
│   ├── include/device_config.h, logger.h
│   └── src/device_config.cpp, main.cpp
│
└── receptor_central_unificado/     ← RECEPTOR V4.3 (DESARROLLO)
    ├── platformio.ini
    ├── include/ (config, hal, logger, event_handler, mqtt_manager)
    └── src/ (config, hal, main, event_handler, mqtt_manager)
```

---

## Hardware

### Emisor (D1 Mini)
| Pin | Función | Notas |
|-----|---------|-------|
| D2 | PIR sensor | INPUT, activo HIGH |
| D3 | Botón timbre | INPUT_PULLUP, activo LOW |

### Receptor (NodeMCU v2)
| Pin | Función | Notas |
|-----|---------|-------|
| D5 | Buzzer/bocina | OUTPUT, activo HIGH |
| D6 | LED indicador | OUTPUT, sincronizado con buzzer |

### Red
| Dispositivo | IP | Puerto |
|-------------|-------|--------|
| Emisor PIR01 | 192.168.0.200 | UDP 4210 |
| Receptor Central | 192.168.0.201 | UDP 4210 |
| Router/Gateway | 192.168.0.1 | — |
| MQTT Broker | 192.168.0.50 | TCP 1883 |

---

## Principios de diseño

1. **UDP siempre tiene prioridad sobre MQTT** — la alarma local funciona sin internet
2. **Nunca bloquear el loop** — ninguna operación puede freezar la recepción UDP
3. **Modo LOCAL resiliente** — si MQTT muere, el sistema sigue funcionando 100%
4. **Sensores independientes** — PIR y timbre no se bloquean mutuamente
5. **Receptor genérico** — no hardcodea sensores, procesa cualquier evento por tipo
6. **Biblioteca reutilizable** — IoTProtocol no sabe de PIR ni alarmas
