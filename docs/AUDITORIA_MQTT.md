# Auditoría de robustez MQTT — Central IoT V4.3

**Estado:** auditoría documentada; no se implementan correcciones funcionales en este documento.
**Alcance:** `receptor_central_unificado/`, su integración con `IoTProtocol`, MQTT Discovery y la compatibilidad V3/V4.
**Referencia de código:** revisión actual del repositorio durante esta auditoría.
**Biblioteca MQTT:** `knolleary/PubSubClient@^2.8`, declarada en `receptor_central_unificado/platformio.ini`.

## 1. Objetivo y método

Se reconstruyó el flujo real desde la recepción de comandos y paquetes UDP hasta las publicaciones MQTT. Se revisaron:

- `receptor_central_unificado/src/mqtt_manager.cpp`;
- `receptor_central_unificado/src/event_handler.cpp`;
- `receptor_central_unificado/src/main.cpp`;
- `receptor_central_unificado/src/mqtt_discovery.cpp`;
- `receptor_central_unificado/src/config.cpp` y `include/config.h`;
- `receptor_central_unificado/platformio.ini`;
- `lib/IoTProtocol/IoTNode.cpp`, para distinguir la deduplicación UDP existente de la frontera MQTT;
- `docs/OPERATIONS.md`, `docs/CHANGELOG.md` y documentación de hallazgos.

La auditoría distingue cuatro estados:

- **CONFIRMADO:** el comportamiento se observa directamente en el código.
- **PARCIAL:** el hallazgo es correcto, pero el código ya contiene una mitigación limitada o el alcance necesita matiz.
- **DECISIÓN CONSCIENTE:** comportamiento conocido que se conserva por compatibilidad, con deuda técnica explícita.
- **NO VERIFICADO:** no existe prueba automatizada o evidencia de broker/HA/hardware que permita afirmar el comportamiento en ejecución real.

## 2. Flujo real observado

### Entrada MQTT

`mqttCallback()` en `mqtt_manager.cpp` convierte el payload en `String`, registra topic y mensaje, y dirige los aliases V3/V4 a dos handlers:

- `TOPIC_BOCINA_CMD` y `TOPIC_V3_BOCINA_CMD` → `procesarComandoBocina()`;
- `TOPIC_MODO` y `TOPIC_V3_MODO` → `procesarComandoModo()`.

Los comandos no llevan un identificador contractual de operación. Cada entrega válida ejecuta la acción recibida.

### Entrada UDP

`IoTNode` valida el paquete, autenticación, sesión, ACK y deduplicación antes de invocar `handleIoTPacket()`. La deduplicación existente es de transporte UDP, mediante `BOOT_ID + SEQ`; no protege la entrada MQTT.

### Salida MQTT

La central publica, según el caso:

- eventos transitorios por topics V4 y aliases V3;
- heartbeat y telemetría retained;
- estados de `STATE_REPORT` retained;
- nombre/status al recibir `HELLO`;
- estados de bocina y modo retained;
- estados `ONLINE/STALE/OFFLINE` cada 30 segundos;
- Discovery retained en `homeassistant/#`.

Cuando `mqttDisponible` es falso, las publicaciones se descartan. No existe una cola MQTT offline ni replay de eventos transitorios.

## 3. Matriz de hallazgos

| ID | Hallazgo | Estado | Severidad | Alcance | Decisión de auditoría |
|---|---|---|---|---|---|
| MQTT-01 | QoS 0 implícito y retornos de `publish()` ignorados | CONFIRMADO | Alta para comandos; media para telemetría | Comandos, eventos, estados y telemetría | Corregir por clases de mensaje; no tratar QoS 1 como confirmación de ejecución |
| MQTT-02 | Comandos sin `CMD_ID` ni deduplicación | CONFIRMADO | Alta | Bocina y modo | Diseñar contrato idempotente antes de implementar |
| MQTT-03 | Reconexión sin backoff exponencial y `connect()` síncrono | PARCIAL | Media | Recuperación MQTT/WiFi | Mantener prioridad UDP y reemplazar intervalos fijos por política medible |
| MQTT-04 | Buffer de 768 bytes sin margen ni prueba de límite | CONFIRMADO | Media | Discovery y publicaciones grandes | Definir límite de payload completo y probar cerca del límite |
| MQTT-05 | `event_handler.cpp` no verifica resultados de publicación | CONFIRMADO | Alta para eventos de alarma | EVENT, heartbeat, DATA y `STATE_REPORT` | Centralizar publicación comprobable y registrar pérdida |
| MQTT-06 | Estados remotos se republican cada 30 s aunque no cambien | CONFIRMADO | Baja/media | Disponibilidad de dispositivos remotos | Publicar cambios y separar heartbeat de disponibilidad |
| MQTT-07 | Duplicación parcial de topics V3/V4 | DECISIÓN CONSCIENTE | Baja como riesgo inmediato; media como deuda | Compatibilidad HA/V3 | Mantener mientras V3 siga soportado; encapsular el fan-out |

