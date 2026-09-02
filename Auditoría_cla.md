Con esto ya tengo el código completo de `emisor_pir_unificado`, `lib/IoTProtocol` y `receptor_central_unificado`. Aquí va la auditoría (Fase 1 del plan).

## Auditoría — genérico vs. específico de alarma

### `lib/IoTProtocol` (la librería en sí)

| Archivo | Elemento | Clasificación | Nota |
|---|---|---|---|
| `IoTProtocol.h/cpp` | Framing (magic, header, CRC16, TLV read/write, versión) | **Genérico** | Sólido. No conoce PIR ni alarma. |
| `IoTProtocol.h` | `enum MsgType` (EVENT, DATA, COMMAND, RESPONSE, ACK, HEARTBEAT, HELLO, CONFIG...) | **Genérico** | Correcto: son tipos de protocolo, no de aplicación. |
| `IoTProtocol.h` | `enum TlvTag` — bloques 0x01–0x9F, 0xE0–0xEF | **Genérico** (mayoría) | Tags de telemetría/diagnóstico/config están bien ubicados en el core. |
| `IoTProtocol.h` | `enum TlvTag::STATE_MOTION, STATE_DOOR, STATE_ALARM, STATE_SMOKE, STATE_FLOOD` (0xA0–0xA6) | **⚠️ Contaminado** | Son conceptos de aplicación (alarma doméstica) metidos en el core del protocolo. |
| `IoTProtocol.h` | `enum EventCode` (MOTION, DOOR_OPEN, TIMBRE, SMOKE, TAMPER, GAS_DETECTED...) | **⚠️ Contaminado** | Este es el caso más claro: el core "sabe" qué es un timbre. |
| `IoTProtocol.h` | `enum DeviceType` (PIR_SENSOR, RELAY, SMOKE_SENSOR, DOOR_SENSOR...) | **⚠️ Contaminado** | Igual: tipos de dispositivo de una alarma doméstica, no genéricos. |
| `IoTProtocol.h` | Comentario "Convención Device IDs": rangos para PIR/botones/temperatura/relés | **⚠️ Contaminado (doc)** | Solo comentario, pero fija en la doc del core una taxonomía de aplicación. |
| `IoTNode.h/cpp` | Cola FIFO, prioridades, reliable/ACK, dedup por BOOT_ID+SEQ, registry, discovery HELLO, heartbeat | **Genérico** | Muy limpio — no menciona alarma en ningún punto. Buen candidato a quedar tal cual. |
| `IoTNode.cpp` | `sendEvent(EventCode event, ...)` | **⚠️ Frontera mixta** | La función en sí (encolar+ACK) es genérica, pero su firma obliga a usar `EventCode` del core. Debería recibir un TLV/payload genérico armado por la aplicación. |
| `IoTAuth.h/cpp` | HMAC-SHA256 truncado, firma/verificación, comparación en tiempo constante | **Genérico** | Bien aislado, ya expone `IoTAuthProvider` como interfaz — esto es casi exactamente el `ISecurity` que planteaba la Fase 4. |
| `IoTStorage.h/cpp` | BOOT_ID persistente, `IoTConfig` (heartbeat, antirebote, name, authKey) | **Mayormente genérico, con acoplamiento a LittleFS** | La interfaz es genérica; la implementación está pegada a LittleFS directamente (no hay `IStorage`). `antireboteMs` es un nombre algo específico de sensor tipo PIR pero es reutilizable igual (podría llamarse `debounceMs`). |
| `IoTConfigHandler.h/cpp` | Aplica CONFIG remoto: heartbeat, antirebote, nombre, auth, reboot, reset stats | **Genérico** | No conoce PIR/timbre. Bien. |

### `emisor_pir_unificado` (aplicación)

| Elemento | Clasificación |
|---|---|
| `device_config.h/cpp`, pines PIR/timbre, antirebote | **Correcto: específico, vive en la app** |
| `main.cpp`: lectura de flancos PIR/timbre, `sendEvent(EventCode::MOTION/TIMBRE, ...)` | **Correcto: específico, vive en la app** — pero depende de `EventCode` del core (ver arriba) |
| `main.cpp`: `sendStateReport()` usando `TlvTag::STATE_MOTION`, `STATE_BUTTON` | **Correcto en ubicación, pero usa tags contaminados del core** |
| Auth wiring (`configureAuthProvider`, callbacks verify/sign) | **Genérico, bien hecho** — este patrón adapter es exactamente el que la Fase 4 quiere formalizar |
| `ota.cpp` | **Específico de la app, correcto** — no debería ir en la librería |

### `receptor_central_unificado` (aplicación)

| Elemento | Clasificación |
|---|---|
| `event_handler.cpp`: `eventCodeToStr`, `duracionPorEvento`, `debeActivarBocina` | **Correcto: específico, vive en la app** |
| `event_handler.cpp`: publica en MQTT genérico `casa/iot/device_%02X/...` | **Genérico/reutilizable, buen patrón** |
| `event_handler.cpp`: topics V3 hardcodeados (`TOPIC_V3_EVENTO`, `TOPIC_V3_TIMBRE`) | **Correcto: específico, vive en la app** |
| `mqtt_manager.cpp`, `mqtt_discovery.cpp` | **Correcto: adaptador externo, no toca el core** — exactamente como debía ser según el diseño de capas |
| `hal.cpp` (Led/Buzzer) | **Correcto: específico de la app** |

## Resumen de contaminación real

El core (`IoTProtocol.h`) tiene **tres puntos concretos** donde conoce la aplicación de alarma, y son los únicos que bloquean reusar la librería en otro proyecto sin editarla:

1. `enum EventCode` — MOTION, TIMBRE, SMOKE, DOOR_OPEN, etc.
2. `enum DeviceType` — PIR_SENSOR, RELAY, SMOKE_SENSOR, etc.
3. Los `TlvTag` de estado 0xA0–0xA6 — STATE_MOTION, STATE_DOOR, STATE_ALARM, etc.

Todo lo demás (`IoTNode`, `IoTAuth`, `IoTProtocol` framing, `IoTStorage`, `IoTConfigHandler`) está **mejor separado de lo que sugería el informe original** — es una base bastante limpia. El trabajo de la Fase 4 del plan se reduce, en la práctica, a mover esos tres enums fuera de `IoTProtocol.h` hacia un `AlarmProfile.h` en el proyecto de aplicación, y cambiar `sendEvent(EventCode, ...)` por algo como `sendEvent(uint8_t appEventTag, ...)` o un `Message` genérico que la app rellena.

¿Quieres que te arme ese cambio concreto (el `AlarmProfile.h` + el ajuste mínimo de `IoTNode::sendEvent`) como primer paso ejecutable de la Fase 4, ya que el resto del core no necesita tocarse?
