# Matriz de unificación V3 + V4

Estado: alcance canónico previo a la implementación por bloques.

> **Actualización 2026-09-02:** la trazabilidad de los diez drafts está en [`DRAFTS_AUDIT.md`](DRAFTS_AUDIT.md). Las filas U-09/U-11 deben interpretarse como “integrado en el código actual; compilación, integración y hardware pendientes”. La sirena continúa en F-05. El vocabulario de alarma pertenece a `lib/AlarmProfile/AlarmProfile.h`, no a `IoTProtocol.h`.

## Objetivo

Conservar `legacy/emisor_pir/` y `legacy/receptor_bocina/` como respaldo congelado de V3. La versión final se construirá sobre `lib/IoTProtocol/`, `emisor_pir_unificado/` y `receptor_central_unificado/`, absorbiendo el comportamiento probado de V3 y las mejoras de V4.

No se promoverá la versión final ni se retirará V3 hasta completar tests host, compilación, simulación y validación hardware agrupada.

## Criterios de clasificación

- **U0 — Obligatoria para unificación:** necesaria para que V4 sustituya funcionalmente a V3.
- **R1 — Robustez/seguridad:** necesaria antes de producción, pero separable de la paridad mínima.
- **F2 — Extensión futura:** idea válida, pero no bloquea la primera versión unificada y requiere contrato adicional.
- **D — Duplicada:** ya está representada por otra fuente canónica.
- **C — Contradictoria:** la documentación afirma un estado que el código no demuestra; debe corregirse el estado, no asumirse la capacidad.
- **N — No decidible:** requiere una decisión de producto, amenaza o hardware antes de implementarse.

## Matriz de trazabilidad