**Nota:** MQTT-01 y MQTT-05 se relacionan, pero no son el mismo defecto. MQTT-01 cubre semántica de entrega y comandos críticos; MQTT-05 cubre específicamente la pérdida silenciosa de eventos y telemetría en `event_handler.cpp`.

## 4. Hallazgo MQTT-01 — QoS y resultados de publicación

### Evidencia

- `receptor_central_unificado/src/mqtt_manager.cpp`, `intentarConexionMQTT()`: el LWT se registra con QoS `0`.
- El mismo archivo usa `mqtt.publish(...)` con los overloads normales, que en PubSubClient publican QoS 0. `retained=true` no equivale a QoS 1 ni a confirmación de ejecución.
- `publicarModoAlarma()`, `publicarEstadoBocina()`, la publicación de uptime, los estados de conexión, IP y estados de bocina no comprueban el retorno.
- `mqtt.subscribe(...)` tampoco comprueba el retorno.
- `mqtt_discovery.cpp::publicarEntidad()` sí comprueba el retorno de `mqtt.publish()` y registra `Discovery MQTT fallo`.

### Comportamiento e impacto

Para telemetría, perder ocasionalmente una publicación puede ser aceptable. Para `TOPIC_BOCINA_CMD`, modo armado/desarmado y eventos de alarma, hay dos fallos distintos:

1. PubSubClient puede no aceptar/encolar la publicación localmente y el código no lo registra.
2. Aunque el broker confirme una publicación MQTT QoS 1, eso solo confirma recepción del broker; no confirma ejecución de la acción en la central.

La central tampoco conserva una cola de publicaciones mientras MQTT está caído. Los eventos transitorios pueden perderse durante una ventana sin disponibilidad.

### Decisión

**Corregir por clases de mensaje**, sin prometer que QoS 1 sustituye un ACK de aplicación:

- comprobar y registrar el resultado de cada publicación relevante;
- definir QoS explícito donde la biblioteca y el contrato lo permitan;
- añadir confirmación de aplicación para comandos críticos;
- mantener telemetría best-effort si se documenta como tal.

### Criterios de aceptación

- Cada publish crítico tiene resultado observable (`accepted`/`failed`) y no solo un log de intento.
- Un fallo local de publicación no se presenta como comando ejecutado.
- El contrato distingue `broker_received` de `command_executed`.
- Existe una prueba que fuerza `publish()` fallido y verifica el registro/estado resultante.

## 5. Hallazgo MQTT-02 — Comandos sin `CMD_ID` ni deduplicación

### Evidencia

`mqtt_manager.cpp::mqttCallback()` acepta los aliases V3/V4 y llama directamente a `procesarComandoBocina()` o `procesarComandoModo()`. El comentario del propio código indica que todavía no existe `CMD_ID` ni deduplicación MQTT contractual.

- `ON` llama a `buzzer.timedOn(...)`.
- `OFF` llama a `buzzer.off()` y publica estado.
- `armado`/`desarmado` actualiza `modoAlarma` y republica el estado.

No hay nonce, identificador de comando, ventana de replay, almacenamiento del último comando ni respuesta de aplicación asociada a una operación.

La deduplicación UDP existente en `IoTNode` no cubre este flujo: solo evita volver a entregar al handler una retransmisión con el mismo `BOOT_ID + SEQ`.

### Impacto

Una repetición por reconexión, retry de Home Assistant, publisher duplicado o mensaje retained puede volver a ejecutar una operación. En particular, `timedOn()` puede reiniciar el temporizador de la bocina aunque se trate de una repetición del mismo comando.

### Decisión

**No implementar deduplicación basada solamente en el texto del payload.** El contrato futuro debe incluir al menos:

```text
command_id
operation
arguments
issued_at o expiry opcional
```

La central debe almacenar una ventana acotada de comandos procesados y responder con el mismo `command_id`, distinguiendo `accepted`, `duplicate`, `executed` y `failed`.

### Criterios de aceptación

- El mismo `command_id` recibido dos veces produce como máximo una ejecución.
- Un comando nuevo con otro `command_id` sí puede ejecutarse.
- La respuesta conserva el `command_id` y el resultado de ejecución.
- La política para mensajes retained, expirados o sin `command_id` está documentada y probada.
- V3 se mantiene mediante un adaptador explícito o se rechaza con una razón observable; no se deduplica de forma ambigua.

