# PROJECT_CONTEXT

Contexto de alto nivel del repositorio `wifi_PIR` (GitHub: `ydiaz1699/wifi_PIR`).

## Qué es

Red de sensores IoT doméstica para alarma/aviso. Un **emisor PIR + timbre**
detecta movimiento y pulsaciones de timbre y los reporta por **UDP** a una
**central receptora**, que acciona una bocina/LED local y publica los eventos
en **MQTT** hacia un broker (Home Assistant u otro). La comunicación entre
nodos usa una librería binaria propia: **IoTProtocol V4.3.0** (formato wire
V4.1, byte de versión `0x41`).

Palabras clave del dominio: `iot`, `udp`, `tlv`, `crc16`, `hmac-sha256`,
`esp8266`, `esp32`.

## Componentes

| Componente | Ruta | Placa / rol |
|---|---|---|
| Emisor PIR + timbre | `emisor_pir_unificado/` | D1 mini (ESP8266). Lee PIR y timbre, encola eventos, OTA. |
| Receptor central | `receptor_central_unificado/` | NodeMCU v2 (ESP8266). Puente UDP→MQTT, bocina/LED. |
| Librería compartida | `lib/IoTProtocol/` | Core del protocolo: nodo, wire, auth, storage, config. |
| Perfil de dominio | `lib/AlarmProfile/` | Enums de aplicación: `EventCode`, `DeviceType`, `StateTag`. |
| Versión legacy | `legacy/` | Emisor/receptor V3.5.1 con protocolo de texto (producción histórica). |
| Pruebas | `tests/` | Checks host g++ y entorno PlatformIO `native`. |
| Documentación | `docs/` | Arquitectura, operación, roadmap, changelog, auditorías. |
| Informes de agentes | `.agents/` | Análisis profundo y reviews (bugs B1–B15). |

## Diagrama de alto nivel

```
┌──────────────────────┐        UDP (IoTProtocol V4.1)        ┌───────────────────────────┐
│  Emisor PIR+Timbre    │  EVENT / HEARTBEAT / STATE_REPORT    │  Receptor Central         │
│  D1 mini (ESP8266)    │ ───────────────────────────────────▶│  NodeMCU v2 (ESP8266)     │
│  ID 0x02 "PIR Entrada"│ ◀─────────────────────────────────── │  ID 0x01 (CENTRAL)        │
│  192.168.0.200        │  ACK / CONFIG / STATE_REQUEST        │  192.168.0.201            │
└──────────────────────┘        puerto UDP 4210                └────────────┬──────────────┘
                                                                            │ MQTT (PubSubClient)
                                                                            ▼
                                                                   ┌─────────────────┐
                                                                   │  Broker MQTT    │
                                                                   │  (Home Assistant│
                                                                   │   / local)      │
                                                                   └─────────────────┘
```

## Objetivo del repo

- Entrega fiable de eventos de movimiento y timbre en una LAN de confianza.
- Protocolo binario reutilizable y extensible (TLV) para otros sensores.
- Operación tolerante a caídas de WiFi/broker (buffering, reconexión no
  bloqueante, modo LOCAL/HA en la central).

## Estado actual

Rama de trabajo `fix/emisor-pir-criticos`. Un análisis profundo de 3 capas
encontró 15 bugs (B1–B15); el emisor **ahora compila y enlaza** tras los
arreglos. Detalle en `CHANGELOG.md`, `TASKS.md` y `.agents/tasks/`.

Para más detalle ver el resto de `.ai/` (empezar por `.ai/README.md`).