| ID | Comportamiento/idea | Procedencia | Estado actual | Clasificación | Destino | Criterio de aceptación |
|---|---|---|---|---|---|---|
| U-01 | Lectura independiente de PIR y TIMBRE | `legacy/emisor_pir/src/main.cpp`, `docs/ARCHITECTURE.md` | V3 probado por código; V4 conserva dos entradas | U0 | Emisor final V4 | Dos activaciones simultáneas generan dos eventos independientes y ninguno bloquea al otro |
| U-02 | PIR por flanco y antirrebote compatible | V3, V4, BUG-003 | V4 usa 200 ms por defecto y mantiene `antireboteMs` configurable | U0 | Emisor final V4 | El valor final es explícito, se prueba con HC-SR501 sostenido y no pierde TIMBRE |
| U-03 | TIMBRE activo-bajo con antirrebote | V3 y V4 | Implementado en ambas líneas | U0 | Emisor final V4 | Una pulsación produce un solo evento; otra pulsación posterior produce otro |
| U-04 | ACK y retransmisión no bloqueantes | V3, `IoTNode` V4 | V3 tiene cuatro eventos en vuelo; V4 una reliable en vuelo y cola | U0 | `IoTNode` | Sensor y loop siguen funcionando durante espera, pérdida y reintentos |
| U-05 | Cola con prioridades y backpressure | `IoTNode.h/.cpp` | V4 implementado parcialmente | U0/R1 | `IoTNode` | URGENT conserva prioridad, BACKGROUND puede descartarse explícitamente y cada drop queda medible |
| U-06 | Drain loop acotado de UDP | V3, H-007 | V3 drena hasta ocho; V4 drena hasta `IOT_MAX_RX_PER_LOOP` (8) por iteración | U0 | `IoTNode` | Una ráfaga acotada se procesa sin pérdida ni monopolización del loop |
| U-07 | ACK de duplicado, efecto único | V3 y V4 | Estructuralmente implementado | U0 | `IoTNode` + perfil alarma | El duplicado recibe ACK válido y no repite la acción |
| U-08 | Deduplicación por BOOT_ID + SEQ | V4, BUG-008 | Ventana deslizante por sesión; reinicio solo avanza a un BOOT_ID nuevo y resetea la ventana | U0 | `IoTNode` | Mismo BOOT_ID+SEQ se descarta; nuevo BOOT_ID con mismo SEQ se acepta |
| U-09 | BOOT_ID persistente | `IoTStorage`, PLAN Fase 2 | Integrado en código; falta hardware | U0/R1 | `IoTStorage` + `IoTNode` | Dos arranques conservando LittleFS producen IDs distintos y cada arranque usa un único ID |
| U-10 | Wire format binario, CRC, TLV y versión | `IoTProtocol.*` | Implementado y probado en codec | U0 | Núcleo común | Roundtrip, CRC, longitud, versión y payload máximo pasan tests host |
| U-11 | HMAC antes de efectos internos | H-002, BUG-010 | `IoTNode` verifica antes de registry, ACK, dedup y callback | U0/R1 | `IoTNode` + `IoTAuth` | Paquete inválido no actualiza registry, dedup, ACK ni callback |
| U-12 | Firma uniforme de mensajes | H-003 | `IoTNode` aplica la política a `sendDirect()` y `enqueue()` | U0/R1 | `IoTNode`/builders | EVENT, HELLO, HEARTBEAT, STATE_REPORT, ACK y RESPONSE siguen una política única |
| U-13 | Anti-replay formal | ROADMAP V4.4, PLAN Fase 4 | Ventana deslizante implementada: fuera de ventana y BOOT_ID anterior se rechazan y se reconocen si solicitan ACK | R1 | `IoTNode` | Replay fuera de ventana se rechaza; paquete nuevo dentro de ventana se acepta |
| U-14 | Validación de BOOT_ID en ACK | auditoría de `IoTNode` | ACK requiere reliable activo y SRC/SEQ coincidentes; el primer ACK autenticado/estructural con BOOT_ID no cero puede bootstrapear cuando no hay sesión remota conocida; después exige BOOT_ID coincidente | R1 | canal reliable | ACK de una sesión anterior o fuera de contexto no confirma; el primer ACK válido establece RemoteDevice y expectedBootId |
| U-15 | Registry y estados ONLINE/STALE/OFFLINE | V4 `IoTNode` y central | Parcialmente implementado | U0/R1 | `IoTNode` + central | HELLO, heartbeat, cambio de IP y timeout actualizan identidad y disponibilidad correctamente |
| U-16 | MQTT fuera del camino crítico UDP | V3 y central V4 | Primer `connect()` diferido al loop después de `node.loop()`; modo LOCAL/HA, sondeo y backoff conservados. `PubSubClient::connect()` sigue siendo síncrono durante cada intento | U0/R1 | Adapter MQTT central | Un evento UDP recibido al arranque se procesa/ACK/ejecuta antes del primer intento MQTT; la validación de broker caído durante intentos posteriores requiere integración/hardware |
| U-17 | Buzzer y alarma PIR/TIMBRE | `receptor_bocina`, central V4 | V4 conserva MOTION/TIMBRE, modos, publicaciones V3 y duraciones 1000/500 ms; temporizador único ahora rollover-safe y no bloqueante. V3 no define prioridad ante simultaneidad | U0 | Perfil alarma central | MOTION solo suena armado, TIMBRE suena también desarmado, cada evento publica su payload V3 y el temporizador no bloquea; no se inventa una política de simultaneidad |
| U-18 | MQTT Discovery/Home Assistant | V3 `mqtt_discovery` y V4 | Integrado en V4: siete entidades retained, `unique_id` estable por central y topics/availability V3 compatibles | U0 | Adapter HA central | Entidades se publican tras conexión MQTT, conservan estados offline por LWT y apuntan a topics V3 que V4 también publica |
| U-19 | Heartbeat, RTT y estadísticas | V4 `IoTNode` | Mayormente implementado | R1 | Telemetría central | Métricas tienen schema estable y no bloquean el loop |
| U-20 | Tests host de protocolo | `tests/` | 10 pruebas codec/TLV/CRC | U0 | `tests/` | Codec, límites y versión siguen pasando tras cambios |
| U-21 | Tests host de dedup, cola, HMAC y efectos | PLAN y auditoría | No existen | U0/R1 | `tests/` | Casos válidos, inválidos, duplicados, overflow y orden de efectos son reproducibles |
| U-22 | Simulador UDP | PLAN Fase 6 | `tools/iot_simulator.py` implementa escenarios stdlib de CRC, TLV, HMAC, retry, dedup, replay, BOOT_ID y dos sensores; no ejecuta C++ ni hardware | R1 | `tools/` | Simula pérdida, delay, duplicado, CRC, HMAC, replay y sensores simultáneos |
| U-23 | Configuración remota autenticada | `IoTConfigHandler` | Handler emisor; no hay flujo completo central | R1 | módulo CONFIG/RESPONSE | CONFIG válida se aplica atómicamente; inválida no cambia nada; respuesta correlacionada |
| F-01 | COMMAND/RESPONSE y relé | enums V4, ROADMAP | Tipos declarados; flujo incompleto | F2 | perfil actuador | Requiere contrato de actuador, autorización y pruebas propias |
| F-02 | Capability Discovery | `docs/universal-protocol/CAPABILITY_DISCOVERY.md` | No implementado | F2 | protocolo + registry | Nodos anuncian capacidades sin romper nodos antiguos |
| F-03 | Event log de 100 eventos | drafts, ROADMAP | No implementado | F2 | central | Ring buffer con overflow y consulta definidos |
| F-04 | Máquina avanzada de alarma/zonas | drafts, ROADMAP | No implementado | F2 | perfil alarma | Tabla de estados, permisos y recuperación aprobada |
| F-05 | Sirena intermitente | drafts, PLAN Fase 8 | No implementado | F2 | perfil alarma | Patrón no bloqueante y configurable, sin alterar baseline sin decisión |
| F-06 | DHCP/discovery de IP | ROADMAP | IP estática actual | F2/N | red y configuración | Requiere política de red real y discovery probado |
| F-07 | AES-GCM/cifrado | ROADMAP | No implementado; HMAC integra/autentica | N | seguridad/wire | Modelo de amenazas, nonce, payload y compatibilidad aprobados |
| F-08 | Persistencia avanzada de central/registry | ROADMAP | No implementado | F2 | `IoTStorage` central | Política de desgaste, lotes y estado tras reboot definidos |
| C-01 | “Auth unificada/terminada” | CHANGELOG/documentación histórica | La frontera de verificación y firma está implementada; la cobertura de pruebas de integración sigue pendiente | C | documentación + implementación U-11/U-12 | No declarar terminado hasta pasar pruebas de orden de efectos |
| C-02 | “BOOT_ID persistente terminado” | documentación histórica | Código integrado; hardware pendiente | C | documentación | Separar aplicado en código de verificado en hardware |
| C-03 | Config documentada como JSON | `IoTStorage.h` vs `.cpp` | Código usa `key=value` | C | storage | Elegir y documentar un solo formato antes de herramientas externas |
| C-04 | Central puede enviar CONFIG por MQTT | comentarios V4 | No existe flujo real | C | backlog F/R | No presentarlo como capacidad hasta implementar y probar el flujo |
| D-01 | Ideas repetidas en drafts/roadmap/plan | informes documentales | Múltiples fuentes | D | `PLAN_EJECUCION_FUTURA.md` + esta matriz | Un estado canónico por idea |
| N-01 | Topología B622/DMZ y red final | drafts | No demostrada por código | N | documentación operativa | Medir gateway, subnet, broker y rutas antes de fijar comportamiento de red |