## 6. Hallazgo MQTT-03 — Reconexión y backoff

### Evidencia

`mqtt_manager.cpp` y `config.cpp` implementan esta política:

- primer intento después de aproximadamente 1 segundo;
- modo LOCAL: sondeo cada 5 minutos;
- modo HA: reintento cada 15 segundos;
- tras tres fallos consecutivos: vuelta a LOCAL;
- primer sondeo posterior acelerado a 60 segundos y después 5 minutos.

Por tanto, el hallazgo de ausencia de backoff exponencial es correcto, pero la implementación no reintenta cada 15 segundos indefinidamente: alterna entre modo HA y fallback LOCAL.

Además, `mqtt.connect()` es síncrono. `setSocketTimeout(2)` limita el timeout de socket de PubSubClient, pero no convierte el handshake completo en no bloqueante. La prioridad UDP se respeta antes y después del intento, no durante el handshake.

La pérdida de WiFi limpia `mqttDisponible`; la transición posterior puede no conservar exactamente la misma marca temporal de caída que una desconexión detectada con WiFi estable.

### Impacto

- Con un único dispositivo, el intervalo fijo es un riesgo operativo moderado, no una emergencia.
- Con más nodos o un broker sobrecargado, la cadencia fija puede generar intentos sincronizados.
- Un `connect()` bloqueante puede retrasar una iteración del loop y, en escenarios extremos, afectar recepción UDP o temporización local.

### Decisión

**Mantener el fallback LOCAL, pero rediseñar la política de recuperación antes de producción:** backoff con límites, jitter opcional, registro de próximo intento y medición del tiempo máximo de bloqueo.

No se debe prometer “nunca bloquea” mientras `mqtt.connect()` siga siendo síncrono.

### Criterios de aceptación

- La secuencia de reintentos está especificada como tabla y cubierta por prueba temporal.
- El intervalo crece hasta un máximo y no se sincroniza accidentalmente entre dispositivos.
- La bocina local y el procesamiento UDP tienen prioridad durante la recuperación.
- Se mide el tiempo máximo de `mqtt.connect()` y se demuestra que no provoca watchdog ni pérdida inaceptable de UDP.
- WiFi perdido y broker perdido tienen transiciones documentadas y consistentes.

## 7. Hallazgo MQTT-04 — Buffer MQTT de 768 bytes

### Evidencia

- `mqtt_manager.cpp::inicializarMQTT()` ejecuta `mqtt.setBufferSize(768)`.
- `mqtt_discovery.cpp::publicarEntidad()` usa `StaticJsonDocument<768>` y `char payload[768]`.
- Discovery comprueba si `serializeJson()` produce longitud cero y si `mqtt.publish()` falla.
- Las publicaciones restantes no tienen una comprobación equivalente ni una prueba de tamaño máximo.

El buffer de PubSubClient corresponde al paquete MQTT completo, no solamente al JSON. Por tanto, un payload cercano a 768 bytes no tiene 768 bytes útiles: también deben caber topic y cabeceras MQTT.

### Impacto

Un crecimiento futuro de Discovery —más atributos, iconos, unidades o `device_class`— puede superar el espacio disponible. El fallo se observaría como publicación rechazada, pero no todas las rutas lo registrarían de forma clara.

### Decisión

**Definir un presupuesto de tamaño**, no aumentar el buffer a ciegas:

- medir `topic + payload + overhead` para cada clase de publicación;
- reservar margen explícito;
- comprobar truncamiento/serialización antes de publicar;
- decidir si Discovery requiere buffer mayor o dividirse por entidad;
- añadir una prueba cercana al límite.

### Criterios de aceptación

- Existe un tamaño máximo documentado para Discovery y publicaciones runtime.
- Ningún payload válido depende de ocupar el 100% del buffer.
- Un payload demasiado grande produce log identificable y no se publica parcialmente.
- La prueba cubre payload válido cercano al límite y payload que lo supera.

## 8. Hallazgo MQTT-05 — Publicaciones de `event_handler.cpp` sin retorno

### Evidencia

En `receptor_central_unificado/src/event_handler.cpp`:

- `publishEvent()` publica el topic V4 y, para MOTION/TIMBRE positivos, el alias V3 sin comprobar retornos.
- `publishHeartbeat()` publica uptime, RSSI, heap, contadores y timeouts sin comprobar retornos.
- `publishStateReport()` publica cada estado retained sin comprobar retornos.
- Las ramas `HELLO` y `DATA` también publican sin propagar éxito/fallo.

El código sí evita publicar cuando `mqttDisponible` es falso, pero eso solo evita una llamada conocida como imposible; no detecta una publicación rechazada durante una conexión aparentemente disponible.

### Impacto

Un evento UDP puede haber sido aceptado y activar la bocina local, pero no llegar a MQTT ni a Home Assistant. Sin registro del retorno, el operador no puede distinguir “evento procesado y publicado” de “evento procesado pero perdido en MQTT”.

Esto es especialmente relevante porque los eventos son no-retained y no existe replay de eventos transitorios.

### Decisión

**Corregir con helpers de publicación tipados por criticidad**, por ejemplo:

- evento de alarma: resultado requerido y log de fallo;
- estado/telemetría: resultado registrado a nivel warning/debug según criticidad;
- Discovery: conservar la comprobación existente y unificar el formato del log.

No convertir automáticamente todos los eventos en retained: eso cambiaría la semántica de Home Assistant.

### Criterios de aceptación

- Cada evento aceptado produce un resultado local: publicado, rechazado o descartado por MQTT no disponible.
- Un fallo en el topic V4 y un fallo en el alias V3 quedan diferenciados.
- La alarma local continúa funcionando aunque MQTT falle.
- Una prueba de publish falso verifica logs y contadores sin duplicar la acción local.

## 9. Hallazgo MQTT-06 — Republicación de estados cada 30 segundos

### Evidencia

`receptor_central_unificado/src/main.cpp`, bloque `lastStatusPub`, recorre los remotos conocidos cada 30 segundos y publica `casa/iot/device_%02X/status` retained cuando `mqttDisponible` es verdadero. No compara contra el último estado publicado.

Esto es distinto de `STATE_REQUEST`: la ventana de solicitudes se ejecuta en t=0, t=3 s y t=10 s en boot o recuperación, y puede reiniciarse al reconectar MQTT para recuperar estados recibidos durante una caída del broker.

### Impacto

- tráfico MQTT periódico innecesario;
- escrituras retained repetidas aunque el valor no cambie;
- más ruido al diagnosticar cambios reales;
- carga pequeña en una central, pero multiplicable si crece el número de remotos.

No es una pérdida de fiabilidad por sí misma y la republicación puede actuar como heartbeat de disponibilidad.

### Decisión

**Separar cambio de estado y heartbeat:**

- publicar inmediatamente al cambiar `ONLINE/STALE/OFFLINE`;
- conservar un heartbeat de disponibilidad más espaciado, por ejemplo 5 minutos, si Home Assistant necesita refresco periódico;
- no confundir ese heartbeat con la recuperación best-effort de `STATE_REQUEST`.

### Criterios de aceptación

- Un estado sin cambios no genera publicaciones cada 30 segundos salvo que esté dentro del heartbeat explícito.
- Cada transición de estado se publica una vez por cambio.
- Una reconexión MQTT restaura retained sin generar tormenta innecesaria.
- La prueba cuenta publicaciones para estado estable, transición y heartbeat.

## 10. Hallazgo MQTT-07 — Duplicación V3/V4

### Evidencia

`config.cpp` define ambos conjuntos de topics. `mqtt_manager.cpp` publica modo, bocina, uptime, IP y disponibilidad en V3/V4. `event_handler.cpp` publica siempre el evento por el topic V4 por dispositivo y solo publica aliases V3 para MOTION/TIMBRE positivos.

Por tanto, la duplicación es deliberada, pero no todos los eventos se publican dos veces: la compatibilidad V3 es parcial y depende del tipo de evento.

`mqtt_discovery.cpp` usa topics V3 para `state_topic`, `command_topic` y disponibilidad con el fin de conservar automatizaciones y entidades existentes.

### Impacto

- duplicación de llamadas y posibilidad de divergencia entre topics;
- mayor superficie de pruebas;
- mantenimiento doble al cambiar payloads;
- riesgo de que un consumidor V3 y uno V4 interpreten estados distintos si solo se corrige un lado.

### Decisión

**Mantener la compatibilidad mientras V3 siga soportado**, pero encapsularla:

- un helper debe publicar el contrato canónico y sus aliases;
- las diferencias intencionales V3/V4 deben estar en una tabla de compatibilidad;
- la retirada de V3 requiere una decisión explícita y migración de Home Assistant.

### Criterios de aceptación

- Cada mensaje con alias tiene una matriz que indica topic, payload y semántica.
- Un cambio de payload no puede actualizar accidentalmente solo uno de los contratos.
- Discovery y automatizaciones existentes siguen probándose antes de retirar V3.
- La documentación marca V3 como compatibilidad mantenida o deprecada, nunca como equivalente total a V4.