## Alcance que se implementa antes del próximo hardware

El primer bloque completo será `U-01..U-22` en lo que sea software verificable: paridad de sensores, reliable/cola, recepción acotada, deduplicación, BOOT_ID, autenticación temprana, firma uniforme, anti-replay, ACK por sesión, registry, MQTT diferido con backoff, alarma, discovery HA, tests y simulador mínimo.

Las filas `F-01..F-08` no se mezclan con el núcleo sin contrato adicional. No se consideran olvidadas: quedan trazadas como extensiones posteriores o decisiones pendientes. V3 no se modifica.

## Alcance verificado de este cambio U-16/U-17/U-18

- `legacy/receptor_bocina/` y `legacy/emisor_pir/` permanecen congelados y sin modificaciones.
- La central V4 inicializa `IoTNode` antes de configurar MQTT. `inicializarMQTT()` no conecta; el primer intento se ejecuta desde `loop()`, después de una oportunidad de `node.loop()` para recibir, procesar y reconocer UDP. El fallback LOCAL, el sondeo de 5 minutos, la reconexión HA de 15 segundos y la prioridad de una bocina activa se conservan.
- V4 publica sus topics dinámicos y, para MOTION/TIMBRE y estados de la central, publica también el contrato V3 (`casa/alarma/...`). Discovery publica siete configuraciones retained con identificadores `central_alarma_*`, para que la migración no reutilice los `unique_id` de la V3. Los topics de runtime se comparten deliberadamente para conservar automatizaciones; V3 y V4 no deben operar simultáneamente porque también comparten disponibilidad MQTT y la IP estática `192.168.0.201`.
- El temporizador físico usa comparación por diferencia de `millis()` y no bloquea. Se conserva el temporizador único y no se define una prioridad nueva para eventos simultáneos porque V3 no la documenta.
- Este cambio no implementa Capability Discovery, Event Log, AES, DHCP ni ningún flujo CONFIG/COMMAND nuevo.

## Gate de promoción

La V4 unificada solo sustituye a V3 cuando demuestre en código, tests, simulador y hardware:

1. PIR, TIMBRE y ambos simultáneamente.
2. ACK, retransmisión, duplicados, fuera de orden y reinicio.
3. BOOT_ID persistente y recuperación ante fallo de storage.
4. HMAC válida, inválida, ausente y replay.
5. WiFi perdido/recuperado y broker MQTT caído/recuperado.
6. Buzzer local, MQTT/HA y estados ONLINE/STALE/OFFLINE.
7. OTA protegida y recuperación serial, si se incluye en la promoción.
8. RAM, flash, RTT, retransmisiones y eventos perdidos medidos.

Hasta cumplir ese gate:

```text
V3.5.1 = producción/respaldo
V4 unificada = candidata en desarrollo
```