## 11. Observaciones adyacentes detectadas

Estas observaciones no se convierten en un octavo hallazgo de esta auditoría, pero deben considerarse al planificar las correcciones:

1. **LWT asimétrico:** el LWT de `mqtt.connect()` usa `TOPIC_V3_ESTADO`. Al conectar también se publica `TOPIC_ESTADO`, pero ese topic V4 no recibe un LWT equivalente. Discovery usa deliberadamente V3, por lo que la asimetría afecta principalmente a consumidores que escuchen el topic V4.
2. **Validación de `EVENT_VALUE`:** `handleIoTPacket()` inicializa `eventValue=1` y no comprueba el retorno de `getTLV_uint8(EVENT_VALUE, eventValue)`. Un EVENT con `EVENT_TYPE` válido y `EVENT_VALUE` ausente puede continuar con valor 1. El builder normal del emisor sí agrega el TLV, pero el receptor no aplica una validación simétrica.
3. **Sin cola MQTT offline:** estados retained pueden reconstruirse en reconexión, pero los eventos no-retained recibidos durante una caída no se reenvían.
4. **Contrato anunciado pero no implementado:** el encabezado de `receptor_central_unificado/src/main.cpp` menciona envío de CONFIG por MQTT, pero `mqtt_manager.cpp` solo implementa comandos de bocina y modo en la ruta auditada. No se debe tratar CONFIG MQTT como funcionalidad disponible hasta que exista su flujo completo.

## 12. Matriz de pruebas y evidencia faltante

No existe actualmente una prueba automatizada de `PubSubClient`, broker falso, LWT real, Discovery contra Home Assistant, deduplicación de comandos MQTT ni del loop completo de la central. Las pruebas host de `IoTNode` y el simulador Python cubren UDP, ACK, replay y `BOOT_ID`, pero no la frontera MQTT.

| Hallazgo | Prueba necesaria | Evidencia actual |
|---|---|---|
| MQTT-01 | Forzar `publish()` fallido; observar resultado y separar broker recibido de ejecución | Discovery comprueba su retorno; el resto no |
| MQTT-02 | Repetir mismo `CMD_ID`, retained y aliases V3/V4; comprobar una sola ejecución | No existe |
| MQTT-03 | Medir t=0, reintentos, fallback y duración de `connect()` con broker/WiFi caído | Código y documentación; no prueba automatizada |
| MQTT-04 | Payload válido cercano al límite y payload sobredimensionado | No existe prueba de límite MQTT |
| MQTT-05 | Forzar fallo por evento, heartbeat, DATA y STATE_REPORT; comprobar logs/contadores | No existe |
| MQTT-06 | Contar publicaciones con estado estable, transición y reconexión | No existe prueba del loop central |
| MQTT-07 | Verificar siete entidades Discovery, aliases, payloads y ausencia de divergencia | Inspección de código; no prueba MQTT/HA automática |

Para pruebas con broker real, seguir `docs/OPERATIONS.md`: registrar broker/topic, payload retained, JSON, availability/LWT, entidad de Home Assistant y comportamiento después de reiniciar broker o HA. No considerar un log de `publish()` como evidencia de recepción por Home Assistant.

## 13. Orden recomendado de futuras correcciones

1. **Contrato de comandos:** definir `CMD_ID`, idempotencia, expiración y respuesta de ejecución.
2. **Observabilidad de publicaciones:** helpers con retorno, criticidad y contadores/logs.
3. **Semántica de entrega:** decidir qué mensajes necesitan QoS y qué cliente MQTT lo soporta; no confundir QoS con ACK de aplicación.
4. **Reconexión:** backoff acotado con medición de bloqueo y preservación de prioridad UDP/local.
5. **Presupuesto de buffer:** límites y pruebas de Discovery/publicaciones grandes.
6. **Disponibilidad:** cachear cambios y separar transición de estado de heartbeat.
7. **Compatibilidad:** centralizar fan-out V3/V4 y mantener una matriz contractual hasta la eventual retirada de V3.

## Conclusión

La cadena UDP → `IoTNode` → deduplicación/ACK → `event_handler` funciona con una frontera MQTT deliberadamente best-effort. La protección contra duplicados existe para retransmisiones UDP, pero no para comandos MQTT. La mayor brecha de producción no es solo el QoS 0: es la ausencia de un contrato de ejecución idempotente y la falta de observabilidad de publicaciones fallidas.

La auditoría confirma los siete hallazgos con los matices indicados. No se implementaron cambios funcionales ni se afirmó validación MQTT real: todavía falta broker/Home Assistant/hardware o un doble de pruebas específico para cerrar esos criterios de aceptación.
